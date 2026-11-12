// DAG-based daily MFD routing with lake-origin source tracing.
//
// v8 DAM CONCEPT
// --------------
// A dam is treated as a detention pond with a bottom outlet (pipe):
//   * flow up to the remaining pipe capacity passes through on the SAME day;
//   * only inflow above the pipe capacity can enter storage;
//   * storage is limited to Vmax;
//   * excess above Vmax spills on the SAME day;
//   * detained storage persists to the next day and is then allowed to drain
//     through the pipe before new inflow uses the remaining daily pipe capacity.
//
// At the model's daily time step, damStats[][9] is interpreted as pipe capacity
// [L/s] and converted to an equivalent daily volume:
//      1 L/s = 86.4 m3/day.
// Thus low-flow days whose total dam throughput remains below the pipe capacity
// are hydraulically transparent: no water is stored merely because a dam exists.
//
// Same-day pipe flow and spill are inserted into damTransit at the selected dam
// outlet during the ONE normal DAG pass. topo_build_v4 provides virtual dependencies
// that guarantee each dam outlet is processed only after all cells that can send
// water into that dam. There is no second pulse-routing pass.
//
// damTransit is already-routed water. It therefore:
//   * bypasses terrestrial reinfiltration;
//   * bypasses hillslope depression storage (dsc_max);
//   * follows the same MFD weights as ordinary routed water;
//   * enters a downstream dam's water balance normally;
//   * is consumed/cleared cell-by-cell during the DAG pass.
//
// IMPORTANT: damTransit must be declared once outside the daily loop and initialised
// to zero. Do NOT reset the complete array at the start of each day: damflow_v8 may
// place end-of-day precipitation overflow in it for routing on the following day.
//
// REQUIRED ORDER IN MAIN PROGRAM:
//      lakeflow_v3 -> flacc_DAG_v8 -> damflow_v8
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

Q_catchmentSRO = 0.0f;
Q_lakeRouted = 0.0f;

const float damCellAreaM2 = static_cast<float>(dx * dy);
const float damLsToM3PerDay = 86.4f;

// Lake-origin fraction retained inside each dam must persist between days.
// This vector is declared static because this include is executed inside the
// daily loop.
static std::vector<float> damStoredLakeM3;
if (static_cast<int>(damStoredLakeM3.size()) != nrDams)
    damStoredLakeM3.assign(nrDams, 0.0f);

// Remaining outlet capacity available during this model day.
std::vector<float> damPipeRemainingM3(nrDams, 0.0f);

// Daily outflow ledger. This is also used when a hydrograph point happens to lie
// inside a dam footprint: in that case we report the dam outlet discharge rather
// than the meaningless flaccMFD value of the skipped storage cell.
std::vector<float> damDailyOutM3(nrDams, 0.0f);
std::vector<float> damDailyOutLakeM3(nrDams, 0.0f);

// Small helper: dam IDs are one-based in the raster/table but row order should
// never be assumed.
auto damFindIndexByID = [&](int damID) -> int
{
    for (int i = 0; i < nrDams; ++i)
    {
        if (static_cast<int>(damStats[i][0]) == damID)
            return i;
    }
    return -1;
};

