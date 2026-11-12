/* interflow_spatial.inc
   Spatially explicit delayed interflow / tile-drain reservoir.

   Required arrays declared in main before the daily loop:
     float interflowStore[xSize][ySize];     // mm water equivalent stored in delayed subsurface reservoir
     float interflowIn[xSize][ySize];        // mm incoming interflow for next-day/local redistribution

   Required scalar parameters declared in main or run.config:
     float kInterflowBase = 0.08f;           // day-1, starting value; recession tau ≈ 1/k = 12.5 d
     float kInterflowMin  = 0.01f;           // day-1
     float kInterflowMax  = 0.35f;           // day-1
     float fracTileDrain  = 0.70f;           // fraction of released interflow sent to mapped drainRoute on drained soils
     float fracInterflowToChannel = 1.00f;   // accounting factor for water reaching stream/channel

   Coupling from soil_waterbalance.inc:
     Replace direct drain routing/Qd_cum for agricultural drained soils with:
       interflowStore[x][y] += Qd_act;
     and do not add Qd_act directly to dsc[riverX][riverY] or Qd_cum there.

   Include after soil_waterbalance.inc and before flacc_DAG.inc.
*/

float Q_interflow_cum = 0.0f; // m3/day for this day; add this to Qd_cum or write separately

// 1. Add incoming lateral interflow from previous routing step and clear inbox.
for(int x=1; x<xSize-1; x++)
{
    for(int y=1; y<ySize-1; y++)
    {
        if(dem[x][y] != noData)
        {
            interflowStore[x][y] += interflowIn[x][y];
            interflowIn[x][y] = 0.0f;
            if(interflowStore[x][y] < 0.0f) interflowStore[x][y] = 0.0f;
        }
    }
}

