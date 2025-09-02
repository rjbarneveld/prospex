readcounter=0;
float inParam0,inParam1,inParam2,inParam3,inParam4,inParam5,inParam6,inParam7,inParam8;
float params[16][9];
for(int soil=0;soil<16;soil++)
	{
	for(int parameter=0;parameter<9;parameter++)
		params[soil][parameter]=0;
	}

readcounter=0;
fin.open(ioFolder + "/" + filename_PARAMS);
if(fin.is_open())
	{
	cout<<filename_PARAMS<<" opened ";
	while(readcounter<16)
		{
		fin>>inParam0>>inParam1>>inParam2>>inParam3>>inParam4>>inParam5>>inParam6>>inParam7>>inParam8;
		params[readcounter][0]=inParam0;//MS theta sat
		params[readcounter][1]=inParam1;//BD
		params[readcounter][2]=inParam2;//LP k sat
		params[readcounter][3]=inParam3;//DR erodibility
		params[readcounter][4]=inParam4;//drainage rate EHD (per timestep)
		params[readcounter][5]=inParam5;//drainage rate subsoil (per timestep)
		params[readcounter][6]=inParam6;//theta fc	
		params[readcounter][7]=inParam7;//MMF erodibility
		params[readcounter][8]=inParam8;//EHD in m
		readcounter++;
		}
	fin.clear();
	fin.close();
	cout<<" and closed ("<<readcounter<<" parameters)"<<endl;
	}
else
	{
	cout<<filename_PARAMS<<" not read"<<endl;
	return 0;
	}
float kG[2] = {{0},{0}};//conductivity i m/time unit (day)
float inKgw = 0;
readcounter=0;
fin.open(ioFolder + "/" + filename_GROUNDWATERCONFIG.c_str());
if(fin.is_open())
	{
	while (fin >> dummystring >> inKgw)
		{
		kG[readcounter] = inKgw;
		readcounter++;
		}
	fin.clear();
	fin.close();
	}
else
	{
	cout<<filename_GROUNDWATERCONFIG<<" not read"<<endl;
	return 0;
	}

	
readcounter=0;
fin.open(ioFolder + "/" + filename_LAKES.c_str());
if(fin.is_open())
	{
	while(!fin.eof())
		{
		if(readcounter==0)
			fin>>dummystring>>dummystring>>dummystring>>dummystring>>dummystring>>dummystring>>dummystring;
		else if(readcounter>0)
			fin>>inParam1>>inParam2>>inParam3>>inParam4>>inParam5>>inParam6>>inParam7;
		readcounter++;
		}
	fin.clear();
	fin.close();
	}
else
	{
	cout<<filename_LAKES<<" not read"<<endl;
	return 0;
	}

int nrLakes=readcounter-2;
float lakeStats[readcounter][9];// 0 area, 1 xIndex, 2 yIndex, 3 z, 4 Q_in, 5 Q_out, 6 V, 7 level, 8 index
for(int i=0;i<nrLakes;i++)
	{
	for(int j=0;j<9;j++)
		lakeStats[i][j]=0;
	}

readcounter=0;
fin.open(ioFolder + "/" + filename_LAKES.c_str());
if(fin.is_open())
	{
	cout<<filename_LAKES<<" opened";
	while(!fin.eof())
		{
		if(readcounter==0)
			fin>>dummystring>>dummystring>>dummystring>>dummystring>>dummystring>>dummystring>>dummystring;
		else if(readcounter>0)
			{
			fin>>inParam1>>inParam2>>inParam3>>inParam4>>inParam5>>inParam6>>inParam7;
			int locLake=readcounter-1;
			lakeStats[locLake][0]=inParam2*10000;//area in ha; CONVERT
			lakeStats[locLake][1]=inParam6;//xIndex
			lakeStats[locLake][2]=inParam7;//yIndex
			lakeStats[locLake][3]=inParam3;//z_out
			lakeStats[locLake][4]=0;//Q_in
			lakeStats[locLake][5]=0;//Q_out
			lakeStats[locLake][6]=0;//V
			lakeStats[locLake][7]=0.0;//level
			lakeStats[locLake][8]=inParam1;//index
			}
		readcounter++;
		}
	fin.clear();
	fin.close();
	cout<<" and closed ("<<readcounter<<" parameters for " <<nrLakes<<" lakes)"<<endl;
	}
else
	{
	cout<<filename_LAKES<<" not read"<<endl;
	return 0;
	}
	
/*GET WEATHER RECORDS: INDEX YYYY MM DD TM RR*/	
int inIndex;
float inTM,inRR,inET,inSWE;
float inYear,inMonth,inDay;
int year[nrDays];
int month[nrDays];
int day[nrDays];
float tm[nrDays];
float rr[nrDays];
float et[nrDays];
float swe[nrDays];
float degDaySpringwheat[nrDays];
float degDayWinterwheat[nrDays];

for(int index=0;index<nrDays;index++)
	{
	tm[index]=0;
	rr[index]=0;
	et[index]=0;
	swe[index]=0;
	degDaySpringwheat[index]=0;
	degDayWinterwheat[index]=0;
	}

readcounter=0;
fin.open(ioFolder + "/" + filename_CLIMA.c_str());
if(fin.is_open())
	{
	cout<<filename_CLIMA<<" opened";
	while(readcounter<nrDays)
		{
		fin>>inIndex>>inYear>>inMonth>>inDay>>inTM>>inRR;
		year[inIndex]=inYear;
		month[inIndex]=inMonth;
		day[inIndex]=inDay;
		if(inTM!=-9999)
			tm[inIndex]=inTM;
		if(inRR!=-9999)
			rr[inIndex]=inRR;
		readcounter++;
		}
	fin.clear();
	fin.close();
	cout<<" and closed ("<<readcounter<<" days)"<<endl;
	}
else
	{
	cout<<filename_CLIMA<<" not read"<<endl;
	return 0;
	}
	
//*GET COORDINATES OF 3 SAMPLE POINTS*/
double X1=0;
double Y1=0;
double X2=0;
double Y2=0;
double X3=0;
double Y3=0;

readcounter=0;
int dummyInt;
float inX, inY;
fin.open(ioFolder + "/" + filename_LUPOINTS.c_str());
if(fin.is_open())
	{
	cout<<filename_LUPOINTS<<" opened";
	while(!fin.eof())
		{
		fin>>dummyInt>>inX>>inY>>inString;
		if(dummyInt==1){X1=inX;Y1=inY;}
		if(dummyInt==2){X2=inX;Y2=inY;}
		if(dummyInt==3){X3=inX;Y3=inY;}		
		readcounter++;
		}
	fin.clear();
	fin.close();
	cout<<" and closed"<<endl;
	}
else
	{
	cout<<filename_LUPOINTS<<" not read"<<endl;
	}
int pixel_1X = (X1 - originX) / dx;
int pixel_1Y = -(Y1 - originY) / dy;
int pixel_2X = (X2 - originX) / dx;
int pixel_2Y = -(Y2 - originY) / dy;
int pixel_3X = (X3 - originX) / dx;
int pixel_3Y = -(Y3 - originY) / dy;
