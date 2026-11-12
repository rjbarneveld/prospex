#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include "/usr/include/gdal/gdal_priv.h"
#include "/usr/include/gdal/cpl_conv.h"

using std::cerr;
using std::cout;
using std::endl;
using std::string;
using std::vector;

static float clampFloat(float value, float lower, float upper)
{
    return std::max(lower, std::min(value, upper));
}

static bool sameGrid(GDALDataset* a, GDALDataset* b)
{
    if(a == nullptr || b == nullptr)
        return false;

    if(a->GetRasterXSize() != b->GetRasterXSize() ||
       a->GetRasterYSize() != b->GetRasterYSize())
        return false;

    double gta[6] = {0};
    double gtb[6] = {0};
    a->GetGeoTransform(gta);
    b->GetGeoTransform(gtb);

    for(int i = 0; i < 6; ++i)
    {
        if(std::fabs(gta[i] - gtb[i]) > 1.0e-8)
            return false;
    }

    return string(a->GetProjectionRef()) == string(b->GetProjectionRef());
}

static vector<float> readBandAsFloat(GDALDataset* dataset)
{
    if(dataset == nullptr || dataset->GetRasterCount() < 1)
        throw std::runtime_error("Invalid raster dataset.");

    const int width = dataset->GetRasterXSize();
    const int height = dataset->GetRasterYSize();
    vector<float> values(static_cast<size_t>(width) * height);

    GDALRasterBand* band = dataset->GetRasterBand(1);
    const CPLErr error = band->RasterIO(
        GF_Read,
        0,
        0,
        width,
        height,
        values.data(),
        width,
        height,
        GDT_Float32,
        0,
        0
    );

    if(error != CE_None)
        throw std::runtime_error("Raster read failed.");

    return values;
}

static bool isNoDataValue(float value, int hasNoData, double noData)
{
    if(!std::isfinite(value))
        return true;

    if(!hasNoData)
        return false;

    if(std::isnan(noData))
        return std::isnan(value);

    return value == static_cast<float>(noData);
}

static void writeRaster(
    const string& path,
    GDALDataset* reference,
    const vector<float>& values,
    float noData
)
{
    const int width = reference->GetRasterXSize();
    const int height = reference->GetRasterYSize();

    GDALDriver* driver = GetGDALDriverManager()->GetDriverByName("GTiff");
    if(driver == nullptr)
        throw std::runtime_error("GTiff driver is unavailable.");

    char** options = nullptr;
    options = CSLSetNameValue(options, "COMPRESS", "DEFLATE");
    options = CSLSetNameValue(options, "TILED", "YES");
    options = CSLSetNameValue(options, "BIGTIFF", "IF_SAFER");

    GDALDataset* output = driver->Create(
        path.c_str(),
        width,
        height,
        1,
        GDT_Float32,
        options
    );
    CSLDestroy(options);

    if(output == nullptr)
        throw std::runtime_error("Could not create output raster: " + path);

    double geotransform[6] = {0};
    reference->GetGeoTransform(geotransform);
    output->SetGeoTransform(geotransform);
    output->SetProjection(reference->GetProjectionRef());

    GDALRasterBand* band = output->GetRasterBand(1);
    band->SetNoDataValue(noData);

    const CPLErr error = band->RasterIO(
        GF_Write,
        0,
        0,
        width,
        height,
        const_cast<float*>(values.data()),
        width,
        height,
        GDT_Float32,
        0,
        0
    );

    GDALClose(output);

    if(error != CE_None)
        throw std::runtime_error("Raster write failed: " + path);
}

