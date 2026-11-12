{
// -----------------------------------------------------------------------------
// forest_mire_soilflow_v1.inc
//
// Standalone daily subsurface-flow module for forest (land-use 30) and mire
// (land-use 60). It runs irrespective of dynamicG and never uses drainRoute.
//
// Required execution order:
//   1. soil_waterbalance_daily_v2_forest_mire_handoff.inc
//   2. optional non-forest dynamic groundwater routine
//   3. this include
//   4. flacc_DAG.inc
//
// The module represents one continuous forest/mire subsurface process in two
// physical zones:
//   * unsaturated SD water above field capacity;
//   * a persistent saturated store with a local, time-varying head.
//
// EHD is deliberately not routed laterally in this first version. The ordinary
// soil-water balance handles rain, ET, EHD storage and EHD -> SD percolation.
//
// Units:
//   SW, SD, saturated store, dsc: mm water depth
//   head, DEM, G: m elevation
//   Q_stream and lake additions: m3/day
// -----------------------------------------------------------------------------

// ---------------------------- calibration parameters -------------------------
const float fmSpecificYield                 = 0.20f;
const float fmResponseDepthM                = 2.00f;
const float fmVerticalMultiplier            = 0.10f; // low SD -> saturated flow
const float fmUnsatLateralMultiplier        = 10.0f; // high lateral SD flow
const float fmSaturatedLateralMultiplier    = 10.0f; // high saturated conductivity
const float fmFluxEpsilonMM                 = 1.0e-7f;
// -----------------------------------------------------------------------------

auto fmIsCell = [&](int xx, int yy) -> bool
{
    return xx >= 0 && xx < xSize &&
           yy >= 0 && yy < ySize &&
           dem[xx][yy] != noData &&
           !isChannelCell(xx, yy) &&
           lakes[xx][yy] == noData &&
           (landuse[xx][yy] == 30 || landuse[xx][yy] == 60);
};

auto fmClamp01 = [](float value) -> float
{
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
};

auto fmLakeIndex = [&](int xx, int yy) -> int
{
    if (xx < 0 || xx >= xSize || yy < 0 || yy >= ySize ||
        lakes[xx][yy] == noData)
        return -1;

    for (int lakeIndex = 0; lakeIndex < nrLakes; ++lakeIndex)
    {
        if (lakes[xx][yy] == lakeStats[lakeIndex][8])
            return lakeIndex;
    }
    return -1;
};

static bool fmInitialised = false;
static std::vector<std::vector<double>> fmBaseHeadM;
static std::vector<std::vector<double>> fmHeadStartM;
static std::vector<std::vector<float>> fmSatStoreMM;
static std::vector<std::vector<float>> fmUnsatDeltaMM;
static std::vector<std::vector<float>> fmSatRechargeMM;
static std::vector<std::vector<float>> fmSatRoutingDeltaMM;
static std::vector<std::vector<float>> fmSatIncomingReservedMM;
static std::ofstream fmDiagnostic;

if (!fmInitialised)
{
    fmBaseHeadM.assign(xSize, std::vector<double>(ySize, noData));
    fmHeadStartM.assign(xSize, std::vector<double>(ySize, noData));
    fmSatStoreMM.assign(xSize, std::vector<float>(ySize, 0.0f));
    fmUnsatDeltaMM.assign(xSize, std::vector<float>(ySize, 0.0f));
    fmSatRechargeMM.assign(xSize, std::vector<float>(ySize, 0.0f));
    fmSatRoutingDeltaMM.assign(xSize, std::vector<float>(ySize, 0.0f));
    fmSatIncomingReservedMM.assign(xSize, std::vector<float>(ySize, 0.0f));

    for (int x = 0; x < xSize; ++x)
    {
        for (int y = 0; y < ySize; ++y)
        {
            if (!fmIsCell(x, y))
                continue;

            double initialHeadM = static_cast<double>(G[x][y][0]);
            const double surfaceM = static_cast<double>(dem[x][y]);

            if (initialHeadM != initialHeadM ||
                initialHeadM > 1.0e30 || initialHeadM < -1.0e30)
            {
                initialHeadM = surfaceM - static_cast<double>(fmResponseDepthM);
            }

            if (initialHeadM > surfaceM)
                initialHeadM = surfaceM;

            const double responseBaseM =
                surfaceM - static_cast<double>(fmResponseDepthM);

            // Deep initial heads define their own zero-storage base. Shallow
            // initial heads retain physically consistent initial storage.
            fmBaseHeadM[x][y] =
                (initialHeadM < responseBaseM) ? initialHeadM : responseBaseM;

            const double initialStorageMM =
                (initialHeadM - fmBaseHeadM[x][y]) *
                1000.0 * static_cast<double>(fmSpecificYield);

            fmSatStoreMM[x][y] = static_cast<float>(
                initialStorageMM > 0.0 ? initialStorageMM : 0.0
            );
        }
    }

    fmDiagnostic.open(
        ioFolder + "/forest_mire_soilflow_" +
        std::to_string(scenario) + "_" + gid + ".csv"
    );

    if (fmDiagnostic.is_open())
    {
        fmDiagnostic
            << "date unsat_vertical_m3 unsat_land_m3 unsat_channel_m3 "
            << "unsat_lake_m3 sat_land_m3 sat_channel_m3 sat_lake_m3 "
            << "exfiltration_m3 unsat_store_m3 sat_store_m3 mean_head_m "
            << "mean_dtg_m active_cells unsat_deadends sat_deadends "
            << "mass_residual_m3\n";
    }

    fmInitialised = true;
}

// Daily diagnostics and mass-balance terms.
double fmStoreBeforeM3 = 0.0;
double fmUnsatVerticalM3 = 0.0;
double fmUnsatLandM3 = 0.0;
double fmUnsatChannelM3 = 0.0;
double fmUnsatLakeM3 = 0.0;
double fmSatLandM3 = 0.0;
double fmSatChannelM3 = 0.0;
double fmSatLakeM3 = 0.0;
double fmExfiltrationM3 = 0.0;
long long fmActiveCells = 0;
long long fmUnsatDeadEnds = 0;
long long fmSatDeadEnds = 0;

for (int x = 0; x < xSize; ++x)
{
    for (int y = 0; y < ySize; ++y)
    {
        fmUnsatDeltaMM[x][y] = 0.0f;
        fmSatRechargeMM[x][y] = 0.0f;
        fmSatRoutingDeltaMM[x][y] = 0.0f;
        fmSatIncomingReservedMM[x][y] = 0.0f;

        if (fmIsCell(x, y))
        {
            fmStoreBeforeM3 +=
                (static_cast<double>(SW[x][y][0]) +
                 static_cast<double>(fmSatStoreMM[x][y])) *
                dx * dy / 1000.0;
        }
    }
}

// -----------------------------------------------------------------------------
// A. Unsaturated SD: low vertical transfer and fast lateral redistribution.
//    All fluxes are calculated from the frozen start-of-module SW state.
// -----------------------------------------------------------------------------
if (tm[index] > drainageTempSD)
{
    for (int x = 1; x < xSize - 1; ++x)
    {
        for (int y = 1; y < ySize - 1; ++y)
        {
            if (!fmIsCell(x, y) || SD[x][y] <= 0.0f)
                continue;

            ++fmActiveCells;

            const float fieldCapacityStorageMM = FC[x][y] * SD[x][y];
            const float mobileStartMM =
                (SW[x][y][0] > fieldCapacityStorageMM)
                ? SW[x][y][0] - fieldCapacityStorageMM
                : 0.0f;

            if (mobileStartMM <= fmFluxEpsilonMM)
                continue;

            // Low vertical conductivity: only a small fraction of mobile SD
            // water becomes saturated-zone recharge each day.
            const float verticalFraction = fmClamp01(
                drainageRate2[x][y] * fmVerticalMultiplier
            );
            const float verticalMM = mobileStartMM * verticalFraction;

            if (verticalMM > fmFluxEpsilonMM)
            {
                fmUnsatDeltaMM[x][y] -= verticalMM;
                fmSatRechargeMM[x][y] += verticalMM;
                fmUnsatVerticalM3 += verticalMM * dx * dy / 1000.0;
            }

            const float mobileAfterVerticalMM = mobileStartMM - verticalMM;
            const float lateralFraction = fmClamp01(
                drainageRate2[x][y] * fmUnsatLateralMultiplier
            );
            float sourceReleaseMM = mobileAfterVerticalMM * lateralFraction;

            if (sourceReleaseMM <= fmFluxEpsilonMM)
                continue;

            // Renormalise existing terrain-routing weights over forest/mire,
            // channel and lake targets only.
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

                    if (nx < 0 || nx >= xSize || ny < 0 || ny >= ySize ||
                        wi < 0 || wi >= static_cast<int>(d[x][y].size()) ||
                        d[x][y][wi] <= 0.0f || dem[nx][ny] == noData)
                        continue;

                    if (fmIsCell(nx, ny) ||
                        isChannelCell(nx, ny) ||
                        lakes[nx][ny] != noData)
                    {
                        eligibleWeightSum += d[x][y][wi];
                    }
                }
            }

            if (eligibleWeightSum <= 0.0f)
            {
                ++fmUnsatDeadEnds;
                continue;
            }

            float remainingSourceMM = sourceReleaseMM;

            for (int X = -1; X <= 1; ++X)
            {
                for (int Y = -1; Y <= 1; ++Y)
                {
                    if (X == 0 && Y == 0)
                        continue;

                    const int nx = x + X;
                    const int ny = y + Y;
                    const int wi = (X + 1) * 3 + (Y + 1);

                    if (nx < 0 || nx >= xSize || ny < 0 || ny >= ySize ||
                        wi < 0 || wi >= static_cast<int>(d[x][y].size()) ||
                        d[x][y][wi] <= 0.0f || dem[nx][ny] == noData ||
                        remainingSourceMM <= fmFluxEpsilonMM)
                        continue;

                    if (!(fmIsCell(nx, ny) ||
                          isChannelCell(nx, ny) ||
                          lakes[nx][ny] != noData))
                        continue;

                    float proposedMM = sourceReleaseMM *
                        d[x][y][wi] / eligibleWeightSum;
                    if (proposedMM > remainingSourceMM)
                        proposedMM = remainingSourceMM;

                    float acceptedMM = 0.0f;

                    if (isChannelCell(nx, ny))
                    {
                        acceptedMM = proposedMM;
                        dsc[nx][ny] += acceptedMM;
                        const double volumeM3 = acceptedMM * dx * dy / 1000.0;
                        Q_stream += static_cast<float>(volumeM3);
                        fmUnsatChannelM3 += volumeM3;
                    }
                    else if (lakes[nx][ny] != noData)
                    {
                        const int locLake = fmLakeIndex(nx, ny);
                        if (locLake >= 0 && lakeStats[locLake][0] > 0.0f)
                        {
                            acceptedMM = proposedMM;
                            const double volumeM3 = acceptedMM * dx * dy / 1000.0;
                            lakeStats[locLake][7] +=
                                static_cast<float>(volumeM3 / lakeStats[locLake][0]);
                            fmUnsatLakeM3 += volumeM3;
                        }
                    }
                    else if (fmIsCell(nx, ny) && SD[nx][ny] > 0.0f)
                    {
                        const float receiverCapacityMM =
                            SWSat[nx][ny] * SD[nx][ny] -
                            (SW[nx][ny][0] + fmUnsatDeltaMM[nx][ny]);

                        if (receiverCapacityMM > 0.0f)
                        {
                            acceptedMM =
                                (proposedMM < receiverCapacityMM)
                                ? proposedMM : receiverCapacityMM;
                            fmUnsatDeltaMM[nx][ny] += acceptedMM;
                            fmUnsatLandM3 += acceptedMM * dx * dy / 1000.0;
                        }
                    }

                    if (acceptedMM > 0.0f)
                    {
                        fmUnsatDeltaMM[x][y] -= acceptedMM;
                        remainingSourceMM -= acceptedMM;
                    }
                }
            }
        }
    }
}

