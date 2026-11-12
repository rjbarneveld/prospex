#include "/usr/include/gdal/gdal_priv.h"
#include "/usr/include/gdal/cpl_conv.h"
#include <vector>
#include <iostream>
#include <string>
#include <stdexcept>

template <typename T>
void ReadGeoTIFFBandToArray2D(
    const std::string& filename,
    std::vector<std::vector<std::vector<T>>>& array,
    int expectedXSize,
    int expectedYSize)
{

    GDALDataset* dataset =
        static_cast<GDALDataset*>(GDALOpen(filename.c_str(), GA_ReadOnly));

    if (dataset == nullptr)
        throw std::runtime_error("Could not open drain-route raster: " + filename);

    const int rasterXSize = dataset->GetRasterXSize();
    const int rasterYSize = dataset->GetRasterYSize();
    const int bandCount = dataset->GetRasterCount();
if (bandCount < 2)
{
    GDALClose(dataset);

    throw std::runtime_error(
        "Drain-route raster '" + filename +
        "' reports only " + std::to_string(bandCount) +
        " raster band(s).");
}
    if (rasterXSize != expectedXSize || rasterYSize != expectedYSize)
    {
        GDALClose(dataset);
        throw std::runtime_error(
            "Drain-route raster dimensions do not match DEM dimensions.");
    }

    if (bandCount < 2)
    {
        GDALClose(dataset);
        throw std::runtime_error(
            "Drain-route raster must contain at least two bands: "
            "band 1 = river X, band 2 = river Y.");
    }

    GDALRasterBand* bandX = dataset->GetRasterBand(1);
    GDALRasterBand* bandY = dataset->GetRasterBand(2);

    std::vector<double> bufferX(
        static_cast<std::size_t>(rasterXSize) * rasterYSize);
    std::vector<double> bufferY(
        static_cast<std::size_t>(rasterXSize) * rasterYSize);

    CPLErr errX = bandX->RasterIO(
        GF_Read,
        0, 0,
        rasterXSize, rasterYSize,
        bufferX.data(),
        rasterXSize, rasterYSize,
        GDT_Float64,
        0, 0);

    CPLErr errY = bandY->RasterIO(
        GF_Read,
        0, 0,
        rasterXSize, rasterYSize,
        bufferY.data(),
        rasterXSize, rasterYSize,
        GDT_Float64,
        0, 0);

    if (errX != CE_None || errY != CE_None)
    {
        GDALClose(dataset);
        throw std::runtime_error(
            "Failed to read drain-route raster bands.");
    }

    for (int y = 0; y < rasterYSize; ++y)
    {
        for (int x = 0; x < rasterXSize; ++x)
        {
            const std::size_t i =
                static_cast<std::size_t>(y) * rasterXSize + x;

            array[x][y][0] = static_cast<T>(bufferX[i]);
            array[x][y][1] = static_cast<T>(bufferY[i]);
        }
    }

    GDALClose(dataset);
}
