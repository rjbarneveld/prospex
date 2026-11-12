{// Entire include is one compound statement for: if(dynamicG==1) #include ...
// -----------------------------------------------------------------------------
// update_soil_layers_dynamic_v2.inc
//
// Conservative preparation of the daily groundwater state.
// Intended execution order:
//   1. update_soil_layers_dynamic_v2.inc
//   2. soil_waterbalance_daily_v2.inc
//   3. lateral_subsurface_dynamic_v2.inc
//
// Main changes relative to the earlier implementation:
//   * G[][1] is committed to G[][0] in a complete first pass, so gradients never
//     mix old and new time levels.
//   * The calibrated EHD and lower-profile depth are stored once as references.
//   * The active unsaturated part of the EHD can shrink under shallow groundwater,
//     but it can recover when groundwater falls.
//   * SD remains a fixed conceptual lower-profile thickness.
//   * Water displaced from an EHD whose active capacity shrinks is conserved by
//     adding it to dsc as saturation-excess water.
//   * Groundwater above the land surface is converted to exfiltration using a
//     specific yield instead of treating head overshoot as an equal water depth.
//   * Channel and lake cells are fixed-head boundaries.
//   * Gref is the terrain-derived lower groundwater reference. Active land
//     groundwater heads are constrained to Gref <= G <= DEM.
//   * slopeG and dG use a normal 3x3 index layout (indices 0..8, centre=4).
//
// Calibration constants for the first dynamic-groundwater version.
// The specific yield controls the conversion between groundwater storage and
// water-table movement: water depth [mm] = Sy * head change [m] * 1000.
// -----------------------------------------------------------------------------
const float gwSpecificYield = 0.20f;
const float gwMinimumActiveEHDmm = 0.0f;
const float gwReferenceToleranceM = 1.0e-6f;

// Fixed reference geometry, initialised on the first dynamic-groundwater day.
static bool gwGeometryInitialised = false;
static std::vector<std::vector<float>> gwEHDReference;
static std::vector<std::vector<float>> gwSDReference;
static std::vector<std::vector<float>> gwInitialHeadReference;

if (!gwGeometryInitialised)
{
    gwEHDReference.assign(
        xSize,
        std::vector<float>(ySize, 0.0f)
    );
    gwSDReference.assign(
        xSize,
        std::vector<float>(ySize, 0.0f)
    );
    gwInitialHeadReference.assign(
        xSize,
        std::vector<float>(ySize, noData)
    );

    for (int x = 0; x < xSize; ++x)
    {
        for (int y = 0; y < ySize; ++y)
        {
            if (dem[x][y] == noData)
                continue;

            // EHDepth[][0] is the calibrated value set in convert_input.inc.
            gwEHDReference[x][y] =
                EHDepth[x][y][0] > 0.0f ? EHDepth[x][y][0] : 0.0f;

            // Keep the lower profile conceptual and fixed. This avoids the
            // water-content jumps caused by changing SD every day.
            gwSDReference[x][y] = SD[x][y] > 0.0f ? SD[x][y] : 0.0f;

            EHDepth[x][y][1] = EHDepth[x][y][0];
            G[x][y][1] = G[x][y][0];
            gwInitialHeadReference[x][y] = G[x][y][0];
        }
    }

    gwGeometryInitialised = true;
}

// Daily diagnostics.
float gwUpdateExfiltrationM3 = 0.0f;
float gwUpdateEHDExcessM3 = 0.0f;
long long gwUpdateLandCells = 0;
long long gwUpdateShallowEHDCells = 0;
long long gwUpdateSurfaceSaturatedCells = 0;
double gwUpdateDtGSumM = 0.0;
double gwUpdateActiveEHDSumMM = 0.0;
float gwUpdateMinHead = 1.0e30f;
float gwUpdateMaxHead = -1.0e30f;

float gwUpdateMinLandHead = 1.0e30f;
float gwUpdateMaxLandHead = -1.0e30f;
long long gwUpdateInvalidHeadCells = 0;
long long gwUpdateFixedChannelCells = 0;
long long gwUpdateFixedLakeCells = 0;
long long gwUpdateInvalidLakeIndexCells = 0;
long long gwUpdateInvalidReferenceCells = 0;
long long gwUpdateReferenceFloorCorrections = 0;
double gwUpdateReferenceFloorCorrectionM3 = 0.0;
double gwUpdateMobileStorageM3 = 0.0;
double gwUpdateMobileStorageByGroupM3[4] = {0.0, 0.0, 0.0, 0.0};
double gwUpdateActiveHeadSumM = 0.0;
long long gwUpdateDtGLt025 = 0;
long long gwUpdateDtG025To05 = 0;
long long gwUpdateDtG05To1 = 0;
long long gwUpdateDtG1To2 = 0;
long long gwUpdateDtG2To5 = 0;
long long gwUpdateDtGGe5 = 0;
double gwUpdatePositiveStorageAnomalyM3 = 0.0;
double gwUpdateNegativeStorageAnomalyM3 = 0.0;
double gwUpdateNetStorageByGroupM3[4] = {0.0, 0.0, 0.0, 0.0};
double gwUpdatePositiveStorageByGroupM3[4] = {0.0, 0.0, 0.0, 0.0};
double gwUpdateHeadAnomalyAbsSumM = 0.0;

auto gwUpdateDiagGroup = [&](int x, int y) -> int
{
    if (landuse[x][y] == 30) return 0;
    if (landuse[x][y] == 60) return 1;
    if (landuse[x][y] >= 20 && landuse[x][y] <= 23) return 2;
    return 3;
};

// -----------------------------------------------------------------------------
// PASS 1: commit the complete previous next-state and impose fixed heads.
// No gradients are calculated until every cell has been updated.
// -----------------------------------------------------------------------------
for (int x = 0; x < xSize; ++x)
{
    for (int y = 0; y < ySize; ++y)
    {
        if (dem[x][y] == noData)
        {
            G[x][y][0] = noData;
            G[x][y][1] = noData;
            continue;
        }

        float newHead = G[x][y][1];

        if (isnan(newHead) || isinf(newHead) || newHead == noData)
        {
            ++gwUpdateInvalidHeadCells;
            newHead = G[x][y][0];
        }

        if (isChannelCell(x, y))
        {
            // The channel water level is represented by the channel DEM level.
            newHead = dem[x][y];
            ++gwUpdateFixedChannelCells;
        }
        else if (lakes[x][y] != noData)
        {
            // lakes.tif and lakeStats.csv now use the same zero-based index.
            const int locLake = static_cast<int>(lakes[x][y]);

            if (locLake >= 0 && locLake < nrLakes)
            {
                newHead = lakeStats[locLake][3];
                ++gwUpdateFixedLakeCells;
            }
            else
            {
                newHead = dem[x][y];
                ++gwUpdateInvalidLakeIndexCells;
            }
        }
        else
        {
            const bool validReference =
                Gref[x][y] == Gref[x][y] &&
                Gref[x][y] != noData &&
                Gref[x][y] <= dem[x][y] + gwReferenceToleranceM;

            if (!validReference)
            {
                ++gwUpdateInvalidReferenceCells;
                // A zero-thickness mobile reservoir is safer than allowing an
                // invalid lower reference to act as an artificial deep sink.
                Gref[x][y] = dem[x][y];
            }

            if (newHead > dem[x][y])
            {
                // Convert head above the surface to an equivalent exfiltrated
                // water depth. With Sy < 1, a head overshoot contains less water
                // than the same geometric depth over the complete cell.
                const float exfiltrationMM =
                    (newHead - dem[x][y]) * 1000.0f * gwSpecificYield;

                if (exfiltrationMM > 0.0f)
                {
                    dsc[x][y] += exfiltrationMM;
                    gwUpdateExfiltrationM3 +=
                        exfiltrationMM * dx * dy / 1000.0f;
                }

                newHead = dem[x][y];
            }

            if (newHead < Gref[x][y])
            {
                const float correctionMM =
                    (Gref[x][y] - newHead) *
                    1000.0f * gwSpecificYield;

                gwUpdateReferenceFloorCorrectionM3 +=
                    correctionMM * dx * dy / 1000.0f;
                ++gwUpdateReferenceFloorCorrections;
                newHead = Gref[x][y];
            }
        }

        G[x][y][0] = newHead;
        G[x][y][1] = newHead; // clean state for today's recharge and exchange

        if (newHead < gwUpdateMinHead) gwUpdateMinHead = newHead;
        if (newHead > gwUpdateMaxHead) gwUpdateMaxHead = newHead;
    }
}

// -----------------------------------------------------------------------------
// PASS 2: update only the active EHD thickness. The calibrated EHD reference and
// the lower-profile thickness remain fixed. Storage is never invented.
// -----------------------------------------------------------------------------
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

        ++gwUpdateLandCells;

        const float depthToGroundwaterMM =
            (dem[x][y] - G[x][y][0]) * 1000.0f;


        const float depthToGroundwaterM = depthToGroundwaterMM / 1000.0f;
        if (depthToGroundwaterM < 0.25f)
            ++gwUpdateDtGLt025;
        else if (depthToGroundwaterM < 0.50f)
            ++gwUpdateDtG025To05;
        else if (depthToGroundwaterM < 1.0f)
            ++gwUpdateDtG05To1;
        else if (depthToGroundwaterM < 2.0f)
            ++gwUpdateDtG1To2;
        else if (depthToGroundwaterM < 5.0f)
            ++gwUpdateDtG2To5;
        else
            ++gwUpdateDtGGe5;

        if (G[x][y][0] < gwUpdateMinLandHead)
            gwUpdateMinLandHead = G[x][y][0];
        if (G[x][y][0] > gwUpdateMaxLandHead)
            gwUpdateMaxLandHead = G[x][y][0];

        if (gwInitialHeadReference[x][y] != noData)
        {
            const double headAnomalyM =
                static_cast<double>(G[x][y][0] - gwInitialHeadReference[x][y]);
            const double storageAnomalyM3 =
                headAnomalyM * static_cast<double>(gwSpecificYield) *
                static_cast<double>(dx) * static_cast<double>(dy);
            const int group = gwUpdateDiagGroup(x, y);

            gwUpdateNetStorageByGroupM3[group] += storageAnomalyM3;
            gwUpdateHeadAnomalyAbsSumM +=
                headAnomalyM < 0.0 ? -headAnomalyM : headAnomalyM;

            if (storageAnomalyM3 >= 0.0)
            {
                gwUpdatePositiveStorageAnomalyM3 += storageAnomalyM3;
                gwUpdatePositiveStorageByGroupM3[group] += storageAnomalyM3;
            }
            else
            {
                gwUpdateNegativeStorageAnomalyM3 += storageAnomalyM3;
            }
        }


        if (Gref[x][y] != noData && Gref[x][y] == Gref[x][y])
        {
            const double activeHeadM = std::max(
                0.0,
                static_cast<double>(G[x][y][0] - Gref[x][y])
            );
            const double mobileStorageM3 =
                activeHeadM * static_cast<double>(gwSpecificYield) *
                static_cast<double>(dx) * static_cast<double>(dy);
            const int group = gwUpdateDiagGroup(x, y);

            gwUpdateActiveHeadSumM += activeHeadM;
            gwUpdateMobileStorageM3 += mobileStorageM3;
            gwUpdateMobileStorageByGroupM3[group] += mobileStorageM3;
        }

        float activeEHD = depthToGroundwaterMM;
        if (activeEHD < gwMinimumActiveEHDmm)
            activeEHD = gwMinimumActiveEHDmm;
        if (activeEHD > gwEHDReference[x][y])
            activeEHD = gwEHDReference[x][y];

        if (activeEHD < gwEHDReference[x][y] - 1.0e-4f)
            ++gwUpdateShallowEHDCells;
        if (activeEHD <= 1.0e-4f)
            ++gwUpdateSurfaceSaturatedCells;

        EHDepth[x][y][0] = activeEHD;
        EHDepth[x][y][1] = activeEHD;

        // Keep the lower profile fixed. Groundwater influence on rapid runoff
        // acts through the active EHD storage, not through a moving SD boundary.
        SD[x][y] = gwSDReference[x][y];

        const float activeEHDCapacityMM =
            SWSat_EHD[x][y] * activeEHD;

        if (SW_EHD[x][y] > activeEHDCapacityMM)
        {
            const float displacedMM =
                SW_EHD[x][y] - activeEHDCapacityMM;

            SW_EHD[x][y] = activeEHDCapacityMM;
            dsc[x][y] += displacedMM;

            gwUpdateEHDExcessM3 +=
                displacedMM * dx * dy / 1000.0f;
        }

        if (SW_EHD[x][y] < 0.0f)
            SW_EHD[x][y] = 0.0f;

        // Keep the lower-store state valid without changing its mass unless a
        // previous numerical error placed it above saturation. Such excess is
        // transferred to groundwater storage, not deleted.
        const float lowerCapacityMM = SWSat[x][y] * SD[x][y];
        if (SW[x][y][0] > lowerCapacityMM)
        {
            const float lowerExcessMM = SW[x][y][0] - lowerCapacityMM;
            SW[x][y][0] = lowerCapacityMM;
            // G[][1] is a true head. Convert the excess water depth to a
            // groundwater-head rise using specific yield.
            G[x][y][1] +=
                lowerExcessMM / (1000.0f * gwSpecificYield);
        }
        if (SW[x][y][0] < 0.0f)
            SW[x][y][0] = 0.0f;
        SW[x][y][1] = SW[x][y][0];

        gwUpdateDtGSumM +=
            static_cast<double>(depthToGroundwaterMM) / 1000.0;
        gwUpdateActiveEHDSumMM += activeEHD;
    }
}

