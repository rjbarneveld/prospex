	//FLACC
	for(int x=0;x<xSize;x++)
		{
		for(int y=0;y<ySize;y++)
			{
			Q_cum[x][y]+=flaccMFD[x][y];
			flaccMFD[x][y]=0;//reset daily flow accumulation
			}
		}

	pctDone=0;
	pctDoneOld=0;
//	#include "lakeflow.inc"

	routingCounter=0;
	while(pctDone<98)//&&(pctDone>0&&(pctDone-pctDoneOld)>1))
		{
		for(int x=1;x<xSize-1;x++)
			{
			for(int y=1;y<ySize-1;y++)
				{
				bool continueFlow=false;
				if(riverQ==0)
					{
					if(dem[x][y]!=noData&&channel[x][y]==noData&&lakes[x][y]==noData)
						continueFlow=true;
					}
				else
					{
					if(dem[x][y]!=noData&&lakes[x][y]==noData)
						continueFlow=true;
					}
				if(continueFlow==true)
					{
	//1. Water balance EHD Reinfiltration
					float availablePoreDepth=(SWSat_EHD[x][y] * EHDepth[x][y][1])-SW_EHD[x][y];
					if(availablePoreDepth<0)
						availablePoreDepth=0;
					float infil=0;
					if(availablePoreDepth>dsc[x][y]&&tm[index]>drainageTempEHD)
						{
						float infil = dsc[x][y];
						dsc[x][y] = 0;
						}
					else if(availablePoreDepth<toDistribute[x][y]&&tm[index]>drainageTempEHD)
						{
						float infil = availablePoreDepth;
						dsc[x][y] -= infil;
						}					
					if(dsc[x][y]<=dsc_max[x][y])
						{
						int A=1;
						}
					else
						{
						toDistribute[x][y]=(dsc[x][y]-dsc_max[x][y]);
						if((flaccMFD[x][y]+toDistribute[x][y])>0)
							flaccMFD[x][y]+=toDistribute[x][y];//this is the input for detachment and sediment routing (Q in mm)
						
						if(flaccMFD[x][y]>flaccMAX[x][y])
							flaccMAX[x][y]=flaccMFD[x][y];
						int wIndex=0;
						//toDistribute[x][y]>0;
						for(int X=-1;X<2;X++)
							{
							for(int Y=-1;Y<2;Y++)
								{
								if(X==0&&Y==0)
									Y++;
								if(d[x][y][wIndex]>0)
									{
									float ssgTemp=0;
									if(channel[x+X][y+Y]==1)
										{
										Q_catchmentSRO+=(toDistribute[x][y]*d[x][y][wIndex])*(dx*dy)/1000;//m³
										}
									else if(lakes[x+X][y+Y]!=noData)
										{
										int locLake=0;
										for(int lakeIndex=0;lakeIndex<nrLakes;lakeIndex++)
											{
											if(lakes[x+X][y+Y]==lakeStats[lakeIndex][8])
												locLake=lakeIndex;
											}
										// 0 area, 1 xIndex, 2 yIndex, 3 z, 4 Q_in, 5 Q_out, 6 V, 7 level, 8 index
										lakeStats[locLake][7]+=(toDistribute[x][y]/1000*d[x][y][wIndex])*(dx*dy)/lakeStats[locLake][0];
										//int outX=lakeStats[locLake][1];
										//int outY=lakeStats[locLake][2];
										}
									else
										{
										dsc[x+X][y+Y]+=(toDistribute[x][y]*d[x][y][wIndex]);
										//flaccMFD[x+X][y+Y]+=toDistribute[x][y]*d[x][y][wIndex]/1000;
										}
									}//end if d > 0
								wIndex++;
								}//endY
							}//endX
						dsc[x][y]=dsc_max[x][y];//toDistribute[x][y];
						}//end if toDistribute>dsc
					toDistribute[x][y]=0;//dsc_max[x][y];
					}//end loop if channel/lakes!=true;
				}//endloop y
			}//endloop x

		float countFLACC=0;
		for(int x=1;x<xSize-1;x++)
			{
			for(int y=1;y<ySize-1;y++)
				{
				//G[x][y][0]=dem[x][y];
				if(dem[x][y]!=noData&&channel[x][y]!=1&&lakes[x][y]==noData)
					{
					//dsc[x][y]=3.27/(1 + exp(20*(slope[x][y]+.01)));
					if(dsc[x][y]<=dsc_max[x][y])
						countFLACC++;
					//else
					//	cout<<"round "<<n<<" "<<dsc[x][y]<<" "<<dsc_max[x][y]<<endl;
					}
				}
			}
		pctDone=countFLACC/DEMcount*100;
//		if(n>0&&((pctDone-pctDoneOld)<.1||pctDone>90))
		if(routingCounter>0&&pctDone>99)
			routingCounter=200;
		pctDoneOld=pctDone;
		routingCounter++;
	}//end loop routingCounter or while
