#include <stdlib.h>
#include <iostream>
#include <vector>
#include<algorithm>
#include<fstream>
#include<iomanip>
#include "/usr/include/gdal/gdal_priv.h"
#include "/usr/include/gdal/cpl_conv.h"
//g++ -std=c++11 lakeStats.cpp -lgdal -o lakeStats
using namespace std;
int main(int argc, char* argv[]) 
{
string folder=argv[1];
ifstream fin;
string filename_LAKES=folder + "/lakes.tif";
string filename_RIVER=folder + "/river.tif";
string filename_DTM=folder + "/dtm10_filled.tif";
string filename_FLACC=folder + "/dtm10_filled.tif";
const char* inDTM = filename_DTM.c_str();
const char* inLAKES = filename_LAKES.c_str();
const char* inRIVER = filename_RIVER.c_str();
const char* inFLACC = filename_FLACC.c_str();
//Open the input DEM
GDALDataset *INDEM;
GDALDataset *INLAKES;
GDALDataset *INRIVER;
GDALDataset *INFLACC;
GDALAllRegister();

INDEM = (GDALDataset*) GDALOpen(inDTM,GA_ReadOnly);
INLAKES = (GDALDataset*) GDALOpen(inLAKES,GA_ReadOnly);
INRIVER = (GDALDataset*) GDALOpen(inRIVER,GA_ReadOnly);
INFLACC = (GDALDataset*) GDALOpen(inFLACC,GA_ReadOnly);

float noData = INDEM->GetRasterBand(1)->GetNoDataValue();
double adfGeoTransform[6];

INDEM->GetGeoTransform( adfGeoTransform );
INLAKES->GetGeoTransform( adfGeoTransform );
INRIVER->GetGeoTransform( adfGeoTransform );
INFLACC->GetGeoTransform( adfGeoTransform );

double dx = adfGeoTransform[1];
double dy = fabs(adfGeoTransform[5]);
double originX = adfGeoTransform[0];
double originY = adfGeoTransform[3];
int xSize = INDEM->GetRasterXSize();
int ySize = INDEM->GetRasterYSize();

GDALRasterBand  *poBandINDEM;
GDALRasterBand  *poBandINLAKES;
GDALRasterBand  *poBandINRIVER;
GDALRasterBand  *poBandINFLACC;

poBandINDEM = INDEM->GetRasterBand( 1 );
poBandINLAKES = INLAKES->GetRasterBand( 1 );
poBandINRIVER = INRIVER->GetRasterBand( 1 );
poBandINFLACC = INFLACC->GetRasterBand( 1 );

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
vector<vector<float>> river = vector<vector<float>>(xSize,vector<float> (ySize,0));
vector<vector<float>> flacc = vector<vector<float>>(xSize,vector<float> (ySize,0));

for(int x=0;x<xSize;x++)
	{
	for(int y=0;y<ySize;y++)
		{
		dem[x][y]=noData;
		lakes[x][y]=noData;
		river[x][y]=noData;		
		flacc[x][y]=noData;		
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
poBandINRIVER->RasterIO(GF_Read,0,0,xSize,ySize,pafScanline,xSize,ySize,GDT_Float32,0,0);
cout<<"poBandINRIVER read"<<endl;
for(int y = 0; y < ySize; y++)
	{
	for(int x = 0; x < xSize; x++)
		{
		if(pafScanline[y*xSize+x]>0)
			river[x][y] = 1;
		}
	}

CPLFree(pafScanline);
cout<<"poBandINVANN assigned to river[x][y]"<<endl;

//assign flacc data to flacc
poBandINFLACC->RasterIO(GF_Read,0,0,xSize,ySize,pafScanline,xSize,ySize,GDT_Float32,0,0);
cout<<"poBandINFLACC read"<<endl;
for(int y = 0; y < ySize; y++)
	{
	for(int x = 0; x < xSize; x++)
		{
		flacc[x][y] = pafScanline[y*xSize+x];
		}
	}

CPLFree(pafScanline);
cout<<"poBandINFLACC assigned to flacc[x][y]"<<endl;

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

for(int index=0;index<nrLakes+1;index++)
	{
	lakeTable[index][0]=0;
	lakeTable[index][1]=9999;//minDEM
	lakeTable[index][2]=0;//minDEM, x projected
	lakeTable[index][3]=0;//minDEM, y projected
	lakeTable[index][4]=0;//minDEM, x index
	lakeTable[index][5]=0;//minDEM, y index
	}
//IDENTIFY OUTFLOW TO LAKE OR RIVER
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
					if(lakes[x+X][y+Y]!=locLake&&river[x+X][y+Y]!=noData)
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
