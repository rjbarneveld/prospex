if(x==pixel_1X&&y==pixel_1Y)
	{
	oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<infil<<" infil loc1"<<endl;		
	oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<toDistribute[x][y]<<" toDistribute loc1"<<endl;		
	oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<dailySurplus[x][y]<<" dailySurplus loc1"<<endl;		
	oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<availablePoreDepth<<" availablePoreDepth loc1"<<endl;
	oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<EHDepth[x][y][1]<<" EHD loc1"<<endl;
	oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<SD[x][y]<<" SD loc1"<<endl;
	oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<G[x][y][0]-dem[x][y]<<" G loc1"<<endl;
	oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<SW_EHD[x][y]/EHDepth[x][y][1]<<" theta_EHD loc1"<<endl;
	oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<SW_EHD[x][y]<<" sw_EHD loc1"<<endl;	
	oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<SW[x][y][0]/SD[x][y]<<" theta_soil loc1"<<endl;	
	oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<dsc[x][y]<<" dsc loc1"<<endl;
	}			

if(x==pixel_2X&&y==pixel_2Y)
	{
	oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<infil<<" infil loc2"<<endl;		
	oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<toDistribute[x][y]<<" toDistribute loc2"<<endl;		
	oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<dailySurplus[x][y]<<" dailySurplus loc2"<<endl;		
	oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<availablePoreDepth<<" availablePoreDepth loc2"<<endl;
	oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<EHDepth[x][y][1]<<" EHD loc2"<<endl;
	oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<SD[x][y]<<" SD loc2"<<endl;
	oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<G[x][y][0]-dem[x][y]<<" G loc2"<<endl;
	oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<SW_EHD[x][y]/EHDepth[x][y][1]<<" theta_EHD loc2"<<endl;
	oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<SW_EHD[x][y]<<" sw_EHD loc2"<<endl;
	oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<SW[x][y][0]/SD[x][y]<<" theta_soil loc2"<<endl;
	oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<dsc[x][y]<<" dsc loc2"<<endl;
	}

if(x==pixel_3X&&y==pixel_3Y)
	{
	oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<infil<<" infil loc3"<<endl;		
	oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<toDistribute[x][y]<<" toDistribute loc3"<<endl;		
	oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<dailySurplus[x][y]<<" dailySurplus loc3"<<endl;		
	oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<availablePoreDepth<<" availablePoreDepth loc3"<<endl;
	oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<EHDepth[x][y][1]<<" EHD loc3"<<endl;
	oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<SD[x][y]<<" SD loc3"<<endl;
	oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<G[x][y][0]-dem[x][y]<<" G loc3"<<endl;
	oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<SW_EHD[x][y]/EHDepth[x][y][1]<<" theta_EHD loc3"<<endl;
	oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<SW_EHD[x][y]<<" sw_EHD loc3"<<endl;
	oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<SW[x][y][0]/SD[x][y]<<" theta_soil loc3"<<endl;
	oLU<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<dsc[x][y]<<" dsc loc3"<<endl;
	}