// Partition a routed inflow volume according to the remaining pipe capacity,
// available storage and spillway overflow. The returned immediate volume must
// be inserted into the dam outlet immediately, while the normal DAG pass is active.
auto damPartitionInflow = [&](int damIndex,
                              float inflowM3,
                              float inflowLakeM3,
                              float& immediateM3,
                              float& immediateLakeM3)
{
    immediateM3 = 0.0f;
    immediateLakeM3 = 0.0f;

    if (damIndex < 0 || damIndex >= nrDams ||
        !std::isfinite(inflowM3) || inflowM3 <= 0.0f)
    {
        return;
    }

    if (!std::isfinite(inflowLakeM3) || inflowLakeM3 < 0.0f)
        inflowLakeM3 = 0.0f;
    if (inflowLakeM3 > inflowM3)
        inflowLakeM3 = inflowM3;

    float lakeFraction = inflowLakeM3 / inflowM3;
    if (lakeFraction < 0.0f) lakeFraction = 0.0f;
    if (lakeFraction > 1.0f) lakeFraction = 1.0f;

    // 1. Free throughflow via the pipe.
    float pipeFlowM3 = std::min(inflowM3, damPipeRemainingM3[damIndex]);
    if (pipeFlowM3 < 0.0f)
        pipeFlowM3 = 0.0f;

    damPipeRemainingM3[damIndex] -= pipeFlowM3;
    if (damPipeRemainingM3[damIndex] < 0.0f)
        damPipeRemainingM3[damIndex] = 0.0f;

    const float excessAfterPipeM3 = inflowM3 - pipeFlowM3;

    // 2. Only the excess above pipe capacity can occupy detention storage.
    float maxStorageM3 = damStats[damIndex][7];
    if (!std::isfinite(maxStorageM3) || maxStorageM3 < 0.0f)
        maxStorageM3 = 0.0f;

    if (!std::isfinite(damStats[damIndex][8]) || damStats[damIndex][8] < 0.0f)
        damStats[damIndex][8] = 0.0f;

    float freeStorageM3 = maxStorageM3 - damStats[damIndex][8];
    if (freeStorageM3 < 0.0f)
        freeStorageM3 = 0.0f;

    const float storedM3 = std::min(excessAfterPipeM3, freeStorageM3);
    const float spillM3 = excessAfterPipeM3 - storedM3;

    damStats[damIndex][8] += storedM3;
    damStoredLakeM3[damIndex] += storedM3 * lakeFraction;

    // 3. Pipe flow and spill stay in today's hydrograph.

    immediateM3 = pipeFlowM3 + spillM3;
    immediateLakeM3 = immediateM3 * lakeFraction;
};

// Insert dam pipe/spill water into its outlet during the SAME normal DAG pass.
// topo_build_v4 guarantees that an ordinary outlet cell has not yet been processed
// when an upstream cell sends flow into the dam.
auto damRouteImmediateToOutlet = [&](int damIndex,
                                     float outM3,
                                     float outLakeM3)
{
    if (damIndex < 0 || damIndex >= nrDams ||
        !std::isfinite(outM3) || outM3 <= 0.0f)
        return;

    if (!std::isfinite(outLakeM3) || outLakeM3 < 0.0f)
        outLakeM3 = 0.0f;
    if (outLakeM3 > outM3)
        outLakeM3 = outM3;

    damDailyOutM3[damIndex] += outM3;
    damDailyOutLakeM3[damIndex] += outLakeM3;

    const int damID = static_cast<int>(damStats[damIndex][0]);
    if (isTerminalDamID(damID))
        return; // explicit export

    const int outX = static_cast<int>(damStats[damIndex][5]);
    const int outY = static_cast<int>(damStats[damIndex][6]);

    if (outX < 0 || outX >= xSize || outY < 0 || outY >= ySize ||
        dem[outX][outY] == noData)
        return; // domain export

    // Current preprocessing should never leave a dam outlet inside another dam.
    // Treat this as a hard diagnostic rather than silently swallowing water.
    if (simulateDams == 1 && dams[outX][outY] != noData)
    {
        std::cerr
            << "ERROR flacc_DAG_v8: outlet of dam ID " << damID
            << " lies inside dam raster ID "
            << static_cast<int>(dams[outX][outY])
            << " at " << outX << ":" << outY << ".\n";
        return;
    }

    if (lakes[outX][outY] != noData)
    {
        const int locLake = static_cast<int>(lakes[outX][outY]);
        if (locLake >= 0 && locLake < nrLakes && lakeStats[locLake][0] > 0.0f)
        {
            lakeStats[locLake][4] += outM3;
            lakeStats[locLake][7] += outM3 / lakeStats[locLake][0];
        }
        return;
    }

    const float outMM = outM3 / damCellAreaM2 * 1000.0f;

    // Once water has passed a dam it is already-routed hydraulic flow. Keep it
    // separate from dsc so it cannot reinfiltrate or be retained by dsc_max.
    // Lake-origin provenance is intentionally not tracked inside damTransit.
    damTransit[outX][outY] += outMM;
};

// -------------------------------------------------------------------------
// 0. Initialise today's dam ledgers and pipe capacities.
//    Storage retained from previous days is released through today's pipe and
//    inserted at the dam outlet before the ordinary DAG pass begins.
// -------------------------------------------------------------------------
std::vector<float> damOldStorageReleaseM3(nrDams, 0.0f);
std::vector<float> damOldStorageReleaseLakeM3(nrDams, 0.0f);

