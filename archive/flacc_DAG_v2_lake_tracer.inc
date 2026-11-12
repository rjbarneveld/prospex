// DAG-based daily MFD routing with a lake-origin source tracer.
// dsc              : total surface/channel water depth [mm]
// dscLakeOrigin    : lake-origin part of dsc [mm]
// flaccMFD         : total outgoing depth from a cell [mm]
// flaccLakeOrigin  : lake-origin outgoing depth from a cell [mm]
// Q_catchmentSRO   : NON-LAKE water crossing land -> channel [m3/day]
// Q_lakeRouted     : lake-origin water crossing land -> channel [m3/day]

Q_catchmentSRO = 0.0f;
Q_lakeRouted = 0.0f;

// Store the preceding timestep's total flux in Q_cum, then reset daily fluxes.
for (int x = 0; x < xSize; ++x)
{
    for (int y = 0; y < ySize; ++y)
    {
        Q_cum[x][y] += flaccMFD[x][y];
        flaccMFD[x][y] = 0.0f;
        flaccLakeOrigin[x][y] = 0.0f;

        // Numerical safety: the tracer can never exceed total water.
        if (dscLakeOrigin[x][y] < 0.0f)
            dscLakeOrigin[x][y] = 0.0f;
        if (dscLakeOrigin[x][y] > dsc[x][y])
            dscLakeOrigin[x][y] = dsc[x][y];
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

    // Perfect-mixing assumption within the cell.
    const float totalBeforeInfiltration = dsc[x][y];
    float lakeBeforeInfiltration = dscLakeOrigin[x][y];
    if (lakeBeforeInfiltration < 0.0f)
        lakeBeforeInfiltration = 0.0f;
    if (lakeBeforeInfiltration > totalBeforeInfiltration)
        lakeBeforeInfiltration = totalBeforeInfiltration;

    // Reinfiltration of routed hillslope water.
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

    // Depression storage remains in the cell; only excess is routed.
    if (dsc[x][y] <= dsc_max[x][y])
        continue;

    const float totalAvailable = dsc[x][y];
    const float lakeAvailable = dscLakeOrigin[x][y];
    float lakeFraction = 0.0f;
    if (totalAvailable > 0.0f)
        lakeFraction = lakeAvailable / totalAvailable;

    if (lakeFraction < 0.0f)
        lakeFraction = 0.0f;
    if (lakeFraction > 1.0f)
        lakeFraction = 1.0f;

    const float toDistribute = totalAvailable - dsc_max[x][y];
    const float lakeToDistribute = toDistribute * lakeFraction;
    const float lakeRetained = dsc_max[x][y] * lakeFraction;

    dsc[x][y] = dsc_max[x][y];
    dscLakeOrigin[x][y] = lakeRetained;

    flaccMFD[x][y] = toDistribute;
    flaccLakeOrigin[x][y] = lakeToDistribute;

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

            const float q = toDistribute * w;
            float qLake = lakeToDistribute * w;

            if (q <= 0.0f)
                continue;

            if (qLake < 0.0f)
                qLake = 0.0f;
            if (qLake > q)
                qLake = q;

            const float qNonLake = q - qLake;

            // Only non-lake water is newly generated hillslope runoff.
            if (!isChannelCell(x, y) && isChannelCell(nx, ny))
            {
                Q_catchmentSRO += qNonLake * dx * dy / 1000.0f;
                Q_lakeRouted += qLake * dx * dy / 1000.0f;
            }

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
                            dscLakeOrigin[outX][outY] += qLake;
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
                    lakeStats[locLake][7] += inflowM3 / lakeStats[locLake][0];
                }

                // No dsc tracer is retained in a lake. Any future release
                // from lake storage is tagged as lake-origin by lakeflow.
            }
            // ---------------- NORMAL / CHANNEL CELL ----------------
            else
            {
                dsc[nx][ny] += q;
                dscLakeOrigin[nx][ny] += qLake;
            }
        }
    }
}

// Hydrograph: outgoing daily total volume at each requested cell.
// Also write a separate lake-origin diagnostic file if it is open.
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
