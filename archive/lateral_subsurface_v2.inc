{// Entire include is one compound statement for: if(dynamicG==1) #include ...
// -----------------------------------------------------------------------------
// lateral_subsurface_dynamic_v2.inc
//
// Conservative daily lateral soil-water and groundwater exchange.
// This file assumes update_soil_layers_dynamic_v2.inc ran earlier in the same
// daily loop and prepared the fixed-head boundaries and reference geometry.
//
// Main principles:
//   * Today's recharge is interpreted as water depth and converted to head rise
//     using specific yield.
//   * Groundwater gradients are recalculated after recharge.
//   * Fluxes are calculated from a frozen state and applied in a second pass.
//   * Pairwise groundwater transfers are limited so heads cannot cross in one
//     explicit daily step.
//   * Groundwater and interflow reaching channels are added to dsc in mm and
//     counted once in Q_stream in m3/day.
//   * Soil interflow is limited to water above field capacity and receiver pore
//     capacity.
//   * SW[][0]/SW[][1] and G[][0]/G[][1] are synchronised at the end of the day.
// -----------------------------------------------------------------------------

const float gwSpecificYield = 0.20f;
const float gwMaxDailyHeadChangeM = 0.25f;
const float gwFluxEpsilonMM = 1.0e-7f;

static std::vector<std::vector<float>> gwHeadStart;
static std::vector<std::vector<float>> gwStorageDeltaMM;
static std::vector<std::vector<float>> soilStorageDeltaMM;
static bool gwLateralBuffersInitialised = false;

if (!gwLateralBuffersInitialised)
{
    gwHeadStart.assign(xSize, std::vector<float>(ySize, noData));
    gwStorageDeltaMM.assign(xSize, std::vector<float>(ySize, 0.0f));
    soilStorageDeltaMM.assign(xSize, std::vector<float>(ySize, 0.0f));
    gwLateralBuffersInitialised = true;
}

float gwRechargeM3 = 0.0f;
float gwRechargeExfiltrationM3 = 0.0f;
float gwToChannelM3 = 0.0f;
float gwToLakeM3 = 0.0f;
float gwLandTransferGrossM3 = 0.0f;
float soilToChannelM3 = 0.0f;
float soilToLakeM3 = 0.0f;
float soilLandTransferGrossM3 = 0.0f;
float gwMaximumAppliedHeadChangeM = 0.0f;
float gwMassResidualM3 = 0.0f;
long long gwActiveSourceCells = 0;
long long gwCellsWithoutLowerHead = 0;
long long gwSurfaceClamps = 0;
long long soilActiveSourceCells = 0;

// Reset daily buffers and reinterpret G[][1]-G[][0] as recharge water input.
for (int x = 0; x < xSize; ++x)
{
    for (int y = 0; y < ySize; ++y)
    {
        gwStorageDeltaMM[x][y] = 0.0f;
        soilStorageDeltaMM[x][y] = 0.0f;
        latflow[x][y] = 0.0f;

        if (dem[x][y] == noData)
        {
            gwHeadStart[x][y] = noData;
            continue;
        }

        if (isChannelCell(x, y))
        {
            gwHeadStart[x][y] = dem[x][y];
            G[x][y][0] = gwHeadStart[x][y];
            G[x][y][1] = gwHeadStart[x][y];
            continue;
        }

        if (lakes[x][y] != noData)
        {
            // lakes.tif and lakeStats.csv now use the same zero-based index.
            const int locLake = static_cast<int>(lakes[x][y]);

            gwHeadStart[x][y] =
                (locLake >= 0 && locLake < nrLakes)
                ? lakeStats[locLake][3]
                : dem[x][y];
            G[x][y][0] = gwHeadStart[x][y];
            G[x][y][1] = gwHeadStart[x][y];
            continue;
        }

        // The soil-water balance currently writes recharge as an equal head
        // increment (Sy=1). Recover the actual recharge water depth, then apply
        // the configured specific yield to obtain the groundwater-head rise.
        float rechargeWaterMM = (G[x][y][1] - G[x][y][0]) * 1000.0f;
        if (rechargeWaterMM < 0.0f || isnan(rechargeWaterMM) || isinf(rechargeWaterMM))
            rechargeWaterMM = 0.0f;

        gwRechargeM3 += rechargeWaterMM * dx * dy / 1000.0f;

        float head = G[x][y][0] +
            rechargeWaterMM / (1000.0f * gwSpecificYield);

        if (head > dem[x][y])
        {
            const float exfiltrationMM =
                (head - dem[x][y]) * 1000.0f * gwSpecificYield;

            if (exfiltrationMM > 0.0f)
            {
                dsc[x][y] += exfiltrationMM;
                gwRechargeExfiltrationM3 +=
                    exfiltrationMM * dx * dy / 1000.0f;
            }

            head = dem[x][y];
            ++gwSurfaceClamps;
        }

        gwHeadStart[x][y] = head;
    }
}

// Recalculate post-recharge slopeG and dG from a frozen head field.
for (int x = 0; x < xSize; ++x)
{
    for (int y = 0; y < ySize; ++y)
    {
        slopeG[x][y] = 0.0f;
        for (std::size_t i = 0; i < dG[x][y].size(); ++i)
            dG[x][y][i] = 0.0f;
    }
}

for (int x = 1; x < xSize - 1; ++x)
{
    for (int y = 1; y < ySize - 1; ++y)
    {
        if (dem[x][y] == noData ||
            isChannelCell(x, y) ||
            lakes[x][y] != noData)
        {
            continue;
        }

        float gx = 0.0f;
        float gy = 0.0f;

        const bool westValid = dem[x - 1][y] != noData;
        const bool eastValid = dem[x + 1][y] != noData;
        const bool northValid = dem[x][y - 1] != noData;
        const bool southValid = dem[x][y + 1] != noData;

        if (westValid && eastValid)
            gx = (gwHeadStart[x + 1][y] - gwHeadStart[x - 1][y]) / (2.0f * dx);
        else if (eastValid)
            gx = (gwHeadStart[x + 1][y] - gwHeadStart[x][y]) / dx;
        else if (westValid)
            gx = (gwHeadStart[x][y] - gwHeadStart[x - 1][y]) / dx;

        if (northValid && southValid)
            gy = (gwHeadStart[x][y + 1] - gwHeadStart[x][y - 1]) / (2.0f * dy);
        else if (southValid)
            gy = (gwHeadStart[x][y + 1] - gwHeadStart[x][y]) / dy;
        else if (northValid)
            gy = (gwHeadStart[x][y] - gwHeadStart[x][y - 1]) / dy;

        const float gradientMagnitude = sqrt(gx * gx + gy * gy);
        slopeG[x][y] = atan(gradientMagnitude);

        const int gwDirX[8] = {-1, -1, -1,  0, 0,  1, 1, 1};
        const int gwDirY[8] = {-1,  0,  1, -1, 1, -1, 0, 1};
        float gradientSum = 0.0f;
        float localGradient[8] = {0.0f};

        for (int dir = 0; dir < 8; ++dir)
        {
            const int X = gwDirX[dir];
            const int Y = gwDirY[dir];
            const int nx = x + X;
            const int ny = y + Y;

            if (dem[nx][ny] == noData ||
                gwHeadStart[nx][ny] == noData ||
                gwHeadStart[nx][ny] >= gwHeadStart[x][y])
            {
                continue;
            }

            const float distance =
                sqrt((X * dx) * (X * dx) + (Y * dy) * (Y * dy));

            if (distance > 0.0f)
            {
                localGradient[dir] =
                    (gwHeadStart[x][y] - gwHeadStart[nx][ny]) / distance;
                gradientSum += localGradient[dir];
            }
        }

        if (gradientSum > 0.0f)
        {
            const int usableDirections =
                dG[x][y].size() < 8 ? static_cast<int>(dG[x][y].size()) : 8;

            for (int dir = 0; dir < usableDirections; ++dir)
            {
                if (localGradient[dir] > 0.0f)
                    dG[x][y][dir] = localGradient[dir] / gradientSum;
            }
        }
        else
        {
            ++gwCellsWithoutLowerHead;
        }
    }
}

// -----------------------------------------------------------------------------
// A. Lateral soil-water/interflow exchange.
// -----------------------------------------------------------------------------
if (tm[index] > drainageTempSD)
{
    for (int x = 1; x < xSize - 1; ++x)
    {
        for (int y = 1; y < ySize - 1; ++y)
        {
            if (dem[x][y] == noData ||
                isChannelCell(x, y) ||
                lakes[x][y] != noData ||
                SD[x][y] <= 0.0f)
            {
                continue;
            }

            const float fieldCapacityStorageMM = FC[x][y] * SD[x][y];
            const float mobileWaterMM =
                SW[x][y][0] > fieldCapacityStorageMM
                ? SW[x][y][0] - fieldCapacityStorageMM
                : 0.0f;

            float releaseFraction = drainageRate2[x][y];
            if (releaseFraction < 0.0f) releaseFraction = 0.0f;
            if (releaseFraction > 1.0f) releaseFraction = 1.0f;

            float remainingSourceMM = mobileWaterMM * releaseFraction;
            if (remainingSourceMM <= gwFluxEpsilonMM)
                continue;

            ++soilActiveSourceCells;

            int wIndex = 0;
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
                    const float weight = d[x][y][wIndex];

                    if (weight <= 0.0f ||
                        dem[nx][ny] == noData ||
                        remainingSourceMM <= gwFluxEpsilonMM)
                    {
                        ++wIndex;
                        continue;
                    }

                    float proposedMM =
                        mobileWaterMM * releaseFraction * weight;
                    if (proposedMM > remainingSourceMM)
                        proposedMM = remainingSourceMM;

                    float acceptedMM = 0.0f;

                    if (isChannelCell(nx, ny))
                    {
                        acceptedMM = proposedMM;
                        dsc[nx][ny] += acceptedMM;
                        const float volumeM3 = acceptedMM * dx * dy / 1000.0f;
                        Q_stream += volumeM3;
                        soilToChannelM3 += volumeM3;
                    }
                    else if (lakes[nx][ny] != noData)
                    {
                        const int locLake =
                            static_cast<int>(lakes[nx][ny]);

                        if (locLake >= 0 && locLake < nrLakes &&
                            lakeStats[locLake][0] > 0.0f)
                        {
                            acceptedMM = proposedMM;
                            const float volumeM3 = acceptedMM * dx * dy / 1000.0f;
                            lakeStats[locLake][7] +=
                                volumeM3 / lakeStats[locLake][0];
                            soilToLakeM3 += volumeM3;
                        }
                    }
                    else if (SD[nx][ny] > 0.0f)
                    {
                        const float receiverCapacityMM =
                            SWSat[nx][ny] * SD[nx][ny] -
                            (SW[nx][ny][0] + soilStorageDeltaMM[nx][ny]);

                        if (receiverCapacityMM > 0.0f)
                        {
                            acceptedMM = proposedMM;
                            if (acceptedMM > receiverCapacityMM)
                                acceptedMM = receiverCapacityMM;

                            soilStorageDeltaMM[nx][ny] += acceptedMM;
                            soilLandTransferGrossM3 +=
                                acceptedMM * dx * dy / 1000.0f;
                        }
                    }

                    if (acceptedMM > 0.0f)
                    {
                        soilStorageDeltaMM[x][y] -= acceptedMM;
                        remainingSourceMM -= acceptedMM;
                    }

                    ++wIndex;
                }
            }
        }
    }
}