for (int i = 0; i < nrDams; ++i)
{
    damStats[i][10] = 0.0f;

    float pipeCapacityLs = damStats[i][9];
    if (!std::isfinite(pipeCapacityLs) || pipeCapacityLs < 0.0f)
        pipeCapacityLs = 0.0f;

    damPipeRemainingM3[i] = pipeCapacityLs * damLsToM3PerDay;

    if (!std::isfinite(damStats[i][8]) || damStats[i][8] < 0.0f)
        damStats[i][8] = 0.0f;

    if (!std::isfinite(damStoredLakeM3[i]) || damStoredLakeM3[i] < 0.0f)
        damStoredLakeM3[i] = 0.0f;
    if (damStoredLakeM3[i] > damStats[i][8])
        damStoredLakeM3[i] = damStats[i][8];

    const float storageBeforeReleaseM3 = damStats[i][8];
    const float storageReleaseM3 =
        std::min(storageBeforeReleaseM3, damPipeRemainingM3[i]);

    if (storageReleaseM3 > 0.0f)
    {
        float storedLakeFraction = 0.0f;
        if (storageBeforeReleaseM3 > 0.0f)
            storedLakeFraction = damStoredLakeM3[i] / storageBeforeReleaseM3;

        if (storedLakeFraction < 0.0f) storedLakeFraction = 0.0f;
        if (storedLakeFraction > 1.0f) storedLakeFraction = 1.0f;

        const float releasedLakeM3 = storageReleaseM3 * storedLakeFraction;

        damStats[i][8] -= storageReleaseM3;
        damStoredLakeM3[i] -= releasedLakeM3;
        damPipeRemainingM3[i] -= storageReleaseM3;

        if (damStats[i][8] < 0.0f) damStats[i][8] = 0.0f;
        if (damStoredLakeM3[i] < 0.0f) damStoredLakeM3[i] = 0.0f;
        if (damPipeRemainingM3[i] < 0.0f) damPipeRemainingM3[i] = 0.0f;

        damOldStorageReleaseM3[i] = storageReleaseM3;
        damOldStorageReleaseLakeM3[i] = releasedLakeM3;
    }
}

// Reset current-day routed fluxes. Q_cum is updated after the single DAG pass.
for (int x = 0; x < xSize; ++x)
{
    for (int y = 0; y < ySize; ++y)
    {
        flaccMFD[x][y] = 0.0f;
        flaccLakeOrigin[x][y] = 0.0f;

        if (dscLakeOrigin[x][y] < 0.0f)
            dscLakeOrigin[x][y] = 0.0f;
        if (dscLakeOrigin[x][y] > dsc[x][y])
            dscLakeOrigin[x][y] = dsc[x][y];
    }
}

// Seed release of storage retained from previous days into the outlet cells.
// Because this happens before the DAG pass, no special topology dependency is
// required for this part of the flow.
for (int i = 0; i < nrDams; ++i)
{
    if (damOldStorageReleaseM3[i] > 0.0f)
    {
        damRouteImmediateToOutlet(
            i,
            damOldStorageReleaseM3[i],
            damOldStorageReleaseLakeM3[i]
        );
    }
}

