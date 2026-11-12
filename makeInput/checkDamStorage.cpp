#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "/usr/include/gdal/gdal_priv.h"
#include "/usr/include/gdal/gdal_alg.h"
#include "/usr/include/gdal/ogrsf_frmts.h"
#include "/usr/include/gdal/cpl_conv.h"
#include "/usr/include/gdal/cpl_string.h"
#include "/usr/include/gdal/cpl_vsi.h"

using namespace std;

// Compile:
// g++ -O3 -std=c++11 checkDamStorage.cpp $(gdal-config --cflags) $(gdal-config --libs) -o checkDamStorage
//
// Usage:
// ./checkDamStorage <folder> <selection.tif> <dem.tif> <dem_ns.tif> <damStats.csv> <output_folder>
//
// Example:
// ./checkDamStorage ioskut2 /measures/sce03/selection03.tif /dtm10_filled.tif /dtm10_filled_ns.tif /measures/sce03/damStats.csv /measures/checkdams
//
// Paths after <folder> may be absolute or relative to <folder> in the same
// style as damStats_v4.cpp.  Output files are:
//   damStats_1m.csv
//   damStats_2m.csv
//   damExtents_1m.gpkg
//   damExtents_2m.gpkg

struct DamStat
{
    int index = 0;
    double area = 0.0;
    double z = 0.0;
    double x = 0.0;
    double y = 0.0;
    int xindex = -1;
    int yindex = -1;
    double volumeMax = 0.0;
    double volumeAct = 0.0;
    double pipeCapacity_ls = 0.0;
};

static string joinFolder(const string &folder, const string &path)
{
    if(path.empty()) return folder;

    // Preserve the convention used in damStats_v4.cpp: arguments beginning
    // with '/' are still appended to the project folder.
    if(!folder.empty() && folder[folder.size()-1] == '/' && path[0] == '/')
        return folder.substr(0, folder.size()-1) + path;
    if(!folder.empty() && folder[folder.size()-1] != '/' && path[0] != '/')
        return folder + "/" + path;
    return folder + path;
}

static bool sameGeoTransform(const double a[6], const double b[6], double tol=1.0e-9)
{
    for(int i=0; i<6; ++i)
        if(std::fabs(a[i]-b[i]) > tol)
            return false;
    return true;
}

static bool validValue(float v, int hasNoData, double noData)
{
    if(!std::isfinite(v)) return false;
    if(!hasNoData) return true;
    if(std::isnan(noData)) return !std::isnan(v);
    return static_cast<double>(v) != noData;
}

static bool readFloatRaster(
    GDALDataset *ds,
    vector<float> &values,
    int width,
    int height,
    const string &label)
{
    if(ds == NULL || ds->GetRasterCount() < 1)
    {
        cerr << "ERROR: cannot read " << label << "." << endl;
        return false;
    }

    values.resize(static_cast<size_t>(width) * height);
    CPLErr err = ds->GetRasterBand(1)->RasterIO(
        GF_Read, 0, 0, width, height,
        values.data(), width, height,
        GDT_Float32, 0, 0
    );

    if(err != CE_None)
    {
        cerr << "ERROR: RasterIO failed for " << label << "." << endl;
        return false;
    }
    return true;
}

static bool readDamStats(
    const string &filename,
    vector<DamStat> &stats,
    vector<char> &hasStat,
    int &maxID)
{
    ifstream in(filename.c_str());
    if(!in.is_open())
    {
        cerr << "ERROR: cannot open dam statistics table: " << filename << endl;
        return false;
    }

    string header;
    getline(in, header);

    vector<DamStat> rows;
    DamStat s;
    maxID = 0;

    while(in >> s.index >> s.area >> s.z >> s.x >> s.y
             >> s.xindex >> s.yindex >> s.volumeMax
             >> s.volumeAct >> s.pipeCapacity_ls)
    {
        if(s.index <= 0)
        {
            cerr << "ERROR: invalid dam index " << s.index
                 << " in " << filename << endl;
            return false;
        }
        rows.push_back(s);
        maxID = std::max(maxID, s.index);
    }

    if(rows.empty())
    {
        cerr << "ERROR: no dam records found in " << filename << endl;
        return false;
    }

    stats.assign(maxID + 1, DamStat());
    hasStat.assign(maxID + 1, 0);

    for(size_t i=0; i<rows.size(); ++i)
    {
        const int id = rows[i].index;
        if(hasStat[id])
        {
            cerr << "ERROR: duplicate dam index " << id
                 << " in " << filename << endl;
            return false;
        }
        stats[id] = rows[i];
        hasStat[id] = 1;
    }

    return true;
}

