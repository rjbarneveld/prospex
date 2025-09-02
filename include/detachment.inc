	int dummyline=0;
	float ccWinterwheat=0;
	ccWinterwheat=degDayWinterwheat[index]/1400;
	if(ccWinterwheat>1)
		ccWinterwheat=1;
	float ccSpringwheat=0;
	ccSpringwheat=degDaySpringwheat[index]/1400;
	if(ccSpringwheat>1)
		ccSpringwheat=1;

for(int x=0;x<xSize;x++)
	{
	for(int y=0;y<ySize;y++)
		{
		if(dem[x][y]!=noData&&(landuse[x][y]>20&&landuse[x][y]<31)&&tm[index]>-2)
			{
			toDistribute_sedi[x][y]=0;
			float Fc = 0;
			float Hc=0;
			float cc_eff=0;//CC[x][y];
			if(vegetation[x][y]==6)cc_eff+=ccWinterwheat;
			else if(vegetation[x][y]==6)cc_eff+=ccWinterwheat;
			if(cc_eff>=1)cc_eff=.95;	
			/*DETACHMENT BY DIRECT OVERLAND FLOW*/
			if(SW_EHD[x][y]>(FC[x][y]*EHDepth[x][y][1]))
				{
				float saturation=(FC[x][y]*EHDepth[x][y][1])/SW_EHD[x][y];
				Hc = 1/pow(saturation,.2)*(1-cc_eff)*erod[x][y]*pow((flaccMFD[x][y]*1.001),1.6) * pow(sin(atan(slope[x][y])),.3)*.000001;//in kg, exponents are 1.5 and 0.3 in original publication
				}
			else
				Hc = (1-cc_eff)*erod[x][y]*pow(flaccMFD[x][y],1.5) * pow(sin(slope[x][y]),.3)*.000001;//in kg
			/*DETACHMENT BY DIRECT IMPACT*/
			//Choi, et al., 2017. Daily Based Morgan–Morgan–Finney (DMMF) Model: A Spatially Distributed Conceptual Soil Erosion Model to Simulate Complex Soil Surface Configurations
			if(swe[index]<.001)
				{
				float KE=0;
				float PH=.5;//plant height for leaf drainage impact
				if(landuse[x][y]==30)
						PH=2.0;//update for forest
				float Uld = (15.8*pow(PH,.5))-5.87;
				float Udt = 10.3 * pow(I,(2/9));
				float cc_tot=0;
				if(landuse[x][y]==30)
					cc_tot=1;
				if(vegetation[x][y]==6)
					{
					cc_tot=ccWinterwheat + CC[x][y];
					if(cc_tot > 1)cc_tot=1;
					}
				else if(vegetation[x][y]==7)
					{
					cc_tot=ccSpringwheat + CC[x][y];
					if(cc_tot > 1)cc_tot=1;			
					}

				KE=rr[index] * ((1-cc_tot)*Udt + cc_tot*Uld);
				Fc = erod[x][y]*(1-cc_tot)*KE*.001;//in kg
				}

			if(tm[index]<drainageTempEHD)//reduced erodibility under (partly) frozen conditions
				Hc*=(1/(1+(drainageTempEHD-tm[index])));
			eroMFD[x][y]+=(Fc + Hc);
			/*DETERMINE TC*/
			//float locManningN=exp(-2.1132 + 0.349*dsc[x][y]);
			//float vegManningN=((pow(locManningN,2)+(50*.000855)/(19.64)),.5);
			//float v=1/vegManningN*pow(.005,(2/3))*pow(slope[x][y],.5);
			//float vref=1/.015*pow(.005,(2/3))*pow(slope[x][y],.5);
			//float factorTC=v/vref;

			TC[x][y]=(TC_red[x][y])*pow((flaccMFD[x][y]),2) * slope[x][y] * .000001;//pow((manningN*.3),-.6);//Komura, 1983
				
			//if(eroMFD[x][y]>gullyThreshold)
			//	TC[x][y]*=(1 + (.25*eroMFD[x][y]));
			float dailyDepo=0;	
			/*SET TODISTRIBUTE SEDI FOR SEDIMENT ROUTING*/
			if((Fc+Hc)>TC[x][y])
				{
				toDistribute_sedi[x][y] = TC[x][y];
				depoMFD[x][y]+=((Fc+Hc)-TC[x][y]);
				dailyDepo=(Fc+Hc)-TC[x][y];
				//dsc[x][y]-=((Fc+Hc)-TC[x][y])*BD;//fill up surface roughness with incoming sediment
				TC[x][y]=0;
				}
			else
				{
				toDistribute_sedi[x][y] = Fc + Hc;
				TC[x][y]-= (Fc + Hc);
				}
			if(x==pixel_1X&&y==pixel_1Y)oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<(Fc + Hc)<<" detachment loc1"<<endl;
			if(x==pixel_2X&&y==pixel_2Y)oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<(Fc + Hc)<<" detachment loc2"<<endl;
			if(x==pixel_3X&&y==pixel_3Y)oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<(Fc + Hc)<<" detachment loc3"<<endl;
			if(x==pixel_1X&&y==pixel_1Y)oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<dailyDepo<<" deposition loc1"<<endl;
			if(x==pixel_2X&&y==pixel_2Y)oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<dailyDepo<<" deposition loc2"<<endl;
			if(x==pixel_3X&&y==pixel_3Y)oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<dailyDepo<<" deposition loc3"<<endl;
//if(x==pixel_aX&&y==pixel_aY&&dailyAcc>0)cout<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<"\tflacc="<<flaccMFD[x][y]<<"\tHc="<<Hc<<"\tFc="<<Fc<<"\tTC="<<TCday<<"\tdailyDepo="<<dailyAcc<<"\tAccDepo="<<depoMFD[x][y]<<endl;			

//if(x==pixel_aX&&y==pixel_aY&&Fc>0&&Hc>0)cout<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<"\tflacc="<<flaccMFD[x][y]<<"\tHc="<<Hc<<"\tFc="<<Fc<<"\tTC="<<TCday<<"\tfactorTC="<<0<<"\tdailyDepo="<<dailyAcc<<"\tAccDepo="<<depoMFD[x][y]<<endl;			
			//------------routine for re-entrainment-----------
			//if(toDistribute_sedi[x][y]<depoMFD[x][y])
			//	{
			//	depoMFD[x][y]-=toDistribute_sedi[x][y];
			//	}
			//else if(depoMFD[x][y]<toDistribute_sedi[x][y])
			//	{
			//	eroMFD[x][y]+=(toDistribute_sedi[x][y]-depoMFD[x][y]);
			//	depoMFD[x][y]=0;
			//	}
			//-------------------------------------------------
			}
		}
	}