// Apply unsaturated redistribution and vertical recharge simultaneously.
for (int x = 0; x < xSize; ++x)
{
    for (int y = 0; y < ySize; ++y)
    {
        if (!fmIsCell(x, y))
            continue;

        SW[x][y][0] += fmUnsatDeltaMM[x][y];
        if (SW[x][y][0] < 0.0f)
            SW[x][y][0] = 0.0f;

        const float capacityMM =
            (SD[x][y] > 0.0f) ? SWSat[x][y] * SD[x][y] : 0.0f;
        if (SW[x][y][0] > capacityMM)
            SW[x][y][0] = capacityMM;

        SW[x][y][1] = SW[x][y][0];
        fmSatStoreMM[x][y] += fmSatRechargeMM[x][y];
    }
}

// -----------------------------------------------------------------------------
// B. Saturated zone: reconstruct local daily head and dG, then route laterally.
// -----------------------------------------------------------------------------
for (int x = 0; x < xSize; ++x)
{
    for (int y = 0; y < ySize; ++y)
    {
        fmHeadStartM[x][y] = noData;

        if (!fmIsCell(x, y))
            continue;

        fmHeadStartM[x][y] = fmBaseHeadM[x][y] +
            static_cast<double>(fmSatStoreMM[x][y]) /
            (1000.0 * static_cast<double>(fmSpecificYield));

        slopeG[x][y] = 0.0f;
        for (std::size_t i = 0; i < dG[x][y].size(); ++i)
            dG[x][y][i] = 0.0f;
    }
}

