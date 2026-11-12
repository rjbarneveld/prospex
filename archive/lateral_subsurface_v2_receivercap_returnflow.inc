{// Entire include is one compound statement for: if(dynamicG==1) #include ...
// -----------------------------------------------------------------------------
// lateral_subsurface_dynamic_v2.inc
//
// Conservative daily lateral soil-water and groundwater exchange.
// This file assumes update_soil_layers_dynamic_v2.inc ran earlier in the same
// daily loop and prepared the fixed-head boundaries and reference geometry.
//
// Main principles:
//   * Soil-water balance writes today's recharge and groundwater ET directly
//     to G[][1] in true head units using specific yield.
//   * Groundwater gradients are recalculated after recharge.
//   * Fluxes are calculated from a frozen state and applied in a second pass.
//   * Potential groundwater release is a fixed fraction of mobile storage above
//     the terrain-derived lower reference Gref, giving exponential local recession.
//   * Pairwise groundwater transfers are limited so heads cannot cross in one
//     explicit daily step.
//   * Cumulative receiver capacity limits the total net daily head rise from
//     all contributing neighbours.
//   * Capacity-rejected shallow groundwater enters a two-day return-flow store
//     and is released from the following day onward.
//   * Groundwater and interflow reaching channels are added to dsc in mm and
//     counted once in Q_stream in m3/day.
//   * Soil interflow is limited to water above field capacity and receiver pore
//     capacity.
//   * SW[][0]/SW[][1] and G[][0]/G[][1] are synchronised at the end of the day.
// -----------------------------------------------------------------------------

const float gwSpecificYield = 0.20f;
const float gwResidenceTimeDays = 10.0f;
const float gwReleaseFraction =
    1.0f - expf(-1.0f / gwResidenceTimeDays);
const float gwMaxDailyHeadChangeM = 0.50f;
const float gwFluxEpsilonMM = 1.0e-7f;

// Shallow return-flow reservoir. Water that cannot remain in the groundwater
// store because the surface or a receiving cell is full is delayed here
// instead of being added directly to dsc in the same timestep.
const float returnFlowResidenceTimeDays = 2.0f;
const float returnFlowReleaseFraction =
    1.0f - expf(-1.0f / returnFlowResidenceTimeDays);

static std::vector<std::vector<float>> gwHeadStart;
static std::vector<std::vector<float>> gwStorageDeltaMM;
static std::vector<std::vector<float>> soilStorageDeltaMM;
static std::vector<std::vector<float>> gwLateralInitialHeadReference;
static std::vector<std::vector<float>> shallowReturnFlowStoreMM;
static bool gwLateralBuffersInitialised = false;

if (!gwLateralBuffersInitialised)
{
    gwHeadStart.assign(xSize, std::vector<float>(ySize, noData));
    gwStorageDeltaMM.assign(xSize, std::vector<float>(ySize, 0.0f));
    soilStorageDeltaMM.assign(xSize, std::vector<float>(ySize, 0.0f));
    gwLateralInitialHeadReference.assign(
        xSize,
        std::vector<float>(ySize, noData)
    );
    shallowReturnFlowStoreMM.assign(
        xSize,
        std::vector<float>(ySize, 0.0f)
    );
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
double gwReturnFlowGeneratedM3 = 0.0;
double gwReturnFlowGeneratedByReceiverCapacityM3 = 0.0;
double gwReturnFlowReleasedM3 = 0.0;
double gwReturnFlowEndStorageM3 = 0.0;
long long gwReceiverCapacityLimitHits = 0;
long long gwReceiverDailyRiseCapHits = 0;
long long gwReceiverTerrainCapacityHits = 0;
double gwReceiverDailyRiseRejectedM3 = 0.0;
long long gwActiveSourceCells = 0;
long long gwCellsWithoutLowerHead = 0;
long long gwSurfaceClamps = 0;
long long soilActiveSourceCells = 0;

long long gwHeadChangeCapHits = 0;
long long gwEqualisationLimitHits = 0;
long long gwCellsAdjacentChannel = 0;
long long gwCellsDischargingChannel = 0;
long long gwEndDtGLt025 = 0;
long long gwEndDtG025To05 = 0;
long long gwEndDtG05To1 = 0;
long long gwEndDtG1To2 = 0;
long long gwEndDtG2To5 = 0;
long long gwEndDtGGe5 = 0;
double gwPotentialOutM3 = 0.0;
double gwActualOutM3 = 0.0;
double gwEqualisationLimitedM3 = 0.0;
double gwRejectedTargetM3 = 0.0;
double gwStartPositiveStorageAnomalyM3 = 0.0;
double gwStartNegativeStorageAnomalyM3 = 0.0;
double gwEndPositiveStorageAnomalyM3 = 0.0;
double gwEndNegativeStorageAnomalyM3 = 0.0;
double gwStartMobileStorageM3 = 0.0;
double gwEndMobileStorageM3 = 0.0;
double gwStartActiveHeadSumM = 0.0;
double gwEndActiveHeadSumM = 0.0;
double gwNetSoilExchangeM3 = 0.0;
double gwSoilRechargeM3 = 0.0;
double gwSoilRemovalM3 = 0.0;
double gwReferenceFloorCorrectionM3 = 0.0;
long long gwInvalidReferenceCells = 0;
long long gwReferenceFloorCorrections = 0;
double gwEndNetStorageByGroupM3[4] = {0.0, 0.0, 0.0, 0.0};
double gwToChannelByGroupM3[4] = {0.0, 0.0, 0.0, 0.0};
double gwToLakeByGroupM3[4] = {0.0, 0.0, 0.0, 0.0};
double gwActualOutByGroupM3[4] = {0.0, 0.0, 0.0, 0.0};
double soilToChannelByGroupM3[4] = {0.0, 0.0, 0.0, 0.0};
double gwGradientSum = 0.0;
double gwPotentialHeadChangeSumM = 0.0;
double gwEndDtGSumM = 0.0;
long long gwGradientCells = 0;
long long gwEndLandCells = 0;

auto gwRecessionDiagGroup = [&](int x, int y) -> int
{
    if (landuse[x][y] == 30) return 0;
    if (landuse[x][y] == 60) return 1;
    if (landuse[x][y] >= 20 && landuse[x][y] <= 23) return 2;
    return 3;
};

const double gwDiagCellAreaM2 =
    static_cast<double>(dx) * static_cast<double>(dy);

// Release water already present in the shallow return-flow store.
// This is done before today's new groundwater overflow is generated, so new
// overflow has at least a one-day delay before entering dsc.
for (int x = 0; x < xSize; ++x)
{
    for (int y = 0; y < ySize; ++y)
    {
        if (dem[x][y] == noData ||
            isChannelCell(x, y) ||
            lakes[x][y] != noData)
        {
            continue;
        }

        float storedMM = shallowReturnFlowStoreMM[x][y];
        if (!std::isfinite(storedMM) || storedMM < 0.0f)
            storedMM = 0.0f;

        const float releasedMM =
            storedMM * returnFlowReleaseFraction;

        if (releasedMM > gwFluxEpsilonMM)
        {
            shallowReturnFlowStoreMM[x][y] = storedMM - releasedMM;
            dsc[x][y] += releasedMM;
            gwReturnFlowReleasedM3 +=
                releasedMM * gwDiagCellAreaM2 / 1000.0;
        }
        else
        {
            shallowReturnFlowStoreMM[x][y] = storedMM;
        }
    }
}

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


        if (gwLateralInitialHeadReference[x][y] == noData)
            gwLateralInitialHeadReference[x][y] = G[x][y][0];

        const bool validReference =
            Gref[x][y] == Gref[x][y] &&
            Gref[x][y] != noData &&
            Gref[x][y] <= dem[x][y];

        if (!validReference)
        {
            ++gwInvalidReferenceCells;
            Gref[x][y] = dem[x][y];
        }

        // G[][1] already contains the net effect of today's recharge and
        // shallow-groundwater ET in true head units.
        float head = G[x][y][1];
        if (isnan(head) || isinf(head) || head == noData)
            head = G[x][y][0];

        const float soilExchangeMM =
            (head - G[x][y][0]) * 1000.0f * gwSpecificYield;
        const double soilExchangeM3 =
            soilExchangeMM * gwDiagCellAreaM2 / 1000.0;
        gwNetSoilExchangeM3 += soilExchangeM3;
        if (soilExchangeMM >= 0.0f)
        {
            gwRechargeM3 += static_cast<float>(soilExchangeM3);
            gwSoilRechargeM3 += soilExchangeM3;
        }
        else
        {
            gwSoilRemovalM3 += -soilExchangeM3;
        }

        if (head > dem[x][y])
        {
            const float exfiltrationMM =
                (head - dem[x][y]) * 1000.0f * gwSpecificYield;

            if (exfiltrationMM > 0.0f)
            {
                // Delay recharge-driven saturation excess rather than adding
                // it directly to same-day overland flow.
                shallowReturnFlowStoreMM[x][y] += exfiltrationMM;
                const double volumeM3 =
                    exfiltrationMM * gwDiagCellAreaM2 / 1000.0;
                gwRechargeExfiltrationM3 +=
                    static_cast<float>(volumeM3);
                gwReturnFlowGeneratedM3 += volumeM3;
            }

            head = dem[x][y];
            ++gwSurfaceClamps;
        }

        if (head < Gref[x][y])
        {
            const float correctionMM =
                (Gref[x][y] - head) * 1000.0f * gwSpecificYield;
            gwReferenceFloorCorrectionM3 +=
                correctionMM * gwDiagCellAreaM2 / 1000.0;
            ++gwReferenceFloorCorrections;
            head = Gref[x][y];
        }

        gwHeadStart[x][y] = head;

        if (gwLateralInitialHeadReference[x][y] != noData)
        {
            const double startHeadAnomalyM =
                static_cast<double>(head - gwLateralInitialHeadReference[x][y]);
            const double startStorageAnomalyM3 =
                startHeadAnomalyM * static_cast<double>(gwSpecificYield) *
                gwDiagCellAreaM2;
            if (startStorageAnomalyM3 >= 0.0)
                gwStartPositiveStorageAnomalyM3 += startStorageAnomalyM3;
            else
                gwStartNegativeStorageAnomalyM3 += startStorageAnomalyM3;
        }

        const double startActiveHeadM = std::max(
            0.0,
            static_cast<double>(head - Gref[x][y])
        );
        gwStartActiveHeadSumM += startActiveHeadM;
        gwStartMobileStorageM3 +=
            startActiveHeadM * static_cast<double>(gwSpecificYield) *
            gwDiagCellAreaM2;
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

        gwGradientSum += gradientMagnitude;
        ++gwGradientCells;

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

                        soilToChannelByGroupM3[
                            gwRecessionDiagGroup(x, y)
                        ] += volumeM3;
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
//
// Each land cell is a linear reservoir above its terrain-derived lower
// reference. The daily release is a constant fraction of remaining mobile
// storage, while dG distributes that release among lower neighbouring heads.
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

        if (Gref[x][y] == noData ||
            Gref[x][y] != Gref[x][y] ||
            Gref[x][y] > dem[x][y])
        {
            ++gwInvalidReferenceCells;
            continue;
        }

        const float activeHeadM =
            gwHeadStart[x][y] > Gref[x][y]
            ? gwHeadStart[x][y] - Gref[x][y]
            : 0.0f;

        const float mobileStorageMM =
            activeHeadM * 1000.0f * gwSpecificYield;

        float potentialWaterOutMM =
            mobileStorageMM * gwReleaseFraction;

        const float maximumWaterOutMM =
            gwMaxDailyHeadChangeM * 1000.0f * gwSpecificYield;

        if (potentialWaterOutMM > maximumWaterOutMM)
        {
            potentialWaterOutMM = maximumWaterOutMM;
            ++gwHeadChangeCapHits;
        }

        const float potentialHeadChangeM =
            potentialWaterOutMM /
            (1000.0f * gwSpecificYield);
        gwPotentialHeadChangeSumM += potentialHeadChangeM;

        gwPotentialOutM3 +=
            potentialWaterOutMM * gwDiagCellAreaM2 / 1000.0;

        if (potentialWaterOutMM <= gwFluxEpsilonMM)
            continue;

        ++gwActiveSourceCells;

        const int gwSourceGroup = gwRecessionDiagGroup(x, y);
        bool gwSourceAdjacentChannel = false;
        bool gwSourceDischargedChannel = false;
        const int gwAdjX[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
        const int gwAdjY[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
        for (int adj = 0; adj < 8; ++adj)
        {
            if (isChannelCell(x + gwAdjX[adj], y + gwAdjY[adj]))
            {
                gwSourceAdjacentChannel = true;
                break;
            }
        }
        if (gwSourceAdjacentChannel)
            ++gwCellsAdjacentChannel;

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

            const bool fixedHeadTarget =
                isChannelCell(nx, ny) || lakes[nx][ny] != noData;

            float equalisationLimitMM =
                headDifferenceM * 1000.0f * gwSpecificYield;
            if (!fixedHeadTarget)
                equalisationLimitMM *= 0.5f;

            float acceptedMM = proposedMM;
            if (acceptedMM > equalisationLimitMM)
            {
                gwEqualisationLimitedM3 +=
                    (acceptedMM - equalisationLimitMM) *
                    gwDiagCellAreaM2 / 1000.0;
                acceptedMM = equalisationLimitMM;
                ++gwEqualisationLimitHits;
            }

            if (acceptedMM <= gwFluxEpsilonMM)
                continue;

            if (isChannelCell(nx, ny))
            {
                dsc[nx][ny] += acceptedMM;
                const float volumeM3 = acceptedMM * dx * dy / 1000.0f;
                Q_stream += volumeM3;
                gwToChannelM3 += volumeM3;
                gwToChannelByGroupM3[gwSourceGroup] += volumeM3;
                gwSourceDischargedChannel = true;
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
                    gwToLakeByGroupM3[gwSourceGroup] += volumeM3;
                }
                else
                {
                    gwRejectedTargetM3 +=
                        acceptedMM * gwDiagCellAreaM2 / 1000.0;
                    acceptedMM = 0.0f;
                }
            }
            else
            {
                // Cumulative receiver-capacity enforcement. The receiver's
                // already accumulated net groundwater change is included, so
                // many source cells cannot collectively raise it by more than
                // the configured daily head-change cap or above the terrain.
                const float maximumDailyRiseMM =
                    gwMaxDailyHeadChangeM * 1000.0f * gwSpecificYield;

                const float currentReceiverDeltaMM =
                    gwStorageDeltaMM[nx][ny];

                const float remainingDailyRiseCapacityMM = std::max(
                    0.0f,
                    maximumDailyRiseMM - currentReceiverDeltaMM
                );

                const float terrainStorageCapacityMM = static_cast<float>(
    std::max(
        0.0,
        (dem[nx][ny] - gwHeadStart[nx][ny]) *
        1000.0 * static_cast<double>(gwSpecificYield)
    )
);

                const float remainingTerrainCapacityMM = std::max(
                    0.0f,
                    terrainStorageCapacityMM - currentReceiverDeltaMM
                );

                const float hydraulicallyFeasibleMM = acceptedMM;

                // The numerical 0.5 m receiver-rise cap is only a stability
                // constraint. Water rejected by this cap remains in the source
                // groundwater store; it must not become surface return flow.
                const float afterDailyRiseCapMM = std::min(
                    hydraulicallyFeasibleMM,
                    remainingDailyRiseCapacityMM
                );

                const float rejectedByDailyRiseCapMM = std::max(
                    0.0f,
                    hydraulicallyFeasibleMM - afterDailyRiseCapMM
                );

                if (rejectedByDailyRiseCapMM > gwFluxEpsilonMM)
                {
                    ++gwReceiverCapacityLimitHits;
                    ++gwReceiverDailyRiseCapHits;
                    gwReceiverDailyRiseRejectedM3 +=
                        rejectedByDailyRiseCapMM *
                        gwDiagCellAreaM2 / 1000.0;
                }

                // Only water rejected because the receiver has no remaining
                // storage below the terrain becomes shallow return flow.
                const float acceptedGroundwaterMM = std::min(
                    afterDailyRiseCapMM,
                    remainingTerrainCapacityMM
                );

                const float divertedToReturnFlowMM = std::max(
                    0.0f,
                    afterDailyRiseCapMM - acceptedGroundwaterMM
                );

                if (acceptedGroundwaterMM > gwFluxEpsilonMM)
                {
                    gwStorageDeltaMM[nx][ny] += acceptedGroundwaterMM;
                    gwLandTransferGrossM3 +=
                        acceptedGroundwaterMM * dx * dy / 1000.0f;
                }

                if (divertedToReturnFlowMM > gwFluxEpsilonMM)
                {
                    // The down-gradient groundwater flux is hydraulically
                    // feasible and within the daily numerical cap, but the
                    // receiver is at the terrain surface. Delay it as return
                    // flow at the receiving cell.
                    shallowReturnFlowStoreMM[nx][ny] +=
                        divertedToReturnFlowMM;

                    const double divertedVolumeM3 =
                        divertedToReturnFlowMM * gwDiagCellAreaM2 / 1000.0;
                    gwReturnFlowGeneratedM3 += divertedVolumeM3;
                    gwReturnFlowGeneratedByReceiverCapacityM3 +=
                        divertedVolumeM3;
                    ++gwReceiverCapacityLimitHits;
                    ++gwReceiverTerrainCapacityHits;
                }

                // Accepted groundwater and terrain-capacity return flow leave
                // the source. Water rejected only by the daily rise cap stays.
                acceptedMM =
                    acceptedGroundwaterMM + divertedToReturnFlowMM;
            }

            if (acceptedMM > 0.0f)
            {
                gwStorageDeltaMM[x][y] -= acceptedMM;
                remainingSourceMM -= acceptedMM;
                actualSourceOutMM += acceptedMM;
            }
        }

        latflow[x][y] =
            actualSourceOutMM / (1000.0f * gwSpecificYield);

        const double actualSourceOutM3 =
            actualSourceOutMM * gwDiagCellAreaM2 / 1000.0;
        gwActualOutM3 += actualSourceOutM3;
        gwActualOutByGroupM3[gwSourceGroup] += actualSourceOutM3;
        if (gwSourceDischargedChannel)
            ++gwCellsDischargingChannel;
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
                    // This should become uncommon after cumulative receiver
                    // limiting. Preserve mass in the delayed return-flow store
                    // if numerical convergence still raises a cell above DEM.
                    shallowReturnFlowStoreMM[x][y] += exfiltrationMM;
                    const double volumeM3 =
                        exfiltrationMM * gwDiagCellAreaM2 / 1000.0;
                    gwRechargeExfiltrationM3 +=
                        static_cast<float>(volumeM3);
                    gwReturnFlowGeneratedM3 += volumeM3;
                }

                newHead = dem[x][y];
                ++gwSurfaceClamps;
            }

            if (Gref[x][y] != noData && newHead < Gref[x][y])
            {
                const float correctionMM =
                    (Gref[x][y] - newHead) *
                    1000.0f * gwSpecificYield;
                gwReferenceFloorCorrectionM3 +=
                    correctionMM * gwDiagCellAreaM2 / 1000.0;
                ++gwReferenceFloorCorrections;
                newHead = Gref[x][y];
            }

            const float headChangeM = newHead - G[x][y][0];
            const float absHeadChangeM = headChangeM < 0.0f ? -headChangeM : headChangeM;
            if (absHeadChangeM > gwMaximumAppliedHeadChangeM)
                gwMaximumAppliedHeadChangeM = absHeadChangeM;

            G[x][y][0] = newHead;
            G[x][y][1] = newHead;

            if (Gref[x][y] != noData && Gref[x][y] == Gref[x][y])
            {
                const double endActiveHeadM = std::max(
                    0.0,
                    static_cast<double>(newHead - Gref[x][y])
                );
                gwEndActiveHeadSumM += endActiveHeadM;
                gwEndMobileStorageM3 +=
                    endActiveHeadM * static_cast<double>(gwSpecificYield) *
                    gwDiagCellAreaM2;
            }

            ++gwEndLandCells;
            const float endDtGM = dem[x][y] - newHead;
            gwEndDtGSumM += endDtGM;
            if (endDtGM < 0.25f)
                ++gwEndDtGLt025;
            else if (endDtGM < 0.50f)
                ++gwEndDtG025To05;
            else if (endDtGM < 1.0f)
                ++gwEndDtG05To1;
            else if (endDtGM < 2.0f)
                ++gwEndDtG1To2;
            else if (endDtGM < 5.0f)
                ++gwEndDtG2To5;
            else
                ++gwEndDtGGe5;

            if (gwLateralInitialHeadReference[x][y] != noData)
            {
                const double endHeadAnomalyM =
                    static_cast<double>(
                        newHead - gwLateralInitialHeadReference[x][y]
                    );
                const double endStorageAnomalyM3 =
                    endHeadAnomalyM * static_cast<double>(gwSpecificYield) *
                    gwDiagCellAreaM2;
                const int group = gwRecessionDiagGroup(x, y);
                gwEndNetStorageByGroupM3[group] += endStorageAnomalyM3;
                if (endStorageAnomalyM3 >= 0.0)
                    gwEndPositiveStorageAnomalyM3 += endStorageAnomalyM3;
                else
                    gwEndNegativeStorageAnomalyM3 += endStorageAnomalyM3;
            }
        }
        else
        {
            // Fixed heads remain fixed after receiving groundwater discharge.
            G[x][y][1] = G[x][y][0];
        }
    }
}