// -------------------------------------------------------------------------
// 1. Catchment DAG routing: ordinary dsc + already-routed damTransit.
// -------------------------------------------------------------------------
for (const Cell& c : topoOrder)
{
    const int x = c.x;
    const int y = c.y;

    if (dem[x][y] == noData)
        continue;
    if (lakes[x][y] != noData)
        continue;
    if (simulateDams == 1 && dams[x][y] != noData)
        continue;

    // damTransit is consumed exactly once when this cell is processed. Any
    // transit sent downstream is written only to cells that occur later in
    // topoOrder (guaranteed for dam outlets by topo_build_v4).
    float transitAvailable = damTransit[x][y];
    damTransit[x][y] = 0.0f;

    if (!std::isfinite(transitAvailable) || transitAvailable < 0.0f)
        transitAvailable = 0.0f;

    // ---------------- ORDINARY dsc COMPONENT ----------------
    const float totalBeforeInfiltration = dsc[x][y];
    float lakeBeforeInfiltration = dscLakeOrigin[x][y];
    if (lakeBeforeInfiltration < 0.0f)
        lakeBeforeInfiltration = 0.0f;
    if (lakeBeforeInfiltration > totalBeforeInfiltration)
        lakeBeforeInfiltration = totalBeforeInfiltration;

    // Only ordinary terrestrial water may reinfiltrate. damTransit bypasses
    // this block completely.
    if (!isChannelCell(x, y) &&
        tm[index] > drainageTempEHD &&
        EHDepth[x][y][1] > 0.0f)
    {
        float availablePoreDepth =
            SWSat_EHD[x][y] * EHDepth[x][y][1] - SW_EHD[x][y];

        if (availablePoreDepth < 0.0f)
            availablePoreDepth = 0.0f;

        float reinfiltration =
            (dsc[x][y] < availablePoreDepth)
            ? dsc[x][y]
            : availablePoreDepth;

        if (reinfiltration < 0.0f)
            reinfiltration = 0.0f;

        float lakeFraction = 0.0f;
        if (totalBeforeInfiltration > 0.0f)
            lakeFraction = lakeBeforeInfiltration / totalBeforeInfiltration;

        float lakeReinfiltration = reinfiltration * lakeFraction;
        if (lakeReinfiltration > dscLakeOrigin[x][y])
            lakeReinfiltration = dscLakeOrigin[x][y];

        dsc[x][y] -= reinfiltration;
        dscLakeOrigin[x][y] -= lakeReinfiltration;
        SW_EHD[x][y] += reinfiltration;
    }

    if (dscLakeOrigin[x][y] < 0.0f)
        dscLakeOrigin[x][y] = 0.0f;
    if (dscLakeOrigin[x][y] > dsc[x][y])
        dscLakeOrigin[x][y] = dsc[x][y];

    float ordinaryToDistribute = 0.0f;
    float lakeToDistribute = 0.0f;

    if (dsc[x][y] > dsc_max[x][y])
    {
        const float totalAvailable = dsc[x][y];
        const float lakeAvailable = dscLakeOrigin[x][y];

        float lakeFraction = 0.0f;
        if (totalAvailable > 0.0f)
            lakeFraction = lakeAvailable / totalAvailable;

        if (lakeFraction < 0.0f) lakeFraction = 0.0f;
        if (lakeFraction > 1.0f) lakeFraction = 1.0f;

        ordinaryToDistribute = totalAvailable - dsc_max[x][y];
        lakeToDistribute = ordinaryToDistribute * lakeFraction;
        const float lakeRetained = dsc_max[x][y] * lakeFraction;

        dsc[x][y] = dsc_max[x][y];
        dscLakeOrigin[x][y] = lakeRetained;
    }

    // Total discharge through this cell is ordinary routed water plus damTransit.
    // Keep the components separate below while distributing them downstream.
    flaccMFD[x][y] = ordinaryToDistribute + transitAvailable;
    flaccLakeOrigin[x][y] = lakeToDistribute;

    if (flaccMFD[x][y] > flaccMAX[x][y])
        flaccMAX[x][y] = flaccMFD[x][y];

    if (flaccMFD[x][y] <= 0.0f)
        continue;

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

            const float w = d[x][y][wIndex];
            ++wIndex;

            if (w <= 0.0f)
                continue;

            const int nx = x + X;
            const int ny = y + Y;

            if (nx < 0 || nx >= xSize || ny < 0 || ny >= ySize)
                continue;
            if (dem[nx][ny] == noData)
                continue;

            const float qOrdinary = ordinaryToDistribute * w;
            const float qTransit = transitAvailable * w;
            const float q = qOrdinary + qTransit;

            float qLake = lakeToDistribute * w;
            if (qLake < 0.0f) qLake = 0.0f;
            if (qLake > qOrdinary) qLake = qOrdinary;

            if (q <= 0.0f)
                continue;

            const float qOrdinaryNonLake = qOrdinary - qLake;

            // Catchment SRO diagnostics count only newly routed ordinary water.
            // damTransit has already crossed its original hillslope/channel boundary.
            if (!isChannelCell(x, y) && isChannelCell(nx, ny))
            {
                Q_catchmentSRO += qOrdinaryNonLake * dx * dy / 1000.0f;
                Q_lakeRouted += qLake * dx * dy / 1000.0f;
            }

            // ---------------- DAM ----------------
            if (simulateDams == 1 && dams[nx][ny] != noData)
            {
                const int damID = static_cast<int>(dams[nx][ny]);
                const int damIndex = damFindIndexByID(damID);

                if (damIndex >= 0)
                {
                    const float inflowM3 = q * damCellAreaM2 / 1000.0f;
                    // We deliberately do not retain lake-origin provenance for
                    // the qTransit part. Only the ordinary component contributes.
                    const float inflowLakeM3 = qLake * damCellAreaM2 / 1000.0f;

                    damStats[damIndex][10] += inflowM3;

                    float immediateM3 = 0.0f;
                    float immediateLakeM3 = 0.0f;
                    damPartitionInflow(
                        damIndex,
                        inflowM3,
                        inflowLakeM3,
                        immediateM3,
                        immediateLakeM3
                    );

                    if (immediateM3 > 0.0f)
                    {
                        damRouteImmediateToOutlet(
                            damIndex,
                            immediateM3,
                            immediateLakeM3
                        );
                    }
                }
                else
                {
                    // Invalid/unmatched dam raster value must never swallow flow.
                    // Preserve the two hydraulic components separately.
                    dsc[nx][ny] += qOrdinary;
                    dscLakeOrigin[nx][ny] += qLake;
                    damTransit[nx][ny] += qTransit;
                }
            }
            // ---------------- LAKE ----------------
            else if (lakes[nx][ny] != noData)
            {
                const int locLake = static_cast<int>(lakes[nx][ny]);

                if (locLake >= 0 && locLake < nrLakes &&
                    lakeStats[locLake][0] > 0.0f)
                {
                    // Lakes receive both ordinary routed water and damTransit.
                    const float inflowM3 = q * damCellAreaM2 / 1000.0f;
                    lakeStats[locLake][4] += inflowM3;
                    lakeStats[locLake][7] += inflowM3 / lakeStats[locLake][0];
                }
            }
            // ---------------- NORMAL / CHANNEL CELL ----------------
            else
            {
                // Ordinary flow remains ordinary and may therefore interact with
                // soil/depression storage at the receiver. damTransit stays in its
                // protected routed-water pathway.
                dsc[nx][ny] += qOrdinary;
                dscLakeOrigin[nx][ny] += qLake;
                damTransit[nx][ny] += qTransit;
            }
        }
    }
}