const int fmDirX[8] = {-1, -1, -1,  0, 0,  1, 1, 1};
const int fmDirY[8] = {-1,  0,  1, -1, 1, -1, 0, 1};

for (int x = 1; x < xSize - 1; ++x)
{
    for (int y = 1; y < ySize - 1; ++y)
    {
        if (!fmIsCell(x, y))
            continue;

        const double sourceHeadM = fmHeadStartM[x][y];
        float localGradient[8] = {0.0f};
        float gradientSum = 0.0f;

        for (int dir = 0; dir < 8; ++dir)
        {
            const int nx = x + fmDirX[dir];
            const int ny = y + fmDirY[dir];

            double targetHeadM = sourceHeadM;
            bool eligible = false;

            if (fmIsCell(nx, ny))
            {
                targetHeadM = fmHeadStartM[nx][ny];
                eligible = true;
            }
            else if (isChannelCell(nx, ny))
            {
                targetHeadM = static_cast<double>(dem[nx][ny]);
                eligible = true;
            }
            else if (lakes[nx][ny] != noData)
            {
                const int locLake = fmLakeIndex(nx, ny);
                if (locLake >= 0)
                {
                    targetHeadM = static_cast<double>(lakeStats[locLake][3]);
                    eligible = true;
                }
            }

            if (!eligible || targetHeadM >= sourceHeadM)
                continue;

            const double distanceM = std::sqrt(
                (fmDirX[dir] * dx) * (fmDirX[dir] * dx) +
                (fmDirY[dir] * dy) * (fmDirY[dir] * dy)
            );

            if (distanceM > 0.0)
            {
                localGradient[dir] = static_cast<float>(
                    (sourceHeadM - targetHeadM) / distanceM
                );
                gradientSum += localGradient[dir];
            }
        }

        if (gradientSum <= 0.0f)
        {
            if (fmSatStoreMM[x][y] > fmFluxEpsilonMM)
                ++fmSatDeadEnds;
            continue;
        }

        const int usableDirections =
            dG[x][y].size() < 8
            ? static_cast<int>(dG[x][y].size()) : 8;

        for (int dir = 0; dir < usableDirections; ++dir)
        {
            if (localGradient[dir] > 0.0f)
                dG[x][y][dir] = localGradient[dir] / gradientSum;
        }

        // Local gradient magnitude represented by the steepest eligible head
        // gradient; dG carries the directional partition.
        float maxGradient = 0.0f;
        for (int dir = 0; dir < 8; ++dir)
            if (localGradient[dir] > maxGradient)
                maxGradient = localGradient[dir];

        slopeG[x][y] = std::atan(maxGradient);
    }
}