static bool writeStatsTable(
    const string &filename,
    const vector<DamStat> &stats,
    const vector<char> &hasStat,
    const vector<double> &area,
    const vector<double> &volume,
    double checkDamHeight)
{
    ofstream out(filename.c_str());
    if(!out.is_open())
    {
        cerr << "ERROR: cannot create " << filename << endl;
        return false;
    }

    out << "index area z x y xindex yindex volumeMax volumeAct pipeCapacity_ls" << endl;
    out << std::setprecision(10);

    for(size_t id=1; id<stats.size(); ++id)
    {
        if(!hasStat[id]) continue;

        const DamStat &s = stats[id];
        out << s.index << " "
            << area[id] << " "
            << (s.z + checkDamHeight) << " "
            << s.x << " "
            << s.y << " "
            << s.xindex << " "
            << s.yindex << " "
            << volume[id] << " "
            << s.volumeAct << " "
            << s.pipeCapacity_ls << endl;
    }

    return true;
}

static bool writeExtentGpkg(
    const string &filename,
    const string &layerName,
    const vector<int> &extent,
    int width,
    int height,
    const double geoTransform[6],
    const char *projection,
    const vector<DamStat> &stats,
    const vector<char> &hasStat,
    const vector<double> &area,
    const vector<double> &volume,
    double checkDamHeight)
{
    const int extentNoData = -9999;

    GDALDriver *memDriver = GetGDALDriverManager()->GetDriverByName("MEM");
    GDALDriver *gpkgDriver = GetGDALDriverManager()->GetDriverByName("GPKG");
    if(memDriver == NULL || gpkgDriver == NULL)
    {
        cerr << "ERROR: required GDAL MEM or GPKG driver unavailable." << endl;
        return false;
    }

    GDALDataset *rasterDS = memDriver->Create("", width, height, 1, GDT_Int32, NULL);
    GDALDataset *maskDS   = memDriver->Create("", width, height, 1, GDT_Byte, NULL);
    if(rasterDS == NULL || maskDS == NULL)
    {
        cerr << "ERROR: could not create in-memory extent raster." << endl;
        if(rasterDS) GDALClose(rasterDS);
        if(maskDS) GDALClose(maskDS);
        return false;
    }

    rasterDS->SetGeoTransform(const_cast<double*>(geoTransform));
    rasterDS->SetProjection(projection ? projection : "");
    maskDS->SetGeoTransform(const_cast<double*>(geoTransform));
    maskDS->SetProjection(projection ? projection : "");

    GDALRasterBand *rasterBand = rasterDS->GetRasterBand(1);
    GDALRasterBand *maskBand = maskDS->GetRasterBand(1);
    rasterBand->SetNoDataValue(extentNoData);

    vector<GByte> mask(extent.size(), 0);
    for(size_t i=0; i<extent.size(); ++i)
        if(extent[i] > 0)
            mask[i] = 255;

    if(rasterBand->RasterIO(
           GF_Write, 0, 0, width, height,
           const_cast<int*>(extent.data()), width, height,
           GDT_Int32, 0, 0) != CE_None ||
       maskBand->RasterIO(
           GF_Write, 0, 0, width, height,
           mask.data(), width, height,
           GDT_Byte, 0, 0) != CE_None)
    {
        cerr << "ERROR: could not write in-memory extent raster." << endl;
        GDALClose(maskDS);
        GDALClose(rasterDS);
        return false;
    }

    VSIStatBufL statBuf;
    if(VSIStatL(filename.c_str(), &statBuf) == 0)
    {
        if(gpkgDriver->Delete(filename.c_str()) != CE_None)
        {
            cerr << "ERROR: could not replace existing " << filename << endl;
            GDALClose(maskDS);
            GDALClose(rasterDS);
            return false;
        }
    }

    GDALDataset *vectorDS = gpkgDriver->Create(
        filename.c_str(), 0, 0, 0, GDT_Unknown, NULL
    );
    if(vectorDS == NULL)
    {
        cerr << "ERROR: could not create " << filename << endl;
        GDALClose(maskDS);
        GDALClose(rasterDS);
        return false;
    }

    OGRSpatialReference srs;
    OGRSpatialReference *srsPtr = NULL;
    if(projection != NULL && projection[0] != '\0' &&
       srs.SetFromUserInput(projection) == OGRERR_NONE)
    {
#if GDAL_VERSION_NUM >= 3000000
        srs.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
#endif
        srsPtr = &srs;
    }

    OGRLayer *layer = vectorDS->CreateLayer(
        layerName.c_str(), srsPtr, wkbPolygon, NULL
    );
    if(layer == NULL)
    {
        cerr << "ERROR: could not create layer " << layerName << endl;
        GDALClose(vectorDS);
        GDALClose(maskDS);
        GDALClose(rasterDS);
        return false;
    }

    OGRFieldDefn fieldID("dam_index", OFTInteger);
    OGRFieldDefn fieldHeight("height_m", OFTReal);
    OGRFieldDefn fieldArea("area_m2", OFTReal);
    OGRFieldDefn fieldZ("water_z", OFTReal);
    OGRFieldDefn fieldVolume("volume_m3", OFTReal);

    if(layer->CreateField(&fieldID) != OGRERR_NONE ||
       layer->CreateField(&fieldHeight) != OGRERR_NONE ||
       layer->CreateField(&fieldArea) != OGRERR_NONE ||
       layer->CreateField(&fieldZ) != OGRERR_NONE ||
       layer->CreateField(&fieldVolume) != OGRERR_NONE)
    {
        cerr << "ERROR: could not create extent attributes in " << filename << endl;
        GDALClose(vectorDS);
        GDALClose(maskDS);
        GDALClose(rasterDS);
        return false;
    }

    char **options = NULL;
    options = CSLSetNameValue(options, "8CONNECTED", "8");
    CPLErr polyErr = GDALPolygonize(
        reinterpret_cast<GDALRasterBandH>(rasterBand),
        reinterpret_cast<GDALRasterBandH>(maskBand),
        reinterpret_cast<OGRLayerH>(layer),
        0,
        options,
        NULL,
        NULL
    );
    CSLDestroy(options);

    if(polyErr != CE_None)
    {
        cerr << "ERROR: polygonisation failed for " << filename << endl;
        GDALClose(vectorDS);
        GDALClose(maskDS);
        GDALClose(rasterDS);
        return false;
    }

    layer->ResetReading();
    OGRFeature *feature = NULL;
    while((feature = layer->GetNextFeature()) != NULL)
    {
        const int id = feature->GetFieldAsInteger("dam_index");
        if(id > 0 && id < static_cast<int>(stats.size()) && hasStat[id])
        {
            feature->SetField("height_m", checkDamHeight);
            feature->SetField("area_m2", area[id]);
            feature->SetField("water_z", stats[id].z + checkDamHeight);
            feature->SetField("volume_m3", volume[id]);

            if(layer->SetFeature(feature) != OGRERR_NONE)
            {
                cerr << "ERROR: could not update attributes for dam "
                     << id << " in " << filename << endl;
                OGRFeature::DestroyFeature(feature);
                GDALClose(vectorDS);
                GDALClose(maskDS);
                GDALClose(rasterDS);
                return false;
            }
        }
        OGRFeature::DestroyFeature(feature);
    }

    GDALClose(vectorDS);
    GDALClose(maskDS);
    GDALClose(rasterDS);
    return true;
}

