// -----------------------------------------------------------------------------
// hydrology_diagnostics_setup.inc
//
// Include once after the normal output files have been opened and before the
// daily simulation loop. It provides:
//
//   HydroDiagState hydroDiagCapture();
//   hydroDiagWrite("stage_name", hydroDiagCapture());
//
// The diagnostic file contains one row per requested stage. The purpose is to
// determine whether persistent runoff is introduced by the soil balance, the
// forest/mire module, routing, or retained surface/channel storage.
// -----------------------------------------------------------------------------

struct HydroDiagState
{
    double ehdFM_m3;
    double sdFM_m3;
    double dscFM_m3;
    double routeableDscFM_m3;
    double surplusFM_m3;
    double flaccFM_m3;

    double ehdOther_m3;
    double sdOther_m3;
    double dscOther_m3;
    double routeableDscOther_m3;
    double surplusOther_m3;
    double flaccOther_m3;

    double dscChannel_m3;
    double routeableDscChannel_m3;
    double flaccChannel_m3;
    double dscLake_m3;

    double fmWaterAboveFC_m3;
    double fmPoreDeficitEHD_m3;
    double fmPoreDeficitSD_m3;

    double meanFmEhdSaturation;
    double meanFmSdSaturation;
    double meanFmDtG_m;

    long long fmCells;
    long long otherLandCells;
    long long channelCells;
    long long lakeCells;

    long long fmEhdSat95Cells;
    long long fmSdAboveFCCells;
    long long fmSdSat95Cells;
    long long fmDscCells;
    long long fmRouteableDscCells;
    long long fmDtGLess0Cells;
    long long fmDtGLess05Cells;
    long long fmDtGLess1Cells;
    long long fmValidHeadCells;

    HydroDiagState()
        : ehdFM_m3(0.0), sdFM_m3(0.0), dscFM_m3(0.0),
          routeableDscFM_m3(0.0), surplusFM_m3(0.0), flaccFM_m3(0.0),
          ehdOther_m3(0.0), sdOther_m3(0.0), dscOther_m3(0.0),
          routeableDscOther_m3(0.0), surplusOther_m3(0.0), flaccOther_m3(0.0),
          dscChannel_m3(0.0), routeableDscChannel_m3(0.0),
          flaccChannel_m3(0.0), dscLake_m3(0.0),
          fmWaterAboveFC_m3(0.0), fmPoreDeficitEHD_m3(0.0),
          fmPoreDeficitSD_m3(0.0), meanFmEhdSaturation(0.0),
          meanFmSdSaturation(0.0), meanFmDtG_m(0.0),
          fmCells(0), otherLandCells(0), channelCells(0), lakeCells(0),
          fmEhdSat95Cells(0), fmSdAboveFCCells(0), fmSdSat95Cells(0),
          fmDscCells(0), fmRouteableDscCells(0), fmDtGLess0Cells(0),
          fmDtGLess05Cells(0), fmDtGLess1Cells(0), fmValidHeadCells(0)
    {}
};

const bool hydroDiagWriteMonthlyMaps = true;
const double hydroDiagMmToM3 = dx * dy / 1000.0;

std::ofstream hydroDiagFile;
hydroDiagFile.open(
    ioFolder + "/hydrology_stage_diagnostic_" +
    std::to_string(scenario) + "_" + gid + ".csv"
);

if (hydroDiagFile.is_open())
{
    hydroDiagFile
        << "date stage liquid_input_mm "
        << "ehd_fm_m3 sd_fm_m3 dsc_fm_m3 routeable_dsc_fm_m3 "
        << "surplus_fm_m3 flacc_fm_m3 "
        << "ehd_other_m3 sd_other_m3 dsc_other_m3 "
        << "routeable_dsc_other_m3 surplus_other_m3 flacc_other_m3 "
        << "dsc_channel_m3 routeable_dsc_channel_m3 flacc_channel_m3 "
        << "dsc_lake_m3 fm_above_fc_m3 fm_ehd_pore_deficit_m3 "
        << "fm_sd_pore_deficit_m3 mean_fm_ehd_sat mean_fm_sd_sat "
        << "mean_fm_dtg_m fm_cells other_land_cells channel_cells lake_cells "
        << "fm_ehd_sat95_cells fm_sd_above_fc_cells fm_sd_sat95_cells "
        << "fm_dsc_cells fm_routeable_dsc_cells fm_dtg_lt0_cells "
        << "fm_dtg_lt05_cells fm_dtg_lt1_cells "
        << "Qd_cum_m3d Q_catchmentSRO_m3d Q_stream_m3d Q_lake_m3d\n";
}
else
{
    std::cerr << "Could not open hydrology diagnostic file in "
              << ioFolder << std::endl;
}