// Calculate saturated transfers from the frozen post-recharge head field.
for (int x = 1; x < xSize - 1; ++x)
{
    for (int y = 1; y < ySize - 1; ++y)
    {
        if (!fmIsCell(x, y) || fmSatStoreMM[x][y] <= fmFluxEpsilonMM)
            continue;

        const float potentialHeadChangeM =
            kG[0] * fmSaturatedLateralMultiplier * std::sin(slopeG[x][y]);

        float potentialOutMM = potentialHeadChangeM *
            1000.0f * fmSpecificYield;
        if (potentialOutMM > fmSatStoreMM[x][y])
            potentialOutMM = fmSatStoreMM[x][y];

        if (potentialOutMM <= fmFluxEpsilonMM)
            continue;

        float remainingSourceMM = potentialOutMM;
        const int usableDirections =
            dG[x][y].size() < 8
            ? static_cast<int>(dG[x][y].size()) : 8;

        for (int dir = 0; dir < usableDirections; ++dir)
        {
            if (dG[x][y][dir] <= 0.0f ||
                remainingSourceMM <= fmFluxEpsilonMM)
                continue;

            const int nx = x + fmDirX[dir];
            const int ny = y + fmDirY[dir];
            const double sourceHeadM = fmHeadStartM[x][y];

            double targetHeadM = sourceHeadM;
            bool fixedHeadTarget = false;
            bool eligible = false;

            if (fmIsCell(nx, ny))
            {
                targetHeadM = fmHeadStartM[nx][ny];
                eligible = true;
            }
            else if (isChannelCell(nx, ny))
            {
                targetHeadM = static_cast<double>(dem[nx][ny]);
                fixedHeadTarget = true;
                eligible = true;
            }
            else if (lakes[nx][ny] != noData)
            {
                const int locLake = fmLakeIndex(nx, ny);
                if (locLake >= 0)
                {
                    targetHeadM = static_cast<double>(lakeStats[locLake][3]);
                    fixedHeadTarget = true;
                    eligible = true;
                }
            }

            if (!eligible || targetHeadM >= sourceHeadM)
                continue;

            float proposedMM = potentialOutMM * dG[x][y][dir];
            if (proposedMM > remainingSourceMM)
                proposedMM = remainingSourceMM;

            float equalisationLimitMM = static_cast<float>(
                (sourceHeadM - targetHeadM) *
                1000.0 * static_cast<double>(fmSpecificYield)
            );
            if (!fixedHeadTarget)
                equalisationLimitMM *= 0.5f;

            float acceptedMM =
                (proposedMM < equalisationLimitMM)
                ? proposedMM : equalisationLimitMM;

            if (acceptedMM <= fmFluxEpsilonMM)
                continue;

            if (isChannelCell(nx, ny))
            {
                dsc[nx][ny] += acceptedMM;
                const double volumeM3 = acceptedMM * dx * dy / 1000.0;
                Q_stream += static_cast<float>(volumeM3);
                fmSatChannelM3 += volumeM3;
            }
            else if (lakes[nx][ny] != noData)
            {
                const int locLake = fmLakeIndex(nx, ny);
                if (locLake >= 0 && lakeStats[locLake][0] > 0.0f)
                {
                    const double volumeM3 = acceptedMM * dx * dy / 1000.0;
                    lakeStats[locLake][7] +=
                        static_cast<float>(volumeM3 / lakeStats[locLake][0]);
                    fmSatLakeM3 += volumeM3;
                }
                else
                {
                    acceptedMM = 0.0f;
                }
            }
            else if (fmIsCell(nx, ny))
            {
                const double maximumTargetStoreMM =
                    (static_cast<double>(dem[nx][ny]) -
                     fmBaseHeadM[nx][ny]) *
                    1000.0 * static_cast<double>(fmSpecificYield);

                const float receiverCapacityMM = static_cast<float>(
                    maximumTargetStoreMM -
                    static_cast<double>(fmSatStoreMM[nx][ny]) -
                    static_cast<double>(fmSatIncomingReservedMM[nx][ny])
                );

                if (receiverCapacityMM <= 0.0f)
                    acceptedMM = 0.0f;
                else if (acceptedMM > receiverCapacityMM)
                    acceptedMM = receiverCapacityMM;

                if (acceptedMM > 0.0f)
                {
                    fmSatRoutingDeltaMM[nx][ny] += acceptedMM;
                    fmSatIncomingReservedMM[nx][ny] += acceptedMM;
                    fmSatLandM3 += acceptedMM * dx * dy / 1000.0;
                }
            }

            if (acceptedMM > 0.0f)
            {
                fmSatRoutingDeltaMM[x][y] -= acceptedMM;
                remainingSourceMM -= acceptedMM;
            }
        }
    }
}

