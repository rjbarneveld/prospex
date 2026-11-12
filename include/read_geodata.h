#include "/usr/include/gdal/gdal_priv.h"
#include <vector>
#include <iostream>

// Template function to handle different numeric types for the array
template <typename T>
void ReadGeoTIFFBandToArray(const std::string& filename, std::vector<std::vector<T>>& array, int xSize, int ySize)
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
            array[x][y] = static_cast<T>(pafScanline[y * xSize + x]);
        }
    }

    // Clean up
    CPLFree(pafScanline);
    GDALClose(poDataset);  // Don't forget to close the dataset

//    std::cout << "GeoTIFF written to vector" << std::endl;
}

template <typename T>
void ReadGeoTIFFBandToArray(
    const std::string& filename,
    std::vector<std::vector<std::vector<T>>>& array,
    int expectedXSize,
    int expectedYSize,
    int component,
    int rasterBand = 1)
{
    GDALDataset* dataset =
        static_cast<GDALDataset*>(GDALOpen(filename.c_str(), GA_ReadOnly));

    if (dataset == nullptr)
    {
        throw std::runtime_error(
            "Could not open raster: " + filename);
    }

    const int rasterXSize = dataset->GetRasterXSize();
    const int rasterYSize = dataset->GetRasterYSize();
    const int bandCount = dataset->GetRasterCount();

    if (rasterXSize != expectedXSize ||
        rasterYSize != expectedYSize)
    {
        GDALClose(dataset);

        throw std::runtime_error(
            "Raster dimensions do not match the DEM: " + filename);
    }

    if (rasterBand < 1 || rasterBand > bandCount)
    {
        GDALClose(dataset);

        throw std::runtime_error(
            "Requested raster band " +
            std::to_string(rasterBand) +
            " does not exist in: " + filename);
    }

    if (array.empty() ||
        array[0].empty() ||
        array[0][0].empty() ||
        component < 0 ||
        component >= static_cast<int>(array[0][0].size()))
    {
        GDALClose(dataset);

        throw std::runtime_error(
            "Invalid array component requested for: " + filename);
    }

    GDALRasterBand* band = dataset->GetRasterBand(rasterBand);

    std::vector<double> buffer(
        static_cast<std::size_t>(rasterXSize) *
        static_cast<std::size_t>(rasterYSize));

    const CPLErr error = band->RasterIO(
        GF_Read,
        0,
        0,
        rasterXSize,
        rasterYSize,
        buffer.data(),
        rasterXSize,
        rasterYSize,
        GDT_Float64,
        0,
        0);

    if (error != CE_None)
    {
        GDALClose(dataset);

        throw std::runtime_error(
            "Failed to read raster band from: " + filename);
    }

    for (int y = 0; y < rasterYSize; ++y)
    {
        for (int x = 0; x < rasterXSize; ++x)
        {
            const std::size_t i =
                static_cast<std::size_t>(y) *
                static_cast<std::size_t>(rasterXSize) +
                static_cast<std::size_t>(x);

            array[x][y][component] =
                static_cast<T>(buffer[i]);
        }
    }

    GDALClose(dataset);
}