// End-of-day shallow return-flow storage.
for (int x = 0; x < xSize; ++x)
{
    for (int y = 0; y < ySize; ++y)
    {
        if (dem[x][y] != noData &&
            !isChannelCell(x, y) &&
            lakes[x][y] == noData)
        {
            gwReturnFlowEndStorageM3 +=
                shallowReturnFlowStoreMM[x][y] *
                gwDiagCellAreaM2 / 1000.0;
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
    gwToChannelM3 + gwToLakeM3 +
    gwReturnFlowGeneratedByReceiverCapacityM3
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
            << "surface_clamps max_applied_head_change_m "
            << "return_flow_generated_m3 return_flow_released_m3 "
            << "return_flow_end_storage_m3 receiver_capacity_limit_hits "
            << "receiver_daily_rise_cap_hits receiver_terrain_capacity_hits "
            << "receiver_daily_rise_rejected_m3 "
            << "receiver_capacity_to_return_flow_m3 gw_mass_residual_m3\n";
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
        << gwReturnFlowGeneratedM3 << " "
        << gwReturnFlowReleasedM3 << " "
        << gwReturnFlowEndStorageM3 << " "
        << gwReceiverCapacityLimitHits << " "
        << gwReceiverDailyRiseCapHits << " "
        << gwReceiverTerrainCapacityHits << " "
        << gwReceiverDailyRiseRejectedM3 << " "
        << gwReturnFlowGeneratedByReceiverCapacityM3 << " "
        << gwMassResidualM3 << "\n";
}



// -----------------------------------------------------------------------------
// Extended groundwater-exchange diagnostic for recession analysis.
// The apparent turnover time uses positive storage anomaly relative to model
// start divided by today's groundwater losses to channel, lake and surface.
// -----------------------------------------------------------------------------
static std::ofstream gwRecessionFluxDiagnostic;
static bool gwRecessionFluxDiagnosticOpened = false;

if (!gwRecessionFluxDiagnosticOpened)
{
    gwRecessionFluxDiagnostic.open(
        ioFolder + "/groundwater_recession_flux_diagnostic_" +
        std::to_string(scenario) + "_" + gid + ".csv"
    );

    if (gwRecessionFluxDiagnostic.is_open())
    {
        gwRecessionFluxDiagnostic
            << "date residence_time_days release_fraction recharge_m3 soil_removal_m3 "
            << "net_soil_exchange_m3 recharge_exfiltration_m3 "
            << "return_flow_generated_m3 return_flow_released_m3 "
            << "return_flow_end_storage_m3 receiver_capacity_limit_hits "
            << "receiver_daily_rise_cap_hits receiver_terrain_capacity_hits "
            << "receiver_daily_rise_rejected_m3 "
            << "receiver_capacity_to_return_flow_m3 "
            << "start_mobile_storage_m3 end_mobile_storage_m3 "
            << "start_mean_active_head_m end_mean_active_head_m "
            << "potential_gw_out_m3 actual_gw_out_m3 "
            << "potential_release_fraction equalisation_limited_m3 rejected_target_m3 "
            << "gw_to_channel_m3 gw_to_lake_m3 gw_land_transfer_gross_m3 "
            << "soil_to_channel_m3 soil_to_lake_m3 soil_land_transfer_gross_m3 "
            << "gw_active_sources cap_hits equalisation_limit_hits cells_without_lower_head "
            << "cells_adjacent_channel cells_discharging_channel surface_clamps "
            << "invalid_reference_cells reference_floor_corrections "
            << "reference_floor_correction_m3 "
            << "mean_gradient mean_potential_head_change_m max_applied_head_change_m "
            << "start_positive_storage_anomaly_m3 start_negative_storage_anomaly_m3 "
            << "end_positive_storage_anomaly_m3 end_negative_storage_anomaly_m3 "
            << "end_net_storage_anomaly_m3 daily_anomaly_change_m3 apparent_turnover_days "
            << "end_mean_dtg_m end_dtg_lt_0p25m end_dtg_0p25_0p5m end_dtg_0p5_1m "
            << "end_dtg_1_2m end_dtg_2_5m end_dtg_ge_5m "
            << "gw_channel_forest_m3 gw_channel_mire_m3 gw_channel_agri_m3 gw_channel_other_m3 "
            << "gw_lake_forest_m3 gw_lake_mire_m3 gw_lake_agri_m3 gw_lake_other_m3 "
            << "gw_actual_out_forest_m3 gw_actual_out_mire_m3 "
            << "gw_actual_out_agri_m3 gw_actual_out_other_m3 "
            << "soil_channel_forest_m3 soil_channel_mire_m3 "
            << "soil_channel_agri_m3 soil_channel_other_m3 "
            << "end_storage_forest_m3 end_storage_mire_m3 "
            << "end_storage_agri_m3 end_storage_other_m3 gw_mass_residual_m3\n";
    }

    gwRecessionFluxDiagnosticOpened = true;
}

if (gwRecessionFluxDiagnostic.is_open())
{
    const double potentialReleaseFraction =
        gwPotentialOutM3 > 0.0
        ? gwActualOutM3 / gwPotentialOutM3
        : 0.0;
    const double endNetStorageAnomalyM3 =
        gwEndPositiveStorageAnomalyM3 + gwEndNegativeStorageAnomalyM3;
    const double startNetStorageAnomalyM3 =
        gwStartPositiveStorageAnomalyM3 + gwStartNegativeStorageAnomalyM3;
    const double dailyAnomalyChangeM3 =
        endNetStorageAnomalyM3 - startNetStorageAnomalyM3;
    // Water transferred from groundwater into the return-flow store is a
    // groundwater loss on the day it is generated. Its later release is not
    // counted again in groundwater turnover.
    const double groundwaterLossM3 =
        static_cast<double>(gwToChannelM3) +
        static_cast<double>(gwToLakeM3) +
        static_cast<double>(gwRechargeExfiltrationM3) +
        gwReturnFlowGeneratedByReceiverCapacityM3;
    const double apparentTurnoverDays =
        groundwaterLossM3 > 0.0
        ? gwEndMobileStorageM3 / groundwaterLossM3
        : 0.0;
    const double meanGradient =
        gwGradientCells > 0
        ? gwGradientSum / static_cast<double>(gwGradientCells)
        : 0.0;
    const double meanPotentialHeadChangeM =
        gwActiveSourceCells > 0
        ? gwPotentialHeadChangeSumM /
          static_cast<double>(gwActiveSourceCells)
        : 0.0;
    const double endMeanDtGM =
        gwEndLandCells > 0
        ? gwEndDtGSumM / static_cast<double>(gwEndLandCells)
        : 0.0;
    const double startMeanActiveHeadM =
        gwEndLandCells > 0
        ? gwStartActiveHeadSumM / static_cast<double>(gwEndLandCells)
        : 0.0;
    const double endMeanActiveHeadM =
        gwEndLandCells > 0
        ? gwEndActiveHeadSumM / static_cast<double>(gwEndLandCells)
        : 0.0;

    gwRecessionFluxDiagnostic
        << year[index] << "-" << month[index] << "-" << day[index] << " "
        << gwResidenceTimeDays << " "
        << gwReleaseFraction << " "
        << gwSoilRechargeM3 << " "
        << gwSoilRemovalM3 << " "
        << gwNetSoilExchangeM3 << " "
        << gwRechargeExfiltrationM3 << " "
        << gwReturnFlowGeneratedM3 << " "
        << gwReturnFlowReleasedM3 << " "
        << gwReturnFlowEndStorageM3 << " "
        << gwReceiverCapacityLimitHits << " "
        << gwReceiverDailyRiseCapHits << " "
        << gwReceiverTerrainCapacityHits << " "
        << gwReceiverDailyRiseRejectedM3 << " "
        << gwReturnFlowGeneratedByReceiverCapacityM3 << " "
        << gwStartMobileStorageM3 << " "
        << gwEndMobileStorageM3 << " "
        << startMeanActiveHeadM << " "
        << endMeanActiveHeadM << " "
        << gwPotentialOutM3 << " "
        << gwActualOutM3 << " "
        << potentialReleaseFraction << " "
        << gwEqualisationLimitedM3 << " "
        << gwRejectedTargetM3 << " "
        << gwToChannelM3 << " "
        << gwToLakeM3 << " "
        << gwLandTransferGrossM3 << " "
        << soilToChannelM3 << " "
        << soilToLakeM3 << " "
        << soilLandTransferGrossM3 << " "
        << gwActiveSourceCells << " "
        << gwHeadChangeCapHits << " "
        << gwEqualisationLimitHits << " "
        << gwCellsWithoutLowerHead << " "
        << gwCellsAdjacentChannel << " "
        << gwCellsDischargingChannel << " "
        << gwSurfaceClamps << " "
        << gwInvalidReferenceCells << " "
        << gwReferenceFloorCorrections << " "
        << gwReferenceFloorCorrectionM3 << " "
        << meanGradient << " "
        << meanPotentialHeadChangeM << " "
        << gwMaximumAppliedHeadChangeM << " "
        << gwStartPositiveStorageAnomalyM3 << " "
        << gwStartNegativeStorageAnomalyM3 << " "
        << gwEndPositiveStorageAnomalyM3 << " "
        << gwEndNegativeStorageAnomalyM3 << " "
        << endNetStorageAnomalyM3 << " "
        << dailyAnomalyChangeM3 << " "
        << apparentTurnoverDays << " "
        << endMeanDtGM << " "
        << gwEndDtGLt025 << " "
        << gwEndDtG025To05 << " "
        << gwEndDtG05To1 << " "
        << gwEndDtG1To2 << " "
        << gwEndDtG2To5 << " "
        << gwEndDtGGe5 << " "
        << gwToChannelByGroupM3[0] << " "
        << gwToChannelByGroupM3[1] << " "
        << gwToChannelByGroupM3[2] << " "
        << gwToChannelByGroupM3[3] << " "
        << gwToLakeByGroupM3[0] << " "
        << gwToLakeByGroupM3[1] << " "
        << gwToLakeByGroupM3[2] << " "
        << gwToLakeByGroupM3[3] << " "
        << gwActualOutByGroupM3[0] << " "
        << gwActualOutByGroupM3[1] << " "
        << gwActualOutByGroupM3[2] << " "
        << gwActualOutByGroupM3[3] << " "
        << soilToChannelByGroupM3[0] << " "
        << soilToChannelByGroupM3[1] << " "
        << soilToChannelByGroupM3[2] << " "
        << soilToChannelByGroupM3[3] << " "
        << gwEndNetStorageByGroupM3[0] << " "
        << gwEndNetStorageByGroupM3[1] << " "
        << gwEndNetStorageByGroupM3[2] << " "
        << gwEndNetStorageByGroupM3[3] << " "
        << gwMassResidualM3 << "\n";
}

} // end lateral_subsurface_dynamic_v2.inc
