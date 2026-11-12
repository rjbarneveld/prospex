// -----------------------------------------------------------------------------
// Terrain properties and MFD routing weights
//
// Routing rule:
//   * Hillslope cells may route to any valid lower neighbour.
//   * Channel cells may route only to lower channel cells.
//   * Lakes and active dams are allowed as terminal storage targets.
//
// This prevents channel water from returning to hillslope cells and then being
// counted again as new hillslope-to-channel runoff.
//
// IMPORTANT:
//   d[x][y] uses a 3 x 3 layout, including the skipped centre position.
//   It therefore needs 9 entries, indexed 0..8. The previous dzTmp[8]
//   declaration wrote past the end of the temporary array at index 8.
// -----------------------------------------------------------------------------

// Set these to false for a literal channel-to-channel-only network.
// Keeping them true preserves inflow into the existing lake/dam storage logic.
constexpr bool CHANNEL_CAN_ENTER_LAKE = true;
constexpr bool CHANNEL_CAN_ENTER_DAM  = true;

// A positive minimum gradient is retained for very small, but genuinely
// downslope, elevation differences. Equal-height neighbours are not connected,
// because arbitrary flat links can introduce cycles into topoOrder.
constexpr float DZ_EPS = 1.0e-4f;

// Diagnostics prepared once with the routing weights.
long long fr_validCells                    = 0;
long long fr_hillslopeCells                = 0;
long long fr_channelCells                  = 0;
long long fr_channelCellsWithRoute         = 0;
long long fr_channelDeadEnds               = 0;
long long fr_channelToChannelConnections   = 0;
long long fr_channelToLakeConnections      = 0;
long long fr_channelToDamConnections       = 0;
long long fr_rejectedChannelToLandLinks    = 0;
long long fr_hillslopeCellsWithoutRoute    = 0;

// -----------------------------------------------------------------------------
// 1. Slope and depression-storage parameterisation
// -----------------------------------------------------------------------------
for (int x = 1; x < xSize - 1; ++x)
{
    for (int y = 1; y < ySize - 1; ++y)
    {
        if (dem[x][y] == noData ||
            dem[x - 1][y] == noData ||
            dem[x + 1][y] == noData ||
            dem[x][y - 1] == noData ||
            dem[x][y + 1] == noData)
        {
            continue;
        }

        const double sx =
            ((dem[x + 1][y - 1] + dem[x + 1][y] + dem[x + 1][y + 1]) -
             (dem[x - 1][y - 1] + dem[x - 1][y] + dem[x - 1][y + 1])) /
            (6.0 * dx);

        const double sy =
            -((dem[x - 1][y - 1] + dem[x][y - 1] + dem[x + 1][y - 1]) -
              (dem[x - 1][y + 1] + dem[x][y + 1] + dem[x + 1][y + 1])) /
            (6.0 * dy);

        slope[x][y] = static_cast<float>(
            std::sqrt(sx * sx + sy * sy)
        );

        const bool sourceIsChannel = channel[x][y] != noData;

        // Channel cells should not retain hillslope depression storage.
        if (sourceIsChannel)
        {
            dsc_max[x][y] = 0.0f;
        }
        else if (landuse[x][y] == 30) // AR5 forest
        {
            dsc_max[x][y] = 5.0f;
        }
        else if (landuse[x][y] > 10 && landuse[x][y] < 20)
        {
            dsc_max[x][y] = 1.0f;
        }
        else if (landuse[x][y] > 20 && landuse[x][y] < 30)
        {
            dsc_max[x][y] =
                3.27f /
                (1.0f + std::exp(20.0f * (slope[x][y] + 0.01f)));
        }
    }
}

std::cout << "Calculating channel-constrained outflow ratios d[x,y,9]"
          << std::endl;

