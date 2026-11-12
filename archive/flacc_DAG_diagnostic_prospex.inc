// ==========================================================
// DIAGNOSTIC DAG-BASED FLOW ROUTING AND FLOW ACCUMULATION (MFD)
// prospex diagnostic version
//
// Purpose:
//   Preserve existing routing behaviour, but write daily diagnostics for:
//     - surface water stored before routing
//     - potential excess before reinfiltration
//     - reinfiltration during routing
//     - excess generated for routing after reinfiltration
//     - routed flux and hillslope -> channel delivery
//     - MFD weight-sum problems
//
// Output:
//   ioFolder + "/flacc_DAG_diagnostics_<scenario>_<gid>.csv"
//
// Units:
//   dsc, dsc_max, flaccMFD are treated as mm water depth over a raster cell.
//   Volumes are converted to m3 with: mm / 1000 * dx * dy.
// ==========================================================

// ----------------------------------------------------------
// 0. Preserve pre-routing state for diagnostics
// ----------------------------------------------------------
std::vector<std::vector<float>> dsc_original(
    xSize, std::vector<float>(ySize, 0.0f)
);

float diag_pre_surface_storage_m3 = 0.0f;
float diag_pre_excess_all_m3       = 0.0f;
float diag_pre_excess_hill_m3      = 0.0f;
float diag_pre_excess_channel_m3   = 0.0f;
float diag_pre_excess_lake_m3      = 0.0f;

int diag_n_active_cells   = 0;
int diag_n_channel_cells  = 0;
int diag_n_lake_cells     = 0;
int diag_n_dam_cells      = 0;

for(int y = 0; y < ySize; y++)
for(int x = 0; x < xSize; x++)
{
    dsc_original[x][y] = dsc[x][y];

    if (dem[x][y] == noData) continue;

    diag_n_active_cells++;
    if (channel[x][y] == 1) diag_n_channel_cells++;
    if (lakes[x][y] != noData) diag_n_lake_cells++;
    if (dams[x][y]  != noData) diag_n_dam_cells++;

    if (dsc[x][y] > 0.0f)
        diag_pre_surface_storage_m3 += dsc[x][y] / 1000.0f * dx * dy;

    if (dsc[x][y] > dsc_max[x][y])
    {
        float excess_m3 = (dsc[x][y] - dsc_max[x][y]) / 1000.0f * dx * dy;
        diag_pre_excess_all_m3 += excess_m3;

        if (channel[x][y] == 1)
            diag_pre_excess_channel_m3 += excess_m3;
        else if (lakes[x][y] != noData)
            diag_pre_excess_lake_m3 += excess_m3;
        else
            diag_pre_excess_hill_m3 += excess_m3;
    }
}

// ----------------------------------------------------------
// 1. Accumulate previous timestep discharge and reset timestep flux
// ----------------------------------------------------------
Q_catchmentSRO = 0.0;

for (int x = 0; x < xSize; x++)
for (int y = 0; y < ySize; y++)
{
    Q_cum[x][y] += flaccMFD[x][y];
    flaccMFD[x][y] = 0.0f;
}

// ----------------------------------------------------------
// 2. Daily diagnostic counters
// ----------------------------------------------------------
float diag_reinfiltration_m3          = 0.0f;
float diag_reinfiltration_hill_m3     = 0.0f;
float diag_reinfiltration_channel_m3  = 0.0f;

float diag_generated_excess_all_m3      = 0.0f;
float diag_generated_excess_hill_m3     = 0.0f;
float diag_generated_excess_channel_m3  = 0.0f;

float diag_routed_total_m3              = 0.0f;
float diag_routed_from_hill_m3          = 0.0f;
float diag_routed_from_channel_m3       = 0.0f;

float diag_delivered_hill_to_channel_m3 = 0.0f;
float diag_delivered_hill_to_lake_m3    = 0.0f;
float diag_delivered_hill_to_dam_m3     = 0.0f;
float diag_lost_outside_or_nodata_m3    = 0.0f;

float diag_wsum_max              = 0.0f;
float diag_wsum_excess_total     = 0.0f;
int   diag_wsum_gt_1_count       = 0;
int   diag_routing_cells         = 0;
int   diag_hill_to_channel_edges = 0;

float diag_max_toDistribute_mm = 0.0f;
int   diag_max_toDistribute_x  = -1;
int   diag_max_toDistribute_y  = -1;

float diag_max_q_to_channel_mm = 0.0f;
int   diag_max_q_channel_x     = -1;
int   diag_max_q_channel_y     = -1;
int   diag_max_q_channel_nx    = -1;
int   diag_max_q_channel_ny    = -1;