const auto hydroDiagIsFM = [&](int xx, int yy) -> bool
{
    return landuse[xx][yy] == 30 || landuse[xx][yy] == 60;
};

const auto hydroDiagValidHead = [&](float value) -> bool
{
    return value == value && value > -1.0e30f && value < 1.0e30f;
};

const auto hydroDiagCapture = [&]() -> HydroDiagState
{
    HydroDiagState s;
    double sumFmEhdSat = 0.0;
    double sumFmSdSat = 0.0;
    long long countFmEhdSat = 0;
    long long countFmSdSat = 0;
    double sumFmDtG = 0.0;

    for (int x = 0; x < xSize; ++x)
    {
        for (int y = 0; y < ySize; ++y)
        {
            if (dem[x][y] == noData)
                continue;

            const double dscMM = dsc[x][y] > 0.0f
                ? static_cast<double>(dsc[x][y]) : 0.0;
            const double flaccMM = flaccMFD[x][y] > 0.0f
                ? static_cast<double>(flaccMFD[x][y]) : 0.0;
            const double routeableMM = dsc[x][y] > dsc_max[x][y]
                ? static_cast<double>(dsc[x][y] - dsc_max[x][y]) : 0.0;

            if (lakes[x][y] != noData)
            {
                ++s.lakeCells;
                s.dscLake_m3 += dscMM * hydroDiagMmToM3;
                continue;
            }

            if (isChannelCell(x, y))
            {
                ++s.channelCells;
                s.dscChannel_m3 += dscMM * hydroDiagMmToM3;
                s.routeableDscChannel_m3 += routeableMM * hydroDiagMmToM3;
                s.flaccChannel_m3 += flaccMM * hydroDiagMmToM3;
                continue;
            }

            const bool fm = hydroDiagIsFM(x, y);
            if (fm)
                ++s.fmCells;
            else
                ++s.otherLandCells;

            const double ehdDepthMM = EHDepth[x][y][1] > 0.0f
                ? static_cast<double>(EHDepth[x][y][1]) : 0.0;
            const double sdDepthMM = SD[x][y] > 0.0f
                ? static_cast<double>(SD[x][y]) : 0.0;
            const double ehdWaterMM = SW_EHD[x][y] > 0.0f
                ? static_cast<double>(SW_EHD[x][y]) : 0.0;
            const double sdWaterMM = SW[x][y][0] > 0.0f
                ? static_cast<double>(SW[x][y][0]) : 0.0;
            const double surplusMM = dailySurplus[x][y] > 0.0f
                ? static_cast<double>(dailySurplus[x][y]) : 0.0;

            const double ehdCapacityMM =
                static_cast<double>(SWSat_EHD[x][y]) * ehdDepthMM;
            const double sdCapacityMM =
                static_cast<double>(SWSat[x][y]) * sdDepthMM;
            const double sdFCMM =
                static_cast<double>(FC[x][y]) * sdDepthMM;

            if (fm)
            {
                s.ehdFM_m3 += ehdWaterMM * hydroDiagMmToM3;
                s.sdFM_m3 += sdWaterMM * hydroDiagMmToM3;
                s.dscFM_m3 += dscMM * hydroDiagMmToM3;
                s.routeableDscFM_m3 += routeableMM * hydroDiagMmToM3;
                s.surplusFM_m3 += surplusMM * hydroDiagMmToM3;
                s.flaccFM_m3 += flaccMM * hydroDiagMmToM3;

                if (sdWaterMM > sdFCMM)
                {
                    s.fmWaterAboveFC_m3 +=
                        (sdWaterMM - sdFCMM) * hydroDiagMmToM3;
                    ++s.fmSdAboveFCCells;
                }

                if (ehdCapacityMM > ehdWaterMM)
                    s.fmPoreDeficitEHD_m3 +=
                        (ehdCapacityMM - ehdWaterMM) * hydroDiagMmToM3;
                if (sdCapacityMM > sdWaterMM)
                    s.fmPoreDeficitSD_m3 +=
                        (sdCapacityMM - sdWaterMM) * hydroDiagMmToM3;

                if (ehdCapacityMM > 0.0)
                {
                    const double sat = ehdWaterMM / ehdCapacityMM;
                    sumFmEhdSat += sat;
                    ++countFmEhdSat;
                    if (sat >= 0.95)
                        ++s.fmEhdSat95Cells;
                }

                if (sdCapacityMM > 0.0)
                {
                    const double sat = sdWaterMM / sdCapacityMM;
                    sumFmSdSat += sat;
                    ++countFmSdSat;
                    if (sat >= 0.95)
                        ++s.fmSdSat95Cells;
                }

                if (dscMM > 0.0)
                    ++s.fmDscCells;
                if (routeableMM > 0.0)
                    ++s.fmRouteableDscCells;

                if (hydroDiagValidHead(G[x][y][0]))
                {
                    const double dtgM =
                        static_cast<double>(dem[x][y]) -
                        static_cast<double>(G[x][y][0]);
                    sumFmDtG += dtgM;
                    ++s.fmValidHeadCells;
                    if (dtgM < 0.0) ++s.fmDtGLess0Cells;
                    if (dtgM < 0.5) ++s.fmDtGLess05Cells;
                    if (dtgM < 1.0) ++s.fmDtGLess1Cells;
                }
            }
            else
            {
                s.ehdOther_m3 += ehdWaterMM * hydroDiagMmToM3;
                s.sdOther_m3 += sdWaterMM * hydroDiagMmToM3;
                s.dscOther_m3 += dscMM * hydroDiagMmToM3;
                s.routeableDscOther_m3 += routeableMM * hydroDiagMmToM3;
                s.surplusOther_m3 += surplusMM * hydroDiagMmToM3;
                s.flaccOther_m3 += flaccMM * hydroDiagMmToM3;
            }
        }
    }

    s.meanFmEhdSaturation = countFmEhdSat > 0
        ? sumFmEhdSat / static_cast<double>(countFmEhdSat) : 0.0;
    s.meanFmSdSaturation = countFmSdSat > 0
        ? sumFmSdSat / static_cast<double>(countFmSdSat) : 0.0;
    s.meanFmDtG_m = s.fmValidHeadCells > 0
        ? sumFmDtG / static_cast<double>(s.fmValidHeadCells) : 0.0;

    return s;
};

