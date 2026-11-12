// Before flacc_DAG.inc
/*float pre_routing_total = 0;
for(int y=0; y<ySize; y++) {
    for(int x=0; x<xSize; x++) {
        if(dem[x][y] != noData && channel[x][y] != 1 && (dsc[x][y] - dsc_max[x][y]) > 0) {
            pre_routing_total += (dsc[x][y] - dsc_max[x][y])* dx * dy / 1000;
        }
    }
}
cout << "Pre-routing hillslope runoff: " << pre_routing_total << " m³" << endl;*/

std::vector<std::vector<float>> dsc_original(
    xSize, std::vector<float>(ySize, 0.0f)
);

for(int y=0; y<ySize; y++)
for(int x=0; x<xSize; x++)
{
    dsc_original[x][y] = dsc[x][y];
}

// ==========================================================
// DAG-BASED FLOW ROUTING AND FLOW ACCUMULATION (MFD)
// flaccMFD = routed water depth passing through cell during this timestep [mm]
// Q_cum    = time-integrated routed water depth [mm]
// ==========================================================
Q_catchmentSRO = 0.0;
//float daysum=0;
// ----------------------------------------------------------
// 0. Accumulate discharge and reset timestep flux
// ----------------------------------------------------------
for (int x = 0; x < xSize; x++)
for (int y = 0; y < ySize; y++)
{
    Q_cum[x][y] += flaccMFD[x][y];
    flaccMFD[x][y] = 0.0f;
}


// ----------------------------------------------------------
// 1. Single-pass routing in topological order
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
    // 2. Reinfiltration (unchanged)
    // ------------------------------------------------------

	if (!isChannelCell(x, y) && tm[index] > drainageTempEHD)
		{
		float availablePoreDepth =
		(SWSat_EHD[x][y] * EHDepth[x][y][1]) - SW_EHD[x][y];
		availablePoreDepth = std::max(0.0f, availablePoreDepth);
		if (availablePoreDepth >= dsc[x][y])
			dsc[x][y] = 0.0f;
		else
			dsc[x][y] -= availablePoreDepth;
		}

    // ------------------------------------------------------
    // 3. Excess surface water generation
    // ------------------------------------------------------
    if (dsc[x][y] <= dsc_max[x][y])
        continue;

    float toDistribute = 0;
    if(dsc[x][y]>dsc_max[x][y])
	{
	toDistribute = dsc[x][y] - dsc_max[x][y];//in mm
	dsc[x][y] = dsc_max[x][y];
	}

    // Local flux production
    flaccMFD[x][y] += toDistribute;
    if (flaccMFD[x][y] > flaccMAX[x][y])
        flaccMAX[x][y] = flaccMFD[x][y];

    // ------------------------------------------------------
    // 4. Route excess to downslope neighbors (MFD)
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

        // Bounds & activity check
        if (nx < 0 || nx >= xSize || ny < 0 || ny >= ySize)
        {
            wIndex++;
            continue;
        }

        if (dem[nx][ny] == noData)
        {
            wIndex++;
            continue;
        }

        float q = toDistribute * w;//in mm
        //cout<<q<<" "<<w<<" "<<landuse[x][y]<<endl;
	// --------------------------------------------------
	// Hillslope → channel runoff (count once)
	// --------------------------------------------------
   	/*if (channel[x][y] != 1 && channel[nx][ny] == 1){
    		Q_catchmentSRO += q/1000 * dx * dy; //CHECK PRODUCT: m³?
    		daysum+=q;
    		//cout<<x<<":"<<y<<" "<<nx<<":"<<ny<<" "<<q<<" "<<dx<<" "<<dy<<endl;
    		}*/
    if (channel[x][y] != 1 && channel[nx][ny] == 1)
        {
        // Only count the LOCAL runoff from this hillslope cell, not accumulated upstream
        Q_catchmentSRO += q  * dx * dy / 1000;
        float local_contribution = max(0.0f, dsc_original[x][y] - dsc_max[x][y]);
        //daysum += q;
        }
    		
        // ---------------- DAM ----------------
        if (dams[nx][ny] != noData)
        {
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
                damStats[damIndex][4] += q * (dx * dy) / 1000.0;

                if (damStats[damIndex][4] >= damStats[damIndex][1])
                {
                    int outX = (damStats[damIndex][2] - originX) / dx;
                    int outY = (originY - damStats[damIndex][3]) / dy;

                    if(outX >= 0 && outX < xSize && outY >= 0 && outY < ySize)
			{
			    dsc[outX][outY] += q;
			    //flaccMFD[outX][outY] += q; THIS WAS A DOUBLE COUNT
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
            for (int i = 0; i < nrLakes; i++)
                if (lakes[nx][ny] == lakeStats[i][8])
                {
                    locLake = i;
                    break;
                }
            if (locLake >= 0)
            	{
            	float inflow = (q / 1000.0 * dx * dy);//m3
                lakeStats[locLake][7] += inflow  / lakeStats[locLake][0];//m
                    
                // Flux still passes into lake cell
                //flaccMFD[nx][ny] += q;
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

//if(daysum>0)
//	cout<<"DAYSUM = "<<daysum<<" ("<<daysum*dx*dy/1000<<")"<<endl;
// ----------------------------------------------------------
// 5. Hydrograph output (unchanged)
// ----------------------------------------------------------
for (const auto& p : pointsHydrograph)
{
    int outX = (p.x - originX) / dx;
    int outY = (originY - p.y) / dy;

    float Q_hydro_m3d = flaccMFD[outX][outY] * dx * dy / 1000.0f; // flaccMFD is mm over one raster cell
    float Q_hydro_ls  = Q_hydro_m3d * 1000.0f / (24.0f * 3600.0f);

    oHydrograph
        << year[index] << "-"
        << month[index] << "-"
        << day[index] << " "
        << p.fid << " "
        << Q_hydro_m3d << " "
        << Q_hydro_ls
        << endl;
}
