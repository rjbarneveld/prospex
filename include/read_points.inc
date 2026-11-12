#include <gdal_priv.h>
#include <ogrsf_frmts.h>
#include <iostream>
#include <vector>

struct PointWithFID {
    double x;
    double y;
    GIntBig fid;  // Feature ID
};

bool readGpkgPointsWithFID(const std::string& filepath,
                           std::vector<PointWithFID>& points)
{
    GDALAllRegister();

    GDALDataset* poDS = static_cast<GDALDataset*>(
        GDALOpenEx(filepath.c_str(),
                   GDAL_OF_VECTOR | GDAL_OF_READONLY,
                   nullptr, nullptr, nullptr));
    if (!poDS) {
        std::cerr << "Failed to open GeoPackage: " << filepath << "\n";
        return false;
    }

    OGRLayer* poLayer = poDS->GetLayer(0);
    if (!poLayer) {
        std::cerr << "No layers found.\n";
        GDALClose(poDS);
        return false;
    }

    poLayer->ResetReading();
    OGRFeature* poFeature = nullptr;

    while ((poFeature = poLayer->GetNextFeature()) != nullptr) {
        OGRGeometry* poGeometry = poFeature->GetGeometryRef();
        if (!poGeometry || wkbFlatten(poGeometry->getGeometryType()) != wkbPoint) {
            OGRFeature::DestroyFeature(poFeature);
            continue;
        }

        const OGRPoint* pt = poGeometry->toPoint();
        if (!pt) { OGRFeature::DestroyFeature(poFeature); continue; }

        points.push_back({pt->getX(), pt->getY(), poFeature->GetFID()});

        OGRFeature::DestroyFeature(poFeature);
    }

    GDALClose(poDS);
    return true;
}

