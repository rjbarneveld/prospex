float et0=0;
if(tm[index]>0)
	et0=calculateET0(tm[index],dem[int(xSize/2)][int(ySize/2)]);

if(index>0)
	{
	if(degDayWinterwheat[index-1]>0)
		{
		if(tm[index]>0)
			degDayWinterwheat[index]=degDayWinterwheat[index-1]+tm[index];
		else
			degDayWinterwheat[index]=degDayWinterwheat[index-1];
		}
	else if(degDaySpringwheat[index-1]>0)
		{
		if(tm[index]>0)
			degDaySpringwheat[index]=degDaySpringwheat[index-1]+tm[index];
		else
			degDaySpringwheat[index]=degDaySpringwheat[index-1];
		}
	}

	for(int x=1;x<xSize-1;x++)
		{
		for(int y=1;y<ySize-1;y++)
			{
			if(dem[x][y]!=noData&&channel[x][y]==noData&&lakes[x][y]==noData)
				{
				Qd=0;
				float etAct=0;
				float etPot=0;
				float availableSWinEHD=SW_EHD[x][y]-.1*EHDepth[x][y][1];//offset by PWP-(FC[x][y]*EHDepth[x][y][1]);

				if(vegetation[x][y]==1)
					etPot=0;
				else if(landuse[x][y]==30||landuse[x][y]==60||vegetation[x][y]==5)//(vegetation[x][y]>0&&vegetation[x][y]<4)
					etPot=et0;
				else if(vegetation[x][y]==6&&degDayWinterwheat[index]>0)
					etPot=et0*calculateKcFromGDD(degDayWinterwheat[index]);
				else if(vegetation[x][y]==7&&degDaySpringwheat[index]>0)
					etPot=et0*calculateKcFromGDD(degDaySpringwheat[index]);
//if(x==pixel_aX&&y==pixel_aY)cout<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<degDayWinterwheat[index]<<"  "<<calculateKcFromGDD(degDayWinterwheat[index])<<endl;
				etPot = max(0.0f,etPot);
				if(etPot>availableSWinEHD)
					etAct=availableSWinEHD;
				else
					etAct=etPot;
				SW_EHD[x][y]-=etAct;
				dailySurplus[x][y]+=rr[index]+snowMelt-etAct;
				if(dailySurplus[x][y]<0)
					dailySurplus[x][y]=0;
				if(landuse[x][y]==11)
					Qd_cum += 0.5f * dailySurplus[x][y] * dx * dy / 1000.0f; // m3/day; urban runoff instantaneous, assuming 50% asphalt/direct drainage
				if(landuse[x][y]==12)
					Qd_cum += 1.0f * dailySurplus[x][y] * dx * dy / 1000.0f; // m3/day; road runoff instantaneous, assuming 100% asphalt/direct drainage
//if(dailySurplus[x][y]>0)cout<<landuse[x][y]<<" "<<dailySurplus[x][y]<<" before\t"<<dsc[x][y]<<" "<<dsc_max[x][y]<<endl;
				dsc[x][y]+=dailySurplus[x][y];
//if(dailySurplus[x][y]>0)cout<<landuse[x][y]<<" "<<dailySurplus[x][y]<<" after\t\t"<<dsc[x][y]<<" "<<dsc_max[x][y]<<endl;
				float availablePoreDepth=0;
				if(EHDepth[x][y][1]>0&&SW_EHD[x][y]<(SWSat_EHD[x][y]*EHDepth[x][y][1]))
					availablePoreDepth = (SWSat_EHD[x][y]*EHDepth[x][y][1])-SW_EHD[x][y];//comparison in mm
				
				float infil=0;
				float infil2=0;
				float ssg=0;

//1. Water balance EHD
				if(availablePoreDepth>dsc[x][y]&&tm[index]>drainageTempEHD)
					{
					infil = dsc[x][y];
					dsc[x][y]=0;
					}
				else if(availablePoreDepth<dsc[x][y]&&tm[index]>drainageTempEHD)
					{
					infil = availablePoreDepth;
					dsc[x][y] -= infil;
					}
				SW_EHD[x][y] += infil;

//2. Water balance total profile
				if(SD[x][y]>0&&EHDepth[x][y][1]>0)
					{
					float thetaEHD = SW_EHD[x][y]/EHDepth[x][y][1];
					float thetaSD = SW[x][y][0]/SD[x][y];
					if(thetaEHD>thetaSD&&tm[index]>drainageTempSD&&thetaEHD>FC[x][y])
						{
						infil2 = (thetaEHD-FC[x][y])*EHDepth[x][y][1]*drainageRate1[x][y];
						SW[x][y][0] += infil2;
						SW_EHD[x][y] -=infil2;
						}
					}//end if both layers > 0
				bool doit=true;
				if(x==pixel_1X&&y==pixel_1Y)oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<infil2<<" infil2 loc1"<<endl;
				if(x==pixel_2X&&y==pixel_2Y)oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<infil2<<" infil2 loc2"<<endl;
				if(x==pixel_3X&&y==pixel_3Y)oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<infil2<<" infil2 loc3"<<endl;
/*----------------------START Qd--------------------------------*/
//case 1: saturation
				if(SW[x][y][0]>(SWSat[x][y]*SD[x][y])&&tm[index]>drainageTempSD)
					{
					Qd = (SW[x][y][0]-(SWSat[x][y]*SD[x][y]));//excess water depth in mm
					if(texture[x][y]>=12&&texture[x][y]<=19)
						{
						Qd_cum += 0.9f * Qd * dx * dy / 1000.0f; // m3/day; 90% of excess water to drain, rest results in higher water table
						if(dynamicG==1)
							G[x][y][0]+=.1*(Qd/1000);
						if(erosion==1)
							{
							ssg = 2.2395 * exp(0.0226*SWSat[x][y]/SD[x][y]) * log10(1+Qd)/6 * Qd * exp(erod[x][y]) * .001;// (1+(1/exp(slope[x][y]*100)))
							ssg_catchment+=ssg;
							}
						}
					else if(dynamicG==1)
						G[x][y][1]+=Qd/1000;//higher groundwater table (for well-drained soils in classes 11 - 13)
					SW[x][y][0]-=Qd;
					}
//case 2: between field capacity and saturation
				else if((SW[x][y][0]/SD[x][y])>FC[x][y])
					{
					float thetaSD = SW[x][y][0]/SD[x][y];
					if(tm[index]>drainageTempSD&&channel[x][y]==noData&&lakes[x][y]==noData)
						{
						Qd = (thetaSD-FC[x][y])*SD[x][y]*drainageRate2[x][y];//mm

						//if(x==pixel_aX&&y==pixel_aY)cout<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<Qd<<" Qd agriculture"<<endl;					
						if(texture[x][y]>=12&&texture[x][y]<=19)
							{
							float Qd_act=0;
							if(SD[x][y]<1000)
								Qd_act = Qd;//in mm
							else
								Qd_act = Qd * 1000.0f / SD[x][y];; //in mm, maximum drainage depth at 1.0 m
							Qd_cum += Qd_act * dx * dy / 1000.0f; // m3/day; Qd_act is mm over one cell
							if(erosion==1)
								{
								ssg = 2.2395 * exp(0.0226*SWSat[x][y]/SD[x][y]) * log10(1+Qd)/6 * Qd * exp(erod[x][y]) * .001;// (1+(1/exp(slope[x][y]*100)))
								ssg_catchment+=ssg;
								}
							}

						else if(dynamicG==1)
							G[x][y][1]+=Qd/1000;
						SW[x][y][0]-=Qd;
						if(x==pixel_1X&&y==pixel_1Y)oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<Qd<<" Qd loc1"<<endl;
						if(x==pixel_2X&&y==pixel_2Y)oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<Qd<<" Qd loc2"<<endl;				
						if(x==pixel_3X&&y==pixel_3Y)oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<Qd<<" Qd loc3"<<endl;				
						}
					}
				/*---------------ROUTING DRAINAGE TO RIVER--------------------------*/
				if(Qd > 0)
					{
					short int riverX = drainRoute[x][y][0];
					short int riverY = drainRoute[x][y][1];
					//if(landuse[x][y]==21||landuse[x][y]==22||landuse[x][y]==23)cout<<x<<":"<<y<<" ("<<landuse[x][y]<<") draining to "<<riverX<<":"<<riverY<<endl;
					if(dem[riverX][riverY] != noData)
						{
						dsc[riverX][riverY] += Qd;
						}
					}

/*----------------------END Qd--------------------------------*/
				#include "write_lupoints.inc"				
				}//end if dem!=channel
			}//x
		}//y
