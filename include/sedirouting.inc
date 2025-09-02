	ERO_overland=0;
	ERO_gully=0;
	float pctDone=0;
	float pctDoneOld=0;
	int n=0;
//	Qd=0;
//	Qd_cum=0;
	while(pctDone<95&&n<200)
		{
		for(int x=1;x<xSize-1;x++)
			{
			for(int y=1;y<ySize-1;y++)
				{
				if(dem[x][y]!=noData&&channel[x][y]==noData&&lakes[x][y]==noData&&tm[index]>-2)
					{
					//eroMFD[x][y] += toDistribute_sedi[x][y];
					int wIndex=0;
					if(toDistribute_sedi[x][y]>TC[x][y])
						{
						TC[x][y]=0;
						toDistribute_sedi[x][y]-=TC[x][y];
						depoMFD[x][y]+=TC[x][y];
						}
					else if(toDistribute_sedi[x][y]<TC[x][y])
						{
						TC[x][y]-=toDistribute_sedi[x][y];
						}	
					for(int X=-1;X<2;X++)
						{
						for(int Y=-1;Y<2;Y++)
							{
							if(X==0&&Y==0)
								Y++;
								if(channel[x+X][y+Y]==noData&&d[x][y][wIndex]>0)
									{
									toDistribute_sedi[x+X][y+Y] += (toDistribute_sedi[x][y] * d[x][y][wIndex]);
									if(toDistribute_sedi[x+X][y+Y] > TC[x+X][y+Y])
										{
										depoMFD[x+X][y+Y] += (toDistribute_sedi[x+X][y+Y] - TC[x+X][y+Y]);
										//dsc[x+X][y+Y] -= (toDistribute_sedi[x+X][y+Y] - TC[x+X][y+Y])*BD;
										if(dsc[x+X][y+Y]<0)
											dsc[x+X][y+Y]=0;
										toDistribute_sedi[x+X][y+Y] = TC[x+X][y+Y];
										TC[x+X][y+Y]=0;
										}
									else if(toDistribute_sedi[x+X][y+Y] < TC[x+X][y+Y])
										{
										//depoMFD[x+X][y+Y] += toDistribute_sedi[x+X][y+Y];
										TC[x+X][y+Y]-=toDistribute_sedi[x][y]*d[x][y][wIndex];
										}
									if(TC[x+X][y+Y]<0)
										TC[x+X][y+Y]=0;
									}//end if channel==false
								else if(channel[x+X][y+Y]!=noData&&gully[x][y]==true)
									ERO_gully += (toDistribute_sedi[x][y] * d[x][y][wIndex]) * (xSize*ySize);
								else if(channel[x+X][y+Y]!=noData)
									ERO_overland +=  (toDistribute_sedi[x][y] * d[x][y][wIndex]) * (xSize*ySize);
								//if(x==pixel_aX&&y==pixel_aY)cout<<wIndex<<" "<<d[x][y][wIndex]<<" ero="<<eroMFD[x][y]<<" depo="<<depoMFD[x][y]<<endl;
//----------------------------------------------------
								wIndex++;
							}//endY
						}//endX
					
					}//end if channel!=true
					toDistribute_sedi[x][y]=0;
					if(eroMFD[x][y]>gullyThreshold)//	{
						gully[x][y]=true;//cout<<"here: "<<eroMFD[x][y]<<" - "<<depoMFD[x][y]<<" "<<eroMFD[x][y]-depoMFD[x][y]<<" "<<gully[x][y]<<endl;}
				}//endloop y
			}//endloop x
			
		float countFLACC=0;
		for(int x=1;x<xSize-1;x++)
			{
			for(int y=1;y<ySize-1;y++)
				{
				if(dem[x][y]!=noData)
					{
					//dsc[x][y]=3.27/(1 + exp(20*(slope[x][y]+.01)));
					if(toDistribute_sedi[x][y]<0.001&&dem[x][y]!=noData&&channel[x][y]==noData&&lakes[x][y]==noData)
						countFLACC++;
					}
				}
			}
		pctDone=countFLACC/DEMcount*100;
		//cout<<" "<<index<<" "<<n<<" "<<pctDone<<"% "<<pctDoneOld<<"%"<<endl;
		if(pctDone>0&&(pctDone-pctDoneOld)<.1)
			n=200;
		pctDoneOld=pctDone;

		//cout<<index<<" "<<n<<" "<<endl;
		n++;
	}//end loop n or while
