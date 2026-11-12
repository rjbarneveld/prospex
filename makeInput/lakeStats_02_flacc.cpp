#include <stdlib.h>
#include <iostream>
#include <vector>
#include<algorithm>
#include<fstream>
#include<iomanip>
#include "/usr/include/gdal/gdal_priv.h"
#include "/usr/include/gdal/cpl_conv.h"
#include "/usr/include/gdal/ogrsf_frmts.h"
//g++ -std=c++11 lakeStats_02_flacc.cpp -lgdal -o lakeStats
using namespace std;
int main(int argc, char* argv[]) 
{
string folder=argv[1];
ifstream fin;
string filename_LAKES=folder + "/lakes.tif";
string filename_RIVER=folder + "/river.tif";
string filename_DTM=folder + "/dtm10_filled.tif";
const char* inDTM = filename_DTM.c_str();
const char* inLAKES = filename_LAKES.c_str();
const char* inRIVER = filename_RIVER.c_str();
//Open the input DEM
GDALDataset *INDEM;
GDALDataset *INLAKES;
GDALDataset *INRIVER;
GDALAllRegister();

INDEM = (GDALDataset*) GDALOpen(inDTM,GA_ReadOnly);
INLAKES = (GDALDataset*) GDALOpen(inLAKES,GA_ReadOnly);
INRIVER = (GDALDataset*) GDALOpen(inRIVER,GA_ReadOnly);

float noData = INDEM->GetRasterBand(1)->GetNoDataValue();
double adfGeoTransform[6];

INDEM->GetGeoTransform( adfGeoTransform );
INLAKES->GetGeoTransform( adfGeoTransform );
INRIVER->GetGeoTransform( adfGeoTransform );

double dx = adfGeoTransform[1];
double dy = fabs(adfGeoTransform[5]);
double originX = adfGeoTransform[0];
double originY = adfGeoTransform[3];
int xSize = INDEM->GetRasterXSize();
int ySize = INDEM->GetRasterYSize();

GDALRasterBand  *poBandINDEM;
GDALRasterBand  *poBandINLAKES;
GDALRasterBand  *poBandINRIVER;
poBandINDEM = INDEM->GetRasterBand( 1 );
poBandINLAKES = INLAKES->GetRasterBand( 1 );
poBandINRIVER = INRIVER->GetRasterBand( 1 );

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

for(int x=0;x<xSize;x++)
	{
	for(int y=0;y<ySize;y++)
		{
		dem[x][y]=noData;
		lakes[x][y]=noData;
		river[x][y]=noData;		
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

float lakeTable[nrLakes+1][8];//0 area, 1 outlet z, 2 outlet x, 3 outlet y, 4 x index, 5 y index, 6 residence time, 7 release fraction
//FIRST ITERATION TO IDENTIFY OUTFLOW TO LAKE OR RIVER
for(int index=0;index<nrLakes+1;index++)
	{
	lakeTable[index][0]=0;//area
	lakeTable[index][1]=0;//z at outlet
	lakeTable[index][2]=0;//outlet x projected
	lakeTable[index][3]=0;//outlet y projected
	lakeTable[index][4]=-1;//outlet x index; -1 means no valid outlet
	lakeTable[index][5]=-1;//outlet y index; -1 means no valid outlet
	lakeTable[index][6]=0;//residence time in days
	lakeTable[index][7]=1;//release fraction (-)
	}

//Read existing flow accumulation raster.
//Expected filename: <input folder>/flacc.tif
string filename_FLACC=folder + "/flacc.tif";
const char* inFLACC = filename_FLACC.c_str();
GDALDataset *INFLACC;
GDALRasterBand *poBandINFLACC;
INFLACC = (GDALDataset*) GDALOpen(inFLACC,GA_ReadOnly);
if(INFLACC==NULL)
	{
	cout<<"Could not open flow accumulation raster: "<<filename_FLACC<<endl;
	return 1;
	}
if(INFLACC->GetRasterXSize()!=xSize || INFLACC->GetRasterYSize()!=ySize)
	{
	cout<<"Flow accumulation raster size does not match DEM/lakes/river."<<endl;
	GDALClose((GDALDatasetH) INFLACC);
	return 1;
	}
poBandINFLACC = INFLACC->GetRasterBand( 1 );

float flaccNoData = poBandINFLACC->GetNoDataValue();
pafScanline = (float *) CPLMalloc(sizeof(float)*xSize*ySize);
poBandINFLACC->RasterIO(GF_Read,0,0,xSize,ySize,pafScanline,xSize,ySize,GDT_Float32,0,0);
cout<<"poBandINFLACC read"<<endl;

vector<vector<float>> flacc = vector<vector<float>>(xSize,vector<float> (ySize,flaccNoData));
for(int y = 0; y < ySize; y++)
	{
	for(int x = 0; x < xSize; x++)
		{
		flacc[x][y] = pafScanline[y*xSize+x];
		}
	}
CPLFree(pafScanline);
GDALClose((GDALDatasetH) INFLACC);
cout<<"poBandINFLACC assigned to flacc[x][y]"<<endl;

//For every lake, search cells just outside its outer boundary.
//The selected outlet is the adjacent outside-boundary cell with highest flacc.
//A candidate may be river, overland, or another lake, but not the same lake.
int dX[8] = {-1, 0, 1,-1, 1,-1, 0, 1};
int dY[8] = {-1,-1,-1, 0, 0, 1, 1, 1};
for(int index=0;index<nrLakes;index++)
	{
	int currentLakeID = lakeID[index];
	float maxFlacc = -1;
	int outletX = -1;
	int outletY = -1;
	
	for(int y = 0; y < ySize; y++)
		{
		for(int x = 0; x < xSize; x++)
			{
			if(lakes[x][y]==currentLakeID)
				{
				lakeTable[index][0] = lakeTable[index][0] + dx*dy;
				
				for(int n=0;n<8;n++)
					{
					int nx = x + dX[n];
					int ny = y + dY[n];
					
					if(nx>=0 && nx<xSize && ny>=0 && ny<ySize)
						{
						//Outside the current lake polygon. This includes river cells,
						//unclassified overland cells, and cells belonging to another lake.
						if(lakes[nx][ny]!=currentLakeID && dem[nx][ny]!=noData && flacc[nx][ny]!=flaccNoData)
							{
							if(flacc[nx][ny]>maxFlacc)
								{
								maxFlacc = flacc[nx][ny];
								outletX = nx;
								outletY = ny;
								}
							}
						}
					}
				}
			}
		}
	
	if(outletX>=0 && outletY>=0)
		{
		lakeTable[index][1] = dem[outletX][outletY]; ;
		lakeTable[index][2] = originX + (outletX + 0.5)*dx;
		lakeTable[index][3] = originY - (outletY + 0.5)*dy;
		lakeTable[index][4] = outletX;
		lakeTable[index][5] = outletY;
		}
	}

//Additional estimate of residence time
for(int index=0;index<nrLakes;index++)
	{
	float lakeAreaHa = max(0.01f, lakeTable[index][0] / 10000.0f);
	float lakeResidenceTimeDays = pow(lakeAreaHa, 0.18f);
	lakeResidenceTimeDays = std::max(0.50f,min(3.00f, lakeResidenceTimeDays));
	float lakeReleaseFraction = 1.0f - exp(-1.0f / lakeResidenceTimeDays);
	lakeTable[index][6]=lakeResidenceTimeDays;
	lakeTable[index][7]=lakeReleaseFraction;
	}

ofstream oFile;
oFile.open(folder + "/lakeStats.csv");
oFile<<"index area z x y xindex yindex residenceTime releaseFraction"<<endl;// 0 area, 1 z, 2 z, 3 x, 4 y, 5 xindex, 6 yindex, 7 residence time, 8 daily release fraction
for(int index=0;index<nrLakes;index++)
	if(lakeTable[index][0]>0)
		oFile<<index<<" "<<lakeTable[index][0]<<" "<<lakeTable[index][1]<<" "<<setprecision(10)<<lakeTable[index][2]<<" "<<setprecision(10)<<lakeTable[index][3]<<" "<<lakeTable[index][4]<<" "<<lakeTable[index][5]<<" "<<lakeTable[index][6]<<" "<<lakeTable[index][7]<<endl;

oFile.close();
oFile.clear();


// -----------------------------------------------------------------------------
// Export valid lake outlet locations to a GeoPackage point layer.
// Geometry uses the same coordinate reference system as the input DEM.
// -----------------------------------------------------------------------------
string outletGpkgName = folder + "/lakeOutlets.gpkg";
GDALDriver *gpkgDriver = GetGDALDriverManager()->GetDriverByName("GPKG");
if(gpkgDriver == NULL)
	{
	cerr << "GPKG driver is not available." << endl;
	return 1;
	}

// Replace an existing output rather than appending duplicate features.
VSIStatBufL outletStat;
if(VSIStatL(outletGpkgName.c_str(), &outletStat) == 0)
	{
	if(gpkgDriver->Delete(outletGpkgName.c_str()) != CE_None)
		{
		cerr << "Could not replace existing GeoPackage: " << outletGpkgName << endl;
		return 1;
		}
	}

GDALDataset *outletDataset = gpkgDriver->Create(
	outletGpkgName.c_str(), 0, 0, 0, GDT_Unknown, NULL
);
if(outletDataset == NULL)
	{
	cerr << "Could not create GeoPackage: " << outletGpkgName << endl;
	return 1;
	}

OGRSpatialReference outletSRS;
const char *demProjection = INDEM->GetProjectionRef();
OGRSpatialReference *outletSRSPtr = NULL;
if(demProjection != NULL && demProjection[0] != '\0')
	{
	if(outletSRS.SetFromUserInput(demProjection) == OGRERR_NONE)
		{
#if GDAL_VERSION_NUM >= 3000000
		outletSRS.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
#endif
		outletSRSPtr = &outletSRS;
		}
	else
		{
		cerr << "Warning: could not parse DEM projection; outlet layer will have no CRS." << endl;
		}
	}

OGRLayer *outletLayer = outletDataset->CreateLayer(
	"lake_outlets", outletSRSPtr, wkbPoint, NULL
);
if(outletLayer == NULL)
	{
	cerr << "Could not create lake_outlets layer." << endl;
	GDALClose((GDALDatasetH) outletDataset);
	return 1;
	}

OGRFieldDefn fieldIndex("lake_index", OFTInteger);
OGRFieldDefn fieldOriginalID("source_id", OFTInteger);
OGRFieldDefn fieldArea("area_m2", OFTReal);
OGRFieldDefn fieldElevation("outlet_z", OFTReal);
OGRFieldDefn fieldXIndex("xindex", OFTInteger);
OGRFieldDefn fieldYIndex("yindex", OFTInteger);
OGRFieldDefn fieldResidence("res_time_d", OFTReal);
OGRFieldDefn fieldRelease("rel_frac", OFTReal);

if(outletLayer->CreateField(&fieldIndex) != OGRERR_NONE ||
   outletLayer->CreateField(&fieldOriginalID) != OGRERR_NONE ||
   outletLayer->CreateField(&fieldArea) != OGRERR_NONE ||
   outletLayer->CreateField(&fieldElevation) != OGRERR_NONE ||
   outletLayer->CreateField(&fieldXIndex) != OGRERR_NONE ||
   outletLayer->CreateField(&fieldYIndex) != OGRERR_NONE ||
   outletLayer->CreateField(&fieldResidence) != OGRERR_NONE ||
   outletLayer->CreateField(&fieldRelease) != OGRERR_NONE)
	{
	cerr << "Could not create one or more outlet attribute fields." << endl;
	GDALClose((GDALDatasetH) outletDataset);
	return 1;
	}

int exportedOutletCount = 0;
for(int index=0; index<nrLakes; index++)
	{
	const int outletXIndex = static_cast<int>(lakeTable[index][4]);
	const int outletYIndex = static_cast<int>(lakeTable[index][5]);

	if(lakeTable[index][0] <= 0 ||
	   outletXIndex < 0 || outletYIndex < 0)
		continue;

	OGRFeature *feature = OGRFeature::CreateFeature(outletLayer->GetLayerDefn());
	if(feature == NULL)
		{
		cerr << "Could not allocate outlet feature for lake " << index << endl;
		GDALClose((GDALDatasetH) outletDataset);
		return 1;
		}

	feature->SetField("lake_index", index);
	feature->SetField("source_id", lakeID[index]);
	feature->SetField("area_m2", static_cast<double>(lakeTable[index][0]));
	feature->SetField("outlet_z", static_cast<double>(lakeTable[index][1]));
	feature->SetField("xindex", outletXIndex);
	feature->SetField("yindex", outletYIndex);
	feature->SetField("res_time_d", static_cast<double>(lakeTable[index][6]));
	feature->SetField("rel_frac", static_cast<double>(lakeTable[index][7]));

	OGRPoint outletPoint(
		static_cast<double>(lakeTable[index][2]),
		static_cast<double>(lakeTable[index][3])
	);
	feature->SetGeometry(&outletPoint);

	if(outletLayer->CreateFeature(feature) != OGRERR_NONE)
		{
		cerr << "Could not write outlet feature for lake " << index << endl;
		OGRFeature::DestroyFeature(feature);
		GDALClose((GDALDatasetH) outletDataset);
		return 1;
		}

	OGRFeature::DestroyFeature(feature);
	exportedOutletCount++;
	}

GDALClose((GDALDatasetH) outletDataset);
cout << exportedOutletCount << " lake outlet points written to "
     << outletGpkgName << endl;


// Convert the original lake IDs in lakes.tif to the same zero-based
// indices that are written to the first column of lakeStats.csv.
for(int y = 0; y < ySize; y++)
	{
	for(int x = 0; x < xSize; x++)
		{
		if(lakes[x][y]>0)
			{
			for(int index=0;index<nrLakes;index++)
				{
				if(lakeID[index]==lakes[x][y])
					{
					lakes[x][y]=index;
					break;
					}
				}
			}
		}
	}
cout<<"lake id's converted to zero-based lakeStats indices"<<endl;

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
