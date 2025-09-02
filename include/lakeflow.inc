	//int lake1=72;
	//int lake2=74;
	//int lake3=83;
	int lake1=0;
	int lake2=1;
	int lake3=2;

	//calculate lake outflow and update toDistribute at outflow points
	float qin=0;
	for(int locLake=0;locLake<nrLakes-1;locLake++)
		{
		// 0 area, 1 xIndex, 2 yIndex, 3 z, 4 Q_in, 5 Q_out (m/day), 6 V, 7 level, 8 index
		float dewPoint=1.066*(tm[index]-10);
		float evapoLake=.5*(700*(tm[index]+.006*lakeStats[locLake][3])/(100-60)+15*(tm[index]-dewPoint))/(80-tm[index]);//https://doi.org/10.1016/0002-1571(77)90007-3
		if(evapoLake<0||tm[index]<0.1)
			evapoLake=0;
		lakeStats[locLake][7]+=(rr[index]-evapoLake)/1000;//in m
		float n=.02; //main channel (1a), normal
		lakeStats[locLake][5]=0;
		float V_out=0;
		int outFlowType=0;//manning (potential is less than available), 1 complete
		
		if(lakeStats[locLake][7]>-9999)
			{
			lakeStats[locLake][5]=(3600*24)*(1/n) * pow((.0+lakeStats[locLake][7]),1.5) *pow(.01,.5);//Q_out as a function of water level over 0 (in m d⁻¹), assumed base level of 0.0 m over 0
			if((lakeStats[locLake][5]*dx)<lakeStats[locLake][0])//if potential outflow is less than available
				{
				V_out = lakeStats[locLake][5]*lakeStats[locLake][7];//in m3d-1 
				if(V_out>0)
					lakeStats[locLake][7] -= (V_out/lakeStats[locLake][0]);//new level after outflow (m)
				}
			else if((lakeStats[locLake][5]*dx)>lakeStats[locLake][0])//if potential flow is larger than available
				{
				V_out = lakeStats[locLake][0] * lakeStats[locLake][7];//all available volume (area [0] * depth [7]
				if(V_out>0)
					lakeStats[locLake][7]=0;
				outFlowType=1;
				}
			int outX=lakeStats[locLake][1];
			int outY=lakeStats[locLake][2];
cout<<locLake<<" "<<outX<<":"<<outY<<" "<<lakes[outX][outY]<<endl;
			if(lakes[outX][outY]!=noData)
				{
				int outLakeIndex=0;
				for(int lake2Index=0;lake2Index<nrLakes;lake2Index++)
					{
					if(lakes[outX][outX]==lakeStats[lake2Index][8])
						outLakeIndex=lake2Index;
					}
				lakeStats[outLakeIndex][7]+=(V_out/lakeStats[outLakeIndex][0]);
				}
				
			else if(channel[outX][outY]!=noData&&locLake>=82&&locLake<=84)
				Q_lake += V_out;
			else
				dsc[outX][outY]+=(V_out/pow(dx,2)*1000);//mm
				//dsc[outX][outY]+=(V_out/pow(dx,2)*1000);
				//cout<<lakeStats[locLake][8]<<" "<<outX<<":"<<outY<<" ("<<lakes[outX][outY]<<") v="<<V_out<<endl;}
			}
	cout<<"lake still ok"<<endl;
		if(lakeStats[locLake][8]==lake1)
			oLake<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<lakeStats[locLake][7]<<" "<<lakeStats[locLake][6]<<" "<<V_out<<" "<<outFlowType<<" "<<lake1<<endl;
		else if(lakeStats[locLake][8]==lake2)
			oLake<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<lakeStats[locLake][7]<<" "<<lakeStats[locLake][6]<<" "<<V_out<<" "<<outFlowType<<" "<<lake2<<endl;
		else if(lakeStats[locLake][8]==lake3)
			oLake<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<lakeStats[locLake][7]<<" "<<lakeStats[locLake][6]<<" "<<V_out<<" "<<outFlowType<<" "<<lake3<<endl;
		}