int main(int argc, char *argv[])
{
    if(argc != 7)
    {
        cerr << "Usage: " << argv[0]
             << " <folder> <selection.tif> <dem.tif> <dem_ns.tif>"
             << " <damStats.csv> <output_folder>" << endl;
        return 1;
    }

    GDALAllRegister();

    const string folder = argv[1];
    const string damFilename = joinFolder(folder, argv[2]);
    const string demFilename = joinFolder(folder, argv[3]);
    const string routingFilename = joinFolder(folder, argv[4]);
    const string statsFilename = joinFolder(folder, argv[5]);
    const string outputFolder = joinFolder(folder, argv[6]);

    if(VSIMkdirRecursive(outputFolder.c_str(), 0755) != 0)
    {
        VSIStatBufL statBuf;
        if(VSIStatL(outputFolder.c_str(), &statBuf) != 0)
        {
            cerr << "ERROR: cannot create output folder " << outputFolder << endl;
            return 1;
        }
    }

    cout << "========== CHECK-DAM STORAGE ANALYSIS ==========" << endl;
    cout << "Dam targets:  " << damFilename << endl;
    cout << "Terrain DEM:  " << demFilename << endl;
    cout << "Routing DEM:  " << routingFilename << endl;
    cout << "Dam table:    " << statsFilename << endl;
    cout << "Output folder:" << outputFolder << endl;

    GDALDataset *demDS = static_cast<GDALDataset*>(GDALOpen(demFilename.c_str(), GA_ReadOnly));
    GDALDataset *routingDS = static_cast<GDALDataset*>(GDALOpen(routingFilename.c_str(), GA_ReadOnly));
    GDALDataset *damDS = static_cast<GDALDataset*>(GDALOpen(damFilename.c_str(), GA_ReadOnly));

    if(demDS == NULL || routingDS == NULL || damDS == NULL)
    {
        cerr << "ERROR: could not open one or more input rasters." << endl;
        if(demDS) GDALClose(demDS);
        if(routingDS) GDALClose(routingDS);
        if(damDS) GDALClose(damDS);
        return 1;
    }

    const int width = demDS->GetRasterXSize();
    const int height = demDS->GetRasterYSize();
    const size_t cellCount = static_cast<size_t>(width) * height;

    double demGT[6], routingGT[6], damGT[6];
    demDS->GetGeoTransform(demGT);
    routingDS->GetGeoTransform(routingGT);
    damDS->GetGeoTransform(damGT);

    if(routingDS->GetRasterXSize() != width || routingDS->GetRasterYSize() != height ||
       damDS->GetRasterXSize() != width || damDS->GetRasterYSize() != height ||
       !sameGeoTransform(demGT, routingGT) || !sameGeoTransform(demGT, damGT))
    {
        cerr << "ERROR: input rasters do not share the same grid." << endl;
        GDALClose(damDS);
        GDALClose(routingDS);
        GDALClose(demDS);
        return 1;
    }

    if(string(demDS->GetProjectionRef()) != string(routingDS->GetProjectionRef()) ||
       string(demDS->GetProjectionRef()) != string(damDS->GetProjectionRef()))
    {
        cerr << "ERROR: input raster CRS definitions differ." << endl;
        GDALClose(damDS);
        GDALClose(routingDS);
        GDALClose(demDS);
        return 1;
    }

    const double dx = std::fabs(demGT[1]);
    const double dy = std::fabs(demGT[5]);
    const double cellArea = dx * dy;
    const double diagonalDistance = std::sqrt(dx*dx + dy*dy);

    int demHasNoData = 0;
    int routingHasNoData = 0;
    int damHasNoData = 0;
    const double demNoData = demDS->GetRasterBand(1)->GetNoDataValue(&demHasNoData);
    const double routingNoData = routingDS->GetRasterBand(1)->GetNoDataValue(&routingHasNoData);
    const double damNoData = damDS->GetRasterBand(1)->GetNoDataValue(&damHasNoData);

    vector<float> dem, routing, damRaster;
    if(!readFloatRaster(demDS, dem, width, height, "DEM") ||
       !readFloatRaster(routingDS, routing, width, height, "routing DEM") ||
       !readFloatRaster(damDS, damRaster, width, height, "dam target raster"))
    {
        GDALClose(damDS);
        GDALClose(routingDS);
        GDALClose(demDS);
        return 1;
    }

    vector<DamStat> stats;
    vector<char> hasStat;
    int maxStatsID = 0;
    if(!readDamStats(statsFilename, stats, hasStat, maxStatsID))
    {
        GDALClose(damDS);
        GDALClose(routingDS);
        GDALClose(demDS);
        return 1;
    }

    int maxRasterID = 0;
    long long targetCells = 0;
    for(size_t i=0; i<cellCount; ++i)
    {
        if(!validValue(damRaster[i], damHasNoData, damNoData))
            continue;
        if(damRaster[i] > 0.0f)
        {
            const int id = static_cast<int>(std::lround(damRaster[i]));
            maxRasterID = std::max(maxRasterID, id);
            ++targetCells;
        }
    }

    const int maxID = std::max(maxStatsID, maxRasterID);
    if(static_cast<int>(stats.size()) <= maxID)
    {
        stats.resize(maxID + 1);
        hasStat.resize(maxID + 1, 0);
    }

    for(size_t i=0; i<cellCount; ++i)
    {
        if(!validValue(damRaster[i], damHasNoData, damNoData) || damRaster[i] <= 0.0f)
            continue;
        const int id = static_cast<int>(std::lround(damRaster[i]));
        if(id <= 0 || id >= static_cast<int>(hasStat.size()) || !hasStat[id])
        {
            cerr << "ERROR: target raster contains dam ID " << id
                 << " without a corresponding damStats.csv record." << endl;
            GDALClose(damDS);
            GDALClose(routingDS);
            GDALClose(demDS);
            return 1;
        }
    }

    vector<char> targetSeen(maxID + 1, 0);
    for(size_t i=0; i<cellCount; ++i)
    {
        if(validValue(damRaster[i], damHasNoData, damNoData) && damRaster[i] > 0.0f)
        {
            const int id = static_cast<int>(std::lround(damRaster[i]));
            if(id > 0 && id <= maxID)
                targetSeen[id] = 1;
        }
    }

    for(int id=1; id<=maxStatsID; ++id)
    {
        if(!hasStat[id]) continue;
        if(id >= static_cast<int>(targetSeen.size()) || !targetSeen[id])
        {
            cerr << "ERROR: damStats.csv contains dam ID " << id
                 << " but the target raster has no cells with that ID." << endl;
            GDALClose(damDS);
            GDALClose(routingDS);
            GDALClose(demDS);
            return 1;
        }
    }

    cout << "Grid: " << width << " x " << height
         << ", cell area=" << cellArea << " m2" << endl;
    cout << "Dam records: " << maxStatsID
         << ", target cells: " << targetCells << endl;

    // -------------------------------------------------------------------------
    // 1. Derive one strictly-downhill SFD receiver from dem_ns for every cell.
    // -------------------------------------------------------------------------
    const int dX[8] = {-1, 0, 1,-1, 1,-1, 0, 1};
    const int dY[8] = {-1,-1,-1, 0, 0, 1, 1, 1};

    vector<int> receiver(cellCount, -1);
    long long validRoutingCells = 0;
    long long noReceiverCells = 0;

    for(int y=0; y<height; ++y)
    {
        for(int x=0; x<width; ++x)
        {
            const size_t i = static_cast<size_t>(y) * width + x;
            if(!validValue(routing[i], routingHasNoData, routingNoData) ||
               !validValue(dem[i], demHasNoData, demNoData))
                continue;

            ++validRoutingCells;

            // A dam cell is already a target; it needs no downstream receiver.
            if(validValue(damRaster[i], damHasNoData, damNoData) && damRaster[i] > 0.0f)
                continue;

            double bestSlope = 0.0;
            int bestReceiver = -1;

            for(int n=0; n<8; ++n)
            {
                const int nx = x + dX[n];
                const int ny = y + dY[n];
                if(nx < 0 || nx >= width || ny < 0 || ny >= height)
                    continue;

                const size_t ni = static_cast<size_t>(ny) * width + nx;
                if(!validValue(routing[ni], routingHasNoData, routingNoData))
                    continue;

                const double dz = static_cast<double>(routing[i]) - routing[ni];
                if(dz <= 0.0)
                    continue;  // strictly downhill, as in the existing topology

                const bool diagonal = (dX[n] != 0 && dY[n] != 0);
                const double distance = diagonal
                    ? diagonalDistance
                    : (dX[n] != 0 ? dx : dy);
                const double slope = dz / distance;

                if(slope > bestSlope)
                {
                    bestSlope = slope;
                    bestReceiver = static_cast<int>(ni);
                }
            }

            receiver[i] = bestReceiver;
            if(bestReceiver < 0)
                ++noReceiverCells;
        }
    }

    cout << "SFD receivers derived." << endl;
    cout << "Valid routing cells: " << validRoutingCells << endl;
    cout << "Non-target cells without a lower SFD receiver: "
         << noReceiverCells << endl;

    // -------------------------------------------------------------------------
    // 2. Trace every cell downstream until it enters the first dam target.
    //    basinID > 0 : first dam encountered
    //    basinID == -1: no selected dam encountered
    //    basinID == -2: not processed yet
    // -------------------------------------------------------------------------
    const int UNKNOWN = -2;
    const int NO_DAM = -1;
    vector<int> basinID(cellCount, UNKNOWN);

    for(size_t i=0; i<cellCount; ++i)
    {
        if(!validValue(routing[i], routingHasNoData, routingNoData) ||
           !validValue(dem[i], demHasNoData, demNoData))
        {
            basinID[i] = NO_DAM;
            continue;
        }

        if(validValue(damRaster[i], damHasNoData, damNoData) && damRaster[i] > 0.0f)
            basinID[i] = static_cast<int>(std::lround(damRaster[i]));
    }

    vector<int> path;
    path.reserve(1024);
    long long cellsAssignedToDam = 0;

    for(size_t start=0; start<cellCount; ++start)
    {
        if(basinID[start] != UNKNOWN)
            continue;

        path.clear();
        int current = static_cast<int>(start);
        int result = NO_DAM;

        while(true)
        {
            if(current < 0)
            {
                result = NO_DAM;
                break;
            }

            if(basinID[current] != UNKNOWN)
            {
                result = basinID[current];
                break;
            }

            path.push_back(current);
            const int next = receiver[current];

            if(next < 0)
            {
                result = NO_DAM;
                break;
            }

            current = next;
        }

        for(size_t p=0; p<path.size(); ++p)
            basinID[path[p]] = result;
    }

    for(size_t i=0; i<cellCount; ++i)
        if(basinID[i] > 0)
            ++cellsAssignedToDam;

    cout << "Cells whose SFD path reaches a selected dam: "
         << cellsAssignedToDam << endl;

    // -------------------------------------------------------------------------
    // 3. Apply the +1 m and +2 m source-cell elevation tests using ORIGINAL DEM.
    // -------------------------------------------------------------------------
    vector<double> area1(maxID + 1, 0.0);
    vector<double> area2(maxID + 1, 0.0);
    vector<double> volume1(maxID + 1, 0.0);
    vector<double> volume2(maxID + 1, 0.0);
    vector<int> extent1(cellCount, -9999);
    vector<int> extent2(cellCount, -9999);

    long long extentCells1 = 0;
    long long extentCells2 = 0;

    for(size_t i=0; i<cellCount; ++i)
    {
        const int id = basinID[i];
        if(id <= 0 || id > maxID || !hasStat[id])
            continue;
        if(!validValue(dem[i], demHasNoData, demNoData))
            continue;

        const double z = dem[i];
        const double level1 = stats[id].z + 1.0;
        const double level2 = stats[id].z + 2.0;

        if(z < level1)
        {
            extent1[i] = id;
            area1[id] += cellArea;
            volume1[id] += (level1 - z) * cellArea;
            ++extentCells1;
        }

        if(z < level2)
        {
            extent2[i] = id;
            area2[id] += cellArea;
            volume2[id] += (level2 - z) * cellArea;
            ++extentCells2;
        }
    }

    cout << "+1 m extent cells: " << extentCells1 << endl;
    cout << "+2 m extent cells: " << extentCells2 << endl;

    const string stats1Filename = outputFolder + "/damStats_1m.csv";
    const string stats2Filename = outputFolder + "/damStats_2m.csv";
    const string gpkg1Filename = outputFolder + "/damExtents_1m.gpkg";
    const string gpkg2Filename = outputFolder + "/damExtents_2m.gpkg";

    if(!writeStatsTable(stats1Filename, stats, hasStat, area1, volume1, 1.0) ||
       !writeStatsTable(stats2Filename, stats, hasStat, area2, volume2, 2.0))
    {
        GDALClose(damDS);
        GDALClose(routingDS);
        GDALClose(demDS);
        return 1;
    }

    if(!writeExtentGpkg(
           gpkg1Filename, "dam_extents_1m", extent1,
           width, height, demGT, demDS->GetProjectionRef(),
           stats, hasStat, area1, volume1, 1.0) ||
       !writeExtentGpkg(
           gpkg2Filename, "dam_extents_2m", extent2,
           width, height, demGT, demDS->GetProjectionRef(),
           stats, hasStat, area2, volume2, 2.0))
    {
        GDALClose(damDS);
        GDALClose(routingDS);
        GDALClose(demDS);
        return 1;
    }

    cout << "========== OUTPUT ==========" << endl;
    cout << stats1Filename << endl;
    cout << stats2Filename << endl;
    cout << gpkg1Filename << endl;
    cout << gpkg2Filename << endl;

    // Per-dam diagnostics, useful for spotting unexpected zero extents.
    for(int id=1; id<=maxID; ++id)
    {
        if(!hasStat[id]) continue;
        cout << "Dam " << id
             << ": 1m area=" << area1[id]
             << " m2 volume=" << volume1[id]
             << " m3; 2m area=" << area2[id]
             << " m2 volume=" << volume2[id] << " m3" << endl;
    }

    GDALClose(damDS);
    GDALClose(routingDS);
    GDALClose(demDS);

    return 0;
}
