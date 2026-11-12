inline void saveGeoTIFF(
    const std::string& filename,
    const std::vector<std::vector<float>>& data,
    int xSize, int ySize,
    double* adfGeoTransform,
    const char* projection)
{
    GDALDriver* driverGeotiff = GetGDALDriverManager()->GetDriverByName("GTiff");
    if (!driverGeotiff) throw std::runtime_error("GTiff driver not available");

    GDALDataset* output = driverGeotiff->Create(
        filename.c_str(), xSize, ySize, 1, GDT_Float32, nullptr);
    if (!output) throw std::runtime_error("Failed to create GeoTIFF: " + filename);

    output->SetGeoTransform(adfGeoTransform);
    output->SetProjection(projection);

    float* rowBuff = (float*) CPLMalloc(sizeof(float) * xSize);
    GDALRasterBand* band = output->GetRasterBand(1);

    for (int y = 0; y < ySize; y++) {
        for (int x = 0; x < xSize; x++) {
            rowBuff[x] = data[x][y];   // directly from vector
        }
        CPLErr err = band->RasterIO(GF_Write, 0, y, xSize, 1,
                                    rowBuff, xSize, 1,
                                    GDT_Float32, 0, 0);
        if (err != CE_None) {
            CPLFree(rowBuff);
            GDALClose((GDALDatasetH) output);
            throw std::runtime_error("RasterIO write failed at row " + std::to_string(y));
        }
    }

    CPLFree(rowBuff);
    GDALClose((GDALDatasetH) output);
}