// -----------------------------------------------------------------------------
// B. Groundwater exchange from the post-recharge groundwater surface.
// -----------------------------------------------------------------------------
for (int x = 1; x < xSize - 1; ++x)
{
    for (int y = 1; y < ySize - 1; ++y)
    {
        if (dem[x][y] == noData ||
            isChannelCell(x, y) ||
            lakes[x][y] != noData)
        {
            continue;
        }
	float locKg = kG[0];
	if(landuse[x][y]==30)
		locKg*=10;
        float potentialHeadChangeM =
            locKg * sin(slopeG[x][y]);

        if (potentialHeadChangeM < 0.0f)
            potentialHeadChangeM = 0.0f;
        if (potentialHeadChangeM > gwMaxDailyHeadChangeM)
            potentialHeadChangeM = gwMaxDailyHeadChangeM;

        const float potentialWaterOutMM =
            potentialHeadChangeM * 1000.0f * gwSpecificYield;

        if (potentialWaterOutMM <= gwFluxEpsilonMM)
            continue;

        ++gwActiveSourceCells;
        float remainingSourceMM = potentialWaterOutMM;
        float actualSourceOutMM = 0.0f;

        const int gwDirX[8] = {-1, -1, -1,  0, 0,  1, 1, 1};
        const int gwDirY[8] = {-1,  0,  1, -1, 1, -1, 0, 1};
        const int usableDirections =
            dG[x][y].size() < 8 ? static_cast<int>(dG[x][y].size()) : 8;

        for (int dir = 0; dir < usableDirections; ++dir)
        {
            const int X = gwDirX[dir];
            const int Y = gwDirY[dir];
            const int nx = x + X;
            const int ny = y + Y;
            const float weight = dG[x][y][dir];

            if (weight <= 0.0f ||
                dem[nx][ny] == noData ||
                remainingSourceMM <= gwFluxEpsilonMM)
            {
                continue;
            }

            const float headDifferenceM =
                gwHeadStart[x][y] - gwHeadStart[nx][ny];

            if (headDifferenceM <= 0.0f)
                continue;

            float proposedMM = potentialWaterOutMM * weight;
            if (proposedMM > remainingSourceMM)
                proposedMM = remainingSourceMM;

            // For two movable land cells, half the storage difference is
            // the maximum transfer that can equalise their heads. A fixed
            // channel/lake boundary can receive the full source difference.
            const bool fixedHeadTarget =
                isChannelCell(nx, ny) || lakes[nx][ny] != noData;

            float equalisationLimitMM =
                headDifferenceM * 1000.0f * gwSpecificYield;
            if (!fixedHeadTarget)
                equalisationLimitMM *= 0.5f;

            float acceptedMM = proposedMM;
            if (acceptedMM > equalisationLimitMM)
                acceptedMM = equalisationLimitMM;

            if (acceptedMM <= gwFluxEpsilonMM)
                continue;

            if (isChannelCell(nx, ny))
            {
                dsc[nx][ny] += acceptedMM;
                const float volumeM3 = acceptedMM * dx * dy / 1000.0f;
                Q_stream += volumeM3;
                gwToChannelM3 += volumeM3;
            }
            else if (lakes[nx][ny] != noData)
            {
                const int locLake =
                    static_cast<int>(lakes[nx][ny]);

                if (locLake >= 0 && locLake < nrLakes &&
                    lakeStats[locLake][0] > 0.0f)
                {
                    const float volumeM3 = acceptedMM * dx * dy / 1000.0f;
                    lakeStats[locLake][7] +=
                        volumeM3 / lakeStats[locLake][0];
                    gwToLakeM3 += volumeM3;
                }
                else
                {
                    acceptedMM = 0.0f;
                }
            }
            else
            {
                gwStorageDeltaMM[nx][ny] += acceptedMM;
                gwLandTransferGrossM3 +=
                    acceptedMM * dx * dy / 1000.0f;
            }

            if (acceptedMM > 0.0f)
            {
                gwStorageDeltaMM[x][y] -= acceptedMM;
                remainingSourceMM -= acceptedMM;
                actualSourceOutMM += acceptedMM;
            }
        }

        // Preserve latflow as a head-equivalent daily source-cell outflow [m].
        latflow[x][y] =
            actualSourceOutMM / (1000.0f * gwSpecificYield);
    }
}

