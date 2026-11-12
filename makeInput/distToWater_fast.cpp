#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <math.h>
#include <algorithm>
#include <vector>
#include <queue>
#include <limits>
#include <cmath>
#include "/usr/include/gdal/gdal_priv.h"
#include "/usr/include/gdal/cpl_conv.h"

using namespace std;

struct Node
{
	float dist;
	int index;
	bool operator>(const Node& other) const
	{
		return dist > other.dist;
	}
};

int main(int argc, char* argv[]) 
{
	if(argc < 2)
	{
		cout << "Usage: " << argv[0] << " <folder>" << endl;
		return 1;
	}

	string folder=argv[1];
	ifstream fin;
	string filename_RIVER=folder + "/river.tif";
	string filename_LAKES=folder + "/lakes.tif";
	string filename_DTM=folder + "/dtm10_filled.tif";
	const char* inDTM = filename_DTM.c_str();
	const char* inRIVER = filename_RIVER.c_str();
	const char* inLAKES = filename_LAKES.c_str();

	GDALDataset *INDEM;
	GDALDataset *INRIVER;
	GDALDataset *INLAKES;
	GDALAllRegister();

	INDEM = (GDALDataset*) GDALOpen(inDTM,GA_ReadOnly);
	INRIVER= (GDALDataset*) GDALOpen(inRIVER,GA_ReadOnly);
	INLAKES = (GDALDataset*) GDALOpen(inLAKES,GA_ReadOnly);

	if(INDEM == NULL || INRIVER == NULL || INLAKES == NULL)
	{
		cout << "Could not open one or more input rasters." << endl;
		return 1;
	}

	float noData = INRIVER->GetRasterBand(1)->GetNoDataValue();
	double adfGeoTransform[6];
	INRIVER->GetGeoTransform( adfGeoTransform );
	double dx = adfGeoTransform[1];
	double dy = fabs(adfGeoTransform[5]);
	int xSize = INRIVER->GetRasterXSize();
	int ySize = INRIVER->GetRasterYSize();
	int nCells = xSize * ySize;

	cout<<xSize<<":"<<ySize<<endl;

	GDALRasterBand  *poBandINDEM;
	GDALRasterBand  *poBandINRIVER;
	GDALRasterBand  *poBandINLAKES;

	poBandINDEM = INDEM->GetRasterBand( 1 );
	poBandINRIVER = INRIVER->GetRasterBand( 1 );
	poBandINLAKES = INLAKES->GetRasterBand( 1 );

	vector<float> dem(nCells, noData);
	vector<float> DtW(nCells, noData);
	vector<unsigned char> valid(nCells, 0);

	float *pafScanline = (float *) CPLMalloc(sizeof(float)*nCells);

	poBandINDEM->RasterIO(GF_Read,0,0,xSize,ySize,pafScanline,xSize,ySize,GDT_Float32,0,0);
	cout<<"poBandINDEM read"<<endl;
	for(int i = 0; i < nCells; i++)
	{
		dem[i] = pafScanline[i];
		if(dem[i] != noData)
			valid[i] = 1;
	}
	cout<<"poBandDEM assigned"<<endl;

	priority_queue<Node, vector<Node>, greater<Node> > pq;
	const float INF = numeric_limits<float>::max();
	for(int i = 0; i < nCells; i++)
	{
		if(valid[i])
			DtW[i] = INF;
	}

	poBandINRIVER->RasterIO(GF_Read,0,0,xSize,ySize,pafScanline,xSize,ySize,GDT_Float32,0,0);
	cout<<"poBandINRIVER read"<<endl;
	for(int i = 0; i < nCells; i++)
	{
		if(valid[i] && pafScanline[i] > 0)
		{
			DtW[i] = 0.0f;
			pq.push(Node{0.0f, i});
		}
	}
	cout<<"river cells queued"<<endl;

	poBandINLAKES->RasterIO(GF_Read,0,0,xSize,ySize,pafScanline,xSize,ySize,GDT_Float32,0,0);
	cout<<"poBandINLAKES read"<<endl;
	for(int i = 0; i < nCells; i++)
	{
		if(valid[i] && pafScanline[i] > 0 && DtW[i] > 0.0f)
		{
			DtW[i] = 0.0f;
			pq.push(Node{0.0f, i});
		}
	}
	CPLFree(pafScanline);
	cout<<"lake cells queued"<<endl;

	cout<<"Estimating distance to water using multi-source Dijkstra"<<endl;

	const int xOff[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
	const int yOff[8] = {-1,-1,-1,  0, 0,  1, 1, 1};
	const float stepCost[8] = {
		(float)sqrt(dx*dx + dy*dy), (float)dy, (float)sqrt(dx*dx + dy*dy),
		(float)dx,                         (float)dx,
		(float)sqrt(dx*dx + dy*dy), (float)dy, (float)sqrt(dx*dx + dy*dy)
	};

	long long processed = 0;
	while(!pq.empty())
	{
		Node cur = pq.top();
		pq.pop();

		if(cur.dist != DtW[cur.index])
			continue;

		int x = cur.index % xSize;
		int y = cur.index / xSize;

		for(int k = 0; k < 8; k++)
		{
			int nx = x + xOff[k];
			int ny = y + yOff[k];

			if(nx < 0 || nx >= xSize || ny < 0 || ny >= ySize)
				continue;

			int ni = ny * xSize + nx;
			if(!valid[ni])
				continue;

			float nd = cur.dist + stepCost[k];
			if(nd < DtW[ni])
			{
				DtW[ni] = nd;
				pq.push(Node{nd, ni});
			}
		}

		processed++;
		if(processed % 1000000 == 0)
			cout << processed << " cells processed" << endl;
	}

	string tmpOutFName = folder +"/DtW.tif";
	const char* outFName = tmpOutFName.c_str();
	GDALDataset *output;
	GDALDriver *driverGeotiff;
	float *rowBuff = (float*) CPLMalloc(sizeof(float)*xSize);
	driverGeotiff = GetGDALDriverManager()->GetDriverByName("GTiff");
	output = driverGeotiff->Create(outFName,xSize,ySize,1,GDT_Float32,NULL);
	output->SetGeoTransform(adfGeoTransform);
	output->SetProjection(INDEM->GetProjectionRef());
	output->GetRasterBand(1)->SetNoDataValue(static_cast<double>(noData));
	cout<<"writing DtW.tif"<<endl;

	for(int y=0;y<ySize;y++)
	{
		for(int x=0;x<xSize;x++)
		{
			int i = y * xSize + x;
			if(valid[i] && DtW[i] != INF)
				rowBuff[x] = DtW[i];
			else
				rowBuff[x] = noData;
		}
		output->GetRasterBand(1)->RasterIO( GF_Write, 0, y, xSize, 1, rowBuff, xSize, 1, GDT_Float32, 0, 0 );
	}

	CPLFree(rowBuff);
	GDALClose( (GDALDatasetH) output );
	GDALClose( (GDALDatasetH) INDEM );
	GDALClose( (GDALDatasetH) INRIVER );
	GDALClose( (GDALDatasetH) INLAKES );

	return 0;
}
