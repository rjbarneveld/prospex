int DEMcount=0;
for(int y = 0; y < ySize; y++)
	{
	for(int x = 0; x < xSize; x++)
		{			
		if(dem[x][y]!=noData&&channel[x][y]==noData&&lakes[x][y]==noData)
			{
			DEMcount++;
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
			else if(landuse[x][y]==50)paramIndex=14;//other open area
			else if(landuse[x][y]==60)paramIndex=15;//mire
			
			EHDepth[x][y][0]=params[paramIndex][8];//effective hydrological depth (mm)
			EHDepth[x][y][1]=params[paramIndex][8];
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
				for(int X=-1;X<2;X++)
					{
					for(int Y=-1;Y<2;Y++)
						TC_red[x+X][y+Y]=1;//e.g. buffer zones
					}
				}
			//SD[x][y]=(dem[x][y]-G[x][y][0])*1000;
			}
		}
	}
cout<<"study area is "<<setprecision(5)<<dx*dx*DEMcount/1e06<<" km²"<<endl;

