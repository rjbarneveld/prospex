//Slope, aspect, curvature
for(int x=1;x<xSize-1;x++)
{
    for(int y=1;y<ySize-1;y++)
    {	
        if(dem[x][y]!=noData&&dem[x-1][y]!=noData&&dem[x+1][y]!=noData&&dem[x][y-1]!=noData&&dem[x][y+1]!=noData)
        {
            double sx=((dem[x+1][y-1]+dem[x+1][y]+dem[x+1][y+1])-(dem[x-1][y-1]+dem[x-1][y]+dem[x-1][y+1]))/(6*dx);
            double sy=-((dem[x-1][y-1]+dem[x][y-1]+dem[x+1][y-1])-(dem[x-1][y+1]+dem[x][y+1]+dem[x+1][y+1]))/(6*dy);
            slope[x][y]=pow((pow(sx,2)+pow(sy,2)),.5);
            if(landuse[x][y]==30)//AR5:30
                dsc_max[x][y]=5;
            else if(landuse[x][y]>10&&landuse[x][y]<20)
                dsc_max[x][y]=1;				
            else if(landuse[x][y]>20&&landuse[x][y]<30)
                dsc_max[x][y]=3.27/(1 + exp(20*(slope[x][y]+.01)));
        }
    }
}

cout<<"Calculating outflow ratios d[x,y,8]"<<endl;	
// ==========================================================
// FLOW DISTRIBUTION (MFD, FLAT-SAFE)
// ==========================================================

constexpr float DZ_EPS = 1e-4f;

for (int x = 1; x < xSize - 1; x++)
{
    for (int y = 1; y < ySize - 1; y++)
    {
        if (dem[x][y] == noData || lakes[x][y] != noData)
            continue;

        float sumDZ = 0.0f;
        float dzTmp[8] = {0.0f};

        int wIndex = 0;

        // --------------------------------------------------
        // 1. Compute downslope gradients (with epsilon)
        // --------------------------------------------------
        for (int X = -1; X <= 1; X++)
        {
            for (int Y = -1; Y <= 1; Y++)
            {
                if (X == 0 && Y == 0)
			{
			    wIndex++;   // CRITICAL
			    continue;
			}

                int nx = x + X;
                int ny = y + Y;

                if (dem[nx][ny] != noData &&
                    //lakes[nx][ny] == noData &&
                    dem[nx][ny] < dem[x][y])
                {
                    float dz = (dem[x][y] - dem[nx][ny]) /
                               sqrt(float(X*X + Y*Y));

                    // ---- FLAT SAFETY ----
                    if (dz < DZ_EPS)
                        dz = DZ_EPS;

                    dzTmp[wIndex] = dz;
                    sumDZ += dz;
                }
                else
                {
                    dzTmp[wIndex] = 0.0f;
                }

                wIndex++;
            }
        }

        // --------------------------------------------------
        // 2. Normalize (or zero)
        // --------------------------------------------------
        wIndex = 0;
        for (int X = -1; X <= 1; X++)
        {
            for (int Y = -1; Y <= 1; Y++)
            {
		if (X == 0 && Y == 0)
		{
		    wIndex++;   // CRITICAL
		    continue;
		}

                if (sumDZ > 0.0f)
                    d[x][y][wIndex] = dzTmp[wIndex] / sumDZ;
                else
                    d[x][y][wIndex] = 0.0f;

                wIndex++;
            }
        }
    }
}