// -----------------------------------------------------------------------------
// PASS 3: calculate pre-recharge groundwater gradients and directional weights
// from the complete, frozen G[][0] field. The lateral module recalculates these
// once more after today's recharge has been added.
// -----------------------------------------------------------------------------
for (int x = 0; x < xSize; ++x)
{
    for (int y = 0; y < ySize; ++y)
    {
        for (std::size_t i = 0; i < dG[x][y].size(); ++i)
            dG[x][y][i] = 0.0f;
        slopeG[x][y] = 0.0f;
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

        // Stable cardinal finite-difference estimate of the head-gradient
        // magnitude. Invalid neighbours fall back to a one-sided difference.
        float gx = 0.0f;
        float gy = 0.0f;

        const bool westValid = dem[x - 1][y] != noData;
        const bool eastValid = dem[x + 1][y] != noData;
        const bool northValid = dem[x][y - 1] != noData;
        const bool southValid = dem[x][y + 1] != noData;

        if (westValid && eastValid)
            gx = (G[x + 1][y][0] - G[x - 1][y][0]) / (2.0f * dx);
        else if (eastValid)
            gx = (G[x + 1][y][0] - G[x][y][0]) / dx;
        else if (westValid)
            gx = (G[x][y][0] - G[x - 1][y][0]) / dx;

        if (northValid && southValid)
            gy = (G[x][y + 1][0] - G[x][y - 1][0]) / (2.0f * dy);
        else if (southValid)
            gy = (G[x][y + 1][0] - G[x][y][0]) / dy;
        else if (northValid)
            gy = (G[x][y][0] - G[x][y - 1][0]) / dy;

        const float gradientMagnitude = sqrt(gx * gx + gy * gy);
        slopeG[x][y] = atan(gradientMagnitude);

        // dG uses a compact eight-neighbour order, independent of the
        // nine-position surface-routing array d.
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
                G[nx][ny][0] == noData ||
                G[nx][ny][0] >= G[x][y][0])
            {
                continue;
            }

            const float distance =
                sqrt((X * dx) * (X * dx) + (Y * dy) * (Y * dy));

            if (distance > 0.0f)
            {
                localGradient[dir] =
                    (G[x][y][0] - G[nx][ny][0]) / distance;
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
    }
}