// -----------------------------------------------------------------------------
// C. Apply all soil and groundwater deltas simultaneously and synchronise states.
// -----------------------------------------------------------------------------
for (int x = 0; x < xSize; ++x)
{
    for (int y = 0; y < ySize; ++y)
    {
        if (dem[x][y] == noData)
            continue;

        if (!isChannelCell(x, y) && lakes[x][y] == noData)
        {
            if (SD[x][y] > 0.0f)
            {
                SW[x][y][0] += soilStorageDeltaMM[x][y];

                const float soilCapacityMM = SWSat[x][y] * SD[x][y];
                if (SW[x][y][0] < 0.0f)
                    SW[x][y][0] = 0.0f;
                if (SW[x][y][0] > soilCapacityMM)
                    SW[x][y][0] = soilCapacityMM;
            }
            SW[x][y][1] = SW[x][y][0];

            float newHead = gwHeadStart[x][y] +
                gwStorageDeltaMM[x][y] /
                (1000.0f * gwSpecificYield);

            if (newHead > dem[x][y])
            {
                const float exfiltrationMM =
                    (newHead - dem[x][y]) * 1000.0f * gwSpecificYield;

                if (exfiltrationMM > 0.0f)
                {
                    dsc[x][y] += exfiltrationMM;
                    gwRechargeExfiltrationM3 +=
                        exfiltrationMM * dx * dy / 1000.0f;
                }

                newHead = dem[x][y];
                ++gwSurfaceClamps;
            }

            const float headChangeM = newHead - G[x][y][0];
            const float absHeadChangeM = headChangeM < 0.0f ? -headChangeM : headChangeM;
            if (absHeadChangeM > gwMaximumAppliedHeadChangeM)
                gwMaximumAppliedHeadChangeM = absHeadChangeM;

            G[x][y][0] = newHead;
            G[x][y][1] = newHead;
        }
        else
        {
            // Fixed heads remain fixed after receiving groundwater discharge.
            G[x][y][1] = G[x][y][0];
        }
    }
}

