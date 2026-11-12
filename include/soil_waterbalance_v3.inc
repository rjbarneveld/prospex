// Daily two-store soil-water balance for prospex-Hydro/Sedi
// Diagnostic file and selected-pixel output removed; water balance unchanged.
// Units:
//   rr, snowMelt, SW_EHD, SW[][0], dsc, EHDepth, SD, Qd: mm water depth
//   SWSat_EHD, SWSat, FC: m3/m3
//   drainageRate1, drainageRate2: fraction of drainable water released per day
//   Qd_cum: m3/day over the catchment
//
// Concept:
//   1. ET is taken once from the effective hydrological depth (EHD).
//   2. Rain + snowmelt first fill the EHD; only the remainder becomes surface water.
//   3. Water above field capacity percolates from EHD to the lower profile.
//   4. Lower-profile drainage is calculated over at most 1 m of active drainage depth,
//      rather than multiplying a drainage fraction by the complete depth to groundwater.
//   5. For non-tile-drained cells with dynamic groundwater, a configurable
//      fraction of gravity drainage recharges G; the remainder stays in SW
//      and remains available for faster lateral interflow.
//   6. Unmet ET demand may be supplied by shallow groundwater where the water
//      table lies within the configured rooting depth. Uptake is limited to mobile
//      groundwater storage above Gref and lowers G in true head units.
//   7. Tile drainage and non-dynamic runs retain the original direct-routing behaviour.

float et0 = 0.0f;
if (tm[index] > 0.0f)
    et0 = calculateET0(tm[index], dem[int(xSize / 2)][int(ySize / 2)]);

// Update growing-degree-day states.
if (index > 0)
{
    if (degDayWinterwheat[index - 1] > 0.0f)
    {
        degDayWinterwheat[index] = degDayWinterwheat[index - 1];
        if (tm[index] > 0.0f)
            degDayWinterwheat[index] += tm[index];
    }
    else if (degDaySpringwheat[index - 1] > 0.0f)
    {
        degDaySpringwheat[index] = degDaySpringwheat[index - 1];
        if (tm[index] > 0.0f)
            degDaySpringwheat[index] += tm[index];
    }
}

// Keep this explicit because drainageRate1/2 are used as daily release fractions.
auto swbClamp01 = [](float value) -> float
{
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
};

auto swbMin = [](float a, float b) -> float
{
    return (a < b) ? a : b;
};

auto swbMax0 = [](float value) -> float
{
    return (value > 0.0f) ? value : 0.0f;
};

const float liquidInput = swbMax0(rr[index] + snowMelt); // mm/day
const float activeDrainDepthMax = 1000.0f;               // mm; calibratable later
const float pwpTheta = 0.10f;                            // preserve existing model assumption

// Fraction of gravity drainage transferred to the slow groundwater store
// for non-tile-drained cells when dynamic groundwater is enabled.
// The remainder stays in SW and is available to lateral_subsurface_v2.inc
// later in the same daily timestep.
const float groundwaterRechargeFraction = 0.70f;

// Shallow-groundwater evapotranspiration settings.
// Groundwater is available only within gwETRootingDepthM below the land surface.
// Uptake is driven only by ET demand that could not be met from the EHD.
// Keep gwETSpecificYield consistent with the dynamic-groundwater routines.
const float gwETRootingDepthM = 1.50f;
const float gwETSpecificYield = 0.20f;
const float gwETMaximumDemandFraction = 1.00f;


// -----------------------------------------------------------------------------
// Recession diagnostic: daily soil-water partitioning and storage state.
// The hydrological change in this version is limited to the explicit
// groundwaterRechargeFraction defined above.
// Land-use groups: 0=forest, 1=mire, 2=agriculture, 3=other.
// -----------------------------------------------------------------------------
auto swbDiagGroup = [&](int x, int y) -> int
{
    if (landuse[x][y] == 30) return 0;
    if (landuse[x][y] == 60) return 1;
    if (landuse[x][y] >= 20 && landuse[x][y] <= 23) return 2;
    return 3;
};

