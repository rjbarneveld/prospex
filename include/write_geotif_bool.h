#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <gdal_priv.h>

inline void saveBoolGeoTIFF(
    const std::string& filename,
    const std::vector<std::vector<bool>>& data,
    int xSize, int ySize,
    double* adfGeoTransform,
    const char* projection)
{
    if (xSize <= 0 || ySize <= 0 ||
        data.size() != static_cast<std::size_t>(xSize)) {
        throw std::invalid_argument("Invalid Boolean raster dimensions");
    }

    for (const auto& column : data) {
        if (column.size() != static_cast<std::size_t>(ySize)) {
            throw std::invalid_argument("Invalid Boolean raster column length");
        }
    }

    GDALDriver* driver = GetGDALDriverManager()->GetDriverByName("GTiff");
    if (!driver) {
        throw std::runtime_error("GTiff driver not available");
    }

    std::vector<GByte> row(xSize);
    std::unique_ptr<GDALDataset, decltype(&GDALClose)> output(
        driver->Create(filename.c_str(), xSize, ySize, 1, GDT_Byte, nullptr),
        GDALClose);

    if (!output) {
        throw std::runtime_error("Failed to create GeoTIFF: " + filename);
    }

    if (output->SetGeoTransform(adfGeoTransform) != CE_None ||
        output->SetProjection(projection) != CE_None) {
        throw std::runtime_error("Failed to set GeoTIFF metadata: " + filename);
    }

    GDALRasterBand* band = output->GetRasterBand(1);
    for (int y = 0; y < ySize; ++y) {
        for (int x = 0; x < xSize; ++x) {
            row[x] = data[x][y] ? 1 : 0;
        }

        if (band->RasterIO(GF_Write, 0, y, xSize, 1,
                           row.data(), xSize, 1, GDT_Byte, 0, 0) != CE_None) {
            throw std::runtime_error(
                "Failed to write row " + std::to_string(y) + ": " + filename);
        }
    }
}
