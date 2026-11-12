// DAG-based daily MFD routing for prospex-Hydro/Sedi
// dsc       : surface/channel water depth present at a cell [mm]
// flaccMFD  : water depth passing out of a cell during this timestep [mm]
// Q_cum     : time-integrated routed depth [mm]
// Q_catchmentSRO: surface runoff crossing from hillslope into channel [m3/day]

Q_catchmentSRO = 0.0f;

// Store the preceding timestep's flux in the cumulative raster, then reset.
for (int x = 0; x < xSize; ++x)
{
    for (int y = 0; y < ySize; ++y)
    {
        Q_cum[x][y] += flaccMFD[x][y];
        flaccMFD[x][y] = 0.0f;
    }
}

for (const Cell& c : topoOrder)
{
    const int x = c.x;
    const int y = c.y;

    if (dem[x][y] == noData)
        continue;
    if (lakes[x][y] != noData)
        continue;
    if (dams[x][y] != noData)
        continue;

    // ----------------------------------------------------------
    // Reinfiltration of routed hillslope water.
    // Water removed from dsc must be added to SW_EHD; otherwise
    // it disappears from the water balance.
    // ----------------------------------------------------------
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

        dsc[x][y] -= reinfiltration;
        SW_EHD[x][y] += reinfiltration;
    }

    // Depression storage remains in the cell; only the excess is routed.
    if (dsc[x][y] <= dsc_max[x][y])
        continue;

    const float toDistribute = dsc[x][y] - dsc_max[x][y];
    dsc[x][y] = dsc_max[x][y];

    // Record the outgoing flux once. Do not also add incoming q values
    // to flaccMFD, because that counts the same water twice.
    flaccMFD[x][y] = toDistribute;

    if (flaccMFD[x][y] > flaccMAX[x][y])
        flaccMAX[x][y] = flaccMFD[x][y];

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

            const float q = toDistribute * w; // mm over one source cell
            if (q <= 0.0f)
                continue;

            // Count true hillslope surface runoff when it first enters a channel.
            // Drainage inserted directly into a channel by soil_waterbalance is
            // therefore not reclassified as overland flow here.
            if (!isChannelCell(x, y) && isChannelCell(nx, ny))
                Q_catchmentSRO += q * dx * dy / 1000.0f;

            // ---------------- DAM ----------------
            if (dams[nx][ny] != noData)
            {
                const int damID = static_cast<int>(dams[nx][ny]);
                int damIndex = -1;

                for (int i = 0; i < nrDams; ++i)
                {
                    if (static_cast<int>(damStats[i][0]) == damID)
                    {
                        damIndex = i;
                        break;
                    }
                }

                if (damIndex >= 0)
                {
                    damStats[damIndex][4] += q * dx * dy / 1000.0f;

                    if (damStats[damIndex][4] >= damStats[damIndex][1])
                    {
                        const int outX = static_cast<int>(
                            (damStats[damIndex][2] - originX) / dx
                        );
                        const int outY = static_cast<int>(
                            (originY - damStats[damIndex][3]) / dy
                        );

                        if (outX >= 0 && outX < xSize &&
                            outY >= 0 && outY < ySize)
                        {
                            dsc[outX][outY] += q;
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
                int locLake = -1;

                for (int i = 0; i < nrLakes; ++i)
                {
                    if (lakes[nx][ny] == lakeStats[i][8])
                    {
                        locLake = i;
                        break;
                    }
                }

                if (locLake >= 0)
                {
                    const float inflowM3 = q * dx * dy / 1000.0f;
                    int outX=lakeStats[locLake][1];
		    int outY=lakeStats[locLake][2];
		                  dsc[outX][outY] += q;
                    lakeStats[locLake][7] += inflowM3 / lakeStats[locLake][0];
                }
            }
            // ---------------- NORMAL / CHANNEL CELL ----------------
            else
            {
                // Transfer water through dsc only. The destination records its
                // outgoing flux when its own turn in topoOrder is reached.
                dsc[nx][ny] += q;
            }
        }
    }
}

// Hydrograph: outgoing daily volume at each requested cell.
for (const auto& p : pointsHydrograph)
{
    const int outX = static_cast<int>((p.x - originX) / dx);
    const int outY = static_cast<int>((originY - p.y) / dy);

    if (outX < 0 || outX >= xSize || outY < 0 || outY >= ySize)
    {
        cerr << "Hydrograph point outside raster: FID=" << p.fid
             << " cell=" << outX << ":" << outY << endl;
        continue;
    }

    const float Q_hydro_m3d =
        flaccMFD[outX][outY] * dx * dy / 1000.0f;
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
