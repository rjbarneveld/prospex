// -----------------------------------------------------------------------------
// source_diagnostics_daily.inc
// Include once per day AFTER flacc_DAG_v2_source_diagnostic.inc.
// Writes daily source/store diagnostics and annual spatial source maps.
// -----------------------------------------------------------------------------

std::vector<double> srcDiagRetainedM3(srcDiagNSources, 0.0);
double srcDiagRetainedForestM3 = 0.0;
double srcDiagRetainedMireM3 = 0.0;
double srcDiagRetainedOtherLandM3 = 0.0;
double srcDiagRetainedChannelM3 = 0.0;

double srcDiagEHDForestM3 = 0.0;
double srcDiagEHDMireM3 = 0.0;
double srcDiagEHDOtherM3 = 0.0;
double srcDiagSDForestM3 = 0.0;
double srcDiagSDMireM3 = 0.0;
double srcDiagSDOtherM3 = 0.0;

double srcDiagDtGForestSum = 0.0;
double srcDiagDtGMireSum = 0.0;
double srcDiagDtGOtherSum = 0.0;
long long srcDiagDtGForestN = 0;
long long srcDiagDtGMireN = 0;
long long srcDiagDtGOtherN = 0;

double srcDiagTracerResidualM3 = 0.0;

for (int yy = 0; yy < ySize; ++yy)
{
    for (int xx = 0; xx < xSize; ++xx)
    {
        if (dem[xx][yy] == noData)
            continue;

        double traceSumMM = 0.0;
        for (int source = 0; source < srcDiagNSources; ++source)
        {
            const double sourceMM =
                srcDiagTracer[srcDiagTraceIndex(xx, yy, source)];
            traceSumMM += sourceMM;
            srcDiagRetainedM3[source] += sourceMM * dx * dy / 1000.0;
        }

        const double currentDscMM = dsc[xx][yy] > 0.0f ? dsc[xx][yy] : 0.0f;
        srcDiagTracerResidualM3 +=
            (currentDscMM - traceSumMM) * dx * dy / 1000.0;

        if (isChannelCell(xx, yy))
            srcDiagRetainedChannelM3 += currentDscMM * dx * dy / 1000.0;
        else if (lakes[xx][yy] == noData)
        {
            if (landuse[xx][yy] == 30)
                srcDiagRetainedForestM3 += currentDscMM * dx * dy / 1000.0;
            else if (landuse[xx][yy] == 60)
                srcDiagRetainedMireM3 += currentDscMM * dx * dy / 1000.0;
            else
                srcDiagRetainedOtherLandM3 += currentDscMM * dx * dy / 1000.0;

            const double ehdM3 = SW_EHD[xx][yy] * dx * dy / 1000.0;
            const double sdM3 = SW[xx][yy][0] * dx * dy / 1000.0;
            const double dtgM = dem[xx][yy] - G[xx][yy][0];

            if (landuse[xx][yy] == 30)
            {
                srcDiagEHDForestM3 += ehdM3;
                srcDiagSDForestM3 += sdM3;
                if (std::isfinite(dtgM))
                {
                    srcDiagDtGForestSum += dtgM;
                    ++srcDiagDtGForestN;
                }
            }
            else if (landuse[xx][yy] == 60)
            {
                srcDiagEHDMireM3 += ehdM3;
                srcDiagSDMireM3 += sdM3;
                if (std::isfinite(dtgM))
                {
                    srcDiagDtGMireSum += dtgM;
                    ++srcDiagDtGMireN;
                }
            }
            else
            {
                srcDiagEHDOtherM3 += ehdM3;
                srcDiagSDOtherM3 += sdM3;
                if (std::isfinite(dtgM))
                {
                    srcDiagDtGOtherSum += dtgM;
                    ++srcDiagDtGOtherN;
                }
            }
        }
    }
}

