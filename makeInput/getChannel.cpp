#include <vector>
#include <queue>
#include <cmath>
#include <iostream>
#include <fstream>
#include "gdal_priv.h"
#include "../include/read_geodata.h"
#include "../include/write_geotif.h"

//g++ -std=c++11 getChannel.cpp $(gdal-config --cflags) $(gdal-config --libs) -o getChannel
//getChannel 
// ----------------------------------------------------------
// Utility: allocate 2D float array
// ----------------------------------------------------------
float** alloc2D(int xSize, int ySize)
{
    float** a = new float*[xSize];
    for (int x = 0; x < xSize; x++)
        a[x] = new float[ySize];
    return a;
}

using namespace std;
int main(int argc, char* argv[])
{
    if (argc < 4)
    {
        std::cerr << "Usage: getChannel dem.tif channel.tif threshold (in m²)\n";
        return 1;
    }

    std::string file_DEM = argv[1];
    std::string outFile = argv[2];
    float accThreshold = std::stof(argv[3]);

    // ------------------------------------------------------
    // 1. Open DEM
    // ------------------------------------------------------
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
	vector<vector<double>> dem = vector<vector<double>>(xSize,vector<double> (ySize,0));
	ReadGeoTIFFBandToArray(file_DEM.c_str(), dem, xSize, ySize);

    std::cout << "DEM read: " << xSize << " x " << ySize << std::endl;

    // ------------------------------------------------------
    // 3. D8 setup
    // ------------------------------------------------------
    const int dX[8] = { -1, 0, 1, -1, 1, -1, 0, 1 };
    const int dY[8] = {  1, 1, 1,  0, 0, -1,-1,-1 };
    const float dist[8] = {
        std::sqrt(2.f), 1.f, std::sqrt(2.f),
        1.f,            1.f,
        std::sqrt(2.f), 1.f, std::sqrt(2.f)
    };

    std::vector<std::vector<int>> receiver(
        xSize, std::vector<int>(ySize, -1));
    std::vector<std::vector<int>> indeg(
        xSize, std::vector<int>(ySize, 0));

    // ------------------------------------------------------
    // 4. Steepest downslope neighbor
    // ------------------------------------------------------
    for (int x = 1; x < xSize - 1; x++)
    for (int y = 1; y < ySize - 1; y++)
    {
        if (dem[x][y] == noData || dem[x][y] < 0) continue;

        float z = dem[x][y];
        float maxSlope = 0.0f;
        int best = -1;

        for (int k = 0; k < 8; k++)
        {
            int nx = x + dX[k];
            int ny = y + dY[k];
            if (dem[nx][ny] == noData) continue;

            float dz = z - dem[nx][ny];
            if (dz <= 0.0f) continue;

            float s = dz / dist[k];
            if (s > maxSlope)
            {
                maxSlope = s;
                best = k;
            }
        }

        receiver[x][y] = best;
        if (best >= 0)
        {
            int nx = x + dX[best];
            int ny = y + dY[best];
            indeg[nx][ny]++;
        }
    }

    // ------------------------------------------------------
    // 5. Flow accumulation (DAG)
    // ------------------------------------------------------
    std::vector<std::vector<float>> acc(xSize, std::vector<float>(ySize, (dx*dy)));

    std::queue<std::pair<int,int>> Q;
    for (int x = 0; x < xSize; x++)
    for (int y = 0; y < ySize; y++)
        if (dem[x][y] != noData && indeg[x][y] == 0)
            Q.emplace(x, y);

    while (!Q.empty())
    {
	std::pair<int,int> p = Q.front();
	Q.pop();
	int x = p.first;
	int y = p.second;

        int k = receiver[x][y];
        if (k < 0) continue;

        int nx = x + dX[k];
        int ny = y + dY[k];

        acc[nx][ny] += acc[x][y];
        if (--indeg[nx][ny] == 0)
            Q.emplace(nx, ny);
    }

    // ------------------------------------------------------
    // 6. Channel extraction
    // ------------------------------------------------------
    float** channel = alloc2D(xSize, ySize);
    int nChannel = 0;

    for (int x = 0; x < xSize; x++)
    for (int y = 0; y < ySize; y++)
    {
        if (dem[x][y] == noData)
            channel[x][y] = noData;
        else if (acc[x][y] >= accThreshold)
        {
            channel[x][y] = 1.0f;
            nChannel++;
        }
        else
            channel[x][y] = 0.0f;
    }

    std::cout << "Channel cells: " << nChannel << std::endl;

    // ------------------------------------------------------
    // 7. Write GeoTIFF
    // ------------------------------------------------------
    std::vector<std::vector<float>> channelVec(
    xSize, std::vector<float>(ySize));
    for (int x = 0; x < xSize; x++)
	    for (int y = 0; y < ySize; y++)
    		channelVec[x][y] = channel[x][y];
saveGeoTIFF(
    outFile,
    channelVec,
    xSize,
    ySize,
    adfGeoTransform,
    INDEM->GetProjectionRef()
);

    std::cout << "Written: " << outFile << std::endl;

    GDALClose(INDEM);
    return 0;
}