// -----------------------------------------------------------------------------
// 2. MFD routing weights
// -----------------------------------------------------------------------------
for (int x = 1; x < xSize - 1; ++x)
{
    for (int y = 1; y < ySize - 1; ++y)
    {
        // Always clear all nine positions, including the unused centre.
        for (int i = 0; i < 9; ++i)
            d[x][y][i] = 0.0f;

        if (dem[x][y] == noData)
            continue;

        // Lake and dam cells are storage cells. Their outflow is handled by
        // their dedicated routines, so they receive no ordinary MFD weights.
        if (lakes[x][y] != noData)
            continue;

        if (simulateDams == 1 && dams[x][y] != noData)
            continue;

        ++fr_validCells;

        const bool sourceIsChannel = channel[x][y] != noData;

        if (sourceIsChannel)
            ++fr_channelCells;
        else
            ++fr_hillslopeCells;

        float gradient[9] = {0.0f};
        float gradientSum = 0.0f;

        int wIndex = 0;

        // -----------------------------------------------------------------
        // 2a. Identify eligible lower neighbours and calculate gradients
        // -----------------------------------------------------------------
        for (int X = -1; X <= 1; ++X)
        {
            for (int Y = -1; Y <= 1; ++Y)
            {
                if (X == 0 && Y == 0)
                {
                    ++wIndex;
                    continue;
                }

                const int nx = x + X;
                const int ny = y + Y;

                if (nx < 0 || nx >= xSize || ny < 0 || ny >= ySize ||
                    dem[nx][ny] == noData)
                {
                    ++wIndex;
                    continue;
                }

                const bool targetIsChannel = channel[nx][ny] != noData;
                const bool targetIsLake    = lakes[nx][ny] != noData;
                const bool targetIsDam =
                    simulateDams == 1 && dams[nx][ny] != noData;

                bool eligibleTarget = true;

                if (sourceIsChannel)
                {
                    eligibleTarget =
                        targetIsChannel ||
                        (CHANNEL_CAN_ENTER_LAKE && targetIsLake) ||
                        (CHANNEL_CAN_ENTER_DAM && targetIsDam);

                    if (!eligibleTarget &&
                        !targetIsChannel &&
                        !targetIsLake &&
                        !targetIsDam)
                    {
                        // This is a potential channel-to-land link that the
                        // old unrestricted MFD calculation would have used.
                        ++fr_rejectedChannelToLandLinks;
                    }
                }

                // Maintain a DAG: only strictly lower neighbours are linked.
                if (!eligibleTarget || dem[nx][ny] >= dem[x][y])
                {
                    ++wIndex;
                    continue;
                }

                const float distance =
                    std::sqrt(static_cast<float>(X * X + Y * Y));

                float dz = (dem[x][y] - dem[nx][ny]) / distance;

                if (dz < DZ_EPS)
                    dz = DZ_EPS;

                gradient[wIndex] = dz;
                gradientSum += dz;

                if (sourceIsChannel)
                {
                    if (targetIsChannel)
                        ++fr_channelToChannelConnections;
                    else if (targetIsLake)
                        ++fr_channelToLakeConnections;
                    else if (targetIsDam)
                        ++fr_channelToDamConnections;
                }

                ++wIndex;
            }
        }

        // -----------------------------------------------------------------
        // 2b. Normalise eligible weights so they sum exactly to one
        // -----------------------------------------------------------------
        if (gradientSum > 0.0f)
        {
            for (int i = 0; i < 9; ++i)
            {
                if (gradient[i] > 0.0f)
                    d[x][y][i] = gradient[i] / gradientSum;
            }

            if (sourceIsChannel)
                ++fr_channelCellsWithRoute;
        }
        else
        {
            // A zero-weight channel cell acts as a network endpoint/outlet.
            // The hydrograph routine can still record its outgoing flux, but
            // many such cells usually indicate channel gaps or DEM/channel
            // inconsistencies and should be inspected.
            if (sourceIsChannel)
                ++fr_channelDeadEnds;
            else
                ++fr_hillslopeCellsWithoutRoute;
        }
    }
}

// -----------------------------------------------------------------------------
// 3. Weight diagnostics
// -----------------------------------------------------------------------------
std::cout
    << "Flow-ratio diagnostics:\n"
    << "  valid source cells: " << fr_validCells << "\n"
    << "  hillslope source cells: " << fr_hillslopeCells << "\n"
    << "  channel source cells: " << fr_channelCells << "\n"
    << "  channel cells with eligible outflow: "
    << fr_channelCellsWithRoute << "\n"
    << "  channel endpoints/dead ends: " << fr_channelDeadEnds << "\n"
    << "  channel -> channel links: "
    << fr_channelToChannelConnections << "\n"
    << "  channel -> lake links: "
    << fr_channelToLakeConnections << "\n"
    << "  channel -> dam links: "
    << fr_channelToDamConnections << "\n"
    << "  rejected channel -> land links: "
    << fr_rejectedChannelToLandLinks << "\n"
    << "  hillslope cells without lower neighbour: "
    << fr_hillslopeCellsWithoutRoute
    << std::endl;

if (fr_channelCells > 0)
{
    const double deadEndFraction =
        static_cast<double>(fr_channelDeadEnds) /
        static_cast<double>(fr_channelCells);

    if (deadEndFraction > 0.05)
    {
        std::cerr
            << "WARNING: " << (100.0 * deadEndFraction)
            << "% of channel cells have no eligible lower channel/lake/dam "
               "target. Check channel continuity and DEM elevations."
            << std::endl;
    }
}
