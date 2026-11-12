#include<iostream>
#include<fstream>
#include <stdlib.h>
#include <math.h>
#include <algorithm>
#include "/usr/include/gdal/gdal_priv.h"
#include "/usr/include/gdal/cpl_conv.h"

using namespace std;
int main(int argc, char* argv[]) 
{
string folder=argv[1];
/*
GDALDataset *INDEM;
GDALDataset *LAKES;
GDALDataset *DtW;
GDALAllRegister();
INDEM = (GDALDataset*) GDALOpen("io/dtm10_filled.tif",GA_ReadOnly);
LAKES = (GDALDataset*) GDALOpen("io/lakes.tif",GA_ReadOnly);
DtW = (GDALDataset*) GDALOpen("io/DtW.tif",GA_ReadOnly);
float noData = INDEM->GetRasterBand(1)->GetNoDataValue();
cout<<"noData = "<<noData<<endl;
double adfGeoTransform[6];
INDEM->GetGeoTransform( adfGeoTransform );
double dx = adfGeoTransform[1];
double dy = fabs(adfGeoTransform[5]);
double originX = adfGeoTransform[0];
double originY = adfGeoTransform[3];
int xSize = INDEM->GetRasterXSize();
int ySize = INDEM->GetRasterYSize();

GDALRasterBand  *poBandINDEM;
GDALRasterBand  *poBandLAKES;
GDALRasterBand  *poBandDtW;

poBandINDEM = INDEM->GetRasterBand( 1 );
poBandLAKES = LAKES->GetRasterBand( 1 );
poBandDtW = DtW->GetRasterBand( 1 );
//Setup driver
const char *pszFormat = "GTiff";
GDALDriver *poDriver;
poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
char *pszSRS_WKT = NULL;*/

ifstream fin;
string filename_LAKES=folder + "/lakes.tif";
string filename_DTM=folder + "/dtm10_filled.tif";
string filename_DTW=folder + "/DtW.tif";
const char* inDTM = filename_DTM.c_str();
const char* inLAKES = filename_LAKES.c_str();
const char* inDTW = filename_DTW.c_str();
//Open the input DEM
GDALDataset *INDEM;
GDALDataset *INLAKES;
GDALDataset *INDTW;
GDALAllRegister();

INDEM = (GDALDataset*) GDALOpen(inDTM,GA_ReadOnly);
INLAKES = (GDALDataset*) GDALOpen(inLAKES,GA_ReadOnly);
INDTW = (GDALDataset*) GDALOpen(inDTW,GA_ReadOnly);
float noData = INDEM->GetRasterBand(1)->GetNoDataValue();
double adfGeoTransform[6];
INDEM->GetGeoTransform( adfGeoTransform );
double dx = adfGeoTransform[1];
double dy = fabs(adfGeoTransform[5]);
double originX = adfGeoTransform[0];
double originY = adfGeoTransform[3];
int xSize = INDEM->GetRasterXSize();
int ySize = INDEM->GetRasterYSize();

cout<<xSize<<":"<<ySize<<endl;

GDALRasterBand  *poBandINDEM;
GDALRasterBand  *poBandINLAKES;
GDALRasterBand  *poBandINDTW;

poBandINDEM = INDEM->GetRasterBand( 1 );
poBandINLAKES = INLAKES->GetRasterBand( 1 );
poBandINDTW = INDTW->GetRasterBand( 1 );
//Setup driver
const char *pszFormat = "GTiff";
GDALDriver *poDriver;
poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
char *pszSRS_WKT = NULL;

//Read into 2d array (float): INDEM
float *pafScanline;
pafScanline = (float *) CPLMalloc(sizeof(float)*xSize*ySize);

//declare layers
vector<vector<float>> dem = vector<vector<float>>(xSize,vector<float> (ySize,0));
vector<vector<int>> lakes = vector<vector<int>>(xSize,vector<int> (ySize,0));
vector<vector<float>> DtW = vector<vector<float>>(xSize,vector<float> (ySize,0));
vector<vector<float>> G = vector<vector<float>>(xSize,vector<float> (ySize,0));
vector<vector<float>> hillslope = vector<vector<float>>(xSize,vector<float> (ySize,0));
vector<vector<float>> finalG = vector<vector<float>>(xSize,vector<float> (ySize,0));

for(int y=0;y<(ySize-0);y++)//set values
	{
	for(int x=0;x<(xSize-0);x++)
		{
		dem[x][y]=noData;
		lakes[x][y]=noData;
		DtW[x][y]=noData;
		G[x][y]=noData;
		hillslope[x][y]=0;
		finalG[x][y]=noData;
		}
	}
cout<<xSize<<":"<<ySize<<endl;

poBandINDEM->RasterIO(GF_Read,0,0,xSize,ySize,pafScanline,xSize,ySize,GDT_Float32,0,0);
cout<<"poBandINDEM read"<<endl;
for(int y = 0; y < ySize; y++)
	{
	for(int x = 0; x < xSize; x++)
		{
		dem[x][y] = pafScanline[y*xSize+x];
		}
	}
CPLFree(pafScanline);
cout<<"poBandINDEM assigned to dem[x][y]"<<endl;

pafScanline = (float *) CPLMalloc(sizeof(float)*xSize*ySize);
poBandINLAKES->RasterIO(GF_Read,0,0,xSize,ySize,pafScanline,xSize,ySize,GDT_Float32,0,0);
cout<<"poBandLAKES read"<<endl;
for(int y = 0; y < ySize; y++)
	{
	for(int x = 0; x < xSize; x++)
		{
		lakes[x][y] = pafScanline[y*xSize+x];
		}
	}
CPLFree(pafScanline);
cout<<"poBandINDEM assigned to lakes[x][y]"<<endl;

pafScanline = (float *) CPLMalloc(sizeof(float)*xSize*ySize);
poBandINDTW->RasterIO(GF_Read,0,0,xSize,ySize,pafScanline,xSize,ySize,GDT_Float32,0,0);
cout<<"poBandG read"<<endl;
for(int y = 0; y < ySize; y++)
	{
	for(int x = 0; x < xSize; x++)
		{
		DtW[x][y] = pafScanline[y*xSize+x];
		}
	}
CPLFree(pafScanline);
cout<<"poBandDtW assigned to DtW[x][y]"<<endl;

for(int y=0;y<(ySize-0);y++)//set values
	{
	for(int x=0;x<(xSize-0);x++)
		{
		if(lakes[x][y]==-9999)
			lakes[x][y]=noData;
		if(DtW[x][y]==-9999)
			DtW[x][y]=noData;
		}
	}
cout<<"still ok"<<endl;
int windowsize=15;
int hws=windowsize/2;
for(int y=hws;y<(ySize-hws);y++)
	{
	for(int x=hws;x<(xSize-hws);x++)
		{
		if(dem[x][y]!=noData)
			{
			float tot_dist=0;
			float c_high=.5;
			float c_low=.5;
			for(int Y=-hws;Y<=hws;Y++)
				{
				for(int X=-hws;X<=hws;X++)
					{
					if((x+X)>=0&&(y+Y)>=0&&(x+X)<xSize&&(y+Y)<ySize&&dem[x+X][y+Y]!=noData)
						{
						float loc_dist=pow((pow((X-x),2)+pow((Y-y),2)),0.5);
						tot_dist+=loc_dist;
						}
					}
				}
			for(int Y=-hws;Y<=hws;Y++)
				{
				for(int X=-hws;X<=hws;X++)
					{
					if((x+X)>=0&&(y+Y)>=0&&(x+X)<xSize&&(y+Y)<ySize&&dem[x+X][y+Y]!=noData)
						{
						float loc_dist=pow((pow((X-x),2)+pow((Y-y),2)),0.5);
						float W=loc_dist/tot_dist;
						if(X==0&&Y==0)
							X++;
						if(lakes[x+X][y+Y]!=noData)
							c_low+=1;
						else if(dem[x+X][y+Y]>dem[x][y])
							c_high+=1;
						else
							c_low+=1;
						}
					}
				}
			if(!isnan(c_low/c_high)&&!isinf(c_low/c_high))
				hillslope[x][y]=log(c_low/c_high);
			else
				hillslope[x][y]=0;
			}
		}
	}
cout<<"still ok3"<<endl;

float maxhs=-999;
float minhs=999;
for(int y=0;y<(ySize-0);y++)//set values
	{
	for(int x=0;x<(xSize-0);x++)
		{
		if(hillslope[x][y]!=noData&&!isnan(hillslope[x][y])&&!isinf(hillslope[x][y]))
			{
			if(hillslope[x][y]<minhs)
				minhs=hillslope[x][y];
			if(hillslope[x][y]>maxhs)
				maxhs=hillslope[x][y];
			}
		}
	}
cout<<minhs<<" "<<maxhs<<endl;
for(int y=0;y<(ySize-0);y++)//set values
	{
	for(int x=0;x<(xSize-0);x++)
		{
		if(!isnan(hillslope[x][y])&&!isinf(hillslope[x][y])&&lakes[x][y]==noData)
			{
			hillslope[x][y]=(hillslope[x][y]-minhs)/(maxhs-minhs);
			}
		if(hillslope[x][y]<0.00001&&hillslope[x][y]>0.00001&&DtW[x][y]<.00001)
			hillslope[x][y]=.47;
		}
	}

cout<<"estimating initial groundwater depth"<<endl;	
for(int x=1;x<xSize-1;x++)
	{
	for(int y=1;y<ySize-1;y++)
		{//cout<<x<<" "<<y<<" "<<lakes[x][y]<<" "<<DtW[x][y]<<endl;
		if(lakes[x][y]==noData&&DtW[x][y]!=noData&&DtW[x][y]>0){
			G[x][y]=dem[x][y]-(1/(1+exp((-DtW[x][y]+10)/50))-0.44)*5 - 2*hillslope[x][y];if(x==1140&&y==372)cout<<(1/(1+exp((-DtW[x][y]+10)/50))-0.44)*10<<" "<<2*hillslope[x][y]<<endl;}
		else if(DtW[x][y]==noData)
			G[x][y]=dem[x][y];
		
		//if(lakes[x][y]==noData&&DtW[x][y]==noData&&dem[x][y]!=noData)
		//	G[x][y]=dem[x][y]-2;
		}
	}

cout<<"smooth G"<<endl;
for(int x=3;x<xSize-4;x++)
	{
	for(int y=3;y<ySize-4;y++)
		{
		if(lakes[x][y]==noData)
			{
			float locSum=0;
			float locCount=0;
			for(int X=-2;X<3;X++)
				{
				for(int Y=-2;Y<3;Y++)
					{
					if(lakes[x+X][y+Y]==noData)
						{
						locSum+=G[x+X][y+Y];
						locCount+=1;
						}
					}
				}
			if(locCount>0)
				finalG[x][y]=locSum/locCount;
//			cout<<x<<":"<<y<<" "<<locCount<<endl;
			}//end if lakes==noData
		}
	}

string tmpOutFName = folder + "/groundwaterLevel_leveled.tif";
const char* outFName = tmpOutFName.c_str();//"io/erosion_.tif";
GDALDataset *output;
GDALDriver *driverGeotiff;
float *rowBuff = (float*) CPLMalloc(sizeof(float)*xSize);
driverGeotiff = GetGDALDriverManager()->GetDriverByName("GTiff");
output = driverGeotiff->Create(outFName,xSize,ySize,1,GDT_Float32,NULL);
output->SetGeoTransform(adfGeoTransform);
output->SetProjection(INDEM->GetProjectionRef());
cout<<"still ok"<<endl;

for(int y=0;y<ySize;y++)
	{
	for(int x=0;x<xSize;x++)
		{
		if(dem[x][y]!=noData)
			rowBuff[x] = (float)finalG[x][y];
		else
			rowBuff[x] = noData;
		}
	output->GetRasterBand(1)->RasterIO( GF_Write, 0, y, xSize, 1, rowBuff, xSize, 1, GDT_Float32, 0, 0 );
	}
GDALClose( (GDALDatasetH) output );

tmpOutFName = folder + "/groundwaterLevel.tif";
outFName = tmpOutFName.c_str();
driverGeotiff = GetGDALDriverManager()->GetDriverByName("GTiff");
output = driverGeotiff->Create(outFName,xSize,ySize,1,GDT_Float32,NULL);
output->SetGeoTransform(adfGeoTransform);
output->SetProjection(INDEM->GetProjectionRef());

for(int y=0;y<ySize;y++)
	{
	for(int x=0;x<xSize;x++)
		{
		if(dem[x][y]>0)
			rowBuff[x] = (float)G[x][y];
		else
			rowBuff[x] = noData;
		}
	output->GetRasterBand(1)->RasterIO( GF_Write, 0, y, xSize, 1, rowBuff, xSize, 1, GDT_Float32, 0, 0 );
	}
GDALClose( (GDALDatasetH) output );

return 0;
}
