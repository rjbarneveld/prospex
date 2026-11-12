	int lake1=53;
	int lake2=118;//elgtjern
	int lake3=189;

	// Local lambdas instead of normal function definitions.
	// This .inc file is included inside the main routine, where free-standing
	// function definitions are not allowed by C++.
	auto makeDateString = [](int yy, int mm, int dd) -> std::string {
		std::ostringstream oss;
		oss << yy << "-"
		    << std::setw(2) << std::setfill('0') << mm << "-"
		    << std::setw(2) << std::setfill('0') << dd;
		return oss.str();
	};

	auto isLeapYear = [](int yy) -> bool {
		return (yy % 4 == 0 && yy % 100 != 0) || (yy % 400 == 0);
	};

	auto dayOfYear = [&](const std::string& date) -> int {
		// date format: "yyyy-mm-dd"
		int yy = std::stoi(date.substr(0, 4));
		int mm = std::stoi(date.substr(5, 2));
		int dd = std::stoi(date.substr(8, 2));

		int daysInMonth[12] = {
			31, 28, 31, 30, 31, 30,
			31, 31, 30, 31, 30, 31
		};

		if(isLeapYear(yy))
			daysInMonth[1] = 29;

		int doy = dd;
		for(int m=0; m<mm-1; m++)
			doy += daysInMonth[m];

		return doy;
	};

	std::string date = makeDateString(year[index], month[index], day[index]);
	float yearDay = static_cast<float>(dayOfYear(date));

	//calculate lake outflow and update toDistribute at outflow points
	float qin = 0;
	float latitudeDeg = 60.0f;
	float latitudeRad = latitudeDeg * 3.14159265358979323846f / 180.0f;

	// FAO-56 extraterrestrial radiation, Ra [MJ m-2 day-1].
	// Used in Oudin PET/open-water evaporation estimate below.
	float Gsc = 0.0820f; // MJ m-2 min-1
	float dr = 1.0f + 0.033f * cosf(2.0f * 3.14159265358979323846f * yearDay / 365.0f);
	float delta = 0.409f * sinf(2.0f * 3.14159265358979323846f * yearDay / 365.0f - 1.39f);
	float acosArg = -tanf(latitudeRad) * tanf(delta);
	acosArg = std::max(-1.0f, std::min(1.0f, acosArg));
	float omega = acosf(acosArg);
	float radiation = (24.0f * 60.0f / 3.14159265358979323846f) * Gsc * dr *
		(omega * sinf(latitudeRad) * sinf(delta) +
		 cosf(latitudeRad) * cosf(delta) * sinf(omega));
	radiation = std::max(0.0f, radiation);

	// Oudin factor for open water. Use 1.0 for bare Oudin PET;
	// values around 1.05-1.20 are a reasonable first lake correction.
	float lakeEvapFactor = 1.10f;

	for(int locLake=0;locLake<nrLakes-1;locLake++)
		{
		if(lakeStats[locLake][3]>0)
			{
			// 0 area, 1 xIndex, 2 yIndex, 3 z, 4 Q_in, 5 Q_out (m/day), 6 V, 7 level (m above , 8 index
			float evapoLake = lakeEvapFactor * std::max(0.0f,
				(radiation / 2.45f) * ((tm[index] + 5.0f) / 100.0f)); // mm/day, Oudin-style
			if(evapoLake<0||tm[index]<0.1)
				evapoLake=0;
			lakeStats[locLake][7]+=(rr[index]-evapoLake)/1000;//in m
			float n=.02; //main channel (1a), normal
			lakeStats[locLake][5]=0;
			float V_out=0;
			int outFlowType=1;//manning (potential is less than available), 1 proportional (.25 of depth)
			float outflowRate=.5;
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
		}