const auto hydroDiagWrite = [&](
    int diagIndex,
    float diagSnowMelt,
    const char* stage,
    const HydroDiagState& s)
{
    if (!hydroDiagFile.is_open())
        return;

    const double liquidInputMM =
        static_cast<double>((rr[diagIndex] + diagSnowMelt) > 0.0f
            ? (rr[diagIndex] + diagSnowMelt) : 0.0f);

    hydroDiagFile
        << year[diagIndex] << "-" << month[diagIndex] << "-"
        << day[diagIndex] << " "
        << stage << " " << liquidInputMM << " "
        << s.ehdFM_m3 << " " << s.sdFM_m3 << " "
        << s.dscFM_m3 << " " << s.routeableDscFM_m3 << " "
        << s.surplusFM_m3 << " " << s.flaccFM_m3 << " "
        << s.ehdOther_m3 << " " << s.sdOther_m3 << " "
        << s.dscOther_m3 << " " << s.routeableDscOther_m3 << " "
        << s.surplusOther_m3 << " " << s.flaccOther_m3 << " "
        << s.dscChannel_m3 << " " << s.routeableDscChannel_m3 << " "
        << s.flaccChannel_m3 << " " << s.dscLake_m3 << " "
        << s.fmWaterAboveFC_m3 << " " << s.fmPoreDeficitEHD_m3 << " "
        << s.fmPoreDeficitSD_m3 << " " << s.meanFmEhdSaturation << " "
        << s.meanFmSdSaturation << " " << s.meanFmDtG_m << " "
        << s.fmCells << " " << s.otherLandCells << " "
        << s.channelCells << " " << s.lakeCells << " "
        << s.fmEhdSat95Cells << " " << s.fmSdAboveFCCells << " "
        << s.fmSdSat95Cells << " " << s.fmDscCells << " "
        << s.fmRouteableDscCells << " " << s.fmDtGLess0Cells << " "
        << s.fmDtGLess05Cells << " " << s.fmDtGLess1Cells << " "
        << Qd_cum << " " << Q_catchmentSRO << " "
        << Q_stream << " " << Q_lake << "\n";
};