const double swbCellAreaM2 = static_cast<double>(dx) * static_cast<double>(dy);
double swbDiagLiquidInputM3 = 0.0;
double swbDiagSurfaceStartM3 = 0.0;
double swbDiagETPotM3 = 0.0;
double swbDiagETActM3 = 0.0;
double swbDiagETEHDM3 = 0.0;
double swbDiagGWETM3 = 0.0;
double swbDiagUnmetETBeforeGWM3 = 0.0;
double swbDiagUnmetETAfterGWM3 = 0.0;
double swbDiagSurfaceToEHDM3 = 0.0;
double swbDiagEHDToLowerM3 = 0.0;
double swbDiagSaturationExcessM3 = 0.0;
double swbDiagGravityDrainageM3 = 0.0;
double swbDiagGravityRechargeM3 = 0.0;
double swbDiagRetainedInterflowM3 = 0.0;
double swbDiagGWRechargeM3 = 0.0;
double swbDiagRoutedDrainageM3 = 0.0;
double swbDiagDirectImperviousM3 = 0.0;
double swbDiagSurfaceEndM3 = 0.0;
double swbDiagEHDStoreM3 = 0.0;
double swbDiagLowerStoreM3 = 0.0;
double swbDiagLowerAboveFCM3 = 0.0;
double swbDiagRechargeByGroupM3[4] = {0.0, 0.0, 0.0, 0.0};
double swbDiagRetainedInterflowByGroupM3[4] = {0.0, 0.0, 0.0, 0.0};
double swbDiagAboveFCByGroupM3[4] = {0.0, 0.0, 0.0, 0.0};
double swbDiagETActByGroupM3[4] = {0.0, 0.0, 0.0, 0.0};
double swbDiagGWETByGroupM3[4] = {0.0, 0.0, 0.0, 0.0};
double swbDiagGWETDepthWeightedM = 0.0;
long long swbDiagActiveCells = 0;
long long swbDiagGWETCells = 0;
long long swbDiagGWETEligibleCells = 0;
long long swbDiagGWETStorageLimitedCells = 0;
long long swbDiagRechargeCells = 0;
long long swbDiagAboveFCCells = 0;