// -------------------------------------------------------------------------
// 2. Final current-day accumulation and hydrographs after the single DAG pass.
// -------------------------------------------------------------------------
for (int x = 0; x < xSize; ++x)
{
    for (int y = 0; y < ySize; ++y)
    {
        Q_cum[x][y] += flaccMFD[x][y];
    }
}

for (const auto& p : pointsHydrograph)
{
    const int outX = static_cast<int>((p.x - originX) / dx);
    const int outY = static_cast<int>((originY - p.y) / dy);

    if (outX < 0 || outX >= xSize || outY < 0 || outY >= ySize)
        continue;

    float Q_hydro_m3d = 0.0f;

    if (simulateDams == 1 && dams[outX][outY] != noData)
    {
        const int damID = static_cast<int>(dams[outX][outY]);
        const int damIndex = damFindIndexByID(damID);

        if (damIndex >= 0)
        {
            // A gauge located inside a detention polygon cannot be represented
            // by flaccMFD at that skipped storage cell. Report the actual dam
            // outlet discharge instead of a spurious zero.
            Q_hydro_m3d = damDailyOutM3[damIndex];

            static std::vector<int> warnedDamHydrographFID;
            bool alreadyWarned = false;
            for (int warnedFID : warnedDamHydrographFID)
            {
                if (warnedFID == p.fid)
                {
                    alreadyWarned = true;
                    break;
                }
            }

            if (!alreadyWarned)
            {
                std::cerr
                    << "WARNING flacc_DAG_v8: hydrograph FID " << p.fid
                    << " lies inside dam ID " << damID
                    << "; reporting dam outlet discharge.\n";
                warnedDamHydrographFID.push_back(p.fid);
            }
        }
    }
    else
    {
        Q_hydro_m3d = flaccMFD[outX][outY] * dx * dy / 1000.0f;
    }

    const float Q_hydro_ls =
        Q_hydro_m3d * 1000.0f / (24.0f * 3600.0f);

    oHydrograph
        << year[index] << "-"
        << month[index] << "-"
        << day[index] << " "
        << p.fid << " "
        << Q_hydro_m3d << " "
        << Q_hydro_ls
        << endl;
}