// -----------------------------------------------------------------------------
// Daily state diagnostic.
// -----------------------------------------------------------------------------
static std::ofstream gwStateDiagnostic;
static bool gwStateDiagnosticOpened = false;

if (!gwStateDiagnosticOpened)
{
    gwStateDiagnostic.open(
        ioFolder + "/groundwater_state_diagnostic_" +
        std::to_string(scenario) + "_" + gid + ".csv"
    );

    if (gwStateDiagnostic.is_open())
    {
        gwStateDiagnostic
            << "date land_cells shallow_ehd_cells surface_saturated_cells "
            << "mean_dtg_m mean_active_ehd_mm previous_head_exfiltration_m3 "
            << "ehd_capacity_excess_m3 min_head_m max_head_m\n";
    }

    gwStateDiagnosticOpened = true;
}

if (gwStateDiagnostic.is_open())
{
    const double meanDtGM =
        gwUpdateLandCells > 0
        ? gwUpdateDtGSumM / static_cast<double>(gwUpdateLandCells)
        : 0.0;

    const double meanActiveEHDMM =
        gwUpdateLandCells > 0
        ? gwUpdateActiveEHDSumMM / static_cast<double>(gwUpdateLandCells)
        : 0.0;

    gwStateDiagnostic
        << year[index] << "-" << month[index] << "-" << day[index] << " "
        << gwUpdateLandCells << " "
        << gwUpdateShallowEHDCells << " "
        << gwUpdateSurfaceSaturatedCells << " "
        << meanDtGM << " "
        << meanActiveEHDMM << " "
        << gwUpdateExfiltrationM3 << " "
        << gwUpdateEHDExcessM3 << " "
        << gwUpdateMinHead << " "
        << gwUpdateMaxHead << "\n";
}



