{
// -----------------------------------------------------------------------------
// forest_subsurface_fast_v2.inc
//
// Fast forest-only subsurface module for daily runs with dynamicG == 0.
//
// Purpose
//   * retain fixed EHD and SD geometry;
//   * route mobile water laterally through SD;
//   * maintain a local, time-varying forest groundwater head;
//   * recalculate slopeG and compact eight-direction dG every day;
//   * route groundwater locally through connected forest cells to channels/lakes;
//   * avoid the full catchment-wide dynamic groundwater solver.
//
// Required companion soil balance:
//   soil_waterbalance_daily_v3_forest.inc
//
// State interpretation
//   forestGWStoreMM: drainable groundwater storage [mm water]
//   forestGWBaseHeadM: local lower/reference head [m elevation]
//   forestGWHeadM: current local groundwater head [m elevation]
//   G[][1]-G[][0]: temporary daily recharge carrier before this include runs
//
// dG uses compact direction order:
//   0 NW, 1 W, 2 SW, 3 N, 4 S, 5 NE, 6 E, 7 SE
// (the actual coordinate arrays below are the source of truth).
// -----------------------------------------------------------------------------

// --------------------------- calibration parameters --------------------------
const float forestSDLateralReleaseFraction       = 0.25f;  // fraction/day above FC
const float forestHorizontalKMultiplier          = 10.0f;  // multiplier on kG[0]
const float forestGWSpecificYield                = 0.20f;
const float forestGWResponseDepthM               = 2.0f;
const float forestGWMaxDailyHeadChangeM          = 0.25f;
const float forestGWDeepLeakageFraction          = 0.0f;   // optional deep loss/day
const float forestFluxEpsilonMM                  = 1.0e-6f;

static bool forestFastInitialised = false;
static std::vector<std::vector<float> > forestGWStoreMM;
static std::vector<std::vector<float> > forestGWBaseHeadM;
static std::vector<std::vector<float> > forestGWHeadM;
static std::vector<std::vector<float> > forestGWDeltaMM;
static std::vector<std::vector<float> > forestSDDeltaMM;
static std::ofstream forestFastDiagnostic;

const int forestGWDirX[8] = {-1, -1, -1,  0, 0,  1, 1, 1};
const int forestGWDirY[8] = {-1,  0,  1, -1, 1, -1, 0, 1};

auto forestIsActiveCell = [&](int xx, int yy) -> bool
{
    return xx > 0 && xx < xSize - 1 &&
           yy > 0 && yy < ySize - 1 &&
           dem[xx][yy] != noData &&
           landuse[xx][yy] == 30 &&
           !isChannelCell(xx, yy) &&
           lakes[xx][yy] == noData;
};

auto forestLakeHead = [&](int xx, int yy) -> float
{
    if (lakes[xx][yy] == noData)
        return noData;

    for (int lakeIndex = 0; lakeIndex < nrLakes; ++lakeIndex)
    {
        if (lakes[xx][yy] == lakeStats[lakeIndex][8])
            return lakeStats[lakeIndex][3];
    }

    return dem[xx][yy];
};

if (!forestFastInitialised)
{
    forestGWStoreMM.assign(xSize, std::vector<float>(ySize, 0.0f));
    forestGWBaseHeadM.assign(xSize, std::vector<float>(ySize, noData));
    forestGWHeadM.assign(xSize, std::vector<float>(ySize, noData));
    forestGWDeltaMM.assign(xSize, std::vector<float>(ySize, 0.0f));
    forestSDDeltaMM.assign(xSize, std::vector<float>(ySize, 0.0f));

    for (int x = 1; x < xSize - 1; ++x)
    {
        for (int y = 1; y < ySize - 1; ++y)
        {
            if (!forestIsActiveCell(x, y) || G[x][y][0] == noData)
                continue;

            const float pendingRechargeM = std::max(
                0.0f,
                G[x][y][1] - G[x][y][0]
            );

            float initialHeadM = G[x][y][0];
            if (initialHeadM > dem[x][y])
                initialHeadM = dem[x][y];

            // The response-depth head is a lower bound for shallow initial
            // groundwater. If initial groundwater is deeper, retain that deeper
            // initial head as the local base so initialization is exact.
            const float responseBaseHeadM =
                dem[x][y] - forestGWResponseDepthM;

            forestGWBaseHeadM[x][y] = std::min(
                initialHeadM,
                responseBaseHeadM
            );

            forestGWStoreMM[x][y] = std::max(
                0.0f,
                (initialHeadM - forestGWBaseHeadM[x][y]) *
                1000.0f * forestGWSpecificYield
            );

            forestGWHeadM[x][y] = initialHeadM;
            G[x][y][0] = initialHeadM;
            // Preserve recharge already added by the soil-water balance on
            // the first day before this module is initialized.
            G[x][y][1] = initialHeadM + pendingRechargeM;
        }
    }

    forestFastDiagnostic.open(
        ioFolder + "/forest_subsurface_fast_v2_" +
        std::to_string(scenario) + "_" + gid + ".csv"
    );

    if (forestFastDiagnostic.is_open())
    {
        forestFastDiagnostic
            << "date recharge_m3 sd_to_channel_m3 sd_to_lake_m3 "
            << "sd_land_transfer_m3 gw_land_transfer_m3 gw_to_channel_m3 "
            << "gw_to_lake_m3 gw_surface_exfiltration_m3 deep_loss_m3 "
            << "gw_store_m3 mean_head_m mean_dtg_m active_forest_cells "
            << "gw_routing_cells gw_dead_end_cells max_head_change_m "
            << "mass_residual_m3\n";
    }

    forestFastInitialised = true;
}

float forestRechargeM3 = 0.0f;
float forestSDToChannelM3 = 0.0f;
float forestSDToLakeM3 = 0.0f;
float forestSDLandTransferM3 = 0.0f;
float forestGWLandTransferM3 = 0.0f;
float forestGWToChannelM3 = 0.0f;
float forestGWToLakeM3 = 0.0f;
float forestGWSurfaceExfiltrationM3 = 0.0f;
float forestGWDeepLossM3 = 0.0f;
float forestStoreStartM3 = 0.0f;
float forestStoreEndM3 = 0.0f;
float forestHeadSumM = 0.0f;
float forestDtGSumM = 0.0f;
float forestMaxHeadChangeM = 0.0f;
long long forestActiveCells = 0;
long long forestGWRoutingCells = 0;
long long forestGWDeadEndCells = 0;

// Reset daily buffers and record storage before today's recharge.
for (int x = 0; x < xSize; ++x)
{
    for (int y = 0; y < ySize; ++y)
    {
        forestSDDeltaMM[x][y] = 0.0f;
        forestGWDeltaMM[x][y] = 0.0f;

        for (std::size_t dir = 0; dir < dG[x][y].size(); ++dir)
            dG[x][y][dir] = 0.0f;

        slopeG[x][y] = 0.0f;

        if (forestIsActiveCell(x, y))
        {
            forestStoreStartM3 +=
                forestGWStoreMM[x][y] * dx * dy / 1000.0f;
        }
    }
}

// -----------------------------------------------------------------------------
// 1. Recover today's forest recharge from G[][1]-G[][0].
// -----------------------------------------------------------------------------
for (int x = 1; x < xSize - 1; ++x)
{
    for (int y = 1; y < ySize - 1; ++y)
    {
        if (!forestIsActiveCell(x, y))
            continue;

        ++forestActiveCells;

        float rechargeMM = (G[x][y][1] - G[x][y][0]) * 1000.0f;
        if (rechargeMM < 0.0f || std::isnan(rechargeMM) || std::isinf(rechargeMM))
            rechargeMM = 0.0f;

        forestGWStoreMM[x][y] += rechargeMM;
        forestRechargeM3 += rechargeMM * dx * dy / 1000.0f;
    }
}

// -----------------------------------------------------------------------------
// 2. Fast lateral interflow through the lower soil profile (SD).
//    Source cells are forest; receiver cells may be any valid land cell.
// -----------------------------------------------------------------------------
if (tm[index] > drainageTempSD)
{
    for (int x = 1; x < xSize - 1; ++x)
    {
        for (int y = 1; y < ySize - 1; ++y)
        {
            if (dem[x][y] == noData ||
                landuse[x][y] != 30 ||
                isChannelCell(x, y) ||
                lakes[x][y] != noData ||
                SD[x][y] <= 0.0f)
            {
                continue;
            }

            const float mobileWaterMM = std::max(
                0.0f,
                SW[x][y][0] - FC[x][y] * SD[x][y]
            );

            const float sourceReleaseMM =
                mobileWaterMM * forestSDLateralReleaseFraction;

            if (sourceReleaseMM <= forestFluxEpsilonMM)
                continue;

            float remainingSourceMM = sourceReleaseMM;

            // Renormalise only over destinations that can actually accept flow.
            float eligibleWeightSum = 0.0f;
            for (int X = -1; X <= 1; ++X)
            {
                for (int Y = -1; Y <= 1; ++Y)
                {
                    if (X == 0 && Y == 0)
                        continue;

                    const int nx = x + X;
                    const int ny = y + Y;
                    const int wi = (X + 1) * 3 + (Y + 1);

                    if (dem[nx][ny] == noData || d[x][y][wi] <= 0.0f)
                        continue;

                    if (isChannelCell(nx, ny) ||
                        lakes[nx][ny] != noData ||
                        SD[nx][ny] > 0.0f)
                    {
                        eligibleWeightSum += d[x][y][wi];
                    }
                }
            }

            if (eligibleWeightSum <= 0.0f)
                continue;

            for (int X = -1; X <= 1; ++X)
            {
                for (int Y = -1; Y <= 1; ++Y)
                {
                    if (X == 0 && Y == 0)
                        continue;

                    const int nx = x + X;
                    const int ny = y + Y;
                    const int wi = (X + 1) * 3 + (Y + 1);
                    const float originalWeight = d[x][y][wi];

                    if (dem[nx][ny] == noData ||
                        originalWeight <= 0.0f ||
                        remainingSourceMM <= forestFluxEpsilonMM)
                    {
                        continue;
                    }

                    const float normalisedWeight =
                        originalWeight / eligibleWeightSum;

                    float proposedMM = sourceReleaseMM * normalisedWeight;
                    if (proposedMM > remainingSourceMM)
                        proposedMM = remainingSourceMM;

                    float acceptedMM = 0.0f;

                    if (isChannelCell(nx, ny))
                    {
                        acceptedMM = proposedMM;
                        dsc[nx][ny] += acceptedMM;

                        const float volumeM3 =
                            acceptedMM * dx * dy / 1000.0f;

                        Q_stream += volumeM3;
                        forestSDToChannelM3 += volumeM3;
                    }
                    else if (lakes[nx][ny] != noData)
                    {
                        int locLake = -1;
                        for (int lakeIndex = 0; lakeIndex < nrLakes; ++lakeIndex)
                        {
                            if (lakes[nx][ny] == lakeStats[lakeIndex][8])
                            {
                                locLake = lakeIndex;
                                break;
                            }
                        }

                        if (locLake >= 0 && lakeStats[locLake][0] > 0.0f)
                        {
                            acceptedMM = proposedMM;
                            const float volumeM3 =
                                acceptedMM * dx * dy / 1000.0f;

                            lakeStats[locLake][7] +=
                                volumeM3 / lakeStats[locLake][0];

                            forestSDToLakeM3 += volumeM3;
                        }
                    }
                    else if (SD[nx][ny] > 0.0f)
                    {
                        float receiverCapacityMM =
                            SWSat[nx][ny] * SD[nx][ny] -
                            (SW[nx][ny][0] + forestSDDeltaMM[nx][ny]);

                        if (receiverCapacityMM > 0.0f)
                        {
                            acceptedMM = std::min(proposedMM, receiverCapacityMM);
                            forestSDDeltaMM[nx][ny] += acceptedMM;
                            forestSDLandTransferM3 +=
                                acceptedMM * dx * dy / 1000.0f;
                        }
                    }

                    if (acceptedMM > 0.0f)
                    {
                        forestSDDeltaMM[x][y] -= acceptedMM;
                        remainingSourceMM -= acceptedMM;
                    }
                }
            }
        }
    }
}

// Apply SD transfers simultaneously.
for (int x = 1; x < xSize - 1; ++x)
{
    for (int y = 1; y < ySize - 1; ++y)
    {
        if (dem[x][y] == noData || SD[x][y] <= 0.0f)
            continue;

        SW[x][y][0] += forestSDDeltaMM[x][y];

        if (SW[x][y][0] < 0.0f)
            SW[x][y][0] = 0.0f;

        const float saturationStorageMM = SWSat[x][y] * SD[x][y];
        if (SW[x][y][0] > saturationStorageMM)
        {
            const float saturationExcessMM =
                SW[x][y][0] - saturationStorageMM;

            SW[x][y][0] = saturationStorageMM;
            dsc[x][y] += saturationExcessMM;
        }

        SW[x][y][1] = SW[x][y][0];
    }
}


// -----------------------------------------------------------------------------
// 3. Build a current local groundwater-head field from the conceptual store.
//    Water that would raise the head above the surface becomes saturation excess.
// -----------------------------------------------------------------------------
for (int x = 0; x < xSize; ++x)
{
    for (int y = 0; y < ySize; ++y)
    {
        if (dem[x][y] == noData)
        {
            forestGWHeadM[x][y] = noData;
            continue;
        }

        if (isChannelCell(x, y))
        {
            forestGWHeadM[x][y] = dem[x][y];
            continue;
        }

        if (lakes[x][y] != noData)
        {
            forestGWHeadM[x][y] = forestLakeHead(x, y);
            continue;
        }

        if (!forestIsActiveCell(x, y))
        {
            forestGWHeadM[x][y] = G[x][y][0];
            continue;
        }

        const float maximumStoreMM = static_cast<float>(
            std::max(
                0.0,
                (static_cast<double>(dem[x][y]) -
                 forestGWBaseHeadM[x][y]) *
                1000.0 *
                static_cast<double>(forestGWSpecificYield)
            )
        );

        if (forestGWStoreMM[x][y] > maximumStoreMM)
        {
            const float exfiltrationMM =
                forestGWStoreMM[x][y] - maximumStoreMM;

            forestGWStoreMM[x][y] = maximumStoreMM;
            dsc[x][y] += exfiltrationMM;

            forestGWSurfaceExfiltrationM3 +=
                exfiltrationMM * dx * dy / 1000.0f;
        }

        forestGWHeadM[x][y] =
            forestGWBaseHeadM[x][y] +
            forestGWStoreMM[x][y] /
            (1000.0f * forestGWSpecificYield);

        if (forestGWHeadM[x][y] > dem[x][y])
            forestGWHeadM[x][y] = dem[x][y];
    }
}

// -----------------------------------------------------------------------------
// 4. Recalculate local, time-specific slopeG and dG for forest groundwater.
//    Only connected forest cells, channels and lakes are eligible destinations.
// -----------------------------------------------------------------------------
for (int x = 1; x < xSize - 1; ++x)
{
    for (int y = 1; y < ySize - 1; ++y)
    {
        if (!forestIsActiveCell(x, y))
            continue;

        float localGradient[8] = {0.0f};
        float gradientSum = 0.0f;
        float gradientSquaredSum = 0.0f;

        for (int dir = 0; dir < 8; ++dir)
        {
            const int nx = x + forestGWDirX[dir];
            const int ny = y + forestGWDirY[dir];

            const bool eligibleTarget =
                forestIsActiveCell(nx, ny) ||
                isChannelCell(nx, ny) ||
                lakes[nx][ny] != noData;

            if (!eligibleTarget ||
                forestGWHeadM[nx][ny] == noData ||
                forestGWHeadM[nx][ny] >= forestGWHeadM[x][y])
            {
                continue;
            }

            const float distanceM = std::sqrt(
                (forestGWDirX[dir] * dx) * (forestGWDirX[dir] * dx) +
                (forestGWDirY[dir] * dy) * (forestGWDirY[dir] * dy)
            );

            if (distanceM <= 0.0f)
                continue;

            localGradient[dir] =
                (forestGWHeadM[x][y] - forestGWHeadM[nx][ny]) /
                distanceM;

            gradientSum += localGradient[dir];
            gradientSquaredSum +=
                localGradient[dir] * localGradient[dir];
        }

        if (gradientSum > 0.0f)
        {
            const int usableDirections =
                dG[x][y].size() < 8
                ? static_cast<int>(dG[x][y].size())
                : 8;

            for (int dir = 0; dir < usableDirections; ++dir)
            {
                if (localGradient[dir] > 0.0f)
                    dG[x][y][dir] = localGradient[dir] / gradientSum;
            }

            // Gradient-weighted mean of the available downslope gradients.
            // This remains consistent with the same destinations used by dG.
            const float effectiveGradient =
                gradientSquaredSum / gradientSum;

            slopeG[x][y] = std::atan(effectiveGradient);
            ++forestGWRoutingCells;
        }
        else
        {
            ++forestGWDeadEndCells;
        }
    }
}

// -----------------------------------------------------------------------------
// 5. Route groundwater from the frozen daily head field.
//    The same-day transfers are buffered and applied simultaneously.
// -----------------------------------------------------------------------------
if (tm[index] > drainageTempSD)
{
    const float localForestKG =
        std::max(0.0f, kG[0] * forestHorizontalKMultiplier);

    for (int x = 1; x < xSize - 1; ++x)
    {
        for (int y = 1; y < ySize - 1; ++y)
        {
            if (!forestIsActiveCell(x, y) ||
                forestGWStoreMM[x][y] <= forestFluxEpsilonMM)
            {
                continue;
            }

            float potentialHeadChangeM =
                localForestKG * std::sin(slopeG[x][y]);

            if (potentialHeadChangeM < 0.0f)
                potentialHeadChangeM = 0.0f;
            if (potentialHeadChangeM > forestGWMaxDailyHeadChangeM)
                potentialHeadChangeM = forestGWMaxDailyHeadChangeM;

            if (potentialHeadChangeM > forestMaxHeadChangeM)
                forestMaxHeadChangeM = potentialHeadChangeM;

            float sourceOutflowMM =
                potentialHeadChangeM *
                1000.0f * forestGWSpecificYield;

            sourceOutflowMM = std::min(
                sourceOutflowMM,
                forestGWStoreMM[x][y]
            );

            if (sourceOutflowMM <= forestFluxEpsilonMM)
                continue;

            float remainingSourceMM = sourceOutflowMM;
            const int usableDirections =
                dG[x][y].size() < 8
                ? static_cast<int>(dG[x][y].size())
                : 8;

            for (int dir = 0; dir < usableDirections; ++dir)
            {
                if (remainingSourceMM <= forestFluxEpsilonMM)
                    break;

                const float weight = dG[x][y][dir];
                if (weight <= 0.0f)
                    continue;

                const int nx = x + forestGWDirX[dir];
                const int ny = y + forestGWDirY[dir];

                float proposedMM = sourceOutflowMM * weight;
                if (proposedMM > remainingSourceMM)
                    proposedMM = remainingSourceMM;

                float acceptedMM = 0.0f;

                if (isChannelCell(nx, ny))
                {
                    acceptedMM = proposedMM;
                    dsc[nx][ny] += acceptedMM;

                    const float volumeM3 =
                        acceptedMM * dx * dy / 1000.0f;

                    Q_stream += volumeM3;
                    forestGWToChannelM3 += volumeM3;
                }
                else if (lakes[nx][ny] != noData)
                {
                    int locLake = -1;
                    for (int lakeIndex = 0; lakeIndex < nrLakes; ++lakeIndex)
                    {
                        if (lakes[nx][ny] == lakeStats[lakeIndex][8])
                        {
                            locLake = lakeIndex;
                            break;
                        }
                    }

                    if (locLake >= 0 && lakeStats[locLake][0] > 0.0f)
                    {
                        acceptedMM = proposedMM;
                        const float volumeM3 =
                            acceptedMM * dx * dy / 1000.0f;

                        lakeStats[locLake][7] +=
                            volumeM3 / lakeStats[locLake][0];

                        forestGWToLakeM3 += volumeM3;
                    }
                }
                else if (forestIsActiveCell(nx, ny))
                {
                    const float maximumTargetStoreMM = static_cast<float>(
                        std::max(
                            0.0,
                            (static_cast<double>(dem[nx][ny]) -
                             forestGWBaseHeadM[nx][ny]) *
                            1000.0 *
                            static_cast<double>(forestGWSpecificYield)
                        )
                    );

                    const float projectedTargetStoreMM =
                        forestGWStoreMM[nx][ny] +
                        forestGWDeltaMM[nx][ny];

                    const float targetCapacityMM = std::max(
                        0.0f,
                        maximumTargetStoreMM - projectedTargetStoreMM
                    );

                    // For equal cell areas and equal specific yield, half the
                    // frozen head difference is the maximum pairwise transfer
                    // before the two heads would cross.
                    const float equalisationLimitMM = std::max(
                        0.0f,
                        0.5f *
                        (forestGWHeadM[x][y] - forestGWHeadM[nx][ny]) *
                        1000.0f * forestGWSpecificYield
                    );

                    acceptedMM = std::min(
                        proposedMM,
                        std::min(targetCapacityMM, equalisationLimitMM)
                    );

                    if (acceptedMM > 0.0f)
                    {
                        forestGWDeltaMM[nx][ny] += acceptedMM;
                        forestGWLandTransferM3 +=
                            acceptedMM * dx * dy / 1000.0f;
                    }
                }

                if (acceptedMM > 0.0f)
                {
                    forestGWDeltaMM[x][y] -= acceptedMM;
                    remainingSourceMM -= acceptedMM;
                }
            }
        }
    }
}

// Optional very slow deep loss from the local groundwater store.
if (forestGWDeepLeakageFraction > 0.0f)
{
    for (int x = 1; x < xSize - 1; ++x)
    {
        for (int y = 1; y < ySize - 1; ++y)
        {
            if (!forestIsActiveCell(x, y))
                continue;

            const float projectedStoreMM = std::max(
                0.0f,
                forestGWStoreMM[x][y] + forestGWDeltaMM[x][y]
            );

            const float deepLossMM = std::min(
                projectedStoreMM,
                projectedStoreMM * forestGWDeepLeakageFraction
            );

            forestGWDeltaMM[x][y] -= deepLossMM;
            forestGWDeepLossM3 +=
                deepLossMM * dx * dy / 1000.0f;
        }
    }
}

// -----------------------------------------------------------------------------
// 6. Apply groundwater transfers, update local heads, and synchronize G.
// -----------------------------------------------------------------------------
for (int x = 1; x < xSize - 1; ++x)
{
    for (int y = 1; y < ySize - 1; ++y)
    {
        if (!forestIsActiveCell(x, y))
            continue;

        forestGWStoreMM[x][y] += forestGWDeltaMM[x][y];
        if (forestGWStoreMM[x][y] < 0.0f)
            forestGWStoreMM[x][y] = 0.0f;

        const float maximumStoreMM = static_cast<float>(
            std::max(
                0.0,
                (static_cast<double>(dem[x][y]) -
                 forestGWBaseHeadM[x][y]) *
                1000.0 *
                static_cast<double>(forestGWSpecificYield)
            )
        );

        if (forestGWStoreMM[x][y] > maximumStoreMM)
        {
            const float exfiltrationMM =
                forestGWStoreMM[x][y] - maximumStoreMM;

            forestGWStoreMM[x][y] = maximumStoreMM;
            dsc[x][y] += exfiltrationMM;

            forestGWSurfaceExfiltrationM3 +=
                exfiltrationMM * dx * dy / 1000.0f;
        }

        forestGWHeadM[x][y] =
            forestGWBaseHeadM[x][y] +
            forestGWStoreMM[x][y] /
            (1000.0f * forestGWSpecificYield);

        if (forestGWHeadM[x][y] > dem[x][y])
            forestGWHeadM[x][y] = dem[x][y];

        G[x][y][0] = forestGWHeadM[x][y];
        G[x][y][1] = forestGWHeadM[x][y];

        forestStoreEndM3 +=
            forestGWStoreMM[x][y] * dx * dy / 1000.0f;
        forestHeadSumM += forestGWHeadM[x][y];
        forestDtGSumM += dem[x][y] - forestGWHeadM[x][y];
    }
}

const float forestMassResidualM3 =
    forestStoreStartM3 + forestRechargeM3 -
    forestGWToChannelM3 - forestGWToLakeM3 -
    forestGWSurfaceExfiltrationM3 - forestGWDeepLossM3 -
    forestStoreEndM3;

if (forestFastDiagnostic.is_open())
{
    const float meanHeadM =
        forestActiveCells > 0
        ? forestHeadSumM / static_cast<float>(forestActiveCells)
        : 0.0f;

    const float meanDtGM =
        forestActiveCells > 0
        ? forestDtGSumM / static_cast<float>(forestActiveCells)
        : 0.0f;

    forestFastDiagnostic
        << year[index] << "-" << month[index] << "-" << day[index] << " "
        << forestRechargeM3 << " "
        << forestSDToChannelM3 << " "
        << forestSDToLakeM3 << " "
        << forestSDLandTransferM3 << " "
        << forestGWLandTransferM3 << " "
        << forestGWToChannelM3 << " "
        << forestGWToLakeM3 << " "
        << forestGWSurfaceExfiltrationM3 << " "
        << forestGWDeepLossM3 << " "
        << forestStoreEndM3 << " "
        << meanHeadM << " "
        << meanDtGM << " "
        << forestActiveCells << " "
        << forestGWRoutingCells << " "
        << forestGWDeadEndCells << " "
        << forestMaxHeadChangeM << " "
        << forestMassResidualM3
        << "\n";
}

} // end forest_subsurface_fast_v2.inc
