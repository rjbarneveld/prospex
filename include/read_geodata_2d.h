#include "/usr/include/gdal/gdal_priv.h"
#include <vector>
#include <iostream>

// Template function to handle different numeric types for the array
template <typename T>
void ReadGeoTIFFBandToArray2D(const std::string& filename, std::vector<std::vector<std::vector<T>>>& array, int xSize, int ySize)
{

    // Open the GeoTIFF file using GDAL
    GDALDataset* poDataset = (GDALDataset*)GDALOpen(filename.c_str(), GA_ReadOnly);
    if (poDataset == nullptr) {
        std::cout << "Error opening GeoTIFF file!" << std::endl;
        return;
    }

    // Get the raster band (usually the first band, but adjust if needed)
    GDALRasterBand* poBand = poDataset->GetRasterBand(1);
    if (poBand == nullptr) {
        std::cout << "Error retrieving raster band!" << std::endl;
        GDALClose(poDataset);
        return;
    }

    // Allocate memory for a scanline
    float* pafScanline = (float*) CPLMalloc(sizeof(float) * xSize * ySize);

    // Read the data from the raster band into the scanline buffer
    if (poBand->RasterIO(GF_Read, 0, 0, xSize, ySize, pafScanline, xSize, ySize, GDT_Float32, 0, 0) != CE_None) {
        std::cout << "Error reading raster band!" << std::endl;
        CPLFree(pafScanline);
        GDALClose(poDataset);
        return;
    }

    // Fill the array with data
    for (int y = 0; y < ySize; ++y) {
        for (int x = 0; x < xSize; ++x) {
        // Convert from float to the appropriate type (T) depending on the template
        array[x][y][0] = static_cast<T>(pafScanline[y * xSize + x]);
	array[x][y][1] = static_cast<T>(pafScanline[y * xSize + x]);
        }
    }

    // Clean up
    CPLFree(pafScanline);
    GDALClose(poDataset);  // Don't forget to close the dataset

//    std::cout << "GeoTIFF written to vector" << std::endl;
}