for (int x = 1; x < xSize - 1; ++x)
{
    for (int y = 1; y < ySize - 1; ++y)
    {
        if (dem[x][y] == noData ||
            channel[x][y] != noData ||
            lakes[x][y] != noData)
        {
            continue;
        }

        Qd = 0.0f;
        float etAct = 0.0f;
        float etPot = 0.0f;
        float availableSWinEHD = 0.0f;
        float availablePoreDepth = 0.0f;
        float infil = 0.0f;   // surface -> EHD [mm]
        float infil2 = 0.0f;  // EHD -> lower profile [mm]
        float ssg = 0.0f;

        const float ehdDepth = (EHDepth[x][y][1] > 0.0f)
                             ? EHDepth[x][y][1]
                             : 0.0f;
        const float profileDepth = (SD[x][y] > 0.0f)
                                 ? SD[x][y]
                                 : 0.0f;


        const int swbDiagLandGroup = swbDiagGroup(x, y);
        const float swbDiagDscStartMM = swbMax0(dsc[x][y]);
        ++swbDiagActiveCells;
        swbDiagLiquidInputM3 += liquidInput * swbCellAreaM2 / 1000.0;
        swbDiagSurfaceStartM3 += swbDiagDscStartMM * swbCellAreaM2 / 1000.0;

        // --------------------------------------------------------------
        // 1. Potential and actual evapotranspiration
        // --------------------------------------------------------------
        if (vegetation[x][y] == 1)
            etPot = 0.0f;
        else if (landuse[x][y] == 30 ||
                 landuse[x][y] == 60 ||
                 vegetation[x][y] == 5)
            etPot = et0;
        else if (vegetation[x][y] == 6 && degDayWinterwheat[index] > 0.0f)
            etPot = et0 * calculateKcFromGDD(degDayWinterwheat[index]);
        else if (vegetation[x][y] == 7 && degDaySpringwheat[index] > 0.0f)
            etPot = et0 * calculateKcFromGDD(degDaySpringwheat[index]);

        etPot = swbMax0(etPot);

        if (ehdDepth > 0.0f)
        {
            availableSWinEHD = swbMax0(
                SW_EHD[x][y] - pwpTheta * ehdDepth
            );
            const float waterAbovePWP = availableSWinEHD;

            const float stressDenominator = swbMax0(
                (FC[x][y] - pwpTheta) * ehdDepth
            );

            float waterStress = 1.0f;
            if (stressDenominator > 0.0f)
                waterStress = swbClamp01(waterAbovePWP / stressDenominator);

            etAct = swbMin(etPot * waterStress, waterAbovePWP);
            SW_EHD[x][y] -= etAct;
            if (SW_EHD[x][y] < 0.0f)
                SW_EHD[x][y] = 0.0f;
        }


        // --------------------------------------------------------------
        // 2. Shallow-groundwater evapotranspiration
        // --------------------------------------------------------------
        const float etActFromEHD = etAct;
        const float unmetETBeforeGW = swbMax0(etPot - etActFromEHD);
        float groundwaterET = 0.0f;
        float groundwaterETDepthM = -1.0f;

        if (dynamicG == 1 &&
            unmetETBeforeGW > 0.0f &&
            gwETRootingDepthM > 0.0f &&
            gwETSpecificYield > 0.0f &&
            G[x][y][1] == G[x][y][1] &&
            G[x][y][1] != noData &&
            G[x][y][1] > -1000.0f &&
            G[x][y][1] < 10000.0f)
        {
            const float dtgM = dem[x][y] - G[x][y][1];

            // Groundwater uptake is allowed only for a shallow water table.
            // A negative dtgM means the head is above the surface; treat its
            // access factor as one, while storage removal remains capped.
            if (dtgM < gwETRootingDepthM)
            {
                ++swbDiagGWETEligibleCells;

                const float nonNegativeDtgM = swbMax0(dtgM);
                const float accessibleSaturatedThicknessM = swbMax0(
                    gwETRootingDepthM - nonNegativeDtgM
                );

                const float depthAccessFraction = swbClamp01(
                    accessibleSaturatedThicknessM / gwETRootingDepthM
                );

                const float groundwaterETDemand =
                    unmetETBeforeGW *
                    depthAccessFraction *
                    swbClamp01(gwETMaximumDemandFraction);

                // Only storage above the terrain-derived lower reference is
                // mobile and available for groundwater ET.
                float mobileGroundwaterStorageMM = 0.0f;
                if (Gref[x][y] != noData &&
                    Gref[x][y] == Gref[x][y] &&
                    G[x][y][1] > Gref[x][y])
                {
                    mobileGroundwaterStorageMM =
                        (G[x][y][1] - Gref[x][y]) *
                        1000.0f * gwETSpecificYield;
                }

                groundwaterET = swbMin(
                    groundwaterETDemand,
                    mobileGroundwaterStorageMM
                );

                if (groundwaterETDemand > mobileGroundwaterStorageMM + 1.0e-6f)
                    ++swbDiagGWETStorageLimitedCells;

                if (groundwaterET > 0.0f)
                {
                    G[x][y][1] -=
                        groundwaterET /
                        (1000.0f * gwETSpecificYield);

                    if (G[x][y][1] < Gref[x][y])
                        G[x][y][1] = Gref[x][y];

                    etAct += groundwaterET;
                    groundwaterETDepthM = nonNegativeDtgM;
                    ++swbDiagGWETCells;
                }
            }
        }

        const float unmetETAfterGW = swbMax0(etPot - etAct);

        swbDiagETPotM3 += etPot * swbCellAreaM2 / 1000.0;
        swbDiagETActM3 += etAct * swbCellAreaM2 / 1000.0;
        swbDiagETEHDM3 += etActFromEHD * swbCellAreaM2 / 1000.0;
        swbDiagGWETM3 += groundwaterET * swbCellAreaM2 / 1000.0;
        swbDiagUnmetETBeforeGWM3 +=
            unmetETBeforeGW * swbCellAreaM2 / 1000.0;
        swbDiagUnmetETAfterGWM3 +=
            unmetETAfterGW * swbCellAreaM2 / 1000.0;
        swbDiagETActByGroupM3[swbDiagLandGroup] +=
            etAct * swbCellAreaM2 / 1000.0;
        swbDiagGWETByGroupM3[swbDiagLandGroup] +=
            groundwaterET * swbCellAreaM2 / 1000.0;

        if (groundwaterET > 0.0f && groundwaterETDepthM >= 0.0f)
        {
            swbDiagGWETDepthWeightedM +=
                groundwaterETDepthM *
                groundwaterET *
                swbCellAreaM2 / 1000.0;
        }

        // --------------------------------------------------------------
        // 3. Directly connected impervious drainage
        // --------------------------------------------------------------
        float directDrainFraction = 0.0f;
        if (landuse[x][y] == 11)
            directDrainFraction = 0.50f; // built-up
        else if (landuse[x][y] == 12)
            directDrainFraction = 1.00f; // road

        const float directDrain = liquidInput * directDrainFraction;
        const float perviousInput = liquidInput - directDrain;

        swbDiagDirectImperviousM3 +=
            directDrain * swbCellAreaM2 / 1000.0;

        // Existing depression storage from the preceding day is allowed to infiltrate.
        float surfaceWater = swbMax0(dsc[x][y]) + perviousInput;

        // --------------------------------------------------------------
        // 4. Surface infiltration into the EHD
        // --------------------------------------------------------------
        if (ehdDepth > 0.0f && tm[index] > drainageTempEHD)
        {
            const float ehdCapacity = swbMax0(
                SWSat_EHD[x][y] * ehdDepth
            );
            const float ehdPoreSpace = swbMax0(
                ehdCapacity - SW_EHD[x][y]
            );
            availablePoreDepth = ehdPoreSpace;

            // At a daily time step this is a saturation-excess formulation:
            // all water that fits in the EHD infiltrates during the day.
            infil = swbMin(surfaceWater, ehdPoreSpace);
            SW_EHD[x][y] += infil;
            surfaceWater -= infil;
        }

        // The remainder is the water available for overland routing.
        dsc[x][y] = swbMax0(surfaceWater);
        dailySurplus[x][y] = dsc[x][y];

        swbDiagSurfaceToEHDM3 += infil * swbCellAreaM2 / 1000.0;
        swbDiagSurfaceEndM3 += dsc[x][y] * swbCellAreaM2 / 1000.0;

        // --------------------------------------------------------------
        // 5. Percolation from EHD to lower soil profile
        // --------------------------------------------------------------
        if (ehdDepth > 0.0f &&
            profileDepth > 0.0f &&
            tm[index] > drainageTempSD)
        {
            const float ehdFieldCapacity = swbMax0(
                FC[x][y] * ehdDepth
            );
            const float ehdDrainable = swbMax0(
                SW_EHD[x][y] - ehdFieldCapacity
            );

            const float lowerCapacity = swbMax0(
                SWSat[x][y] * profileDepth
            );
            const float lowerPoreSpace = swbMax0(
                lowerCapacity - SW[x][y][0]
            );

            const float rate1 = swbClamp01(drainageRate1[x][y]);
            infil2 = ehdDrainable * rate1;
            infil2 = swbMin(infil2, lowerPoreSpace);
            infil2 = swbMin(infil2, SW_EHD[x][y]);

            SW_EHD[x][y] -= infil2;
            SW[x][y][0] += infil2;
        }


        swbDiagEHDToLowerM3 += infil2 * swbCellAreaM2 / 1000.0;

        // --------------------------------------------------------------
        // 6. Drainage from lower profile
        // --------------------------------------------------------------
        float soilDrainage = 0.0f;
        float saturationExcess = 0.0f;
        float gravityDrainage = 0.0f;

        if (profileDepth > 0.0f && tm[index] > drainageTempSD)
        {
            const float profileCapacity = swbMax0(
                SWSat[x][y] * profileDepth
            );

            // Any numerical/physical saturation excess drains immediately.
            saturationExcess = swbMax0(
                SW[x][y][0] - profileCapacity
            );

            if (saturationExcess > 0.0f)
                SW[x][y][0] -= saturationExcess;

            const float thetaProfile = SW[x][y][0] / profileDepth;
            const float activeDrainDepth = swbMin(
                profileDepth,
                activeDrainDepthMax
            );

            // Calculate gravity drainage over an active depth rather than the
            // complete DEM-to-groundwater profile. This removes the depth-factor bias.
            const float drainableAboveFC = swbMax0(
                (thetaProfile - FC[x][y]) * activeDrainDepth
            );
            const float rate2 = swbClamp01(drainageRate2[x][y]);
            gravityDrainage = drainableAboveFC * rate2;

            // Never remove more than the water stored above field capacity
            // in the complete lower profile.
            const float totalWaterAboveFC = swbMax0(
                SW[x][y][0] - FC[x][y] * profileDepth
            );
            gravityDrainage = swbMin(gravityDrainage, totalWaterAboveFC);

            // Do not remove gravityDrainage here. Its destination is decided
            // below: all of it is removed for tile-drained/non-dynamic cells,
            // whereas only groundwaterRechargeFraction is removed for
            // non-tile-drained cells with dynamic groundwater.
        }


        swbDiagSaturationExcessM3 +=
            saturationExcess * swbCellAreaM2 / 1000.0;
        swbDiagGravityDrainageM3 +=
            gravityDrainage * swbCellAreaM2 / 1000.0;

        // --------------------------------------------------------------
        // 7. Drainage destination and exact accounting
        // --------------------------------------------------------------
        const bool tileDrained =
            (texture[x][y] >= 12 && texture[x][y] <= 19);

        float drainageDeliveredToRiver = directDrain;
        float groundwaterRechargeMM = 0.0f;
        float gravityRechargeMM = 0.0f;
        float retainedForInterflowMM = 0.0f;

        if (tileDrained || dynamicG == 0)
        {
            // Preserve the existing behaviour for tile-drained cells and runs
            // without dynamic groundwater: all gravity drainage leaves SW and
            // is delivered to the configured drainage destination.
            SW[x][y][0] -= gravityDrainage;
            if (SW[x][y][0] < 0.0f)
                SW[x][y][0] = 0.0f;

            soilDrainage = saturationExcess + gravityDrainage;
            Qd = soilDrainage;
            drainageDeliveredToRiver += soilDrainage;
        }
        else
        {
            // Dynamic groundwater on non-tile-drained cells:
            // saturation excess cannot remain in the saturated profile and
            // therefore enters G completely. Only the selected fraction of
            // gravity drainage enters G; the remainder stays in SW and can be
            // released as faster lateral interflow later in this timestep.
            gravityRechargeMM =
                gravityDrainage * groundwaterRechargeFraction;

            retainedForInterflowMM =
                gravityDrainage - gravityRechargeMM;

            SW[x][y][0] -= gravityRechargeMM;
            if (SW[x][y][0] < 0.0f)
                SW[x][y][0] = 0.0f;

            groundwaterRechargeMM =
                saturationExcess + gravityRechargeMM;

            soilDrainage = groundwaterRechargeMM;
            Qd = soilDrainage;

            if (groundwaterRechargeMM > 0.0f)
            {
                // G[][1] is a true groundwater head. Convert recharge water
                // depth to head rise using specific yield here, rather than
                // using the former temporary Sy=1 convention.
                G[x][y][1] +=
                    groundwaterRechargeMM /
                    (1000.0f * gwETSpecificYield);
            }
        }

        swbDiagGravityRechargeM3 +=
            gravityRechargeMM * swbCellAreaM2 / 1000.0;

        const double retainedInterflowM3 =
            retainedForInterflowMM * swbCellAreaM2 / 1000.0;

        swbDiagRetainedInterflowM3 += retainedInterflowM3;
        swbDiagRetainedInterflowByGroupM3[swbDiagLandGroup] +=
            retainedInterflowM3;


        if (groundwaterRechargeMM > 0.0f)
        {
            const double rechargeM3 =
                groundwaterRechargeMM * swbCellAreaM2 / 1000.0;
            swbDiagGWRechargeM3 += rechargeM3;
            swbDiagRechargeByGroupM3[swbDiagLandGroup] += rechargeM3;
            ++swbDiagRechargeCells;
        }

        swbDiagRoutedDrainageM3 +=
            drainageDeliveredToRiver * swbCellAreaM2 / 1000.0;

        if (drainageDeliveredToRiver > 0.0f)
        {
            const int riverX = static_cast<int>(drainRoute[x][y][0]);
            const int riverY = static_cast<int>(drainRoute[x][y][1]);

            if (riverX >= 0 && riverX < xSize &&
                riverY >= 0 && riverY < ySize &&
                dem[riverX][riverY] != noData)
            	{
                // The exact same water depth is reported and routed.
                dsc[riverX][riverY] += drainageDeliveredToRiver;
                Qd_cum += drainageDeliveredToRiver * dx * dy / 1000.0f;
            	}
            /*else
            {
                cerr << "Invalid drainRoute at "
                     << x << ":" << y << " -> "
                     << riverX << ":" << riverY << endl;
            }*/
        }


        swbDiagEHDStoreM3 +=
            swbMax0(SW_EHD[x][y]) * swbCellAreaM2 / 1000.0;
        swbDiagLowerStoreM3 +=
            swbMax0(SW[x][y][0]) * swbCellAreaM2 / 1000.0;

        if (profileDepth > 0.0f)
        {
            const float waterAboveFCMM = swbMax0(
                SW[x][y][0] - FC[x][y] * profileDepth
            );
            const double waterAboveFCM3 =
                waterAboveFCMM * swbCellAreaM2 / 1000.0;
            swbDiagLowerAboveFCM3 += waterAboveFCM3;
            swbDiagAboveFCByGroupM3[swbDiagLandGroup] += waterAboveFCM3;
            if (waterAboveFCMM > 1.0e-6f)
                ++swbDiagAboveFCCells;
        }

        // Preserve the existing subsurface-erosion hook for tile drainage.
        if (erosion == 1 && tileDrained && soilDrainage > 0.0f && profileDepth > 0.0f)
        {
            ssg = 2.2395f
                * exp(0.0226f * SWSat[x][y] / profileDepth)
                * log10(1.0f + soilDrainage) / 6.0f
                * soilDrainage
                * exp(erod[x][y])
                * 0.001f;
            ssg_catchment += ssg;
        }

    }
}
