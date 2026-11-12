// End-of-day dam storage maintenance for prospex-Hydro/Sedi v8.
//
// REQUIRED EXECUTION ORDER:
//      lakeflow_v3 -> flacc_DAG_v8 -> damflow_v8
//
// IMPORTANT v8 behaviour:
//   * Column 9 is pipe capacity [L/s], NOT a release fraction.
//   * The pipe is handled in flacc_DAG_v8.
//   * Detained water is NOT emptied at the end of the day.
//   * Stored water persists and is allowed to drain through the pipe at the
//     start of the following day's flacc_DAG_v8 pass.
//   * This routine only adds direct precipitation to the pond and handles any
//     resulting overflow above Vmax.
//   * Overflow to an ordinary routing cell is written to damTransit, not dsc,
//     and is therefore routed on the following day without reinfiltration.
//
// damStats columns:
//   0 : dam ID
//   1 : dam surface area [m2]
//   2 : outlet elevation
//   3 : outlet x coordinate
//   4 : outlet y coordinate
//   5 : outlet raster x index
//   6 : outlet raster y index
//   7 : maximum storage volume Vmax [m3]
//   8 : current stored volume [m3]
//   9 : pipe capacity [L/s]
//  10 : current-day routed inflow ledger [m3/day]

// -------------------------------------------------------------------------
// 1. Add direct precipitation to pond storage.
// -------------------------------------------------------------------------
for (int i = 0; i < nrDams; ++i)
{
    if (!std::isfinite(damStats[i][8]) || damStats[i][8] < 0.0f)
        damStats[i][8] = 0.0f;

    const float damAreaM2 = damStats[i][1];
    if (damAreaM2 > 0.0f && rr[index] > 0.0f)
        damStats[i][8] += damAreaM2 * rr[index] / 1000.0f;

    if (!std::isfinite(damStoredLakeM3[i]) || damStoredLakeM3[i] < 0.0f)
        damStoredLakeM3[i] = 0.0f;
    if (damStoredLakeM3[i] > damStats[i][8])
        damStoredLakeM3[i] = damStats[i][8];
}

// -------------------------------------------------------------------------
// 2. Build direct dam-to-dam outlet graph so precipitation-driven overflow
//    can cascade through multiple detention ponds in the same end-of-day pass.
// -------------------------------------------------------------------------
std::vector<int> damV8DownstreamDam(nrDams, -1);
std::vector<int> damV8Indegree(nrDams, 0);

for (int i = 0; i < nrDams; ++i)
{
    const int outX = static_cast<int>(damStats[i][5]);
    const int outY = static_cast<int>(damStats[i][6]);

    if (outX < 0 || outX >= xSize || outY < 0 || outY >= ySize)
        continue;
    if (dem[outX][outY] == noData)
        continue;
    if (dams[outX][outY] == noData)
        continue;

    const int downstreamDamID = static_cast<int>(dams[outX][outY]);

    for (int j = 0; j < nrDams; ++j)
    {
        if (j == i)
            continue;

        if (static_cast<int>(damStats[j][0]) == downstreamDamID)
        {
            damV8DownstreamDam[i] = j;
            ++damV8Indegree[j];
            break;
        }
    }
}

std::queue<int> damV8Queue;
for (int i = 0; i < nrDams; ++i)
{
    if (damV8Indegree[i] == 0)
        damV8Queue.push(i);
}

std::vector<int> damV8Order;
damV8Order.reserve(nrDams);

while (!damV8Queue.empty())
{
    const int i = damV8Queue.front();
    damV8Queue.pop();
    damV8Order.push_back(i);

    const int j = damV8DownstreamDam[i];
    if (j >= 0)
    {
        --damV8Indegree[j];
        if (damV8Indegree[j] == 0)
            damV8Queue.push(j);
    }
}

if (static_cast<int>(damV8Order.size()) < nrDams)
{
    std::vector<unsigned char> added(nrDams, 0);
    for (int i : damV8Order)
        added[i] = 1;

    for (int i = 0; i < nrDams; ++i)
    {
        if (!added[i])
            damV8Order.push_back(i);
    }
}

// -------------------------------------------------------------------------
// 3. Spill only storage above Vmax. DO NOT release ordinary retained storage.
// -------------------------------------------------------------------------
for (int damIndex : damV8Order)
{
    float storageM3 = damStats[damIndex][8];
    if (!std::isfinite(storageM3) || storageM3 < 0.0f)
        storageM3 = 0.0f;

    float maxStorageM3 = damStats[damIndex][7];
    if (!std::isfinite(maxStorageM3) || maxStorageM3 < 0.0f)
        maxStorageM3 = 0.0f;

    float overflowM3 = 0.0f;
    float overflowLakeM3 = 0.0f;

    if (storageM3 > maxStorageM3)
    {
        overflowM3 = storageM3 - maxStorageM3;

        float lakeFraction = 0.0f;
        if (storageM3 > 0.0f)
            lakeFraction = damStoredLakeM3[damIndex] / storageM3;

        if (lakeFraction < 0.0f) lakeFraction = 0.0f;
        if (lakeFraction > 1.0f) lakeFraction = 1.0f;

        overflowLakeM3 = overflowM3 * lakeFraction;

        damStats[damIndex][8] = maxStorageM3;
        damStoredLakeM3[damIndex] -= overflowLakeM3;

        if (damStoredLakeM3[damIndex] < 0.0f)
            damStoredLakeM3[damIndex] = 0.0f;
        if (damStoredLakeM3[damIndex] > damStats[damIndex][8])
            damStoredLakeM3[damIndex] = damStats[damIndex][8];
    }
    else
    {
        damStats[damIndex][8] = storageM3;
    }

    if (overflowM3 <= 0.0f)
        continue;

    const int damID = static_cast<int>(damStats[damIndex][0]);
    if (isTerminalDamID(damID))
        continue; // explicit export

    const int downstreamDamIndex = damV8DownstreamDam[damIndex];
    if (downstreamDamIndex >= 0)
    {
        // End-of-day overflow received by another dam becomes storage there;
        // its own Vmax is checked later in this upstream -> downstream pass.
        damStats[downstreamDamIndex][8] += overflowM3;
        damStoredLakeM3[downstreamDamIndex] += overflowLakeM3;
        continue;
    }

    const int outX = static_cast<int>(damStats[damIndex][5]);
    const int outY = static_cast<int>(damStats[damIndex][6]);

    if (outX < 0 || outX >= xSize || outY < 0 || outY >= ySize ||
        dem[outX][outY] == noData)
    {
        continue; // outside active model domain = export
    }

    // lakeflow/flacc have already completed for today. If the outlet is a
    // lake, add the overflow to lake storage now so it is available to the next
    // day's lakeflow. Otherwise keep it as already-routed damTransit for the
    // following day's DAG pass.
    if (lakes[outX][outY] != noData)
    {
        const int locLake = static_cast<int>(lakes[outX][outY]);
        if (locLake >= 0 && locLake < nrLakes && lakeStats[locLake][0] > 0.0f)
        {
            lakeStats[locLake][4] += overflowM3;
            lakeStats[locLake][7] += overflowM3 / lakeStats[locLake][0];
        }
        continue;
    }

    const float overflowMM = overflowM3 / (dx * dy) * 1000.0f;
    damTransit[outX][outY] += overflowMM;
}
