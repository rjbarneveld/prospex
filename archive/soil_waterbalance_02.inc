/*------------------------------------------------------------------------------
 INTERFLOW RESERVOIR — declare these three variables in v_013_infiltration_dam.cpp
 BEFORE the main daily loop (alongside Q_catchment etc.):

     float interflow_store = 0.0f;   // m³ — drains slowly to outlet each day
     float gw_store        = 0.0f;   // m³ — deep percolation, very slow release
     float k_interflow     = 0.15f;  // day⁻¹  tune in [0.05, 0.40]
     float k_gw            = 0.02f;  // day⁻¹  tune in [0.005, 0.05]
     float interflow_frac  = 0.70f;  // fraction of tile-drain Qd going to fast store
                                     //        remainder goes instantaneous (as before)
------------------------------------------------------------------------------*/

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
				if(etPot>availableSWinEHD)
					etAct=availableSWinEHD;
				else
					etAct=etPot;
				SW_EHD[x][y]-=etAct;
				dailySurplus[x][y]+=rr[index]+snowMelt-etAct;
				if(dailySurplus[x][y]<0)
					dailySurplus[x][y]=0;

				if(landuse[x][y]==11)
					Qd_cum+=(0.5*dailySurplus[x][y]*pow(dx,2)/1000);//urban runoff instantaneous, assuming 50% asfalt/direct drainage
				if(landuse[x][y]==12)
					Qd_cum+=(1.0*dailySurplus[x][y]*pow(dx,2)/1000);//road runoff instantaneous, assuming 100% asfalt/direct drainage
				dsc[x][y]+=dailySurplus[x][y];
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
					dsc[x][y] -= availablePoreDepth;
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
						/*----------------------------------------------------------
						 Deep percolation: a fixed fraction of downward flux beyond
						 the EHD feeds the slow groundwater store (gw_store).
						 This represents water that bypasses the tile drain system
						 and discharges to the stream on timescales of weeks–months.
						 Tune gw_perc_frac in [0.02, 0.15] against winter baseflow.
						----------------------------------------------------------*/
						float gw_perc_frac = 0.05f;
						gw_store += gw_perc_frac * infil2 * (dx*dy) / 1000.0f; // m³
						}
					}//end if both layers > 0
				bool doit=true;
				if(x==pixel_1X&&y==pixel_1Y)oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<infil2<<" infil2 loc1"<<endl;
				if(x==pixel_2X&&y==pixel_2Y)oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<infil2<<" infil2 loc2"<<endl;
				if(x==pixel_3X&&y==pixel_3Y)oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<infil2<<" infil2 loc3"<<endl;
/*----------------------START Qd--------------------------------*/
//case 1: saturation excess — all goes instantaneous (fast lateral drainage)
				if(SW[x][y][0]>(SWSat[x][y]*SD[x][y])&&tm[index]>drainageTempSD)
					{
					Qd = (SW[x][y][0]-(SWSat[x][y]*SD[x][y]));//excess water depth in mm
					if(texture[x][y]>=12&&texture[x][y]<=19)
						{
						Qd_cum += .9*Qd*pow(dx,2)/1000;//90% of excess water to drain, rest results in higher water table
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
//case 2: between field capacity and saturation — split between fast (tile drain)
//         and slow (interflow store) to produce a realistic recession limb
				else if((SW[x][y][0]/SD[x][y])>FC[x][y])
					{
					float thetaSD = SW[x][y][0]/SD[x][y];
					if(tm[index]>drainageTempSD&&channel[x][y]==noData&&lakes[x][y]==noData)
						{
						Qd = (thetaSD-FC[x][y])*SD[x][y]*drainageRate2[x][y];//

						//if(x==pixel_aX&&y==pixel_aY)cout<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<Qd<<" Qd agriculture"<<endl;					
						if(texture[x][y]>=12&&texture[x][y]<=19)
							{
							float Qd_act=0;
							if(SD[x][y]<500)
								Qd_act = Qd;//in mm
							else
								Qd_act = 500/SD[x][y]; //in mm, maximum drainage depth at 0.5 m

							/*----------------------------------------------------------
							 INTERFLOW SPLIT
							 interflow_frac of Qd_act enters the catchment interflow
							 store and is released gradually over subsequent days.
							 The remainder (1-interflow_frac) is routed instantaneously
							 to the nearest stream cell as before (tile drain fast path).
							 Tune interflow_frac in [0.5, 0.9] — start with 0.7.
							----------------------------------------------------------*/
							float Qd_fast = (1.0f - interflow_frac) * Qd_act;
							float Qd_slow = interflow_frac           * Qd_act;

							// Fast path: instantaneous tile drain (original behaviour)
							Qd_cum += Qd_fast * (dx*dy)/1000; // m³
							short int riverX=drainRoute[x][y][0];
							short int riverY=drainRoute[x][y][1];
							dsc[riverX][riverY] += Qd_fast;

							// Slow path: feeds interflow reservoir
							interflow_store += Qd_slow * (dx*dy) / 1000.0f; // m³

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
/*----------------------END Qd--------------------------------*/
				#include "write_lupoints.inc"				
				}//end if dem!=channel
			}//x
		}//y

/*------------------------------------------------------------------------------
 INTERFLOW + GROUNDWATER RESERVOIR RELEASE
 Executed once per day after the spatial loop above.
 Both stores decay linearly; their combined output is added to Qd_cum.
 The interflow store captures the 1-10 day recession signal.
 The groundwater store captures the slow winter/dry-season baseflow.

 Diagnostic output is written to oQE via the existing variables — no new
 output file needed. The sum Q_if_out + Q_gw_out is added to Qd_cum so it
 appears in the "q_drain" column of QE_*.csv and in oQsource as "drain".
 If you want them in separate columns, add them to the oQE format string.
------------------------------------------------------------------------------*/
{
float Q_if_out  = k_interflow * interflow_store;  // m³/day
float Q_gw_out  = k_gw        * gw_store;         // m³/day

// Guard against numerical underflow
if(interflow_store < 0.0f) interflow_store = 0.0f;
if(gw_store        < 0.0f) gw_store        = 0.0f;

interflow_store -= Q_if_out;
gw_store        -= Q_gw_out;

Qd_cum += Q_if_out + Q_gw_out;

// Optional console diagnostics (comment out after calibration)
cout << "  IF_store=" << interflow_store
     << " Q_if="  << Q_if_out
     << " GW_store=" << gw_store
     << " Q_gw="  << Q_gw_out << endl;
}
