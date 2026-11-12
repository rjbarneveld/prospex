int DEMcount=0;
//correction for misalignment between AR5-81 and lakes
int correctedWaterCells = 0;

for(int y = 0; y < ySize; y++)
	{
	for(int x = 0; x < xSize; x++)
		{
		if(channel[x][y]!=1)
			channel[x][y] = noData;
		
		if(dem[x][y] != noData && landuse[x][y] == 81 && lakes[x][y] == noData && channel[x][y] == noData)
			{
			landuse[x][y] = 30;   // forest
			correctedWaterCells++;
			}
		}
	}

/*cout << "Corrected "
     << correctedWaterCells
     << " orphaned AR5 water cells (81 -> 30)"
     << endl;
     std::vector<std::vector<float>> landuseExport(
    xSize,
    std::vector<float>(ySize, noData)
);*/

for(int y = 0; y < ySize; y++)
	{
	for(int x = 0; x < xSize; x++)
		{
		if(simulateDams == 1 && dams[x][y]!=noData)
			{
			channel[x][y]=noData;
			}
		if(riverQ==0&&dem[x][y]!=noData&&channel[x][y]==noData&&lakes[x][y]==noData)
			DEMcount++;
		else if(riverQ==1&&dem[x][y]!=noData&&lakes[x][y]==noData)
			DEMcount++;
		if(dem[x][y]!=noData&&channel[x][y]==noData&&lakes[x][y]==noData)
			{
			int paramIndex=-1;
			if(landuse[x][y]==21||landuse[x][y]==22||landuse[x][y]==23)
				{
				if(texture[x][y]==10)paramIndex=0;
				else if(texture[x][y]==11)paramIndex=1;
				else if(texture[x][y]==12)paramIndex=2;
				else if(texture[x][y]==13)paramIndex=3;
				else if(texture[x][y]==14)paramIndex=4;
				else if(texture[x][y]==15)paramIndex=5;
				else if(texture[x][y]==16)paramIndex=6;
				else if(texture[x][y]==17)paramIndex=7;
				else if(texture[x][y]==18)paramIndex=8;
				else if(texture[x][y]==19)paramIndex=9;
				else if(texture[x][y]==99)paramIndex=10;
				else if(texture[x][y]==0)paramIndex=6;//handle exceptions of where ar5=agriculture but no soil map available
				}
			else if(landuse[x][y]==11)paramIndex=11;//built up
			else if(landuse[x][y]==12)paramIndex=12;//roads
			else if(landuse[x][y]==30)paramIndex=13;//forest
			else if(landuse[x][y]==50)paramIndex=13;//other open area, use same as forest
			else if(landuse[x][y]==60)paramIndex=15;//mire
if(paramIndex < 0 || paramIndex > 15)
{
    cerr << "Invalid paramIndex ("<<paramIndex<<") at "
         << x << "," << y
         << " landuse=" << landuse[x][y]
         << " texture=" << texture[x][y]
         << " dem=" << dem[x][y]
         << endl;
    continue;
}

			EHDepth[x][y][0]=EHD_scaling*params[paramIndex][8];//effective hydrological depth (mm)
			EHDepth[x][y][1]=EHD_scaling*params[paramIndex][8];
			SD[x][y]= 1000*(dem[x][y] - G[x][y][0]) - EHDepth[x][y][0];//soil depth (between EHD and G) in mm
			SWSat[x][y]=params[paramIndex][0];//in m3/m3
			SWSat_EHD[x][y]=params[paramIndex][0];//in m3/m3
			FC[x][y]=params[paramIndex][6];//theta field capacity / pF 4.2 m3/m3
			SW_EHD[x][y]=.8*SWSat_EHD[x][y]*EHDepth[x][y][0];//mm
			SW[x][y][0]=.8*SWSat[x][y]*SD[x][y];//mm initial water content at 80% of maximum
			SW[x][y][1]=.8*SWSat[x][y]*SD[x][y];//mm initial water content at 80% of maximum

			erod[x][y]=params[paramIndex][3];
			drainageRate1[x][y]=params[paramIndex][4];
			drainageRate2[x][y]=params[paramIndex][5];
			k[x][y]=params[paramIndex][2];
			if(landuse[x][y]>20&&landuse[x][y]<30)
				{
				dsc_max[x][y]=3.27/(1 + exp(20*(slope[x][y]+.01)));
				CC[x][y]=.1;//crop cover (0-1) mulch etc.; additional to vegetation cover (ccWinterwheat, ccSpringwheat)
				}
			else if(landuse[x][y]==30)
				{
				dsc_max[x][y]=3.27/(1 + exp(20*(slope[x][y]+.01)));
				CC[x][y]=.9;
				}
			else if(vegetation[x][y]==5)
				{
				dsc_max[x][y]=1.27/(1 + exp(20*(slope[x][y]+.01)));
				CC[x][y]=.9;
				}			
			if(channel[x][y]==1)
				{
				dsc_max[x][y]=0;
				/*for(int X=-1;X<2;X++)
					{
					for(int Y=-1;Y<2;Y++)
						TC_red[x+X][y+Y]=1;//e.g. buffer zones
					}*/
				}
			//SD[x][y]=(dem[x][y]-G[x][y][0])*1000;
			//SET PARAMETERS FOR PEAT RESTORATION
			if(landuse[x][y]==60)
				{
				EHDepth[x][y][0]=250;
				dsc_max[x][y]=25;
				}
			
			}
		}
	}
cout<<"study area is "<<setprecision(5)<<dx*dx*DEMcount/1e06<<" km²"<<endl;

