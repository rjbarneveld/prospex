//prospex-Hydro/Sedi
//Process-based Spatially Explicit
//prospex-Hydro/Sedi is a versatile model that allows for the evaluation of any change in the spatial arrangement of a catchment on runoff and erosion and deposition
#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <fstream>
#include "/usr/include/gdal/gdal_priv.h"
#include "/usr/include/gdal/cpl_conv.h"
#include "include/read_geodata.h"
#include "include/read_geodata_2d.h"
#include "include/write_geotif.h"
#include "include/write_geotif_2d.h"
#include "include/et0.h"
#include "include/kc.h"
#define PI 3.1425
//g++ -std=c++11 v_01_infiltration.cpp -lgdal -o v_01_infiltration
//./v_011_infiltration input.config

using namespace std;
int main(int argc, char* argv[]) 
{
string dummystring;
/*DECLARE CONFIGURATION VARIABLES*/
string ioFolder;
string filename_CLIMA;
int nrDays=0;
string filename_PARAMS;
string filename_LAKES;
string filename_LUPOINTS;
string filename_GROUNDWATERCONFIG;

string file_DEM, file_TEXT, file_LU, file_VEGETATION, file_CHANNEL, file_LAKES, file_GROUNDWATER, file_MEASURES;
string gid="case";
short int scenario=0;
short int riverQ=0;//0 no flow accumulation in river system, 1 continue until lake
short int dynamicG=0;//0 no vertical or lateral groundwater movement
short int gullyThreshold=10;
short int erosion=0;//1 model erosion

ifstream fin;
#include "include/read_config.inc"

GDALDataset *INDEM;
GDALAllRegister();
INDEM = (GDALDataset*) GDALOpen(file_DEM.c_str(),GA_ReadOnly);
float noData = INDEM->GetRasterBand(1)->GetNoDataValue();
double adfGeoTransform[6];
INDEM->GetGeoTransform( adfGeoTransform );
double dx = adfGeoTransform[1];
double dy = fabs(adfGeoTransform[5]);
double originX = adfGeoTransform[0];
double originY = adfGeoTransform[3];
int xSize = INDEM->GetRasterXSize();
int ySize = INDEM->GetRasterYSize();

/*READ MODEL INPUT AND ASSIGN VALUES TO ARRAYS, TABLES*/
#include "include/read_tables.inc"
#include "include/declare_arrays.inc"

ReadGeoTIFFBandToArray(file_DEM.c_str(), dem, xSize, ySize);
//PATCH FOR FAULTY NODATA VALUE
for(int y=0;y<ySize;y++)
	{
	for(int x=0;x<xSize;x++)
		{
		if(dem[x][y]<0)
			dem[x][y]=noData;
		}
	}
		
ReadGeoTIFFBandToArray(file_LU.c_str(), landuse, xSize, ySize);
ReadGeoTIFFBandToArray(file_VEGETATION.c_str(), vegetation, xSize, ySize);
ReadGeoTIFFBandToArray(file_TEXT.c_str(), texture, xSize, ySize);
ReadGeoTIFFBandToArray(file_CHANNEL.c_str(), channel, xSize, ySize);
ReadGeoTIFFBandToArray(file_LAKES.c_str(), lakes, xSize, ySize);
ReadGeoTIFFBandToArray2D(file_GROUNDWATER.c_str(), G, xSize, ySize);
if(file_MEASURES.c_str()==ioFolder + "/dummy.tif")
	{int A=0;}
else
	ReadGeoTIFFBandToArray(file_MEASURES.c_str(), measures, xSize, ySize);
cout<<"All geotifs read and written to arrays"<<endl;

/*READ RUN-SPECIFIC INPUT AND UPDATE ARRAYS*/
#include "run.config"
#include "include/convert_input.inc"
#include "include/flowratios.inc"
#include "include/parameterise_measures.inc"
if(scenario!=0)
	#include "include/parameterise_measures.inc"
float Q_catchment=0;
float Q_stream=0;
float Q_catchmentSRO=0;
float Qd=0;
float Qd_cum=0;
float Q_lake=0;
float ssg_catchment=0;
float ERO_overland=0;
float ERO_gully=0;
cout<<"scenario "<<scenario<<endl;

/*DECLARE AND OPEN OUTPUT FILES (TIMESERIES)*/
ofstream oLU;
ofstream oSnow;
ofstream oQE;
ofstream oQsource;
ofstream oLake;
oLU.open(ioFolder + "/lu_points_timeseries" + to_string(scenario) + ".csv");oLU<<"date value parameter land_use"<<endl;
oSnow.open(ioFolder + "/snow_timeseries.csv");oSnow<<"date rr tm swe accumulation melt"<<endl;
oQE.open(ioFolder + "/QE" + to_string(scenario) + ".csv");oQE<<"date q_drain q_overland q_stream q_lake e_drain e_overland e_gully"<<endl;
oQsource.open(ioFolder + "/oQ.csv");
oQsource<<"date Q_m3d source"<<endl;
oLake.open(ioFolder + "/lakesOut.csv");
oLake<<"date level volume q_out qin lake"<<endl;


/*BEGIN MAIN ROUTINE: 1. SOIL WATER BALANCE, 2 LATERAL SUBSURFACE, 3 OVERLAND FLOW, 4 EROSION/DEPOSITION*/
for(int index=0;index<nrDays;index++)//nrDays)
	{
	cout<<index<<" "<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<tm[index]<<" "<<rr[index]<<endl;
	/*CALCULATING GROUNDWATER LEVEL IF DYNAMIC*/
	if(dynamicG==1)
		#include "include/update_soil_layers.inc"
	Q_catchment=0;
	Q_catchmentSRO=0;
	Q_stream=0;
	Qd_cum=0;
	Q_lake=0;
	ssg_catchment=0;
	
	/*CALCULATE SNOW WATER EQUIVALENT*/
	float snowAccumulation=0;
	float snowMelt=0;
	if(tm[index]<maxRain&&rr[index]>0)
		{
		snowAccumulation=rr[index];
		rr[index]=0;
		}
	else if(swe[index]>0)
		swe[index+1]+=rr[index];
	if(tm[index]>minMelt&&swe[index]>0)
		snowMelt=snowParam1 * pow((tm[index]-minMelt),snowParam2);
	if(snowMelt>swe[index])
		snowMelt=swe[index];//CHECK THIS
	if(index<nrDays-1)
		swe[index+1]=swe[index]+snowAccumulation-snowMelt;
	oSnow<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<rr[index]<<" "<<tm[index]<<" "<<swe[index]<<" "<<snowAccumulation<<" "<<snowMelt<<endl;
	
	/*UPDATE SOIL WATER BALANCE*/
	#include "include/soil_waterbalance.inc"
	
	/*CALCULATING GROUNDWATER FLOW IF DYNAMIC*/
	if(dynamicG==1)
		#include "include/lateral_subsurface.inc"
	/*FLOW ROUTING*/
	float pctDone=0;
	float pctDoneOld=0;
	int routingCounter=0;
	#include "include/flacc.inc"
		Q_catchment+=Qd_cum;
	/*CALCULATE EROSION AND DEPOSITION*/
	if(erosion==1)
		{
		#include "include/detachment.inc"	
		#include "include/sedirouting.inc"
		}

	oQE<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<Qd_cum<<" "<<Q_catchmentSRO<<" "<<Q_stream<<" "<<Q_lake<<" "<<ssg_catchment<<" "<<ERO_overland<<" "<<ERO_gully<<endl;
	oQsource<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<Q_catchment<<" drain"<<endl;
	oQsource<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<Q_catchmentSRO<<" overland"<<endl;
	oQsource<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<Q_stream<<" stream"<<endl;
	oQsource<<year[index]<<"-"<<month[index]<<"-"<<day[index]<<" "<<Q_lake<<" lake"<<endl;
	/*FIXED HARVEST DATE AT SEPTEMBER 1st(UPDATE REQUIRED)*/
	if(month[index]==9&&day[index]==1)
		{
		degDayWinterwheat[index]=0;
		degDaySpringwheat[index]=0;
		}
	else if(month[index]==10&&day[index]==1)
		{
		degDayWinterwheat[index]=.01;
		}
	else if(month[index]==5&&day[index]==15)
		{
		degDaySpringwheat[index]=.01;
		}
	/*RESET ROUGHNESS AND GULLY*/
	for(int y=0;y<ySize;y++)
		{
		for(int x=0;x<xSize;x++)
			{
			if(vegetation[x][y]==6&&month[index]==9&&day[index]==1)//reset roughness and gully: tillage winter wheat
				{
				gully[x][y]=false;
				if(scenario==0)
					dsc_max[x][y]=3.27/(1 + exp(20*(slope[x][y]+.01)));
				else
					dsc_max[x][y]=1.27/(1 + exp(20*(slope[x][y]+.01)));
				eroMFD[x][y]=0;
				depoMFD[x][y]=0;				
				}
			else if(vegetation[x][y]==7&&month[index]==5&&day[index]==1)//reset roughness and gully: tillage spring wheat
				{
				gully[x][y]=false;
				dsc_max[x][y]=1.27/(1 + exp(20*(slope[x][y]+.01)));
				eroMFD[x][y]=0;
				depoMFD[x][y]=0;
				}
			else if(vegetation[x][y]==6&&month[index]==10&&day[index]==1)//reduced roughness: seedbed winter wheat
				{
				dsc_max[x][y]=1.27/(1 + exp(20*(slope[x][y]+.01)));
				}
			else if(vegetation[x][y]==7&&month[index]==5&&day[index]==15)//reduced roughness: seedbed spring wheat
				{
				dsc_max[x][y]=1.27/(1 + exp(20*(slope[x][y]+.01)));
				}
			}
		}

	/*RESET EROSION LAYERS; SPRING TILLAGE ON THE 1ST OF MAY (UPDATE REQUIRED)*/
	if(month[index]==12&&day[index]==31)
		{
		/*WRITE MAPS*/
		//#include "include/write_geotif.inc"
		saveGeoTIFF(ioFolder + "/q_" + std::to_string(year[index]) + "_" + gid + ".tif", Q_cum, xSize, ySize, adfGeoTransform, INDEM->GetProjectionRef());
		saveGeoTIFF(ioFolder + "/qMax_" + std::to_string(year[index]) + "_" + gid + ".tif", flaccMAX, xSize, ySize, adfGeoTransform, INDEM->GetProjectionRef());		
		saveGeoTIFF(ioFolder + "/sw_" + std::to_string(year[index]) + "_" + gid + ".tif", SW_EHD, xSize, ySize, adfGeoTransform, INDEM->GetProjectionRef());
		if(dynamicG==1)	
			saveGeoTIFF2d(ioFolder + "/groundwaterLevel_" + std::to_string(year[index]) + "_" + gid + ".tif", G, xSize, ySize, 0, adfGeoTransform, INDEM->GetProjectionRef());
		for(int y=0;y<ySize;y++)
			{
			for(int x=0;x<xSize;x++)
				{
				eroMFD[x][y]=0;
				depoMFD[x][y]=0;
				Q_cum[x][y]=0;
				flaccMAX[x][y]=0;
				}
			}
		}
	}//end nrDays

oLU.close();oLU.clear();
oSnow.close();oSnow.clear();
oQE.close();oQE.clear();
oQsource.close();oQsource.clear();
oLake.close();oLake.clear();
return 0;
}
