string inString,inDummy;
int readcounter=0;
fin.open(argv[1]);// projectfolder/input.config
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
		else if(readcounter==13)file_DRAINROUTE=ioFolder + "/" + inString;		
		else if(readcounter==14)file_MEASURES=ioFolder + "/" + inString;
		else if(readcounter==15)scenario=stoi(inString);
		else if(readcounter==16)riverQ=stoi(inString);
		else if(readcounter==17)dynamicG=stoi(inString);
		else if(readcounter==18)erosion=stoi(inString);		
		else if(readcounter==19)gullyThreshold=stoi(inString);
		else if(readcounter==20)filename_LUPOINTS=inString;
		else if(readcounter==21)filename_GROUNDWATERCONFIG=inString;
		else if(readcounter==22)simulateDams=stoi(inString);
		else if(readcounter==23&&simulateDams==1)filename_DAMS=inString;
		else if(readcounter==24&&simulateDams==1)file_DAMS=ioFolder + "/" + inString;
		else if(readcounter==25)filename_HYDROGRAPH=ioFolder + "/" + inString;
		readcounter++;
		}
	fin.clear();
	fin.close();
	cout<<"input files read"<<endl;
	}
else
	cout<<"check location of configuration file"<<endl;
