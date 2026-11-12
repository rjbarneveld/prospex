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
ifstream fin;
string filename_RIVER=folder + "/river.tif";
string filename_LAKES=folder + "/lakes.tif";
string filename_DTM=folder + "/dtm10_filled.tif";
const char* inDTM = filename_DTM.c_str();
const char* inRIVER = filename_RIVER.c_str();
const char* inLAKES = filename_LAKES.c_str();
//Open the input DEM
GDALDataset *INDEM;
GDALDataset *INRIVER;
GDALDataset *INLAKES;
GDALAllRegister();

INDEM = (GDALDataset*) GDALOpen(inDTM,GA_ReadOnly);
INRIVER= (GDALDataset*) GDALOpen(inRIVER,GA_ReadOnly);
INLAKES = (GDALDataset*) GDALOpen(inLAKES,GA_ReadOnly);
float noData = INRIVER->GetRasterBand(1)->GetNoDataValue();
double adfGeoTransform[6];
INRIVER->GetGeoTransform( adfGeoTransform );
double dx = adfGeoTransform[1];
double dy = fabs(adfGeoTransform[5]);
double originX = adfGeoTransform[0];
double originY = adfGeoTransform[3];
int xSize = INRIVER->GetRasterXSize();
int ySize = INRIVER->GetRasterYSize();

cout<<xSize<<":"<<ySize<<endl;

GDALRasterBand  *poBandINDEM;
GDALRasterBand  *poBandINRIVER;
GDALRasterBand  *poBandINLAKES;

poBandINDEM = INDEM->GetRasterBand( 1 );
poBandINRIVER = INRIVER->GetRasterBand( 1 );
poBandINLAKES = INLAKES->GetRasterBand( 1 );
//Setup driver
const char *pszFormat = "GTiff";
GDALDriver *poDriver;
poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
char *pszSRS_WKT = NULL;

//Read into 2d array (float): INDEM
float *pafScanline;
pafScanline = (float *) CPLMalloc(sizeof(float)*xSize*ySize);

//declare layers
vector<vector<float>> river = vector<vector<float>>(xSize,vector<float> (ySize,0));
vector<vector<float>> lake = vector<vector<float>>(xSize,vector<float> (ySize,0));
vector<vector<float>> dem = vector<vector<float>>(xSize,vector<float> (ySize,0));
vector<vector<float>> DtW = vector<vector<float>>(xSize,vector<float> (ySize,0));

for(int y=0;y<(ySize-0);y++)//set values
	{
	for(int x=0;x<(xSize-0);x++)
		{
		river[x][y]=noData;
		dem[x][y]=noData;
		lake[x][y]=noData;
		DtW[x][y]=noData;
		}
	}


pafScanline = (float *) CPLMalloc(sizeof(float)*xSize*ySize);
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
cout<<"poBandDEM assigned to texture[x][y]"<<endl;

pafScanline = (float *) CPLMalloc(sizeof(float)*xSize*ySize);
poBandINRIVER->RasterIO(GF_Read,0,0,xSize,ySize,pafScanline,xSize,ySize,GDT_Float32,0,0);
cout<<"poBandINRIVER read"<<endl;
for(int y = 0; y < ySize; y++)
	{
	for(int x = 0; x < xSize; x++)
		{
		if(pafScanline[y*xSize+x]>0)
			river[x][y] = pafScanline[y*xSize+x];
		}
	}
CPLFree(pafScanline);
cout<<"poBandINRIVER assigned to river[x][y]"<<endl;

pafScanline = (float *) CPLMalloc(sizeof(float)*xSize*ySize);
poBandINLAKES->RasterIO(GF_Read,0,0,xSize,ySize,pafScanline,xSize,ySize,GDT_Float32,0,0);
cout<<"poBandINLAKES read"<<endl;
for(int y = 0; y < ySize; y++)
	{
	for(int x = 0; x < xSize; x++)
		{
		if(pafScanline[y*xSize+x]>0)
			lake[x][y] = pafScanline[y*xSize+x];
		}
	}
CPLFree(pafScanline);
cout<<"poBandINLAKES assigned to lake[x][y]"<<endl;

cout<<"Estimating initial depth to groundwater"<<endl;

float window=100;
float minDist=window;
for(int x=0;x<xSize;x++)
	{cout<<x<<" of "<<xSize<<endl;
	for(int y=0;y<ySize;y++)
		{
		if(dem[x][y]!=noData&&river[x][y]==noData&&lake[x][y]==noData)
			{
			minDist=window;
			for(int X=x-window;X<x+window;X++)
				{
				for(int Y=y-window;Y<y+window;Y++)
					{
					if(X>0&&X<xSize-1&&Y>0&&Y<ySize-1)
						{
						if(river[X][Y]!=noData||lake[X][Y]!=noData)
							{
							float dist=pow((pow((x-X),2)+pow((y-Y),2)),.5);
							if(dist<minDist)
								minDist=dist;
							}
						}
					}//Y
				}//X
			DtW[x][y]=minDist*10;
			}//if dem
		}//y
	}//x
/*
for(int x=2;x<xSize-3;x++)
	{
	for(int y=2;y<ySize-3;y++)
		{
		if(dem[x][y]!=noData&&lakes[x][y]==noData&&channel[x][y]==noData)
			{//cout<<x<<":"<<y<<endl;
			G[x][y]=0;
			float DtL=0;
			bool stopDown=false;
			int nextX=x;
			int nextY=y;
			int tmpNxtX=0;
			int tmpNxtY=0;
			while (stopDown==false)
				{
				float maxSlope=0;
				bool diagonal=true;

				for(int X=-1;X<2;X++)
					{
					for(int Y=-1;Y<2;Y++)
						{
						if(X==0&&Y==0)
							Y++;
						if(nextX+X==0||nextY+Y==0||nextX+X==xSize-1||nextY+Y==ySize-1)
							goto exitLoop;
						else
							{
							if(dem[nextX+X][nextY+Y]!=noData&&dem[nextX+X][nextY+Y]<dem[nextX][nextY])
								{
								float locSlope=(dem[nextX][nextY]-dem[X+nextX][Y+nextY])/(fabs(X)+fabs(Y));
								if(locSlope>maxSlope)
									{
									maxSlope=locSlope;
									tmpNxtX=nextX+X;
									tmpNxtY=nextY+Y;
									if(X==0||Y==0)
										diagonal=false;
									else
										diagonal=true;
									}
								}
							}
						}//end Y
					}//end X
				nextX=tmpNxtX;
				nextY=tmpNxtY;
				if(diagonal==true)
					DtL+=pow(2,.5)*dx;
				else
					DtL+=dx;
				if(nextX==0||nextY==0||lakes[nextX][nextY]!=noData||channel[nextX][nextY]!=noData||DtL>1500)
					stopDown=true;
				}//end while
				exitLoop:			G[x][y]=DtL;
			}//end if dem!=nodata
		}//end y
	}//end x
*/

string tmpOutFName = folder +"/DtW.tif";
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
			rowBuff[x] = (float)DtW[x][y];
		else
			rowBuff[x] = noData;
		}
	output->GetRasterBand(1)->RasterIO( GF_Write, 0, y, xSize, 1, rowBuff, xSize, 1, GDT_Float32, 0, 0 );
	}
GDALClose( (GDALDatasetH) output );

return 0;
}	
