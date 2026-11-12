#include <stdlib.h>
#include <iostream>
#include <vector>
#include<algorithm>
#include<fstream>
#include<iomanip>
#include "/usr/include/gdal/gdal_priv.h"
#include "/usr/include/gdal/cpl_conv.h"
#include "/usr/include/gdal/ogrsf_frmts.h"
//g++ -std=c++11 damStats_02_flacc.cpp -lgdal -o damStats
using namespace std;
int main(int argc, char* argv[]) 
{
string folder=argv[1];
ifstream fin;
string filename_DAMS =			folder + argv[2];//"/measures/sce01/selection01.tif";
string filename_RIVER =			folder + argv[3];//"/river.tif";
string filename_DTM =			folder + argv[4];//"/dtm10_clipped.tif";
string filename_DTMNS =			folder + argv[5];//"/dtm10_clipped_ns.tif";
string filename_DTMNSFLACC =	folder + argv[6];//"/dtm10_clipped_ns_flacc.tif";
const char* inDTM = filename_DTM.c_str();
const char* inDTMNS = filename_DTMNS.c_str();
const char* inDTMNSFLACC = filename_DTM.c_str();
const char* inDAMS = filename_DAMS.c_str();
const char* inRIVER = filename_RIVER.c_str();
//Open the input DEM
GDALDataset *INDEM;
GDALDataset *INDEMNS;
GDALDataset *INDEMNSFLACC;
GDALDataset *INDAMS;
GDALDataset *INRIVER;
GDALAllRegister();

INDEM = (GDALDataset*) GDALOpen(inDTM,GA_ReadOnly);
INDEMNS = (GDALDataset*) GDALOpen(inDTMNS,GA_ReadOnly);
INDEMNSFLACC = (GDALDataset*) GDALOpen(inDTMNSFLACC,GA_ReadOnly);
INDAMS = (GDALDataset*) GDALOpen(inDAMS,GA_ReadOnly);
INRIVER = (GDALDataset*) GDALOpen(inRIVER,GA_ReadOnly);

float noData = INDEM->GetRasterBand(1)->GetNoDataValue();
double adfGeoTransform[6];

INDEM->GetGeoTransform( adfGeoTransform );
INDEMNS->GetGeoTransform( adfGeoTransform );
INDAMS->GetGeoTransform( adfGeoTransform );
INRIVER->GetGeoTransform( adfGeoTransform );

double dx = adfGeoTransform[1];
double dy = fabs(adfGeoTransform[5]);
double originX = adfGeoTransform[0];
double originY = adfGeoTransform[3];
int xSize = INDEM->GetRasterXSize();
int ySize = INDEM->GetRasterYSize();

GDALRasterBand  *poBandINDEM;
GDALRasterBand  *poBandINDEMNS;
GDALRasterBand  *poBandINDEMNSFLACC;
GDALRasterBand  *poBandINDAMS;
GDALRasterBand  *poBandINRIVER;
poBandINDEM = INDEM->GetRasterBand( 1 );
poBandINDEMNS = INDEMNS->GetRasterBand( 1 );
poBandINDEMNSFLACC = INDEMNSFLACC->GetRasterBand( 1 );
poBandINDAMS = INDAMS->GetRasterBand( 1 );
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
vector<vector<float>> dem_ns = vector<vector<float>>(xSize,vector<float> (ySize,0));
vector<vector<float>> dem_ns_flacc = vector<vector<float>>(xSize,vector<float> (ySize,0));
vector<vector<float>> dams = vector<vector<float>>(xSize,vector<float> (ySize,0));
vector<vector<float>> river = vector<vector<float>>(xSize,vector<float> (ySize,0));

for(int x=0;x<xSize;x++)
	{
	for(int y=0;y<ySize;y++)
		{
		dem[x][y]=noData;
		dem_ns[x][y]=noData;
		dem_ns_flacc[x][y]=noData;
		dams[x][y]=noData;
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

poBandINDEMNS->RasterIO(GF_Read,0,0,xSize,ySize,pafScanline,xSize,ySize,GDT_Float32,0,0);
cout<<"poBandINDEMNS read"<<endl;
for(int y = 0; y < ySize; y++)
	{
	for(int x = 0; x < xSize; x++)
		{
		dem_ns[x][y] = pafScanline[y*xSize+x];
		}
	}
cout<<"poBandINDEMNS assigned to dem_ns[x][y]"<<endl;

poBandINDEMNSFLACC->RasterIO(GF_Read,0,0,xSize,ySize,pafScanline,xSize,ySize,GDT_Float32,0,0);
cout<<"poBandINDEMNSFLACC read"<<endl;
for(int y = 0; y < ySize; y++)
	{
	for(int x = 0; x < xSize; x++)
		{
		dem_ns_flacc[x][y] = pafScanline[y*xSize+x];
		}
	}
cout<<"poBandINDEMNSFLACC assigned to dem_ns[x][y]"<<endl;

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
cout<<"poBandINRIVER assigned to river[x][y]"<<endl;

int nrDams=0;
vector<int> damID;
pafScanline = (float *) CPLMalloc(sizeof(float)*xSize*ySize);
poBandINDAMS->RasterIO(GF_Read,0,0,xSize,ySize,pafScanline,xSize,ySize,GDT_Float32,0,0);
cout<<"poBandINDAMS read"<<endl;
for(int y = 0; y < ySize; y++)
	{
	for(int x = 0; x < xSize; x++)
		{
		dams[x][y] = pafScanline[y*xSize+x];
		if(dams[x][y]>0)
			{
			if(damID.size()==0)
				{
				damID.push_back(dams[x][y]);
				}
			else if(damID.size()==1&&dams[x][y]!=damID[0])
				{
				damID.push_back (dams[x][y]);
				}
			else if(damID.size()==1&&dams[x][y]==damID[0])
				{
				int A=0;
				}
			else
				{
				bool notListed=true;
				for(int index=0;index<damID.size();index++)
					{
					if(damID[index]==dams[x][y])
						notListed=false;
					}
				if(notListed==true)
					{
					damID.push_back (dams[x][y]);
					}
				}
			}
		}
	}
CPLFree(pafScanline);
cout<<"poBandTEXT assigned to dam[x][y]"<<endl;
sort(damID.begin(),damID.end());
nrDams=damID.size();
cout<<nrDams<<" dams in data set"<<endl;

for(int index=0;index<nrDams;index++)
		{
		cout<<damID[index]<<endl;
		}

float damTable[nrDams+1][7];//0 area, 1 outlet z, 2 outlet x, 3 outlet y, 4 x index, 5 y index, 6 volume
//FIRST ITERATION TO IDENTIFY OUTFLOW TO LAKE OR RIVER
for(int index=0;index<nrDams+1;index++)
	{
	damTable[index][0]=0;//area
	damTable[index][1]=0;//z at outlet
	damTable[index][2]=0;//outlet x projected
	damTable[index][3]=0;//outlet y projected
	damTable[index][4]=-1;//outlet x index; -1 means no valid outlet
	damTable[index][5]=-1;//outlet y index; -1 means no valid outlet
	damTable[index][6]=0;//dam volume in m3
	}

//For every lake, search cells just outside its outer boundary.
//The selected outlet is the adjacent outside-boundary cell with highest flacc.
//A candidate may be river, overland, or another lake, but not the same lake.
int dX[8] = {-1, 0, 1,-1, 1,-1, 0, 1};
int dY[8] = {-1,-1,-1, 0, 0, 1, 1, 1};
for(int index=0;index<nrDams;index++)
	{
	int currentdamID = damID[index];
	float maxFlacc = -1;
	int outletX = -1;
	int outletY = -1;
	
	for(int y = 0; y < ySize; y++)
		{
		for(int x = 0; x < xSize; x++)
			{
			if(dams[x][y]==currentdamID)
				{
				damTable[index][0] += dx*dy;
				damTable[index][6] = (dem_ns[x][y] - dem[x][y]) * dx*dy;
				for(int n=0;n<8;n++)
					{
					int nx = x + dX[n];
					int ny = y + dY[n];
					
					if(nx>=0 && nx<xSize && ny>=0 && ny<ySize)
						{
						//Outside the current dam polygon. This includes river cells,
						//unclassified overland cells, and cells belonging to another dam.
						if(dams[nx][ny]!=currentdamID && dem[nx][ny]!=noData && dem_ns_flacc[nx][ny]!=noData)
							{
							if(dem_ns_flacc[nx][ny]>maxFlacc)
								{
								maxFlacc = dem_ns_flacc[nx][ny];
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
		damTable[index][1] = dem[outletX][outletY]; ;
		damTable[index][2] = originX + (outletX + 0.5)*dx;
		damTable[index][3] = originY - (outletY + 0.5)*dy;
		damTable[index][4] = outletX;
		damTable[index][5] = outletY;
		}
	}

ofstream oFile;
oFile.open(folder + "/damStats.csv");
oFile<<"index area z x y xindex yindex residenceTime releaseFraction"<<endl;// 0 area, 1 z, 2 z, 3 x, 4 y, 5 xindex, 6 yindex
for(int index=0;index<nrDams;index++)
	if(damTable[index][0]>0)
		oFile<<index<<" "<<damTable[index][0]<<" "<<damTable[index][1]<<" "<<setprecision(10)<<damTable[index][2]<<" "<<setprecision(10)<<damTable[index][3]<<" "<<damTable[index][4]<<" "<<damTable[index][5]<<" "<<damTable[index][6]<<endl;

oFile.close();
oFile.clear();


// -----------------------------------------------------------------------------
// Export valid lake outlet locations to a GeoPackage point layer.
// Geometry uses the same coordinate reference system as the input DEM.
// -----------------------------------------------------------------------------
string outletGpkgName = folder + "/damOutlets.gpkg";
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
	"dam_outlets", outletSRSPtr, wkbPoint, NULL
);
if(outletLayer == NULL)
	{
	cerr << "Could not create dam_outlets layer." << endl;
	GDALClose((GDALDatasetH) outletDataset);
	return 1;
	}

OGRFieldDefn fieldIndex("dam_index", OFTInteger);
OGRFieldDefn fieldOriginalID("source_id", OFTInteger);
OGRFieldDefn fieldArea("area_m2", OFTReal);
OGRFieldDefn fieldElevation("outlet_z", OFTReal);
OGRFieldDefn fieldXIndex("xindex", OFTInteger);
OGRFieldDefn fieldYIndex("yindex", OFTInteger);
OGRFieldDefn fieldVolume("volume_m3", OFTReal);

if(outletLayer->CreateField(&fieldIndex) != OGRERR_NONE ||
   outletLayer->CreateField(&fieldOriginalID) != OGRERR_NONE ||
   outletLayer->CreateField(&fieldArea) != OGRERR_NONE ||
   outletLayer->CreateField(&fieldElevation) != OGRERR_NONE ||
   outletLayer->CreateField(&fieldXIndex) != OGRERR_NONE ||
   outletLayer->CreateField(&fieldYIndex) != OGRERR_NONE ||
   outletLayer->CreateField(&fieldVolume) != OGRERR_NONE)
	{
	cerr << "Could not create one or more outlet attribute fields." << endl;
	GDALClose((GDALDatasetH) outletDataset);
	return 1;
	}

int exportedOutletCount = 0;
for(int index=0; index<nrDams; index++)
	{
	const int outletXIndex = static_cast<int>(damTable[index][4]);
	const int outletYIndex = static_cast<int>(damTable[index][5]);

	if(damTable[index][0] <= 0 ||
	   outletXIndex < 0 || outletYIndex < 0)
		continue;

	OGRFeature *feature = OGRFeature::CreateFeature(outletLayer->GetLayerDefn());
	if(feature == NULL)
		{
		cerr << "Could not allocate outlet feature for dam " << index << endl;
		GDALClose((GDALDatasetH) outletDataset);
		return 1;
		}

	feature->SetField("dam_index", index);
	feature->SetField("source_id", damID[index]);
	feature->SetField("area_m2", static_cast<double>(damTable[index][0]));
	feature->SetField("outlet_z", static_cast<double>(damTable[index][1]));
	feature->SetField("xindex", outletXIndex);
	feature->SetField("yindex", outletYIndex);
	feature->SetField("volume_m3", static_cast<double>(damTable[index][7]));

	OGRPoint outletPoint(
		static_cast<double>(damTable[index][2]),
		static_cast<double>(damTable[index][3])
	);
	feature->SetGeometry(&outletPoint);

	if(outletLayer->CreateFeature(feature) != OGRERR_NONE)
		{
		cerr << "Could not write outlet feature for dam " << index << endl;
		OGRFeature::DestroyFeature(feature);
		GDALClose((GDALDatasetH) outletDataset);
		return 1;
		}

	OGRFeature::DestroyFeature(feature);
	exportedOutletCount++;
	}

GDALClose((GDALDatasetH) outletDataset);
cout << exportedOutletCount << " dam outlet points written to "
     << outletGpkgName << endl;


// Convert the original dam IDs in tif file to the same zero-based
// indices that are written to the first column of lakeStats.csv.
for(int y = 0; y < ySize; y++)
	{
	for(int x = 0; x < xSize; x++)
		{
		if(dams[x][y]>0)
			{
			for(int index=0;index<nrDams;index++)
				{
				if(damID[index]==dams[x][y])
					{
					dams[x][y]=index;
					break;
					}
				}
			}
		}
	}
cout<<"lake id's converted to zero-based lakeStats indices"<<endl;

/*SAVE FOR OUTPUT*/
string tmpOutFName = folder + "/measures/sce01/selection01.tif";;
const char* outFName = tmpOutFName.c_str();
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
		if(dem[x][y]!=noData&&dams[x][y]!=noData)
			rowBuff[x] = (float)dams[x][y];
		else
			rowBuff[x] = noData;
		}
	output->GetRasterBand(1)->RasterIO( GF_Write, 0, y, xSize, 1, rowBuff, xSize, 1, GDT_Float32, 0, 0 );
			//RasterIO(GF_Write,0,i,width,1,rowBuff,height,1,GDT_Float32,0,0);
	}		
GDALClose( (GDALDatasetH) output );


return 0;
}