if (srcDiagDailyFile.is_open())
{
    srcDiagDailyFile
        << year[index] << "-" << month[index] << "-" << day[index] << " "
        << rr[index] << " " << snowMelt;

    for (int source = 0; source < srcDiagNSources; ++source)
        srcDiagDailyFile << " " << srcDiagDailyAddedM3[source];
    for (int source = 0; source < srcDiagNSources; ++source)
        srcDiagDailyFile << " " << srcDiagDailyRemovedM3[source];
    for (int source = 0; source < srcDiagNSources; ++source)
        srcDiagDailyFile << " " << srcDiagDailyReinfiltrationM3[source];
    for (int source = 0; source < srcDiagNSources; ++source)
        srcDiagDailyFile << " " << srcDiagDailySROM3[source];
    for (int source = 0; source < srcDiagNSources; ++source)
        srcDiagDailyFile << " " << srcDiagDailyLakeSinkM3[source];
    for (int source = 0; source < srcDiagNSources; ++source)
        srcDiagDailyFile << " " << srcDiagRetainedM3[source];

    srcDiagDailyFile
        << " " << srcDiagRetainedForestM3
        << " " << srcDiagRetainedMireM3
        << " " << srcDiagRetainedOtherLandM3
        << " " << srcDiagRetainedChannelM3
        << " " << srcDiagEHDForestM3
        << " " << srcDiagEHDMireM3
        << " " << srcDiagEHDOtherM3
        << " " << srcDiagSDForestM3
        << " " << srcDiagSDMireM3
        << " " << srcDiagSDOtherM3
        << " " << (srcDiagDtGForestN > 0 ? srcDiagDtGForestSum / srcDiagDtGForestN : 0.0)
        << " " << (srcDiagDtGMireN > 0 ? srcDiagDtGMireSum / srcDiagDtGMireN : 0.0)
        << " " << (srcDiagDtGOtherN > 0 ? srcDiagDtGOtherSum / srcDiagDtGOtherN : 0.0)
        << " " << Q_catchmentSRO
        << " " << Qd_cum
        << " " << Q_stream
        << " " << Q_lake
        << " " << srcDiagTracerResidualM3
        << "\n";
}

const bool srcDiagWriteAnnualMaps =
    (month[index] == 12 && day[index] == 31) || (index == nrDays - 1);

if (srcDiagWriteAnnualMaps)
{
    std::vector<std::vector<float> > srcDiagMap(
        xSize, std::vector<float>(ySize, noData));

    for (int source = 0; source < srcDiagNSources; ++source)
    {
        for (int xx = 0; xx < xSize; ++xx)
        {
            for (int yy = 0; yy < ySize; ++yy)
            {
                if (dem[xx][yy] == noData)
                {
                    srcDiagMap[xx][yy] = noData;
                    continue;
                }

                const std::size_t cell = srcDiagCellIndex(xx, yy);
                srcDiagMap[xx][yy] = srcDiagAnnualLocalAdded[
                    cell * static_cast<std::size_t>(srcDiagNSources)
                    + static_cast<std::size_t>(source)
                ];
            }
        }

        saveGeoTIFF(
            ioFolder + "/diag_local_add_" + srcDiagLabels[source] + "_" +
            std::to_string(year[index]) + "_" + gid + ".tif",
            srcDiagMap, xSize, ySize, adfGeoTransform,
            INDEM->GetProjectionRef()
        );

        for (int xx = 0; xx < xSize; ++xx)
        {
            for (int yy = 0; yy < ySize; ++yy)
            {
                if (dem[xx][yy] == noData)
                {
                    srcDiagMap[xx][yy] = noData;
                    continue;
                }

                const std::size_t cell = srcDiagCellIndex(xx, yy);
                srcDiagMap[xx][yy] = srcDiagAnnualSROCrossing[
                    cell * static_cast<std::size_t>(srcDiagNSources)
                    + static_cast<std::size_t>(source)
                ];
            }
        }

        saveGeoTIFF(
            ioFolder + "/diag_sro_crossing_" + srcDiagLabels[source] + "_" +
            std::to_string(year[index]) + "_" + gid + ".tif",
            srcDiagMap, xSize, ySize, adfGeoTransform,
            INDEM->GetProjectionRef()
        );
    }

    std::fill(srcDiagAnnualLocalAdded.begin(), srcDiagAnnualLocalAdded.end(), 0.0f);
    std::fill(srcDiagAnnualSROCrossing.begin(), srcDiagAnnualSROCrossing.end(), 0.0f);
}
