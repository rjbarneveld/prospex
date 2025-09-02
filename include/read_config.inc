string inString,inDummy;
int readcounter=0;
fin.open(argv[1]);//input.config
if(fin.is_open())
	{
	cout<<argv[1]<<" opened, ";
	while(!fin.eof())
		{
		fin >> inDummy>>inString;
		if(readcounter==0)ioFolder=inString;
		else if(readcounter==1)filename_CLIMA=inString;
		else if(readcounter==2)nrDays=stoi(inString);
		else if(readcounter==3)filename_PARAMS=inString;
		else if(readcounter==4)filename_LAKES=inString;
		else if(readcounter==5)gid=inString;
		else if(readcounter==6)file_DEM=ioFolder + "/" + inString;
		else if(readcounter==7)file_TEXT=ioFolder + "/" + inString;
		else if(readcounter==8)file_LU=ioFolder + "/" + inString;
		else if(readcounter==9)file_VEGETATION=ioFolder + "/" + inString;
		else if(readcounter==10)file_CHANNEL=ioFolder + "/" + inString;
		else if(readcounter==11)file_LAKES=ioFolder + "/" + inString;
		else if(readcounter==12)file_GROUNDWATER=ioFolder + "/" + inString;
		else if(readcounter==13)file_MEASURES=ioFolder + "/" + inString;
		else if(readcounter==14)scenario=stoi(inString);
		else if(readcounter==15)riverQ=stoi(inString);
		else if(readcounter==16)dynamicG=stoi(inString);
		else if(readcounter==17)erosion=stoi(inString);		
		else if(readcounter==18)gullyThreshold=stoi(inString);
		else if(readcounter==19)filename_LUPOINTS=inString;
		else if(readcounter==20)filename_GROUNDWATERCONFIG=inString;		
		readcounter++;
		}
	fin.clear();
	fin.close();
	cout<<"input files read"<<endl;
	}
else
	cout<<"check location of configuration file"<<endl;
