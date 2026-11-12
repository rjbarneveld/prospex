#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <limits>
#include "/usr/include/gdal/gdal_priv.h"
#include "/usr/include/gdal/cpl_conv.h"

using namespace std;

struct Cell {
    int x;
    int y;
    float z;
};

int main(int argc, char* argv[])
{
    if(argc < 2) {
        cerr << "Usage: " << argv[0] << " <folder>" << endl;
        return 1;
    }

    string folder = argv[1];
    string filename_RIVER = folder + "/river.tif";
    string filename_LAKES = folder + "/lakes.tif";
    string filename_DTM   = folder + "/dtm10_filled.tif";

    const char* inDTM   = filename_DTM.c_str();
    const char* inRIVER = filename_RIVER.c_str();
    const char* inLAKES = filename_LAKES.c_str();

    GDALAllRegister();

    GDALDataset *INDEM   = (GDALDataset*) GDALOpen(inDTM,   GA_ReadOnly);
    GDALDataset *INRIVER = (GDALDataset*) GDALOpen(inRIVER, GA_ReadOnly);
    GDALDataset *INLAKES = (GDALDataset*) GDALOpen(inLAKES, GA_ReadOnly);

    if(!INDEM || !INRIVER || !INLAKES) {
        cerr << "Could not open one or more input rasters." << endl;
        return 1;
    }

    GDALRasterBand *poBandINDEM   = INDEM->GetRasterBand(1);
    GDALRasterBand *poBandINRIVER = INRIVER->GetRasterBand(1);
    GDALRasterBand *poBandINLAKES = INLAKES->GetRasterBand(1);

    int hasNoData = 0;
    float noData = static_cast<float>(poBandINRIVER->GetNoDataValue(&hasNoData));
    if(!hasNoData) noData = -9999.0f;

    double adfGeoTransform[6];
    INRIVER->GetGeoTransform(adfGeoTransform);
    double dx = adfGeoTransform[1];
    double dy = fabs(adfGeoTransform[5]);

    int xSize = INRIVER->GetRasterXSize();
    int ySize = INRIVER->GetRasterYSize();
    const int n = xSize * ySize;

    cout << xSize << ":" << ySize << endl;

    vector<float> dem(n, noData);
    vector<unsigned char> isWater(n, 0);
    vector<float> DtW(n, noData);
    vector<Cell> topoOrder;
    topoOrder.reserve(n);

    float *pafScanline = (float *) CPLMalloc(sizeof(float) * n);

    // ----------------------------------------------------------
    // Read DEM and build topological order candidate list
    // ----------------------------------------------------------
    poBandINDEM->RasterIO(GF_Read, 0, 0, xSize, ySize,
                          pafScanline, xSize, ySize, GDT_Float32, 0, 0);
    cout << "poBandINDEM read" << endl;

    for(int y = 0; y < ySize; y++) {
        for(int x = 0; x < xSize; x++) {
            int i = y * xSize + x;
            dem[i] = pafScanline[i];
            if(dem[i] != noData) {
                topoOrder.push_back({x, y, dem[i]});
            }
        }
    }
    cout << "poBandDEM assigned" << endl;

    // ----------------------------------------------------------
    // Read river cells as target water cells
    // ----------------------------------------------------------
    poBandINRIVER->RasterIO(GF_Read, 0, 0, xSize, ySize,
                            pafScanline, xSize, ySize, GDT_Float32, 0, 0);
    cout << "poBandINRIVER read" << endl;

    for(int i = 0; i < n; i++) {
        if(pafScanline[i] > 0.0f) {
            isWater[i] = 1;
            DtW[i] = 0.0f;
        }
    }
    cout << "poBandINRIVER assigned" << endl;

    // ----------------------------------------------------------
    // Read lake cells as target water cells
    // ----------------------------------------------------------
    poBandINLAKES->RasterIO(GF_Read, 0, 0, xSize, ySize,
                            pafScanline, xSize, ySize, GDT_Float32, 0, 0);
    cout << "poBandINLAKES read" << endl;

    for(int i = 0; i < n; i++) {
        if(pafScanline[i] > 0.0f) {
            isWater[i] = 1;
            DtW[i] = 0.0f;
        }
    }
    CPLFree(pafScanline);
    cout << "poBandINLAKES assigned" << endl;

    // ----------------------------------------------------------
    // DAG / topological order
    // Process from low to high, so every cell's downstream target
    // has already received a distance.
    // ----------------------------------------------------------
    cout << "Sorting cells by elevation" << endl;
    sort(topoOrder.begin(), topoOrder.end(), [](const Cell& a, const Cell& b) {
        return a.z < b.z;
    });

    cout << "Estimating distance to water using DEM DAG" << endl;
    const float INF = numeric_limits<float>::max() / 4.0f;
    const float maxFallback = 100.0f * static_cast<float>(dx); // keeps old programme's 100-cell cap behaviour

    for(const Cell& c : topoOrder) {
        int x = c.x;
        int y = c.y;
        int i = y * xSize + x;

        if(dem[i] == noData) continue;
        if(isWater[i]) {
            DtW[i] = 0.0f;
            continue;
        }

        float bestDist = INF;
        float bestDropPerDist = -INF;

        // First preference: the steepest downslope neighbour that already has a water distance.
        for(int X = -1; X <= 1; X++) {
            for(int Y = -1; Y <= 1; Y++) {
                if(X == 0 && Y == 0) continue;

                int nx = x + X;
                int ny = y + Y;
                if(nx < 0 || nx >= xSize || ny < 0 || ny >= ySize) continue;

                int ni = ny * xSize + nx;
                if(dem[ni] == noData) continue;
                if(DtW[ni] == noData) continue;

                double step = sqrt((X * dx) * (X * dx) + (Y * dy) * (Y * dy));
                float dropPerDist = static_cast<float>((dem[i] - dem[ni]) / step);

                if(dem[ni] < dem[i] && dropPerDist > bestDropPerDist) {
                    bestDropPerDist = dropPerDist;
                    bestDist = DtW[ni] + static_cast<float>(step);
                }
            }
        }

        // Fallback for flats in a filled DEM: allow equal-elevation neighbours that already drain to water.
        // This preserves the DAG because equal-elevation neighbours are only accepted if they were earlier in order.
        if(bestDist == INF) {
            for(int X = -1; X <= 1; X++) {
                for(int Y = -1; Y <= 1; Y++) {
                    if(X == 0 && Y == 0) continue;

                    int nx = x + X;
                    int ny = y + Y;
                    if(nx < 0 || nx >= xSize || ny < 0 || ny >= ySize) continue;

                    int ni = ny * xSize + nx;
                    if(dem[ni] == noData) continue;
                    if(DtW[ni] == noData) continue;
                    if(dem[ni] > dem[i]) continue;

                    double step = sqrt((X * dx) * (X * dx) + (Y * dy) * (Y * dy));
                    float candidate = DtW[ni] + static_cast<float>(step);
                    if(candidate < bestDist) bestDist = candidate;
                }
            }
        }

        if(bestDist != INF) {
            DtW[i] = bestDist;
        } else {
            // Same effective upper value as old window=100 code when no water was found.
            DtW[i] = maxFallback;
        }
    }

    // ----------------------------------------------------------
    // Write output: same filename as original programme
    // ----------------------------------------------------------
    string tmpOutFName = folder + "/DtW.tif";
    const char* outFName = tmpOutFName.c_str();

    GDALDriver *driverGeotiff = GetGDALDriverManager()->GetDriverByName("GTiff");
    GDALDataset *output = driverGeotiff->Create(outFName, xSize, ySize, 1, GDT_Float32, NULL);
    output->SetGeoTransform(adfGeoTransform);
    output->SetProjection(INDEM->GetProjectionRef());
    output->GetRasterBand(1)->SetNoDataValue(static_cast<double>(noData));

    float *rowBuff = (float*) CPLMalloc(sizeof(float) * xSize);
    for(int y = 0; y < ySize; y++) {
        for(int x = 0; x < xSize; x++) {
            int i = y * xSize + x;
            if(dem[i] != noData)
                rowBuff[x] = DtW[i];
            else
                rowBuff[x] = noData;
        }
        output->GetRasterBand(1)->RasterIO(GF_Write, 0, y, xSize, 1,
                                           rowBuff, xSize, 1, GDT_Float32, 0, 0);
    }

    CPLFree(rowBuff);
    GDALClose((GDALDatasetH) output);
    GDALClose((GDALDatasetH) INDEM);
    GDALClose((GDALDatasetH) INRIVER);
    GDALClose((GDALDatasetH) INLAKES);

    return 0;
}
