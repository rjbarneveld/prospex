{
    /*
       validate_inputs.inc

       Include after read_config.inc and GDALAllRegister(), but before opening
       or reading the DEM and before declare_arrays.inc.

       This validator assumes that all model rasters are on exactly the same
       grid as the DEM and that the current model uses one common NoData value.
    */

    int validationErrors = 0;
    int validationWarnings = 0;

    const double geoTolerance = 1.0e-7;
    const double valueTolerance = 1.0e-5;
    const bool scanRasterValues = true;

    auto fail = [&](const std::string& message)
    {
        std::cerr << "[INPUT ERROR] " << message << std::endl;
        ++validationErrors;
    };

    auto warn = [&](const std::string& message)
    {
        std::cerr << "[INPUT WARNING] " << message << std::endl;
        ++validationWarnings;
    };

    auto info = [&](const std::string& message)
    {
        std::cout << "[INPUT OK] " << message << std::endl;
    };

    auto absDouble = [](double v) -> double
    {
        return v < 0.0 ? -v : v;
    };

    // Portable replacements for CPLIsNan/CPLIsInf. Some GDAL/CPL
    // installations do not expose those macros in C++ builds.
    auto isNanValue = [](double v) -> bool
    {
        return v != v;
    };

    auto isInfValue = [&](double v) -> bool
    {
        if(isNanValue(v))
            return false;

        const double largestFiniteDouble = 1.7976931348623157e308;
        return v > largestFiniteDouble || v < -largestFiniteDouble;
    };

    auto nearlyEqual = [&](double a, double b, double tolerance) -> bool
    {
        const double scaleA = absDouble(a);
        const double scaleB = absDouble(b);
        const double scale = (scaleA > scaleB ? scaleA : scaleB);
        return absDouble(a - b) <= tolerance * (1.0 + scale);
    };

    auto pathExists = [](const std::string& path) -> bool
    {
        VSIStatBufL statBuffer;
        return !path.empty() && VSIStatL(path.c_str(), &statBuffer) == 0;
    };

    auto fileIsNonEmpty = [](const std::string& path) -> bool
    {
        VSIStatBufL statBuffer;
        return !path.empty() &&
               VSIStatL(path.c_str(), &statBuffer) == 0 &&
               VSI_ISREG(statBuffer.st_mode) &&
               statBuffer.st_size > 0;
    };

    auto joinPath = [](const std::string& folder,
                       const std::string& name) -> std::string
    {
        if(folder.empty())
            return name;
        if(name.empty())
            return name;
        if(name[0] == '/' ||
           (name.size() > 1 && name[1] == ':'))
            return name;
        if(folder[folder.size() - 1] == '/')
            return folder + name;
        return folder + "/" + name;
    };

    // Validate a configured path without changing the configuration variable.
    // The legacy read_tables.inc expects table names such as params.config and
    // prepends ioFolder itself. Mutating filename_PARAMS to
    // ioFolder/params.config here would therefore cause a doubled path later.
    auto resolveFile = [&](const std::string& path,
                           const std::string& label,
                           bool required) -> bool
    {
        if(path.empty())
        {
            if(required)
                fail(label + " path is empty.");
            return false;
        }

        if(fileIsNonEmpty(path))
        {
            info(label + ": " + path);
            return true;
        }

        const std::string alternative = joinPath(ioFolder, path);
        if(alternative != path && fileIsNonEmpty(alternative))
        {
            info(label + ": " + alternative +
                 " (resolved relative to ioFolder; configured value retained as " +
                 path + ")");
            return true;
        }

        if(required)
            fail(label + " does not exist or is empty: " + path);
        else
            warn(label + " does not exist or is empty: " + path);

        return false;
    };

    std::cout << "\n========== INPUT VALIDATION ==========" << std::endl;

    if(ioFolder.empty())
    {
        fail("ioFolder is empty.");
    }
    else
    {
        VSIStatBufL folderStat;
        if(VSIStatL(ioFolder.c_str(), &folderStat) != 0 ||
           !VSI_ISDIR(folderStat.st_mode))
        {
            fail("Output/input folder does not exist or is not a directory: " +
                 ioFolder);
        }
        else
        {
            const std::string testFile =
                joinPath(ioFolder, ".prospex_write_test.tmp");
            VSILFILE* testHandle = VSIFOpenL(testFile.c_str(), "wb");
            if(testHandle == nullptr)
            {
                fail("Folder is not writable: " + ioFolder);
            }
            else
            {
                const char marker = '1';
                VSIFWriteL(&marker, 1, 1, testHandle);
                VSIFCloseL(testHandle);
                VSIUnlink(testFile.c_str());
                info("Folder exists and is writable: " + ioFolder);
            }
        }
    }

    if(nrDays <= 0)
        fail("nrDays must be greater than zero.");

    if(dynamicG != 0 && dynamicG != 1)
        fail("dynamicG must be 0 or 1.");
    if(riverQ != 0 && riverQ != 1)
        fail("riverQ must be 0 or 1.");
    if(erosion != 0 && erosion != 1)
        fail("erosion must be 0 or 1.");
    if(simulateDams != 0 && simulateDams != 1)
        fail("simulateDams must be 0 or 1.");

    resolveFile(filename_CLIMA, "Climate table", true);
    resolveFile(filename_PARAMS, "Parameter table", true);
    resolveFile(filename_LAKES, "Lake table", true);
    resolveFile(filename_LUPOINTS, "Land-use points table", true);
    resolveFile(filename_GROUNDWATERCONFIG,
                "Groundwater configuration table", dynamicG == 1);

    if(simulateDams == 1)
        resolveFile(filename_DAMS, "Dam table", true);

    resolveFile(file_DEM, "DEM raster", true);
    resolveFile(file_TEXT, "Texture raster", true);
    resolveFile(file_LU, "Land-use raster", true);
    resolveFile(file_VEGETATION, "Vegetation raster", true);
    resolveFile(file_CHANNEL, "Channel raster", true);
    resolveFile(file_LAKES, "Lake raster", true);
    resolveFile(file_GROUNDWATER, "Groundwater raster", true);
    resolveFile(file_DRAINROUTE, "Drain-route raster", true);
    resolveFile(filename_HYDROGRAPH, "Hydrograph point file", true);

    const std::string dummyMeasures = joinPath(ioFolder, "dummy.tif");
    const bool measuresAreDummy =
        file_MEASURES.empty() || file_MEASURES == dummyMeasures;

    if(!measuresAreDummy)
        resolveFile(file_MEASURES, "Measures raster", true);
    else
        info("Measures raster disabled (dummy.tif).");

    if(simulateDams == 1)
        resolveFile(file_DAMS, "Dam raster", true);

    int demXSize = 0;
    int demYSize = 0;
    int demBandCount = 0;
    double demGeoTransform[6] = {0, 0, 0, 0, 0, 0};
    std::string demProjection;
    double demNoData = 0.0;
    bool demHasNoData = false;

    GDALDataset* demDataset = nullptr;

    if(fileIsNonEmpty(file_DEM))
    {
        demDataset = static_cast<GDALDataset*>(
            GDALOpen(file_DEM.c_str(), GA_ReadOnly));

        if(demDataset == nullptr)
        {
            fail("GDAL cannot open DEM: " + file_DEM);
        }
        else
        {
            demXSize = demDataset->GetRasterXSize();
            demYSize = demDataset->GetRasterYSize();
            demBandCount = demDataset->GetRasterCount();

            if(demXSize <= 0 || demYSize <= 0)
                fail("DEM has invalid dimensions.");
            if(demBandCount < 1)
                fail("DEM has no raster bands.");

            if(demDataset->GetGeoTransform(demGeoTransform) != CE_None)
                fail("DEM has no valid geotransform.");

            const char* projection = demDataset->GetProjectionRef();
            if(projection == nullptr || projection[0] == '\0')
                fail("DEM has no projection/CRS.");
            else
                demProjection = projection;

            if(demBandCount >= 1)
            {
                int success = FALSE;
                demNoData =
                    demDataset->GetRasterBand(1)->GetNoDataValue(&success);
                demHasNoData = success != FALSE;
                if(!demHasNoData)
                    fail("DEM band 1 has no declared NoData value.");
            }

            std::ostringstream message;
            message << "DEM grid is " << demXSize << " x " << demYSize
                    << ", bands=" << demBandCount;
            if(demHasNoData)
                message << ", NoData=" << std::setprecision(15)
                        << demNoData;
            info(message.str());
        }
    }

    auto sameProjection = [&](const std::string& candidateWkt) -> bool
    {
        if(demProjection.empty() || candidateWkt.empty())
            return false;

        OGRSpatialReference demSrs;
        OGRSpatialReference candidateSrs;

        if(demSrs.SetFromUserInput(demProjection.c_str()) != OGRERR_NONE ||
           candidateSrs.SetFromUserInput(candidateWkt.c_str()) != OGRERR_NONE)
        {
            return demProjection == candidateWkt;
        }

        return demSrs.IsSame(&candidateSrs) != FALSE;
    };

    auto valueIsNoData = [&](double value,
                             bool hasNoData,
                             double noDataValue) -> bool
    {
        if(!hasNoData)
            return false;
        if(isNanValue(noDataValue))
            return isNanValue(value);
        return nearlyEqual(value, noDataValue, valueTolerance);
    };

    auto scanSingleBand = [&](GDALDataset* dataset,
                              const std::string& label,
                              int bandNumber,
                              bool hasNoData,
                              double noDataValue)
    {
        if(!scanRasterValues)
            return;

        GDALRasterBand* band = dataset->GetRasterBand(bandNumber);
        std::vector<double> row(
            static_cast<std::size_t>(dataset->GetRasterXSize()), 0.0);

        long long validCount = 0;
        long long invalidNumberCount = 0;

        for(int y = 0; y < dataset->GetRasterYSize(); ++y)
        {
            if(band->RasterIO(GF_Read,
                              0, y,
                              dataset->GetRasterXSize(), 1,
                              row.data(),
                              dataset->GetRasterXSize(), 1,
                              GDT_Float64,
                              0, 0) != CE_None)
            {
                fail(label + " could not be read at row " +
                     std::to_string(y) + ".");
                return;
            }

            for(int x = 0; x < dataset->GetRasterXSize(); ++x)
            {
                const double value = row[x];
                if(valueIsNoData(value, hasNoData, noDataValue))
                    continue;
                if(isNanValue(value) || isInfValue(value))
                    ++invalidNumberCount;
                else
                    ++validCount;
            }
        }

        if(validCount == 0 && label != "Lake raster")
            fail(label + " contains no valid cells in band " +
                 std::to_string(bandNumber) + ".");

        if(invalidNumberCount > 0)
            fail(label + " contains " +
                 std::to_string(invalidNumberCount) +
                 " NaN/Inf values not marked as NoData in band " +
                 std::to_string(bandNumber) + ".");
    };

    auto validateRaster = [&](const std::string& path,
                              const std::string& label,
                              int minimumBands,
                              int maximumBands,
                              bool requireCommonNoData,
                              bool scanValues) -> bool
    {
        if(!fileIsNonEmpty(path))
            return false;

        GDALDataset* dataset = static_cast<GDALDataset*>(
            GDALOpen(path.c_str(), GA_ReadOnly));

        if(dataset == nullptr)
        {
            fail("GDAL cannot open " + label + ": " + path);
            return false;
        }

        bool ok = true;
        const int xSizeLocal = dataset->GetRasterXSize();
        const int ySizeLocal = dataset->GetRasterYSize();
        const int bandCount = dataset->GetRasterCount();

        if(demDataset != nullptr &&
           (xSizeLocal != demXSize || ySizeLocal != demYSize))
        {
            fail(label + " dimensions " +
                 std::to_string(xSizeLocal) + " x " +
                 std::to_string(ySizeLocal) +
                 " do not match DEM dimensions " +
                 std::to_string(demXSize) + " x " +
                 std::to_string(demYSize) + ".");
            ok = false;
        }

        if(bandCount < minimumBands ||
           (maximumBands > 0 && bandCount > maximumBands))
        {
            std::ostringstream message;
            message << label << " has " << bandCount << " band(s); expected ";
            if(minimumBands == maximumBands)
                message << minimumBands;
            else
                message << minimumBands << " to " << maximumBands;
            message << ".";
            fail(message.str());
            ok = false;
        }

        double localGeoTransform[6] = {0, 0, 0, 0, 0, 0};
        if(dataset->GetGeoTransform(localGeoTransform) != CE_None)
        {
            fail(label + " has no valid geotransform.");
            ok = false;
        }
        else if(demDataset != nullptr)
        {
            for(int i = 0; i < 6; ++i)
            {
                if(!nearlyEqual(localGeoTransform[i],
                                demGeoTransform[i],
                                geoTolerance))
                {
                    fail(label + " geotransform differs from DEM at element " +
                         std::to_string(i) + ".");
                    ok = false;
                    break;
                }
            }
        }

        const char* projection = dataset->GetProjectionRef();
        const std::string localProjection =
            projection == nullptr ? std::string() : std::string(projection);

        if(localProjection.empty())
        {
            fail(label + " has no projection/CRS.");
            ok = false;
        }
        else if(demDataset != nullptr &&
                !sameProjection(localProjection))
        {
            fail(label + " projection/CRS does not match the DEM.");
            ok = false;
        }

        const int bandsToCheck =
            bandCount < minimumBands ? bandCount : minimumBands;

        for(int bandIndex = 1; bandIndex <= bandsToCheck; ++bandIndex)
        {
            GDALRasterBand* band = dataset->GetRasterBand(bandIndex);
            if(band == nullptr)
            {
                fail(label + " band " + std::to_string(bandIndex) +
                     " cannot be accessed.");
                ok = false;
                continue;
            }

            int success = FALSE;
            const double localNoData = band->GetNoDataValue(&success);
            const bool hasLocalNoData = success != FALSE;

            if(requireCommonNoData)
            {
                if(!hasLocalNoData)
                {
                    fail(label + " band " +
                         std::to_string(bandIndex) +
                         " has no declared NoData value.");
                    ok = false;
                }
                else if(demHasNoData &&
                        !nearlyEqual(localNoData,
                                     demNoData,
                                     valueTolerance))
                {
                    std::ostringstream message;
                    message << label << " band " << bandIndex
                            << " NoData=" << std::setprecision(15)
                            << localNoData
                            << " differs from DEM NoData="
                            << demNoData << ".";
                    fail(message.str());
                    ok = false;
                }
            }

            if(scanValues)
                scanSingleBand(dataset,
                               label,
                               bandIndex,
                               hasLocalNoData,
                               localNoData);

            /*
               A model input may be sparse inside the catchment (for example
               channels or lakes), but it may never contain active values
               outside the valid DEM mask.
            */
            if(demDataset != nullptr &&
               path != file_DEM &&
               xSizeLocal == demXSize &&
               ySizeLocal == demYSize &&
               demHasNoData &&
               hasLocalNoData)
            {
                GDALRasterBand* demBand = demDataset->GetRasterBand(1);
                std::vector<double> demRow(
                    static_cast<std::size_t>(demXSize), 0.0);
                std::vector<double> inputRow(
                    static_cast<std::size_t>(demXSize), 0.0);

                long long valuesOutsideDem = 0;
                int firstOutsideX = -1;
                int firstOutsideY = -1;

                for(int maskY = 0; maskY < demYSize; ++maskY)
                {
                    const CPLErr demReadError = demBand->RasterIO(
                        GF_Read, 0, maskY, demXSize, 1,
                        demRow.data(), demXSize, 1,
                        GDT_Float64, 0, 0);
                    const CPLErr inputReadError = band->RasterIO(
                        GF_Read, 0, maskY, demXSize, 1,
                        inputRow.data(), demXSize, 1,
                        GDT_Float64, 0, 0);

                    if(demReadError != CE_None ||
                       inputReadError != CE_None)
                    {
                        fail(label +
                             " mask comparison failed at row " +
                             std::to_string(maskY) + ".");
                        ok = false;
                        break;
                    }

                    for(int maskX = 0; maskX < demXSize; ++maskX)
                    {
                        const bool outsideCatchment =
                            valueIsNoData(demRow[maskX],
                                          true, demNoData);
                        const bool inputIsNoData =
                            valueIsNoData(inputRow[maskX],
                                          true, localNoData);

                        if(outsideCatchment && !inputIsNoData)
                        {
                            if(firstOutsideX < 0)
                            {
                                firstOutsideX = maskX;
                                firstOutsideY = maskY;
                            }
                            ++valuesOutsideDem;
                        }
                    }
                }

                if(valuesOutsideDem > 0)
                {
                    fail(label + " band " +
                         std::to_string(bandIndex) + " contains " +
                         std::to_string(valuesOutsideDem) +
                         " non-NoData cells outside the DEM mask; first at " +
                         std::to_string(firstOutsideX) + ":" +
                         std::to_string(firstOutsideY) + ".");
                    ok = false;
                }
            }
        }

        if(ok)
        {
            info(label + " grid, CRS, bands and NoData are valid.");
        }

        GDALClose(dataset);
        return ok;
    };

    if(demDataset != nullptr)
    {
        validateRaster(file_DEM,
                       "DEM raster",
                       1, 1,
                       true,
                       true);
        validateRaster(file_TEXT,
                       "Texture raster",
                       1, 1,
                       true,
                       true);
        validateRaster(file_LU,
                       "Land-use raster",
                       1, 1,
                       true,
                       true);
        validateRaster(file_VEGETATION,
                       "Vegetation raster",
                       1, 1,
                       true,
                       true);
        validateRaster(file_CHANNEL,
                       "Channel raster",
                       1, 1,
                       true,
                       true);
        validateRaster(file_LAKES,
                       "Lake raster",
                       1, 1,
                       true,
                       true);
        validateRaster(file_GROUNDWATER,
                       "Groundwater raster",
                       1, 1,
                       true,
                       true);

        if(!measuresAreDummy)
        {
            validateRaster(file_MEASURES,
                           "Measures raster",
                           1, 1,
                           true,
                           true);
        }

        if(simulateDams == 1)
        {
            validateRaster(file_DAMS,
                           "Dam raster",
                           1, 1,
                           true,
                           true);
        }

        if(validateRaster(file_DRAINROUTE,
                          "Drain-route raster",
                          2, 2,
                          true,
                          false))
        {
            GDALDataset* routeDataset = static_cast<GDALDataset*>(
                GDALOpen(file_DRAINROUTE.c_str(), GA_ReadOnly));

            if(routeDataset != nullptr)
            {
                GDALRasterBand* bandX = routeDataset->GetRasterBand(1);
                GDALRasterBand* bandY = routeDataset->GetRasterBand(2);

                if(bandX == nullptr || bandY == nullptr)
                {
                    fail("Drain-route raster bands cannot be accessed after reopening.");
                    GDALClose(routeDataset);
                    routeDataset = nullptr;
                }

                if(routeDataset != nullptr)
                {
                int successX = FALSE;
                int successY = FALSE;
                const double noDataX = bandX->GetNoDataValue(&successX);
                const double noDataY = bandY->GetNoDataValue(&successY);

                std::vector<double> rowX(
                    static_cast<std::size_t>(demXSize), 0.0);
                std::vector<double> rowY(
                    static_cast<std::size_t>(demXSize), 0.0);

                long long validRoutes = 0;
                long long invalidPairs = 0;
                long long invalidDestinations = 0;
                long long nonIntegerDestinations = 0;

                for(int y = 0; y < demYSize; ++y)
                {
                    const CPLErr errX = bandX->RasterIO(
                        GF_Read, 0, y, demXSize, 1,
                        rowX.data(), demXSize, 1,
                        GDT_Float64, 0, 0);
                    const CPLErr errY = bandY->RasterIO(
                        GF_Read, 0, y, demXSize, 1,
                        rowY.data(), demXSize, 1,
                        GDT_Float64, 0, 0);

                    if(errX != CE_None || errY != CE_None)
                    {
                        fail("Drain-route raster could not be read at row " +
                             std::to_string(y) + ".");
                        break;
                    }

                    for(int x = 0; x < demXSize; ++x)
                    {
                        const bool xIsNoData =
                            valueIsNoData(rowX[x], successX != FALSE, noDataX);
                        const bool yIsNoData =
                            valueIsNoData(rowY[x], successY != FALSE, noDataY);

                        if(xIsNoData && yIsNoData)
                            continue;

                        if(xIsNoData != yIsNoData ||
                           isNanValue(rowX[x]) || isInfValue(rowX[x]) ||
                           isNanValue(rowY[x]) || isInfValue(rowY[x]))
                        {
                            ++invalidPairs;
                            continue;
                        }

                        const int destinationX =
                            static_cast<int>(rowX[x] + 0.5);
                        const int destinationY =
                            static_cast<int>(rowY[x] + 0.5);

                        if(absDouble(rowX[x] - destinationX) > valueTolerance ||
                           absDouble(rowY[x] - destinationY) > valueTolerance)
                        {
                            ++nonIntegerDestinations;
                            continue;
                        }

                        if(destinationX < 0 || destinationX >= demXSize ||
                           destinationY < 0 || destinationY >= demYSize)
                        {
                            ++invalidDestinations;
                            continue;
                        }

                        ++validRoutes;
                    }
                }

                if(validRoutes == 0)
                    fail("Drain-route raster contains no valid route destinations.");
                if(invalidPairs > 0)
                    fail("Drain-route raster contains " +
                         std::to_string(invalidPairs) +
                         " cells where X/Y are inconsistently NoData or invalid.");
                if(nonIntegerDestinations > 0)
                    fail("Drain-route raster contains " +
                         std::to_string(nonIntegerDestinations) +
                         " non-integer route destinations.");
                if(invalidDestinations > 0)
                    fail("Drain-route raster contains " +
                         std::to_string(invalidDestinations) +
                         " destinations outside the raster extent.");

                if(validRoutes > 0 &&
                   invalidPairs == 0 &&
                   nonIntegerDestinations == 0 &&
                   invalidDestinations == 0)
                {
                    info("Drain-route destinations are paired, integer and in range (" +
                         std::to_string(validRoutes) + " valid routes).");
                }

                GDALClose(routeDataset);
                routeDataset = nullptr;
                }
            }
            else
            {
                fail("GDAL cannot reopen drain-route raster for destination checks: " +
                     file_DRAINROUTE);
            }
        }
    }

    if(demDataset != nullptr)
    {
        GDALClose(demDataset);
        demDataset = nullptr;
    }

    if(fileIsNonEmpty(filename_HYDROGRAPH))
    {
        GDALDataset* vectorDataset = static_cast<GDALDataset*>(
            GDALOpenEx(filename_HYDROGRAPH.c_str(),
                       GDAL_OF_VECTOR | GDAL_OF_READONLY,
                       nullptr, nullptr, nullptr));

        if(vectorDataset == nullptr)
        {
            fail("GDAL/OGR cannot open hydrograph point file: " +
                 filename_HYDROGRAPH);
        }
        else
        {
            const int layerCount = vectorDataset->GetLayerCount();
            if(layerCount < 1)
            {
                fail("Hydrograph point file has no layers.");
            }
            else
            {
                OGRLayer* layer = vectorDataset->GetLayer(0);
                if(layer == nullptr)
                {
                    fail("Hydrograph point file reports a layer, but layer 0 "
                         "cannot be opened.");
                }
                else
                {
                    /*
                       Do not call GetFeatureCount(TRUE) here. TRUE forces the
                       driver to execute a complete count operation, and some
                       damaged/partially written GeoPackages have been observed
                       to fail inside the SQLite/GPKG driver. Reading the first
                       feature is sufficient to establish that the layer is
                       non-empty and also lets us inspect the actual geometry.
                    */
                    layer->ResetReading();
                    OGRFeature* firstFeature = layer->GetNextFeature();

                    if(firstFeature == nullptr)
                    {
                        fail("Hydrograph point layer contains no readable features.");
                    }
                    else
                    {
                        OGRGeometry* geometry = firstFeature->GetGeometryRef();
                        if(geometry == nullptr)
                        {
                            fail("The first hydrograph feature has no geometry.");
                        }
                        else
                        {
                            const OGRwkbGeometryType geometryType =
                                wkbFlatten(geometry->getGeometryType());

                            if(geometryType != wkbPoint &&
                               geometryType != wkbMultiPoint)
                            {
                                fail("Hydrograph layer contains non-point geometry.");
                            }
                            else
                            {
                                // Do not request or print a feature count here.
                                // Some driver/build combinations have returned
                                // nonsensical count values even though feature
                                // iteration itself works correctly.
                                info("Hydrograph point file is readable and contains "
                                     "at least one point feature.");
                            }
                        }

                        OGRFeature::DestroyFeature(firstFeature);
                    }
                }
            }

            GDALClose(vectorDataset);
            vectorDataset = nullptr;
        }
    }

    std::cout << "========== VALIDATION SUMMARY ==========" << std::endl;
    std::cout << "Errors: " << validationErrors
              << ", warnings: " << validationWarnings << std::endl;

    if(validationErrors > 0)
    {
        std::cerr << "Input validation failed. Model execution stopped."
                  << std::endl;
        return 1;
    }

    std::cout << "All required inputs passed validation.\n" << std::endl;
}