// ----------------------------------------------------------
// 3. Single-pass routing in topological order
// ----------------------------------------------------------
for (const Cell& c : topoOrder)
{
    int x = c.x;
    int y = c.y;

    // Skip inactive cells
    if (dem[x][y] == noData) continue;
    if (lakes[x][y] != noData) continue;
    if (dams[x][y]  != noData) continue;

    // ------------------------------------------------------
    // 3a. Reinfiltration diagnostic + original behaviour
    // ------------------------------------------------------
    float dsc_before_reinf = dsc[x][y];

    float availablePoreDepth =
        (SWSat_EHD[x][y] * EHDepth[x][y][1]) - SW_EHD[x][y];

    if (availablePoreDepth < 0.0f)
        availablePoreDepth = 0.0f;

    if (tm[index] > drainageTempEHD)
    {
        if (availablePoreDepth >= dsc[x][y])
            dsc[x][y] = 0.0f;
        else
            dsc[x][y] -= availablePoreDepth;
    }

    float reinf_mm = dsc_before_reinf - dsc[x][y];
    if (reinf_mm > 0.0f)
    {
        float reinf_m3 = reinf_mm / 1000.0f * dx * dy;
        diag_reinfiltration_m3 += reinf_m3;
        if (channel[x][y] == 1) diag_reinfiltration_channel_m3 += reinf_m3;
        else                    diag_reinfiltration_hill_m3    += reinf_m3;
    }

    // ------------------------------------------------------
    // 3b. Excess surface water available for routing
    // ------------------------------------------------------
    if (dsc[x][y] <= dsc_max[x][y])
        continue;

    float toDistribute = dsc[x][y] - dsc_max[x][y];
    dsc[x][y] = dsc_max[x][y];

    diag_routing_cells++;

    float toDistribute_m3 = toDistribute / 1000.0f * dx * dy;
    diag_generated_excess_all_m3 += toDistribute_m3;
    if (channel[x][y] == 1) diag_generated_excess_channel_m3 += toDistribute_m3;
    else                    diag_generated_excess_hill_m3    += toDistribute_m3;

    if (toDistribute > diag_max_toDistribute_mm)
    {
        diag_max_toDistribute_mm = toDistribute;
        diag_max_toDistribute_x = x;
        diag_max_toDistribute_y = y;
    }

    // Local flux production
    flaccMFD[x][y] += toDistribute;
    if (flaccMFD[x][y] > flaccMAX[x][y])
        flaccMAX[x][y] = flaccMFD[x][y];

    // ------------------------------------------------------
    // 3c. Check outgoing MFD weight sum for this cell
    // ------------------------------------------------------
    float wsum = 0.0f;
    for (int wi = 0; wi < 9; wi++)
    {
        if (wi == 4) continue;
        if (d[x][y][wi] > 0.0f) wsum += d[x][y][wi];
    }

    if (wsum > diag_wsum_max)
        diag_wsum_max = wsum;

    if (wsum > 1.001f)
    {
        diag_wsum_gt_1_count++;
        diag_wsum_excess_total += (wsum - 1.0f);
    }

    // ------------------------------------------------------
    // 3d. Route excess to downslope neighbors (MFD)
    // ------------------------------------------------------
    int wIndex = 0;
    for (int X = -1; X <= 1; X++)
    for (int Y = -1; Y <= 1; Y++)
    {
        if (X == 0 && Y == 0)
        {
            wIndex++;
            continue;
        }

        float w = d[x][y][wIndex];
        if (w <= 0.0f)
        {
            wIndex++;
            continue;
        }

        int nx = x + X;
        int ny = y + Y;

        float q = toDistribute * w;
        float q_m3 = q / 1000.0f * dx * dy;

        // Bounds & activity check
        if (nx < 0 || nx >= xSize || ny < 0 || ny >= ySize)
        {
            diag_lost_outside_or_nodata_m3 += q_m3;
            wIndex++;
            continue;
        }

        if (dem[nx][ny] == noData)
        {
            diag_lost_outside_or_nodata_m3 += q_m3;
            wIndex++;
            continue;
        }

        diag_routed_total_m3 += q_m3;
        if (channel[x][y] == 1) diag_routed_from_channel_m3 += q_m3;
        else                    diag_routed_from_hill_m3    += q_m3;

        // --------------------------------------------------
        // Hillslope -> channel runoff accounting
        // Original behaviour preserved: Q_catchmentSRO is
        // routed flux q entering channel cells from non-channel cells.
        // --------------------------------------------------
        if (channel[x][y] != 1 && channel[nx][ny] == 1)
        {
            Q_catchmentSRO += q_m3;
            diag_delivered_hill_to_channel_m3 += q_m3;
            diag_hill_to_channel_edges++;

            if (q > diag_max_q_to_channel_mm)
            {
                diag_max_q_to_channel_mm = q;
                diag_max_q_channel_x  = x;
                diag_max_q_channel_y  = y;
                diag_max_q_channel_nx = nx;
                diag_max_q_channel_ny = ny;
            }
        }

        // ---------------- DAM ----------------
        if (dams[nx][ny] != noData)
        {
            if (channel[x][y] != 1)
                diag_delivered_hill_to_dam_m3 += q_m3;

            int damID = dams[nx][ny];
            int damIndex = -1;

            for (int i = 0; i < nrDams; i++)
                if (damStats[i][0] == damID)
                {
                    damIndex = i;
                    break;
                }

            if (damIndex >= 0)
            {
                damStats[damIndex][4] += q_m3;

                if (damStats[damIndex][4] >= damStats[damIndex][1])
                {
                    int outX = (damStats[damIndex][2] - originX) / dx;
                    int outY = (originY - damStats[damIndex][3]) / dy;

                    if(outX >= 0 && outX < xSize && outY >= 0 && outY < ySize)
                    {
                        dsc[outX][outY] += q;
                        flaccMFD[outX][outY] += q;
                    }
                    else
                    {
                        cerr << "Dam outlet outside raster: "
                             << outX << " " << outY << endl;
                    }

                    damStats[damIndex][4] = damStats[damIndex][1];
                }
            }
        }

        // ---------------- LAKE ----------------
        else if (lakes[nx][ny] != noData)
        {
            if (channel[x][y] != 1)
                diag_delivered_hill_to_lake_m3 += q_m3;

            int locLake = -1;
            for (int i = 0; i < nrLakes; i++)
                if (lakes[nx][ny] == lakeStats[i][8])
                {
                    locLake = i;
                    break;
                }

            if (locLake >= 0)
            {
                lakeStats[locLake][7] +=
                    q_m3 / lakeStats[locLake][0];

                // Flux still passes into lake cell
                flaccMFD[nx][ny] += q;
            }
        }

        // ---------------- NORMAL / CHANNEL CELL ----------------
        else
        {
            dsc[nx][ny] += q;
            flaccMFD[nx][ny] += q;
        }

        wIndex++;
    }
}