// Apply saturated transfers and convert above-surface storage to exfiltration.
for (int x = 0; x < xSize; ++x)
{
    for (int y = 0; y < ySize; ++y)
    {
        if (!fmIsCell(x, y))
            continue;

        fmSatStoreMM[x][y] += fmSatRoutingDeltaMM[x][y];
        if (fmSatStoreMM[x][y] < 0.0f)
            fmSatStoreMM[x][y] = 0.0f;

        const double maximumStoreMM =
            (static_cast<double>(dem[x][y]) - fmBaseHeadM[x][y]) *
            1000.0 * static_cast<double>(fmSpecificYield);

        if (static_cast<double>(fmSatStoreMM[x][y]) > maximumStoreMM)
        {
            const float excessMM = static_cast<float>(
                static_cast<double>(fmSatStoreMM[x][y]) - maximumStoreMM
            );
            fmSatStoreMM[x][y] -= excessMM;
            dsc[x][y] += excessMM;
            fmExfiltrationM3 += excessMM * dx * dy / 1000.0;
        }

        const double currentHeadM = fmBaseHeadM[x][y] +
            static_cast<double>(fmSatStoreMM[x][y]) /
            (1000.0 * static_cast<double>(fmSpecificYield));

        // Keep G available for maps and diagnostics. When dynamicG is enabled,
        // the full groundwater routines must skip forest/mire cells.
        G[x][y][0] = static_cast<float>(currentHeadM);
        G[x][y][1] = static_cast<float>(currentHeadM);
    }
}

