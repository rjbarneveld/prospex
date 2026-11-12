	int lake1=53;
	int lake2=118;//elgtjern
	int lake3=189;

	std::string makeDateString(int year, int month, int day) {
	    std::ostringstream oss;

	    oss << year << "-"
		<< std::setw(2) << std::setfill('0') << month << "-"
		<< std::setw(2) << std::setfill('0') << day;

	    return oss.str();
	}

	bool isLeapYear(int year) {
	    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
	}

	int dayOfYear(const std::string& date) {
	    // date format: "yyyy-mm-dd"
	    int year  = std::stoi(date.substr(0, 4));
	    int month = std::stoi(date.substr(5, 2));
	    int day   = std::stoi(date.substr(8, 2));

	    std::array<int, 12> daysInMonth = {
		31, 28, 31, 30, 31, 30,
		31, 31, 30, 31, 30, 31
	    };

	    if (isLeapYear(year)) {
		daysInMonth[1] = 29;
	    }

	    int doy = day;
	    for (int m = 0; m < month - 1; ++m) {
		doy += daysInMonth[m];
	    }

	    return doy;
	}
	string date = makeDateString(year[index], month[index], day[index]);
	float yearDay = dayOfYear(date);

	//calculate lake outflow and update toDistribute at outflow points
	float qin = 0;
	float radiation = 0;
	float phi = 0;
	float delta = 0;
	float omega = 0;
	float latitude = 60;
	//(24*60)/3.142*Gsc*dr*
	radiation = max(0,(1));
	for(int locLake=0;locLake<nrLakes-1;locLake++)
		{
		// 0 area, 1 xIndex, 2 yIndex, 3 z, 4 Q_in, 5 Q_out (m/day), 6 V, 7 level (m above , 8 index
		float Td=tm[index]-2.0f;
		Td = std::min(Td, tm[index]);
		float Tm=tm[index]+.006*lakeStats[locLake][3];
		float latitudeDeg = 60.0f;
//		float evapoLake = (700.0f * Tm / (100.0f - latitudeDeg) + 15.0f * (tm[index] - Td)) / (80.0f - tm[index]);//Linacre, https://doi.org/10.1016/0002-1571(77)90007-3
Oudin -->		float evapoLake
		cout<<evapoLake<<" "<<lakeStats[locLake][3]<<endl;
		if(locLake==0)cout<<evapoLake<<endl;
		if(evapoLake<0||tm[index]<0.1)
			evapoLake=0;
		lakeStats[locLake][7]+=(rr[index]-evapoLake)/1000;//in m
		float n=.02; //main channel (1a), normal
		lakeStats[locLake][5]=0;
		float V_out=0;
		int outFlowType=1;//manning (potential is less than available), 1 proportional (.25 of depth)
		float outflowRate=.25;
		if(lakeStats[locLake][7]>0)
			{
			if(outFlowType==0)
				{
				lakeStats[locLake][5]=(3600*24)*(1/n) * pow((.0+lakeStats[locLake][7]),1.5) *pow(.01,.5);//Q_out as a function of water level over 0 (in m d⁻¹), assumed base level of 0.0 m over 0|
				V_out=lakeStats[locLake][5]*lakeStats[locLake][7]*dx;//volume out (m³)
				//V_out*=.1;//small streams
				if(V_out>(lakeStats[locLake][0]*lakeStats[locLake][7]))//if potential outflow is less than available
					{
					V_out = lakeStats[locLake][0]*lakeStats[locLake][7];//in m3d-1 
					lakeStats[locLake][7] -= (V_out/lakeStats[locLake][0]);//new level after outflow (m)
					}
				else if(V_out<(lakeStats[locLake][0]*lakeStats[locLake][7])&&V_out>0)
					{
					lakeStats[locLake][7]=0;
					outFlowType=1;
					}
				}//outFlowType=0
			else if(outFlowType==1)
				{
				V_out=outflowRate*lakeStats[locLake][0]*lakeStats[locLake][7];//volume out (m³)
				lakeStats[locLake][7]*=(1-outflowRate);
				}//outFlowType=1
			int outX=lakeStats[locLake][1];
			int outY=lakeStats[locLake][2];
			if(lakes[outX][outY]!=noData)
				{
				int outLake=lakes[outX][outY];
				int outLakeIndex=0;
				for(int lakeIndex2=0;lakeIndex2<nrLakes;lakeIndex2++)
					{
					if(lakeStats[lakeIndex2][8]==outLake)
						outLakeIndex=lakeIndex2;
					}
				lakeStats[outLakeIndex][7]+=(V_out/lakeStats[outLakeIndex][0]);
				}
			else if(lakeStats[locLake][8]==21||lakeStats[locLake][8]==22||lakeStats[locLake][8]==98||lakeStats[locLake][8]==9||lakeStats[locLake][8]==82||lakeStats[locLake][8]==83)//channel[outX][outY]!=noData&&locLake>=82&&locLake<=84)//			else if(lakeStats[locLake][8]==83)
				Q_lake += V_out;
			else if(channel[outX][outY]==1)
				Q_lake += V_out;
			else
				dsc[outX][outY]+=(V_out/pow(dx,2)*1000);//mm
			}
		if(lakeStats[locLake][8]==lake1)
			oLake<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<lakeStats[locLake][7]<<" "<<lakeStats[locLake][6]<<" "<<V_out<<" "<<outFlowType<<" "<<lake1<<endl;
		else if(lakeStats[locLake][8]==lake2)
			oLake<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<lakeStats[locLake][7]<<" "<<lakeStats[locLake][6]<<" "<<V_out<<" "<<outFlowType<<" "<<lake2<<endl;
		else if(lakeStats[locLake][8]==lake3)
			oLake<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<lakeStats[locLake][7]<<" "<<lakeStats[locLake][6]<<" "<<V_out<<" "<<outFlowType<<" "<<lake3<<endl;
		}