// Storage accounting residual. Internal land-to-land transfers cancel by design.
double gwDeltaSumMM = 0.0;
for (int x = 0; x < xSize; ++x)
{
    for (int y = 0; y < ySize; ++y)
    {
        if (dem[x][y] != noData &&
            !isChannelCell(x, y) &&
            lakes[x][y] == noData)
        {
            gwDeltaSumMM += gwStorageDeltaMM[x][y];
        }
    }
}

gwMassResidualM3 = static_cast<float>(
    gwDeltaSumMM * dx * dy / 1000.0 +
    gwToChannelM3 + gwToLakeM3
);

// -----------------------------------------------------------------------------
// Daily flux diagnostic.
// -----------------------------------------------------------------------------
static std::ofstream gwFluxDiagnostic;
static bool gwFluxDiagnosticOpened = false;

if (!gwFluxDiagnosticOpened)
{
    gwFluxDiagnostic.open(
        ioFolder + "/groundwater_flux_diagnostic_" +
        std::to_string(scenario) + "_" + gid + ".csv"
    );

    if (gwFluxDiagnostic.is_open())
    {
        gwFluxDiagnostic
            << "date recharge_m3 recharge_exfiltration_m3 "
            << "gw_to_channel_m3 gw_to_lake_m3 gw_land_transfer_gross_m3 "
            << "soil_to_channel_m3 soil_to_lake_m3 soil_land_transfer_gross_m3 "
            << "gw_active_sources gw_cells_without_lower_head soil_active_sources "
            << "surface_clamps max_applied_head_change_m gw_mass_residual_m3\n";
    }

    gwFluxDiagnosticOpened = true;
}

if (gwFluxDiagnostic.is_open())
{
    gwFluxDiagnostic
        << year[index] << "-" << month[index] << "-" << day[index] << " "
        << gwRechargeM3 << " "
        << gwRechargeExfiltrationM3 << " "
        << gwToChannelM3 << " "
        << gwToLakeM3 << " "
        << gwLandTransferGrossM3 << " "
        << soilToChannelM3 << " "
        << soilToLakeM3 << " "
        << soilLandTransferGrossM3 << " "
        << gwActiveSourceCells << " "
        << gwCellsWithoutLowerHead << " "
        << soilActiveSourceCells << " "
        << gwSurfaceClamps << " "
        << gwMaximumAppliedHeadChangeM << " "
        << gwMassResidualM3 << "\n";
}

} // end lateral_subsurface_dynamic_v2.inc