// ----------------------------------------------------------
// 4. Post-routing diagnostics
// ----------------------------------------------------------
float diag_post_surface_storage_m3 = 0.0f;
float diag_post_excess_all_m3      = 0.0f;
float diag_post_excess_hill_m3     = 0.0f;
float diag_post_excess_channel_m3  = 0.0f;
float diag_flacc_sum_m3            = 0.0f;
float diag_flacc_max_mm            = 0.0f;
int   diag_flacc_max_x             = -1;
int   diag_flacc_max_y             = -1;

for(int y = 0; y < ySize; y++)
for(int x = 0; x < xSize; x++)
{
    if (dem[x][y] == noData) continue;

    if (dsc[x][y] > 0.0f)
        diag_post_surface_storage_m3 += dsc[x][y] / 1000.0f * dx * dy;

    if (dsc[x][y] > dsc_max[x][y])
    {
        float excess_m3 = (dsc[x][y] - dsc_max[x][y]) / 1000.0f * dx * dy;
        diag_post_excess_all_m3 += excess_m3;
        if (channel[x][y] == 1) diag_post_excess_channel_m3 += excess_m3;
        else                    diag_post_excess_hill_m3    += excess_m3;
    }

    if (flaccMFD[x][y] > 0.0f)
    {
        diag_flacc_sum_m3 += flaccMFD[x][y] / 1000.0f * dx * dy;
        if (flaccMFD[x][y] > diag_flacc_max_mm)
        {
            diag_flacc_max_mm = flaccMFD[x][y];
            diag_flacc_max_x = x;
            diag_flacc_max_y = y;
        }
    }
}

