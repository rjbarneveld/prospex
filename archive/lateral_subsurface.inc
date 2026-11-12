	for(int x=1;x<xSize-1;x++)
		{
		for(int y=1;y<ySize-1;y++)
			{
			if(dem[x][y]!=noData&&channel[x][y]!=1&&lakes[x][y]==noData)
				{
				float flux=0;
				float totFlux=0;
				int wIndex=0;
				float locSWmin=0;
				float theta       = SD[x][y] > 0 ? SW[x][y][0] / SD[x][y] : 0.0f;
				float theta_index = 0;

				for(int X=-1;X<2;X++)
					{
					for(int Y=-1;Y<2;Y++)
						{
						if(X==0&&Y==0)
							Y++;
						//lateral drainage
						theta_index = SD[x+X][y+Y] > 0 ? SW[x+X][y+Y][0] / SD[x+X][y+Y] : 0.0f;
						if(tm[index]>drainageTempSD&&theta_index<theta&&channel[x+X][y+Y]==noData&&lakes[x+X][y+Y]==noData)//&&SW[x][y][0]>(FC[x][y]*SD[x][y]))
							{
							float potFlow = d[x][y][wIndex]*(theta-theta_index)*drainageRate2[x][y];//(mm)
							if(potFlow>0&&potFlow<((SWSat[x+X][y+Y]*SD[x+X][y+Y])-SW[x+X][y+Y][0]))
								{
								SW[x+X][y+Y][1] += potFlow;
								locSWmin += potFlow;
								}
							else if(potFlow>0&&potFlow>((SWSat[x+X][y+Y]*SD[x+X][y+Y])-SW[x+X][y+Y][0]))
								{
								potFlow=((SWSat[x+X][y+Y]*SD[x+X][y+Y])-SW[x+X][y+Y][0]);
								SW[x+X][y+Y][1] += potFlow;
								locSWmin += potFlow;
								}
							}
						//drainage towards stream
						else if(tm[index]>drainageTempSD&&channel[x+X][y+Y]==1&&SW[x][y][0]>(FC[x][y]*SD[x][y])&&lakes[x+X][y+Y]==noData)
							{
							float potFlow = (SW[x][y][0] - (FC[x][y]*SD[x][y]))*d[x][y][wIndex]*drainageRate2[x][y];
							if(potFlow<0)potFlow=0;
							if(riverQ==1)
								toDistribute[x+X][y+Y]+=potFlow;
							locSWmin += potFlow;
							}
						Q_stream+=locSWmin*.001*dx*dy;
						//drainage towards lake
						/*int locLake=0;
						for(int lakeIndex=0;lakeIndex<nrLakes;lakeIndex++)
							if(lakes[x+X][y+Y]==lakeStats[lakeIndex][8])
								locLake=lakeIndex;
						if(tm[index]>drainageTempSD&&SW[x][y][0]>(FC[x][y]*SD[x][y])&&channel[x+X][y+Y]!=1&&lakes[x+X][y+Y]!=noData)
							{
							float potFlow = (SW[x][y][0] - (FC[x][y]*SD[x][y]))*d[x][y][wIndex]*drainageRate2[x][y];
							if(potFlow<0)potFlow=0;
							lakeStats[locLake][7] += (potFlow*pow(dx,2)/1000)/lakeStats[locLake][0];
							locSWmin += potFlow;
							}*/
						//groundwater flow						
						flux=0;
						if(G[x+X][y+Y][0]<G[x][y][0]&&G[x+X][y+Y][0]!=noData)
							flux=kG[0]*sin(slopeG[x][y])*dG[x][y][wIndex];
						latflow[x][y]+=flux;
						totFlux+=flux;
						if(flux>0)
							{
							G[x+X][y+Y][1]+=flux;
							if(channel[x+X][y+Y]==1)
								{
								Q_stream+=kG[0]*sin(slopeG[x][y])*dG[x][y][wIndex]*dx*dy;
								if(riverQ==1)
									toDistribute[x+X][y+Y]+=(flux*dx*dy*.001);
								}
							else if(lakes[x+X][y+Y]!=noData)
								{
								int locLake=0;
								for(int lakeIndex=0;lakeIndex<nrLakes;lakeIndex++)
									{
									if(lakes[x+X][y+Y]==lakeStats[lakeIndex][8])
										locLake=lakeIndex;
									}
								lakeStats[locLake][7]+=(flux*dx*dy)/lakeStats[locLake][0];
								}
							}
						wIndex++;
						}//endY
					}//endX for lateral flow
				G[x][y][1]-=totFlux;
				SW[x][y][1]-=locSWmin;
				}//endloop y
			}//endloop x
		}//end if dem!=noData
