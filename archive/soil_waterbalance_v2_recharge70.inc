// Daily two-store soil-water balance for prospex-Hydro/Sedi
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
//   5. Exactly the same drainage depth is removed from soil, reported in Qd_cum,
//      and routed to the channel. This prevents drainage being re-labelled as overland flow.

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
// Fraction of gravity drainage assigned to the slow groundwater store on
// non-tile-drained cells when dynamic groundwater is enabled. The remainder
// stays in SW and is available to lateral_subsurface_v2.inc as interflow.
const float groundwaterRechargeFraction = 0.70f;

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
        // 2. Directly connected impervious drainage
        // --------------------------------------------------------------
        float directDrainFraction = 0.0f;
        if (landuse[x][y] == 11)
            directDrainFraction = 0.50f; // built-up
        else if (landuse[x][y] == 12)
            directDrainFraction = 1.00f; // road

        const float directDrain = liquidInput * directDrainFraction;
        const float perviousInput = liquidInput - directDrain;

        // Existing depression storage from the preceding day is allowed to infiltrate.
        float surfaceWater = swbMax0(dsc[x][y]) + perviousInput;

        // --------------------------------------------------------------
        // 3. Surface infiltration into the EHD
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

        // --------------------------------------------------------------
        // 4. Percolation from EHD to lower soil profile
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

        // --------------------------------------------------------------
        // 5. Drainage from lower profile
        // --------------------------------------------------------------
        float saturationExcess = 0.0f;
        float gravityDrainage = 0.0f;

        if (profileDepth > 0.0f && tm[index] > drainageTempSD)
        {
            const float profileCapacity = swbMax0(
                SWSat[x][y] * profileDepth
            );

            // Water above saturation cannot remain in the lower profile.
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
            // complete DEM-to-groundwater profile.
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
            gravityDrainage = swbMin(
                gravityDrainage,
                totalWaterAboveFC
            );
        }

        // --------------------------------------------------------------
        // 6. Drainage destination and exact accounting
        // --------------------------------------------------------------
        const bool tileDrained =
            (texture[x][y] >= 12 && texture[x][y] <= 19);

        float soilDrainage = 0.0f;
        float drainageDeliveredToRiver = directDrain;
        float groundwaterRechargeMM = 0.0f;
        float retainedForInterflowMM = 0.0f;

        if (tileDrained || dynamicG == 0)
        {
            // Preserve the original behaviour for tile-drained soils and for
            // simulations without dynamic groundwater: all gravity drainage
            // leaves the lower profile immediately.
            SW[x][y][0] -= gravityDrainage;

            soilDrainage =
                saturationExcess + gravityDrainage;

            drainageDeliveredToRiver += soilDrainage;
        }
        else
        {
            // Only a selected fraction of gravity drainage enters the slow
            // groundwater store. The remainder stays in SW and can be routed
            // as faster lateral interflow by lateral_subsurface_v2.inc later
            // in the same daily time step.
            const float gravityRechargeMM =
                gravityDrainage * swbClamp01(groundwaterRechargeFraction);

            retainedForInterflowMM =
                gravityDrainage - gravityRechargeMM;

            // Remove only the part actually transferred to groundwater.
            SW[x][y][0] -= gravityRechargeMM;

            groundwaterRechargeMM =
                saturationExcess + gravityRechargeMM;

            soilDrainage = groundwaterRechargeMM;

            if (groundwaterRechargeMM > 0.0f)
                G[x][y][1] += groundwaterRechargeMM / 1000.0f;
        }

        if (SW[x][y][0] < 0.0f)
            SW[x][y][0] = 0.0f;

        Qd = soilDrainage;

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

        if (x == pixel_1X && y == pixel_1Y)
        {
            oLU << year[index] << "-" << month[index] << "-" << day[index]
                << " " << infil2 << " infil2 loc1" << endl;
            oLU << year[index] << "-" << month[index] << "-" << day[index]
                << " " << Qd << " Qd loc1" << endl;
        }
        if (x == pixel_2X && y == pixel_2Y)
        {
            oLU << year[index] << "-" << month[index] << "-" << day[index]
                << " " << infil2 << " infil2 loc2" << endl;
            oLU << year[index] << "-" << month[index] << "-" << day[index]
                << " " << Qd << " Qd loc2" << endl;
        }
        if (x == pixel_3X && y == pixel_3Y)
        {
            oLU << year[index] << "-" << month[index] << "-" << day[index]
                << " " << infil2 << " infil2 loc3" << endl;
            oLU << year[index] << "-" << month[index] << "-" << day[index]
                << " " << Qd << " Qd loc3" << endl;
        }

        bool doit = true;
        #include "write_lupoints.inc"
    }
}