// -----------------------------------------------------------------------------
// Extended start-of-day groundwater-state diagnostic for recession analysis.
// Storage anomalies are relative to the groundwater head at model start.
// -----------------------------------------------------------------------------
static std::ofstream gwRecessionStateDiagnostic;
static bool gwRecessionStateDiagnosticOpened = false;

if (!gwRecessionStateDiagnosticOpened)
{
    gwRecessionStateDiagnostic.open(
        ioFolder + "/groundwater_recession_state_diagnostic_" +
        std::to_string(scenario) + "_" + gid + ".csv"
    );

    if (gwRecessionStateDiagnostic.is_open())
    {
        gwRecessionStateDiagnostic
            << "date land_cells invalid_head_cells fixed_channel_cells fixed_lake_cells "
            << "invalid_lake_index_cells invalid_reference_cells "
            << "reference_floor_corrections reference_floor_correction_m3 "
            << "shallow_ehd_cells surface_saturated_cells "
            << "dtg_lt_0p25m dtg_0p25_0p5m dtg_0p5_1m dtg_1_2m dtg_2_5m dtg_ge_5m "
            << "mean_dtg_m mean_active_ehd_mm mean_abs_head_anomaly_m "
            << "mean_active_head_above_ref_m mobile_storage_m3 "
            << "positive_storage_anomaly_m3 negative_storage_anomaly_m3 net_storage_anomaly_m3 "
            << "net_storage_forest_m3 net_storage_mire_m3 net_storage_agri_m3 net_storage_other_m3 "
            << "positive_storage_forest_m3 positive_storage_mire_m3 "
            << "positive_storage_agri_m3 positive_storage_other_m3 "
            << "mobile_storage_forest_m3 mobile_storage_mire_m3 "
            << "mobile_storage_agri_m3 mobile_storage_other_m3 "
            << "previous_head_exfiltration_m3 ehd_capacity_excess_m3 "
            << "min_land_head_m max_land_head_m\n";
    }

    gwRecessionStateDiagnosticOpened = true;
}