// 2. Release a fraction of each cell's interflow storage.
for(int x=1; x<xSize-1; x++)
{
    for(int y=1; y<ySize-1; y++)
    {
        if(dem[x][y] == noData || lakes[x][y] != noData) continue;
        if(interflowStore[x][y] <= 0.0f) continue;

        // Temperature gate: frozen soil suppresses interflow release.
        if(tm[index] <= drainageTempSD) continue;

        // Spatially variable recession coefficient.
        // drainageRate2 is already parameterized by soil/land use in your model.
        float k = kInterflowBase;
        if(drainageRate2[x][y] > 0.0f) k *= (0.5f + drainageRate2[x][y]);
        if(slope[x][y] > 0.0f)         k *= (1.0f + 5.0f * slope[x][y]);
        if(k < kInterflowMin) k = kInterflowMin;
        if(k > kInterflowMax) k = kInterflowMax;

        float qRelease = k * interflowStore[x][y]; // mm/day
        if(qRelease > interflowStore[x][y]) qRelease = interflowStore[x][y];
        interflowStore[x][y] -= qRelease;

        // Drained agricultural mineral soils: route most delayed water to mapped drain outlet.
        bool drainedSoil = (texture[x][y] >= 12 && texture[x][y] <= 19);
        bool hasDrainRoute = (drainRoute[x][y][0] > 0 && drainRoute[x][y][1] > 0 &&
                              drainRoute[x][y][0] < xSize && drainRoute[x][y][1] < ySize);

        if(drainedSoil && hasDrainRoute)
        {
            short int riverX = drainRoute[x][y][0];
            short int riverY = drainRoute[x][y][1];

            float qTile = fracTileDrain * qRelease;
            float qMatrix = qRelease - qTile;

            // Delayed tile/interflow contribution reaches channel water storage for routing.
            if(dem[riverX][riverY] != noData)
            {
                dsc[riverX][riverY] += qTile;
                Q_interflow_cum += fracInterflowToChannel * qTile * dx * dy / 1000.0f;
            }

            // Remaining matrix part moves one cell downslope and may continue on later days.
            if(qMatrix > 0.0f)
            {
                float wSum = 0.0f;
                int wIndex = 0;
                for(int X=-1; X<2; X++)
                {
                    for(int Y=-1; Y<2; Y++)
                    {
                        if(X==0 && Y==0) { wIndex++; continue; }
                        int nx = x + X;
                        int ny = y + Y;
                        if(dem[nx][ny] != noData && dem[nx][ny] < dem[x][y])
                            wSum += d[x][y][wIndex];
                        wIndex++;
                    }
                }

                if(wSum <= 0.0f)
                {
                    interflowStore[x][y] += qMatrix; // no downslope receiver; keep stored
                }
                else
                {
                    wIndex = 0;
                    for(int X=-1; X<2; X++)
                    {
                        for(int Y=-1; Y<2; Y++)
                        {
                            if(X==0 && Y==0) { wIndex++; continue; }
                            int nx = x + X;
                            int ny = y + Y;
                            if(dem[nx][ny] != noData && dem[nx][ny] < dem[x][y])
                            {
                                float qPart = qMatrix * d[x][y][wIndex] / wSum;
                                if(channel[nx][ny] == 1)
                                {
                                    dsc[nx][ny] += qPart;
                                    Q_interflow_cum += fracInterflowToChannel * qPart * dx * dy / 1000.0f;
                                }
                                else if(lakes[nx][ny] != noData)
                                {
                                    int locLake = 0;
                                    for(int lakeIndex=0; lakeIndex<nrLakes; lakeIndex++)
                                        if(lakes[nx][ny] == lakeStats[lakeIndex][8]) locLake = lakeIndex;
                                    lakeStats[locLake][7] += (qPart * dx * dy / 1000.0f) / lakeStats[locLake][0];
                                    Q_lake += fracInterflowToChannel * qPart * dx * dy / 1000.0f;
                                }
                                else
                                {
                                    interflowIn[nx][ny] += qPart;
                                }
                            }
                            wIndex++;
                        }
                    }
                }
            }
        }
        else
        {
            // Non-drained soils: route all released interflow downslope through the matrix.
            float wSum = 0.0f;
            int wIndex = 0;
            for(int X=-1; X<2; X++)
            {
                for(int Y=-1; Y<2; Y++)
                {
                    if(X==0 && Y==0) { wIndex++; continue; }
                    int nx = x + X;
                    int ny = y + Y;
                    if(dem[nx][ny] != noData && dem[nx][ny] < dem[x][y])
                        wSum += d[x][y][wIndex];
                    wIndex++;
                }
            }

            if(wSum <= 0.0f)
            {
                interflowStore[x][y] += qRelease;
            }
            else
            {
                wIndex = 0;
                for(int X=-1; X<2; X++)
                {
                    for(int Y=-1; Y<2; Y++)
                    {
                        if(X==0 && Y==0) { wIndex++; continue; }
                        int nx = x + X;
                        int ny = y + Y;
                        if(dem[nx][ny] != noData && dem[nx][ny] < dem[x][y])
                        {
                            float qPart = qRelease * d[x][y][wIndex] / wSum;
                            if(channel[nx][ny] == 1)
                            {
                                dsc[nx][ny] += qPart;
                                Q_interflow_cum += fracInterflowToChannel * qPart * dx * dy / 1000.0f;
                            }
                            else if(lakes[nx][ny] != noData)
                            {
                                int locLake = 0;
                                for(int lakeIndex=0; lakeIndex<nrLakes; lakeIndex++)
                                    if(lakes[nx][ny] == lakeStats[lakeIndex][8]) locLake = lakeIndex;
                                lakeStats[locLake][7] += (qPart * dx * dy / 1000.0f) / lakeStats[locLake][0];
                                Q_lake += fracInterflowToChannel * qPart * dx * dy / 1000.0f;
                            }
                            else
                            {
                                interflowIn[nx][ny] += qPart;
                            }
                        }
                        wIndex++;
                    }
                }
            }
        }
    }
}

// Keep legacy output column alive unless you add a separate q_interflow column.
Qd_cum += Q_interflow_cum;