// ----------------------------------------------------------
// 5. Write diagnostic CSV row
// ----------------------------------------------------------
{
    string diagFile = ioFolder + "/flacc_DAG_diagnostics_" +
                      to_string(scenario) + "_" + gid + ".csv";

    ifstream diagCheck(diagFile.c_str());
    bool diagExists = diagCheck.good();
    diagCheck.close();

    ofstream oDiag(diagFile.c_str(), ios::app);

    if (!diagExists)
    {
        oDiag
        << "date "
        << "pre_surface_storage_m3 "
        << "pre_excess_all_m3 pre_excess_hill_m3 pre_excess_channel_m3 pre_excess_lake_m3 "
        << "reinfiltration_m3 reinfiltration_hill_m3 reinfiltration_channel_m3 "
        << "generated_excess_all_m3 generated_excess_hill_m3 generated_excess_channel_m3 "
        << "routed_total_m3 routed_from_hill_m3 routed_from_channel_m3 "
        << "Q_catchmentSRO_m3 delivered_hill_to_channel_m3 delivered_hill_to_lake_m3 delivered_hill_to_dam_m3 "
        << "lost_outside_or_nodata_m3 "
        << "post_surface_storage_m3 post_excess_all_m3 post_excess_hill_m3 post_excess_channel_m3 "
        << "flacc_sum_m3 flacc_max_mm flacc_max_x flacc_max_y "
        << "wsum_max wsum_gt_1_count wsum_excess_total "
        << "routing_cells hill_to_channel_edges "
        << "max_toDistribute_mm max_toDistribute_x max_toDistribute_y "
        << "max_q_to_channel_mm max_q_channel_x max_q_channel_y max_q_channel_nx max_q_channel_ny "
        << "n_active_cells n_channel_cells n_lake_cells n_dam_cells"
        << endl;
    }

    oDiag
    << year[index] << "-" << month[index] << "-" << day[index] << " "
    << diag_pre_surface_storage_m3 << " "
    << diag_pre_excess_all_m3 << " "
    << diag_pre_excess_hill_m3 << " "
    << diag_pre_excess_channel_m3 << " "
    << diag_pre_excess_lake_m3 << " "
    << diag_reinfiltration_m3 << " "
    << diag_reinfiltration_hill_m3 << " "
    << diag_reinfiltration_channel_m3 << " "
    << diag_generated_excess_all_m3 << " "
    << diag_generated_excess_hill_m3 << " "
    << diag_generated_excess_channel_m3 << " "
    << diag_routed_total_m3 << " "
    << diag_routed_from_hill_m3 << " "
    << diag_routed_from_channel_m3 << " "
    << Q_catchmentSRO << " "
    << diag_delivered_hill_to_channel_m3 << " "
    << diag_delivered_hill_to_lake_m3 << " "
    << diag_delivered_hill_to_dam_m3 << " "
    << diag_lost_outside_or_nodata_m3 << " "
    << diag_post_surface_storage_m3 << " "
    << diag_post_excess_all_m3 << " "
    << diag_post_excess_hill_m3 << " "
    << diag_post_excess_channel_m3 << " "
    << diag_flacc_sum_m3 << " "
    << diag_flacc_max_mm << " "
    << diag_flacc_max_x << " "
    << diag_flacc_max_y << " "
    << diag_wsum_max << " "
    << diag_wsum_gt_1_count << " "
    << diag_wsum_excess_total << " "
    << diag_routing_cells << " "
    << diag_hill_to_channel_edges << " "
    << diag_max_toDistribute_mm << " "
    << diag_max_toDistribute_x << " "
    << diag_max_toDistribute_y << " "
    << diag_max_q_to_channel_mm << " "
    << diag_max_q_channel_x << " "
    << diag_max_q_channel_y << " "
    << diag_max_q_channel_nx << " "
    << diag_max_q_channel_ny << " "
    << diag_n_active_cells << " "
    << diag_n_channel_cells << " "
    << diag_n_lake_cells << " "
    << diag_n_dam_cells
    << endl;

    oDiag.close();
}

// ----------------------------------------------------------
// 6. Console warning on suspicious days
// ----------------------------------------------------------
if (diag_wsum_gt_1_count > 0 || Q_catchmentSRO > diag_generated_excess_hill_m3 * 1.25f)
{
    cout << "FLACC_DIAG "
         << year[index] << "-" << month[index] << "-" << day[index]
         << " SRO=" << Q_catchmentSRO
         << " generated_hill=" << diag_generated_excess_hill_m3
         << " routed_hill=" << diag_routed_from_hill_m3
         << " reinf=" << diag_reinfiltration_m3
         << " wsum_max=" << diag_wsum_max
         << " wsum_gt1=" << diag_wsum_gt_1_count
         << endl;
}

// ----------------------------------------------------------
// 7. Hydrograph output (unchanged)
// ----------------------------------------------------------
for (const auto& p : pointsHydrograph)
{
    int outX = (p.x - originX) / dx;
    int outY = (originY - p.y) / dy;

    oHydrograph
        << year[index] << "-"
        << month[index] << "-"
        << day[index] << " "
        << p.fid << " "
        << flaccMFD[outX][outY] << " "
        << 1000.0 * flaccMFD[outX][outY] / (24.0 * 3600.0)
        << endl;
}