if (gwRecessionStateDiagnostic.is_open())
{
    const double meanDtGRecessionM =
        gwUpdateLandCells > 0
        ? gwUpdateDtGSumM / static_cast<double>(gwUpdateLandCells)
        : 0.0;
    const double meanActiveEHDRecessionMM =
        gwUpdateLandCells > 0
        ? gwUpdateActiveEHDSumMM / static_cast<double>(gwUpdateLandCells)
        : 0.0;
    const double meanAbsHeadAnomalyM =
        gwUpdateLandCells > 0
        ? gwUpdateHeadAnomalyAbsSumM / static_cast<double>(gwUpdateLandCells)
        : 0.0;
    const double netStorageAnomalyM3 =
        gwUpdatePositiveStorageAnomalyM3 + gwUpdateNegativeStorageAnomalyM3;
    const double meanActiveHeadAboveRefM =
        gwUpdateLandCells > 0
        ? gwUpdateActiveHeadSumM / static_cast<double>(gwUpdateLandCells)
        : 0.0;

    gwRecessionStateDiagnostic
        << year[index] << "-" << month[index] << "-" << day[index] << " "
        << gwUpdateLandCells << " "
        << gwUpdateInvalidHeadCells << " "
        << gwUpdateFixedChannelCells << " "
        << gwUpdateFixedLakeCells << " "
        << gwUpdateInvalidLakeIndexCells << " "
        << gwUpdateInvalidReferenceCells << " "
        << gwUpdateReferenceFloorCorrections << " "
        << gwUpdateReferenceFloorCorrectionM3 << " "
        << gwUpdateShallowEHDCells << " "
        << gwUpdateSurfaceSaturatedCells << " "
        << gwUpdateDtGLt025 << " "
        << gwUpdateDtG025To05 << " "
        << gwUpdateDtG05To1 << " "
        << gwUpdateDtG1To2 << " "
        << gwUpdateDtG2To5 << " "
        << gwUpdateDtGGe5 << " "
        << meanDtGRecessionM << " "
        << meanActiveEHDRecessionMM << " "
        << meanAbsHeadAnomalyM << " "
        << meanActiveHeadAboveRefM << " "
        << gwUpdateMobileStorageM3 << " "
        << gwUpdatePositiveStorageAnomalyM3 << " "
        << gwUpdateNegativeStorageAnomalyM3 << " "
        << netStorageAnomalyM3 << " "
        << gwUpdateNetStorageByGroupM3[0] << " "
        << gwUpdateNetStorageByGroupM3[1] << " "
        << gwUpdateNetStorageByGroupM3[2] << " "
        << gwUpdateNetStorageByGroupM3[3] << " "
        << gwUpdatePositiveStorageByGroupM3[0] << " "
        << gwUpdatePositiveStorageByGroupM3[1] << " "
        << gwUpdatePositiveStorageByGroupM3[2] << " "
        << gwUpdatePositiveStorageByGroupM3[3] << " "
        << gwUpdateMobileStorageByGroupM3[0] << " "
        << gwUpdateMobileStorageByGroupM3[1] << " "
        << gwUpdateMobileStorageByGroupM3[2] << " "
        << gwUpdateMobileStorageByGroupM3[3] << " "
        << gwUpdateExfiltrationM3 << " "
        << gwUpdateEHDExcessM3 << " "
        << (gwUpdateMinLandHead < 1.0e29f ? gwUpdateMinLandHead : noData) << " "
        << (gwUpdateMaxLandHead > -1.0e29f ? gwUpdateMaxLandHead : noData) << "\n";
}

} // end update_soil_layers_dynamic_v2.inc