int main(int argc, char* argv[])
{
    if(argc < 2 || argc > 3)
    {
        cerr << "Usage: " << argv[0]
             << " <folder> [maximum_groundwater_depth_m]\n";
        cerr << "Example: " << argv[0] << " iolierelva 5\n";
        cerr << "         " << argv[0] << " iolierelva 2\n";
        return 1;
    }

    const string folder = argv[1];
    const float maximumDepthM =
        argc == 3 ? std::stof(argv[2]) : 5.0f;

    if(!(maximumDepthM > 0.0f))
    {
        cerr << "Maximum groundwater depth must be greater than zero.\n";
        return 1;
    }

    // Parameters that can be calibrated later.
    // A broad local mean represents subdued terrain-controlled groundwater.
    const float terrainWindowRadiusM = 150.0f;

    // Distance at which the distance-to-water term reaches 63% of its limit.
    const float distanceScaleM = 100.0f;

    // Positive/negative terrain position reaches full strength at this relief.
    const float terrainPositionScaleM = 8.0f;

    // When maximumDepthM is 5 m, high terrain is limited to 2 m depth.
    // When maximumDepthM is 2 m, the global 2 m limit applies everywhere.
    const float uplandMaximumDepthM =
        std::min(2.0f, maximumDepthM);

    // Valley bottoms are made shallower than the distance-only estimate.
    const float valleyShallowingFraction = 0.60f;

    // Small smoothing radius applied to groundwater depth, not head.
    const int smoothingRadiusCells = 2;

    GDALAllRegister();

    const string demPath = folder + "/dtm10_filled.tif";
    const string lakesPath = folder + "/lakes.tif";
    const string dtwPath = folder + "/DtW.tif";

    GDALDataset* demDataset = static_cast<GDALDataset*>(
        GDALOpen(demPath.c_str(), GA_ReadOnly)
    );
    GDALDataset* lakesDataset = static_cast<GDALDataset*>(
        GDALOpen(lakesPath.c_str(), GA_ReadOnly)
    );
    GDALDataset* dtwDataset = static_cast<GDALDataset*>(
        GDALOpen(dtwPath.c_str(), GA_ReadOnly)
    );

    if(demDataset == nullptr || lakesDataset == nullptr || dtwDataset == nullptr)
    {
        cerr << "Could not open one or more input rasters:\n"
             << "  " << demPath << "\n"
             << "  " << lakesPath << "\n"
             << "  " << dtwPath << "\n";
        if(demDataset) GDALClose(demDataset);
        if(lakesDataset) GDALClose(lakesDataset);
        if(dtwDataset) GDALClose(dtwDataset);
        return 1;
    }

    if(!sameGrid(demDataset, lakesDataset) ||
       !sameGrid(demDataset, dtwDataset))
    {
        cerr << "Input rasters do not have the same grid and projection.\n";
        GDALClose(demDataset);
        GDALClose(lakesDataset);
        GDALClose(dtwDataset);
        return 1;
    }

    try
    {
        const int width = demDataset->GetRasterXSize();
        const int height = demDataset->GetRasterYSize();
        const size_t cellCount = static_cast<size_t>(width) * height;

        double geotransform[6] = {0};
        demDataset->GetGeoTransform(geotransform);
        const float cellSizeM = static_cast<float>(
            0.5 * (std::fabs(geotransform[1]) + std::fabs(geotransform[5]))
        );

        const int terrainRadiusCells = std::max(
            1,
            static_cast<int>(std::round(terrainWindowRadiusM / cellSizeM))
        );

        GDALRasterBand* demBand = demDataset->GetRasterBand(1);
        GDALRasterBand* lakesBand = lakesDataset->GetRasterBand(1);
        GDALRasterBand* dtwBand = dtwDataset->GetRasterBand(1);

        int demHasNoData = 0;
        int lakesHasNoData = 0;
        int dtwHasNoData = 0;
        const double demNoData = demBand->GetNoDataValue(&demHasNoData);
        const double lakesNoData = lakesBand->GetNoDataValue(&lakesHasNoData);
        const double dtwNoData = dtwBand->GetNoDataValue(&dtwHasNoData);

        const float outputNoData = demHasNoData
            ? static_cast<float>(demNoData)
            : -9999.0f;

        const vector<float> dem = readBandAsFloat(demDataset);
        const vector<float> lakes = readBandAsFloat(lakesDataset);
        const vector<float> dtw = readBandAsFloat(dtwDataset);

        vector<unsigned char> validDem(cellCount, 0);
        vector<unsigned char> lakeCell(cellCount, 0);

        for(size_t i = 0; i < cellCount; ++i)
        {
            validDem[i] = !isNoDataValue(dem[i], demHasNoData, demNoData);
            lakeCell[i] = validDem[i] &&
                !isNoDataValue(lakes[i], lakesHasNoData, lakesNoData);
        }

        // Integral images provide an efficient broad local terrain mean.
        const int integralWidth = width + 1;
        const int integralHeight = height + 1;
        vector<double> integralSum(
            static_cast<size_t>(integralWidth) * integralHeight,
            0.0
        );
        vector<long long> integralCount(
            static_cast<size_t>(integralWidth) * integralHeight,
            0
        );

        for(int y = 0; y < height; ++y)
        {
            double rowSum = 0.0;
            long long rowCount = 0;

            for(int x = 0; x < width; ++x)
            {
                const size_t sourceIndex =
                    static_cast<size_t>(y) * width + x;
                if(validDem[sourceIndex])
                {
                    rowSum += dem[sourceIndex];
                    ++rowCount;
                }

                const size_t integralIndex =
                    static_cast<size_t>(y + 1) * integralWidth + (x + 1);
                const size_t aboveIndex =
                    static_cast<size_t>(y) * integralWidth + (x + 1);

                integralSum[integralIndex] =
                    integralSum[aboveIndex] + rowSum;
                integralCount[integralIndex] =
                    integralCount[aboveIndex] + rowCount;
            }
        }

        auto rectangleSum = [&](const vector<double>& integral,
                                int x0, int y0, int x1, int y1) -> double
        {
            const size_t a = static_cast<size_t>(y0) * integralWidth + x0;
            const size_t b = static_cast<size_t>(y0) * integralWidth + x1;
            const size_t c = static_cast<size_t>(y1) * integralWidth + x0;
            const size_t d = static_cast<size_t>(y1) * integralWidth + x1;
            return integral[d] - integral[b] - integral[c] + integral[a];
        };

        auto rectangleCount = [&](int x0, int y0, int x1, int y1) -> long long
        {
            const size_t a = static_cast<size_t>(y0) * integralWidth + x0;
            const size_t b = static_cast<size_t>(y0) * integralWidth + x1;
            const size_t c = static_cast<size_t>(y1) * integralWidth + x0;
            const size_t d = static_cast<size_t>(y1) * integralWidth + x1;
            return integralCount[d] - integralCount[b] -
                   integralCount[c] + integralCount[a];
        };

        vector<float> terrainPosition(cellCount, outputNoData);
        vector<float> localMaximumDepth(cellCount, outputNoData);
        vector<float> rawDepth(cellCount, outputNoData);
        vector<float> finalDepth(cellCount, outputNoData);
        vector<float> groundwaterHead(cellCount, outputNoData);
        vector<float> groundwaterReferenceHead(cellCount, outputNoData);
        vector<float> initialActiveThickness(cellCount, outputNoData);

        long long missingDtwCells = 0;
        double sumDepth = 0.0;
        long long depthCellCount = 0;

        for(int y = 0; y < height; ++y)
        {
            for(int x = 0; x < width; ++x)
            {
                const size_t index = static_cast<size_t>(y) * width + x;
                if(!validDem[index])
                    continue;

                if(lakeCell[index])
                {
                    // Lake cells are fixed-head boundaries in the dynamic model.
                    terrainPosition[index] = 0.0f;
                    localMaximumDepth[index] = 0.0f;
                    rawDepth[index] = 0.0f;
                    finalDepth[index] = 0.0f;
                    groundwaterHead[index] = dem[index];
                    groundwaterReferenceHead[index] = dem[index];
                    initialActiveThickness[index] = 0.0f;
                    continue;
                }

                const int x0 = std::max(0, x - terrainRadiusCells);
                const int y0 = std::max(0, y - terrainRadiusCells);
                const int x1 = std::min(width, x + terrainRadiusCells + 1);
                const int y1 = std::min(height, y + terrainRadiusCells + 1);

                const long long localCount = rectangleCount(x0, y0, x1, y1);
                const double localMean = localCount > 0
                    ? rectangleSum(integralSum, x0, y0, x1, y1) /
                      static_cast<double>(localCount)
                    : dem[index];

                const float tpi = dem[index] - static_cast<float>(localMean);
                terrainPosition[index] = tpi;

                const float ridgeFactor = clampFloat(
                    tpi / terrainPositionScaleM,
                    0.0f,
                    1.0f
                );
                const float valleyFactor = clampFloat(
                    -tpi / terrainPositionScaleM,
                    0.0f,
                    1.0f
                );

                const float cellMaximumDepth =
                    maximumDepthM -
                    ridgeFactor *
                    (maximumDepthM - uplandMaximumDepthM);
                localMaximumDepth[index] = cellMaximumDepth;

                float distanceDepth = cellMaximumDepth;
                if(!isNoDataValue(dtw[index], dtwHasNoData, dtwNoData) &&
                   dtw[index] >= 0.0f)
                {
                    distanceDepth = maximumDepthM *
                        (1.0f - std::exp(-dtw[index] / distanceScaleM));
                }
                else
                {
                    ++missingDtwCells;
                }

                float depth = distanceDepth *
                    (1.0f - valleyShallowingFraction * valleyFactor);
                depth = clampFloat(depth, 0.0f, cellMaximumDepth);

                rawDepth[index] = depth;
            }
        }

        // Smooth depth, then reapply all physical limits. Smoothing head directly
        // can create groundwater above the DEM or below the configured depth cap.
        for(int y = 0; y < height; ++y)
        {
            for(int x = 0; x < width; ++x)
            {
                const size_t index = static_cast<size_t>(y) * width + x;
                if(!validDem[index])
                    continue;

                if(lakeCell[index])
                {
                    finalDepth[index] = 0.0f;
                    groundwaterHead[index] = dem[index];
                    groundwaterReferenceHead[index] = dem[index];
                    initialActiveThickness[index] = 0.0f;
                    continue;
                }

                double depthSum = 0.0;
                int count = 0;

                for(int dyCell = -smoothingRadiusCells;
                    dyCell <= smoothingRadiusCells;
                    ++dyCell)
                {
                    const int ny = y + dyCell;
                    if(ny < 0 || ny >= height)
                        continue;

                    for(int dxCell = -smoothingRadiusCells;
                        dxCell <= smoothingRadiusCells;
                        ++dxCell)
                    {
                        const int nx = x + dxCell;
                        if(nx < 0 || nx >= width)
                            continue;

                        const size_t neighbourIndex =
                            static_cast<size_t>(ny) * width + nx;
                        if(!validDem[neighbourIndex] ||
                           lakeCell[neighbourIndex] ||
                           rawDepth[neighbourIndex] == outputNoData)
                            continue;

                        depthSum += rawDepth[neighbourIndex];
                        ++count;
                    }
                }

                float depth = count > 0
                    ? static_cast<float>(depthSum / count)
                    : rawDepth[index];

                depth = clampFloat(
                    depth,
                    0.0f,
                    localMaximumDepth[index]
                );

                finalDepth[index] = depth;
                groundwaterHead[index] = dem[index] - depth;

                // The lower reference is the terrain-derived aquifer base:
                // terrain elevation minus the local maximum groundwater depth.
                // This is shallower on ridges/uplands and deeper in lower terrain.
                groundwaterReferenceHead[index] =
                    dem[index] - localMaximumDepth[index];

                initialActiveThickness[index] = std::max(
                    0.0f,
                    groundwaterHead[index] - groundwaterReferenceHead[index]
                );

                sumDepth += depth;
                ++depthCellCount;
            }
        }

        writeRaster(
            folder + "/groundwaterLevel.tif",
            demDataset,
            groundwaterHead,
            outputNoData
        );
        writeRaster(
            folder + "/groundwaterDepth.tif",
            demDataset,
            finalDepth,
            outputNoData
        );
        writeRaster(
            folder + "/groundwaterTerrainPosition.tif",
            demDataset,
            terrainPosition,
            outputNoData
        );
        writeRaster(
            folder + "/groundwaterLocalMaximumDepth.tif",
            demDataset,
            localMaximumDepth,
            outputNoData
        );
        writeRaster(
            folder + "/groundwaterReferenceLevel.tif",
            demDataset,
            groundwaterReferenceHead,
            outputNoData
        );
        writeRaster(
            folder + "/groundwaterInitialActiveThickness.tif",
            demDataset,
            initialActiveThickness,
            outputNoData
        );

        cout << "========== INITIAL GROUNDWATER ESTIMATION ==========\n";
        cout << "Grid:                         " << width << " x " << height << "\n";
        cout << "Maximum groundwater depth:    " << maximumDepthM << " m\n";
        cout << "Upland maximum depth:          " << uplandMaximumDepthM << " m\n";
        cout << "Terrain window radius:         " << terrainWindowRadiusM << " m\n";
        cout << "Distance-to-water scale:       " << distanceScaleM << " m\n";
        cout << "Mean estimated land depth:     "
             << (depthCellCount > 0 ? sumDepth / depthCellCount : 0.0)
             << " m\n";
        cout << "Active land cells lacking DtW: " << missingDtwCells << "\n";
        cout << "Written:\n"
             << "  " << folder << "/groundwaterLevel.tif\n"
             << "  " << folder << "/groundwaterDepth.tif\n"
             << "  " << folder << "/groundwaterTerrainPosition.tif\n"
             << "  " << folder << "/groundwaterLocalMaximumDepth.tif\n"
             << "  " << folder << "/groundwaterReferenceLevel.tif\n"
             << "  " << folder << "/groundwaterInitialActiveThickness.tif\n";
        cout << "====================================================\n";
    }
    catch(const std::exception& exception)
    {
        cerr << "ERROR: " << exception.what() << endl;
        GDALClose(demDataset);
        GDALClose(lakesDataset);
        GDALClose(dtwDataset);
        return 1;
    }

    GDALClose(demDataset);
    GDALClose(lakesDataset);
    GDALClose(dtwDataset);
    return 0;
}
