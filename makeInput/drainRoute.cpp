#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <math.h>
#include <algorithm>
#include <vector>
#include <map>
#include <queue>
#include <utility>
#include "/usr/include/gdal/gdal_priv.h"
#include "/usr/include/gdal/cpl_conv.h"
#include "../include/read_geodata.h"

using namespace std;

// Simple struct for coordinates
struct Point { int x, y; };

int main(int argc, char* argv[])
{
    if(argc < 2){
        cerr << "Usage: " << argv[0] << " <ioFolder>" << endl;
        return 1;
    }

    string ioFolder = argv[1];
    string file_DEM = ioFolder + "/dtm10_filled.tif";
    string file_RIVER = ioFolder + "/river.tif";
    string file_LU = ioFolder + "/landuse.tif";
    string file_FIELD = ioFolder + "/property.tif";

    GDALAllRegister();
    GDALDataset *INDEM = (GDALDataset*) GDALOpen(file_DEM.c_str(), GA_ReadOnly);
    if(!INDEM){ cerr<<"Failed to open DEM"<<endl; return 1; }

    float noData = INDEM->GetRasterBand(1)->GetNoDataValue();
    double adfGeoTransform[6];
    INDEM->GetGeoTransform(adfGeoTransform);
    int xSize = INDEM->GetRasterXSize();
    int ySize = INDEM->GetRasterYSize();

    double dx = adfGeoTransform[1];
    double dy = fabs(adfGeoTransform[5]);
    double originX = adfGeoTransform[0];
    double originY = adfGeoTransform[3];

    // --- Allocate rasters ---
    vector<vector<float>> dem(xSize, vector<float>(ySize, noData));
    vector<vector<float>> river(xSize, vector<float>(ySize, noData));
    vector<vector<unsigned int>> field(xSize, vector<unsigned int>(ySize, 0));
    vector<vector<int>> lu(xSize, vector<int>(ySize, 0));

    ReadGeoTIFFBandToArray(file_DEM.c_str(), dem, xSize, ySize);
    ReadGeoTIFFBandToArray(file_RIVER.c_str(), river, xSize, ySize);
    ReadGeoTIFFBandToArray(file_FIELD.c_str(), field, xSize, ySize);
    ReadGeoTIFFBandToArray(file_LU.c_str(), lu, xSize, ySize);

    // --- Step 1: Unique field IDs ---
    std::map<std::pair<int,unsigned int>, int> combo2id;
    int nextID = 1;

    string fieldIDFile = ioFolder + "/field_id.tif";
    GDALDriver *driver = GetGDALDriverManager()->GetDriverByName("GTiff");
    GDALDataset *fieldIDOutput = driver->Create(fieldIDFile.c_str(), xSize, ySize, 1, GDT_Float32, NULL);
    fieldIDOutput->SetGeoTransform(adfGeoTransform);
    const char* projRef = INDEM->GetProjectionRef();
    if(projRef) fieldIDOutput->SetProjection(projRef);

    float *rowBuff = (float*) CPLMalloc(sizeof(float)*xSize);

    vector<vector<int>> fieldID(xSize, vector<int>(ySize, -1));

    for(int y=0;y<ySize;y++){
        for(int x=0;x<xSize;x++){
            if(dem[x][y]!=noData && lu[x][y]!=noData && lu[x][y]>20 && lu[x][y]<30 && field[x][y]!=noData){
                auto key = make_pair(lu[x][y], field[x][y]);
                auto it = combo2id.find(key);
                if(it==combo2id.end()){
                    combo2id[key] = nextID;
                    fieldID[x][y] = nextID;
                    nextID++;
                } else {
                    fieldID[x][y] = it->second;
                }
            } else fieldID[x][y] = -1;

            rowBuff[x] = (fieldID[x][y]>0)? fieldID[x][y] : noData;
        }
        if(fieldIDOutput->GetRasterBand(1)->RasterIO(GF_Write,0,y,xSize,1,rowBuff,xSize,1,GDT_Float32,0,0)!=CE_None)
            cerr<<"Error writing fieldID row "<<y<<endl;
    }
    GDALClose((GDALDatasetH) fieldIDOutput);

    // --- Step 2: Compute flow direction (D8) ---
    // D8 offsets
    int dx8[8] = {1,1,0,-1,-1,-1,0,1};
    int dy8[8] = {0,-1,-1,-1,0,1,1,1};

    vector<vector<int>> flowDir(xSize, vector<int>(ySize, -1));

    for(int y=1;y<ySize-1;y++){
        for(int x=1;x<xSize-1;x++){
            if(dem[x][y]==noData) continue;
            float minSlope = 1e9;
            int bestDir = -1;
            for(int i=0;i<8;i++){
                int nx = x + dx8[i];
                int ny = y + dy8[i];
                if(dem[nx][ny]==noData) continue;
                float slope = (dem[x][y]-dem[nx][ny]) / sqrt((nx-x)*(nx-x)*dx*dx + (ny-y)*(ny-y)*dy*dy);
                if(slope>0 && slope<minSlope){
                    minSlope = slope;
                    bestDir = i;
                }
            }
            flowDir[x][y] = bestDir;
        }
    }

    // --- Step 3: Nearest river coordinates along drainage ---
    vector<vector<Point>> nearestRiver(xSize, vector<Point>(ySize, {-1,-1}));
    queue<Point> q;

    for(int y=0;y<ySize;y++)
        for(int x=0;x<xSize;x++)
            if(river[x][y]==1){
                nearestRiver[x][y] = {x,y};
                q.push({x,y});
            }

    // BFS propagation along flow direction (reverse)
    while(!q.empty()){
        Point p = q.front(); q.pop();
        int x=p.x, y=p.y;
        for(int i=0;i<8;i++){
            int nx = x + dx8[i];
            int ny = y + dy8[i];
            if(nx<0 || nx>=xSize || ny<0 || ny>=ySize) continue;
            if(flowDir[nx][ny]==( (i+4)%8 ) && nearestRiver[nx][ny].x==-1){ // check if neighbor flows into p
                nearestRiver[nx][ny] = nearestRiver[x][y];
                q.push({nx,ny});
            }
        }
    }

    // --- Step 4: Write nearest river coordinates ---
    string outRiverCoordFile = ioFolder + "/drainRoute.tif";
    GDALDataset *riverOutput = driver->Create(outRiverCoordFile.c_str(), xSize, ySize, 2, GDT_Float32, NULL);
    riverOutput->SetGeoTransform(adfGeoTransform);
    if(projRef) riverOutput->SetProjection(projRef);

    float *rowX = (float*) CPLMalloc(sizeof(float)*xSize);
    float *rowY = (float*) CPLMalloc(sizeof(float)*xSize);

    for(int y=0;y<ySize;y++){
        for(int x=0;x<xSize;x++){
            if(nearestRiver[x][y].x!=-1){
                rowX[x] = nearestRiver[x][y].x;
                rowY[x] = nearestRiver[x][y].y;
            } else {
                rowX[x] = noData;
                rowY[x] = noData;
            }
        }
        riverOutput->GetRasterBand(1)->RasterIO(GF_Write,0,y,xSize,1,rowX,xSize,1,GDT_Float32,0,0);
        riverOutput->GetRasterBand(2)->RasterIO(GF_Write,0,y,xSize,1,rowY,xSize,1,GDT_Float32,0,0);
    }

    CPLFree(rowBuff);
    CPLFree(rowX);
    CPLFree(rowY);
    GDALClose((GDALDatasetH) riverOutput);
    GDALClose((GDALDatasetH) INDEM);

    cout<<"Processing complete!"<<endl;
    return 0;
}