// Final storage and diagnostic mass balance.
double fmStoreAfterM3 = 0.0;
double fmUnsatStoreM3 = 0.0;
double fmSatStoreM3 = 0.0;
double fmHeadSumM = 0.0;
double fmDtGSumM = 0.0;
long long fmHeadCount = 0;

for (int x = 0; x < xSize; ++x)
{
    for (int y = 0; y < ySize; ++y)
    {
        if (!fmIsCell(x, y))
            continue;

        const double unsatM3 =
            static_cast<double>(SW[x][y][0]) * dx * dy / 1000.0;
        const double satM3 =
            static_cast<double>(fmSatStoreMM[x][y]) * dx * dy / 1000.0;

        fmUnsatStoreM3 += unsatM3;
        fmSatStoreM3 += satM3;
        fmStoreAfterM3 += unsatM3 + satM3;

        const double headM = fmBaseHeadM[x][y] +
            static_cast<double>(fmSatStoreMM[x][y]) /
            (1000.0 * static_cast<double>(fmSpecificYield));
        fmHeadSumM += headM;
        fmDtGSumM += static_cast<double>(dem[x][y]) - headM;
        ++fmHeadCount;
    }
}

const double fmExternalOutM3 =
    fmUnsatChannelM3 + fmUnsatLakeM3 +
    fmSatChannelM3 + fmSatLakeM3 + fmExfiltrationM3;

const double fmMassResidualM3 =
    fmStoreBeforeM3 - fmStoreAfterM3 - fmExternalOutM3;

if (fmDiagnostic.is_open())
{
    fmDiagnostic
        << year[index] << "-" << month[index] << "-" << day[index] << " "
        << fmUnsatVerticalM3 << " "
        << fmUnsatLandM3 << " "
        << fmUnsatChannelM3 << " "
        << fmUnsatLakeM3 << " "
        << fmSatLandM3 << " "
        << fmSatChannelM3 << " "
        << fmSatLakeM3 << " "
        << fmExfiltrationM3 << " "
        << fmUnsatStoreM3 << " "
        << fmSatStoreM3 << " "
        << (fmHeadCount > 0 ? fmHeadSumM / fmHeadCount : 0.0) << " "
        << (fmHeadCount > 0 ? fmDtGSumM / fmHeadCount : 0.0) << " "
        << fmActiveCells << " "
        << fmUnsatDeadEnds << " "
        << fmSatDeadEnds << " "
        << fmMassResidualM3
        << "\n";
}
}
