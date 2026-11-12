// -----------------------------------------------------------------------------
// hydrology_diagnostics_snapshot.inc
// Include after flacc_DAG (and preferably after lakeflow) inside the daily loop.
// Writes monthly spatial snapshots on day 1 when enabled in the setup include.
// -----------------------------------------------------------------------------
{
if (hydroDiagWriteMonthlyMaps && day[index] == 1)
{
    std::vector<std::vector<float>> diagFmDsc(
        xSize, std::vector<float>(ySize, noData));
    std::vector<std::vector<float>> diagFmRouteableDsc(
        xSize, std::vector<float>(ySize, noData));
    std::vector<std::vector<float>> diagFmEhdSat(
        xSize, std::vector<float>(ySize, noData));
    std::vector<std::vector<float>> diagFmSdSat(
        xSize, std::vector<float>(ySize, noData));
    std::vector<std::vector<float>> diagFmDtG(
        xSize, std::vector<float>(ySize, noData));

    for (int x = 0; x < xSize; ++x)
    {
        for (int y = 0; y < ySize; ++y)
        {
            if (dem[x][y] == noData ||
                !(landuse[x][y] == 30 || landuse[x][y] == 60) ||
                isChannelCell(x, y) || lakes[x][y] != noData)
                continue;

            diagFmDsc[x][y] = dsc[x][y];
            diagFmRouteableDsc[x][y] =
                dsc[x][y] > dsc_max[x][y]
                ? dsc[x][y] - dsc_max[x][y] : 0.0f;

            const float ehdCapacity =
                SWSat_EHD[x][y] * EHDepth[x][y][1];
            if (ehdCapacity > 0.0f)
                diagFmEhdSat[x][y] = SW_EHD[x][y] / ehdCapacity;

            const float sdCapacity = SWSat[x][y] * SD[x][y];
            if (sdCapacity > 0.0f)
                diagFmSdSat[x][y] = SW[x][y][0] / sdCapacity;

            const float head = G[x][y][0];
            if (head == head && head > -1.0e30f && head < 1.0e30f)
                diagFmDtG[x][y] = dem[x][y] - head;
        }
    }

    const std::string diagDate =
        std::to_string(year[index]) + "_" +
        std::to_string(month[index]) + "_" +
        std::to_string(day[index]);

    saveGeoTIFF(
        ioFolder + "/diag_fm_dsc_mm_" + diagDate + ".tif",
        diagFmDsc, xSize, ySize, adfGeoTransform, INDEM->GetProjectionRef());
    saveGeoTIFF(
        ioFolder + "/diag_fm_routeable_dsc_mm_" + diagDate + ".tif",
        diagFmRouteableDsc, xSize, ySize, adfGeoTransform, INDEM->GetProjectionRef());
    saveGeoTIFF(
        ioFolder + "/diag_fm_ehd_saturation_" + diagDate + ".tif",
        diagFmEhdSat, xSize, ySize, adfGeoTransform, INDEM->GetProjectionRef());
    saveGeoTIFF(
        ioFolder + "/diag_fm_sd_saturation_" + diagDate + ".tif",
        diagFmSdSat, xSize, ySize, adfGeoTransform, INDEM->GetProjectionRef());
    saveGeoTIFF(
        ioFolder + "/diag_fm_dtg_m_" + diagDate + ".tif",
        diagFmDtG, xSize, ySize, adfGeoTransform, INDEM->GetProjectionRef());
}
}
