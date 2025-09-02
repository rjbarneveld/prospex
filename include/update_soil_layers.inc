for(int x=1;x<xSize-1;x++)
		{
		for(int y=1;y<ySize-1;y++)
			{
			G[x][y][0]=G[x][y][1];
			if(channel[x][y]!=noData)
				G[x][y][0]=dem[x][y];
			else if(lakes[x][y]!=noData)
				{
				int locLake=0;
				for(int lakeIndex=0;lakeIndex<nrLakes;lakeIndex++)
					{
					if(lakes[x][y]==lakeStats[lakeIndex][8])
						locLake=lakeIndex;
					}
				G[x][y][0]=lakeStats[locLake][3];//+lakeStats[locLake][7];
				}
			else{
				G[x][y][0]-= kG[1]*cos(slopeG[x][y]);//account for seepage at a rate that is proportional to the lateral flow (lateral_subsurface.inc)
				
				//if(x==pixel_aX&&y==pixel_aY)cout<<"out: "<<kG*cos(slopeG[x][y])<<" ";
				}
			EHDepth[x][y][0]=EHDepth[x][y][1];
			SD[x][y]=1000*(dem[x][y] - G[x][y][0]) - EHDepth[x][y][0];
			if(G[x][y][0]!=noData&&dynamicG==1)
					{
					if(!isinf(G[x+1][y-1][0]+G[x+1][y][0]+G[x+1][y+1][0]+G[x-1][y-1][0]+G[x-1][y][0]+G[x-1][y+1][0]+G[x][y-1][0]+G[x][y+1][0])&&!isnan(G[x+1][y-1][0]+G[x+1][y][0]+G[x+1][y+1][0]+G[x-1][y-1][0]+G[x-1][y][0]+G[x-1][y+1][0]+G[x][y-1][0]+G[x][y+1][0]))
						{
						float sGx=((G[x+1][y-1][0]+G[x+1][y][0]+G[x+1][y+1][0])-(G[x-1][y-1][0]+G[x-1][y][0]+G[x-1][y+1][0]))/(6*dx);
						float sGy=-((G[x-1][y-1][0]+G[x][y-1][0]+G[x+1][y-1][0])-(G[x-1][y+1][0]+G[x][y+1][0]+G[x+1][y+1][0]))/(6*dy);
						slopeG[x][y]=atan(pow((pow(sGx,2)+pow(sGy,2)),.5));//in radians	
						}
					else 
						slopeG[x][y]=0;		
					float sumDZ=0;
					int wIndex=0;
					for(int X=-1;X<2;X++)
						{
						for(int Y=-1;Y<2;Y++)
							{
							float dHor=dx;
							if(X!=0||Y!=0)
								dHor*=1.414;
							if(X==0&&Y==0)
								Y++;
							if(dem[x+X][y+Y]==noData&&lakes[x][y]==noData&&channel[x][y]==noData)
								G[x][y][0]=dem[x][y]-4;
							if(G[x+X][y+Y][0]<G[x][y][0]&&G[x+X][y+Y][0]!=noData)
								sumDZ+=((G[x][y][0]-G[x+X][y+Y][0])/dHor);
							wIndex++;
							}
						}
					wIndex=0;
					for(int X=-1;X<2;X++)
						{
						for(int Y=-1;Y<2;Y++)
							{
							float dHor=dx;
							if(X!=0||Y!=0)
								dHor*=1.414;
							if(X==0&&Y==0)
								Y++;
							if(G[x+X][y+Y][0]<G[x][y][0]&&G[x+X][y+Y][0]!=noData&&sumDZ>0)
								dG[x][y][wIndex]=((G[x][y][0]-G[x+X][y+Y][0])/dHor)/sumDZ;
							else
								dG[x][y][wIndex]=0;
							if(isinf(dG[x][y][wIndex]))
								{
								cout<<x<<":"<<y<<" G[0]="<<G[x][y][0]<<" G[0+X]="<<G[x+X][y+Y][0]<<" sumDZ="<<sumDZ<<" dist="<<pow((pow(X,2)+pow(Y,2)),0.5)<<endl;
								return 0;
								}
							wIndex++;
							}
						}
					}
			
				}
			}
	//updating SD and EHD as a function of groundwater level
	for(int x=1;x<xSize-1;x++)
			{
			for(int y=1;y<ySize-1;y++)
				{
				dailySurplus[x][y]=0;
				if(dem[x][y]!=noData&&lakes[x][y]==noData&&channel[x][y]!=1)
					{
					float DtG=1000*(dem[x][y]-G[x][y][0]);//in mm
					if(DtG<0)
						{
						EHDepth[x][y][1]=0;
						SW_EHD[x][y]=0;
						SD[x][y]=0;
						SW[x][y][0]=0;
						dailySurplus[x][y]=(-DtG);
						G[x][y][0]=dem[x][y];
						}
					else if(DtG<EHDepth[x][y][0])
						{
						EHDepth[x][y][1]=DtG;
						SW_EHD[x][y]=EHDepth[x][y][1]*SWSat_EHD[x][y];
						SD[x][y]=0;
						SW[x][y][0]=0;
						}
					else if(DtG>EHDepth[x][y][0])
						{
						EHDepth[x][y][1]=EHDepth[x][y][0];
//						SW_EHD[x][y]=EHDepth[x][y][1]*SWSat_EHD[x][y];
						SD[x][y]=(DtG-EHDepth[x][y][0]);
						//SW[x][y][0]=SD[x][y]*SW[x][y];
						}
					}
				}
			}
