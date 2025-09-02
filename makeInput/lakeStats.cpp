#include <stdlib.h>
#include <iostream>
#include <vector>
#include<algorithm>
#include<fstream>
#include<iomanip>
#include "/usr/include/gdal/gdal_priv.h"
#include "/usr/include/gdal/cpl_conv.h"
//g++ -std=c++11 lakes.cpp -lgdal -o lakes
using namespace std;
int main(int argc, char* argv[]) 
{
string folder=argv[1];
ifstream fin;
string filename_LAKES=folder + "/lakes.tif";
string filename_VANN=folder + "/vann.tif";
string filename_DTM=folder + "/dtm10_filled.tif";
const char* inDTM = filename_DTM.c_str();
const char* inLAKES = filename_LAKES.c_str();
const char* inVANN = filename_VANN.c_str();
//Open the input DEM
GDALDataset *INDEM;
GDALDataset *INLAKES;
GDALDataset *INVANN;
GDALAllRegister();

INDEM = (GDALDataset*) GDALOpen(inDTM,GA_ReadOnly);
INLAKES = (GDALDataset*) GDALOpen(inLAKES,GA_ReadOnly);
INVANN = (GDALDataset*) GDALOpen(inVANN,GA_ReadOnly);

float noData = INDEM->GetRasterBand(1)->GetNoDataValue();
double adfGeoTransform[6];

INDEM->GetGeoTransform( adfGeoTransform );
INLAKES->GetGeoTransform( adfGeoTransform );
INVANN->GetGeoTransform( adfGeoTransform );

double dx = adfGeoTransform[1];
double dy = fabs(adfGeoTransform[5]);
double originX = adfGeoTransform[0];
double originY = adfGeoTransform[3];
int xSize = INDEM->GetRasterXSize();
int ySize = INDEM->GetRasterYSize();

GDALRasterBand  *poBandINDEM;
GDALRasterBand  *poBandINLAKES;
GDALRasterBand  *poBandINVANN;
poBandINDEM = INDEM->GetRasterBand( 1 );
poBandINLAKES = INLAKES->GetRasterBand( 1 );
poBandINVANN = INVANN->GetRasterBand( 1 );

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
vector<vector<float>> lakes = vector<vector<float>>(xSize,vector<float> (ySize,0));
vector<vector<float>> water = vector<vector<float>>(xSize,vector<float> (ySize,0));

for(int x=0;x<xSize;x++)
	{
	for(int y=0;y<ySize;y++)
		{
		dem[x][y]=noData;
		lakes[x][y]=noData;
		water[x][y]=noData;		
		}
	}

//assign elevation data to dem
poBandINDEM->RasterIO(GF_Read,0,0,xSize,ySize,pafScanline,xSize,ySize,GDT_Float32,0,0);
cout<<"poBandINDEM read"<<endl;
for(int y = 0; y < ySize; y++)
	{
	for(int x = 0; x < xSize; x++)
		{
		dem[x][y] = pafScanline[y*xSize+x];
		}
	}
cout<<"poBandINDEM assigned to dem[x][y]"<<endl;

//assign water data to water
poBandINVANN->RasterIO(GF_Read,0,0,xSize,ySize,pafScanline,xSize,ySize,GDT_Float32,0,0);
cout<<"poBandINVANN read"<<endl;
for(int y = 0; y < ySize; y++)
	{
	for(int x = 0; x < xSize; x++)
		{
		if(pafScanline[y*xSize+x]>0)
			water[x][y] = 1;
		}
	}

CPLFree(pafScanline);
cout<<"poBandINVANN assigned to water[x][y]"<<endl;

int nrLakes=0;
vector<int> lakeID;
pafScanline = (float *) CPLMalloc(sizeof(float)*xSize*ySize);
poBandINLAKES->RasterIO(GF_Read,0,0,xSize,ySize,pafScanline,xSize,ySize,GDT_Float32,0,0);
cout<<"poBandINLAKES read"<<endl;
for(int y = 0; y < ySize; y++)
	{
	for(int x = 0; x < xSize; x++)
		{
		lakes[x][y] = pafScanline[y*xSize+x];
		if(lakes[x][y]>0)
			{
			if(lakeID.size()==0)
				{
				lakeID.push_back(lakes[x][y]);
				}
			else if(lakeID.size()==1&&lakes[x][y]!=lakeID[0])
				{
				lakeID.push_back (lakes[x][y]);
				}
			else if(lakeID.size()==1&&lakes[x][y]==lakeID[0])
				{
				int A=0;
				}
			else
				{
				bool notListed=true;
				for(int index=0;index<lakeID.size();index++)
					{
					if(lakeID[index]==lakes[x][y])
						notListed=false;
					}
				if(notListed==true)
					{
					lakeID.push_back (lakes[x][y]);
					}
				}
			}
		}
	}
CPLFree(pafScanline);
cout<<"poBandTEXT assigned to lake[x][y]"<<endl;
sort(lakeID.begin(),lakeID.end());
nrLakes=lakeID.size();
cout<<nrLakes<<" lakes in data set"<<endl;

for(int index=0;index<nrLakes;index++)
		{
		cout<<lakeID[index]<<endl;
		}
/*		
for(int y = 0; y < ySize; y++)
	{
	for(int x = 0; x < xSize; x++)
		{
		if(lakes[x][y]>0)
			{
			for(int index=0;index<nrLakes+1;index++)
				{
				if(lakeID[index]==lakes[x][y])
					lakes[x][y]=index+1;
				}
			}
		}
	}
cout<<"lake id's converted"<<endl;*/

float lakeTable[nrLakes+1][6];//0 Area, 1 minz, 2/4 x_minz, 3/6 y_minz
//FIRST ITERATION TO IDENTIFY OUTFLOW TO LAKE OR RIVER
for(int index=0;index<nrLakes+1;index++)
	{
	lakeTable[index][0]=0;
	lakeTable[index][1]=9999;//minDEM
	lakeTable[index][2]=0;//minDEM, x projected
	lakeTable[index][3]=0;//minDEM, y projected
	lakeTable[index][4]=0;//minDEM, x index
	lakeTable[index][5]=0;//minDEM, y index
	}
for(int x=1;x<xSize-1;x++)
	{
	for(int y=1;y<ySize-1;y++)
		{
		if(lakes[x][y]>0)
			{
			int locLake=lakes[x][y];
			int locIndex=0;
			for(int index=0;index<lakeID.size();index++)
				{
				if(lakeID[index]==locLake)
					{
					locIndex=index;
					}
				}
			lakeTable[locIndex][0]+=(dx*dy);
			for(int X=-1;X<2;X++)
				{
				for(int Y=-1;Y<2;Y++)
					{
//					if(lakes[x+X][y+Y]==noData)
					if(lakes[x+X][y+Y]!=locLake&&water[x+X][y+Y]!=noData)
						{
						if(dem[x+X][y+Y]<lakeTable[locIndex][1])
							{
							lakeTable[locIndex][1]=dem[x+X][y+Y];
							lakeTable[locIndex][2]=originX+(x+X)*dx;
							lakeTable[locIndex][3]=originY-(y+Y)*dy;
							lakeTable[locIndex][4]=x+X;
							lakeTable[locIndex][5]=y+Y;
							}
						}
					}
				}
			}
		}
	}
//SECOND ITERATION TO DETERMINE OUTFLOW TO OVERLAND FLOW	
for(int x=1;x<xSize-1;x++)
	{
	for(int y=1;y<ySize-1;y++)
		{
		if(lakes[x][y]>0)
			{
			int locLake=lakes[x][y];
			int locIndex=0;
			for(int index=0;index<lakeID.size();index++)
				{
				if(lakeID[index]==locLake)
					{
					locIndex=index;
					}
				}
			if(lakeTable[locIndex][1]==9999)
				{
				for(int X=-1;X<2;X++)
					{
					for(int Y=-1;Y<2;Y++)
						{
	//					if(lakes[x+X][y+Y]==noData)
						if(lakes[x+X][y+Y]!=locLake&&water[x+X][y+Y]==noData)
							{
							if(dem[x+X][y+Y]<lakeTable[locIndex][1])
								{
								lakeTable[locIndex][1]=dem[x+X][y+Y];
								lakeTable[locIndex][2]=originX+(x+X)*dx;
								lakeTable[locIndex][3]=originY-(y+Y)*dy;
								lakeTable[locIndex][4]=x+X;
								lakeTable[locIndex][5]=y+Y;
								}
							}
						}
					}
				}
			}
		}
	}
/*
for(int index=0;index<nrLakes;index++)
	if(lakeTable[index][0]>0)
		cout<<index+1<<" area="<<lakeTable[index][0]<<" lowest elevation of "<<lakeTable[index][1]<<" at "<<setprecision(10)<<lakeTable[index][2]<<":"<<setprecision(10)<<lakeTable[index][3]<<endl;
*/
ofstream oFile;
oFile.open(folder + "/lakeStats.csv");
oFile<<"index area z x y xindex yindex"<<endl;
for(int index=0;index<nrLakes;index++)
	if(lakeTable[index][0]>0)
		oFile<<index+1<<" "<<lakeTable[index][0]<<" "<<lakeTable[index][1]<<" "<<setprecision(10)<<lakeTable[index][2]<<" "<<setprecision(10)<<lakeTable[index][3]<<" "<<lakeTable[index][4]<<" "<<lakeTable[index][5]<<endl;

oFile.close();
oFile.clear();

/*SAVE FOR OUTPUT*/
string tmpOutFName = folder + "/lakes.tif";
const char* outFName = tmpOutFName.c_str();//"io/erosion_.tif";
GDALDataset *output;
GDALDriver *driverGeotiff;
float *rowBuff = (float*) CPLMalloc(sizeof(float)*xSize);
driverGeotiff = GetGDALDriverManager()->GetDriverByName("GTiff");
output = driverGeotiff->Create(outFName,xSize,ySize,1,GDT_Float32,NULL);
output->SetGeoTransform(adfGeoTransform);
output->SetProjection(INDEM->GetProjectionRef());
output->GetRasterBand(1)->SetNoDataValue(static_cast<double>(noData));
//output->SetNoDataValue(INDEM->GetRasterBand(1)->GetNoDataValue());
for(int y=0;y<ySize;y++)
	{
	for(int x=0;x<xSize;x++)
		{
		if(dem[x][y]!=noData&&lakes[x][y]!=noData)
			rowBuff[x] = (float)lakes[x][y];
		else
			rowBuff[x] = noData;
		}
	output->GetRasterBand(1)->RasterIO( GF_Write, 0, y, xSize, 1, rowBuff, xSize, 1, GDT_Float32, 0, 0 );
			//RasterIO(GF_Write,0,i,width,1,rowBuff,height,1,GDT_Float32,0,0);
	}		
GDALClose( (GDALDatasetH) output );


return 0;
}
