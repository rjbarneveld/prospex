// -----------------------------------------------------------------------------
// source_diagnostics_setup.inc
// Include ONCE after output files are opened and before the daily loop.
// Diagnostic only: does not change dsc or any hydrological state.
//
// Sources traced through dsc and the DAG:
//   0 carry-over / initial dsc
//   1 update_soil_layers additions
//   2 lateral_subsurface additions
//   3 soil_waterbalance additions
//   4 lakeflow additions
// -----------------------------------------------------------------------------

const int srcDiagCarry = 0;
const int srcDiagUpdate = 1;
const int srcDiagLateral = 2;
const int srcDiagSoil = 3;
const int srcDiagLake = 4;
const int srcDiagNSources = 5;

const char* srcDiagLabels[srcDiagNSources] = {
    "carry", "gw_update", "lateral", "soil", "lake"
};

const std::size_t srcDiagNCells =
    static_cast<std::size_t>(xSize) * static_cast<std::size_t>(ySize);

std::vector<float> srcDiagTracer(
    srcDiagNCells * static_cast<std::size_t>(srcDiagNSources), 0.0f);
std::vector<float> srcDiagBeforeDsc(srcDiagNCells, 0.0f);
std::vector<float> srcDiagOutgoingTracer(
    srcDiagNCells * static_cast<std::size_t>(srcDiagNSources), 0.0f);

// Spatial accumulators [mm over the source cell] for annual maps.
std::vector<float> srcDiagAnnualLocalAdded(
    srcDiagNCells * static_cast<std::size_t>(srcDiagNSources), 0.0f);
std::vector<float> srcDiagAnnualSROCrossing(
    srcDiagNCells * static_cast<std::size_t>(srcDiagNSources), 0.0f);

std::vector<double> srcDiagDailyAddedM3(srcDiagNSources, 0.0);
std::vector<double> srcDiagDailyRemovedM3(srcDiagNSources, 0.0);
std::vector<double> srcDiagDailyReinfiltrationM3(srcDiagNSources, 0.0);
std::vector<double> srcDiagDailySROM3(srcDiagNSources, 0.0);
std::vector<double> srcDiagDailyLakeSinkM3(srcDiagNSources, 0.0);
std::vector<double> srcDiagDailyDamSinkM3(srcDiagNSources, 0.0);

bool srcDiagInitialised = false;

std::ofstream srcDiagDailyFile;
srcDiagDailyFile.open(
    ioFolder + "/source_diagnostic_" +
    std::to_string(scenario) + "_" + gid + ".csv"
);

if (srcDiagDailyFile.is_open())
{
    srcDiagDailyFile
        << "date rr_mm snowmelt_mm "
        << "add_carry_m3 add_update_m3 add_lateral_m3 add_soil_m3 add_lake_m3 "
        << "remove_carry_m3 remove_update_m3 remove_lateral_m3 remove_soil_m3 remove_lake_m3 "
        << "reinfil_carry_m3 reinfil_update_m3 reinfil_lateral_m3 reinfil_soil_m3 reinfil_lake_m3 "
        << "sro_carry_m3 sro_update_m3 sro_lateral_m3 sro_soil_m3 sro_lake_m3 "
        << "lake_sink_carry_m3 lake_sink_update_m3 lake_sink_lateral_m3 lake_sink_soil_m3 lake_sink_lake_m3 "
        << "retained_carry_m3 retained_update_m3 retained_lateral_m3 retained_soil_m3 retained_lake_m3 "
        << "retained_forest_m3 retained_mire_m3 retained_other_land_m3 retained_channel_m3 "
        << "ehd_forest_m3 ehd_mire_m3 ehd_other_land_m3 "
        << "sd_forest_m3 sd_mire_m3 sd_other_land_m3 "
        << "mean_dtg_forest_m mean_dtg_mire_m mean_dtg_other_m "
        << "Q_catchmentSRO_m3d Qd_cum_m3d Q_stream_m3d Q_lake_m3d "
        << "tracer_residual_m3\n";
}

std::ofstream srcDiagHydroFile;
srcDiagHydroFile.open(
    ioFolder + "/hydrograph_source_diagnostic_" +
    std::to_string(scenario) + "_" + gid + ".csv"
);

if (srcDiagHydroFile.is_open())
{
    srcDiagHydroFile
        << "date location total_m3d carry_m3d gw_update_m3d lateral_m3d soil_m3d lake_m3d residual_m3d\n";
}

auto srcDiagCellIndex = [&](int xx, int yy) -> std::size_t
{
    return static_cast<std::size_t>(yy) * static_cast<std::size_t>(xSize)
         + static_cast<std::size_t>(xx);
};

auto srcDiagTraceIndex = [&](int xx, int yy, int source) -> std::size_t
{
    return srcDiagCellIndex(xx, yy) * static_cast<std::size_t>(srcDiagNSources)
         + static_cast<std::size_t>(source);
};

auto srcDiagResetDaily = [&]()
{
    std::fill(srcDiagDailyAddedM3.begin(), srcDiagDailyAddedM3.end(), 0.0);
    std::fill(srcDiagDailyRemovedM3.begin(), srcDiagDailyRemovedM3.end(), 0.0);
    std::fill(srcDiagDailyReinfiltrationM3.begin(), srcDiagDailyReinfiltrationM3.end(), 0.0);
    std::fill(srcDiagDailySROM3.begin(), srcDiagDailySROM3.end(), 0.0);
    std::fill(srcDiagDailyLakeSinkM3.begin(), srcDiagDailyLakeSinkM3.end(), 0.0);
    std::fill(srcDiagDailyDamSinkM3.begin(), srcDiagDailyDamSinkM3.end(), 0.0);
    std::fill(srcDiagOutgoingTracer.begin(), srcDiagOutgoingTracer.end(), 0.0f);
};

auto srcDiagSynchroniseCell = [&](int xx, int yy, float targetDsc)
{
    if (targetDsc < 0.0f || !std::isfinite(targetDsc))
        targetDsc = 0.0f;

    double traceSum = 0.0;
    for (int source = 0; source < srcDiagNSources; ++source)
        traceSum += srcDiagTracer[srcDiagTraceIndex(xx, yy, source)];

    if (traceSum > 1.0e-12)
    {
        const float scale = static_cast<float>(targetDsc / traceSum);
        for (int source = 0; source < srcDiagNSources; ++source)
            srcDiagTracer[srcDiagTraceIndex(xx, yy, source)] *= scale;
    }
    else if (targetDsc > 0.0f)
    {
        srcDiagTracer[srcDiagTraceIndex(xx, yy, srcDiagCarry)] = targetDsc;
    }
};

auto srcDiagBeginDay = [&]()
{
    srcDiagResetDaily();

    for (int yy = 0; yy < ySize; ++yy)
    {
        for (int xx = 0; xx < xSize; ++xx)
        {
            if (dem[xx][yy] == noData)
                continue;

            const float currentDsc =
                (dsc[xx][yy] > 0.0f && std::isfinite(dsc[xx][yy]))
                ? dsc[xx][yy] : 0.0f;

            if (!srcDiagInitialised)
            {
                for (int source = 0; source < srcDiagNSources; ++source)
                    srcDiagTracer[srcDiagTraceIndex(xx, yy, source)] = 0.0f;

                srcDiagTracer[srcDiagTraceIndex(xx, yy, srcDiagCarry)] = currentDsc;
            }
            else
            {
                srcDiagSynchroniseCell(xx, yy, currentDsc);
            }
        }
    }

    srcDiagInitialised = true;
};

auto srcDiagSnapshotDsc = [&]()
{
    for (int yy = 0; yy < ySize; ++yy)
    {
        for (int xx = 0; xx < xSize; ++xx)
        {
            const std::size_t cell = srcDiagCellIndex(xx, yy);
            srcDiagBeforeDsc[cell] =
                (dem[xx][yy] != noData && dsc[xx][yy] > 0.0f && std::isfinite(dsc[xx][yy]))
                ? dsc[xx][yy] : 0.0f;
        }
    }
};

auto srcDiagReconcileStage = [&](int stageSource)
{
    const double cellAreaToM3 = dx * dy / 1000.0;

    for (int yy = 0; yy < ySize; ++yy)
    {
        for (int xx = 0; xx < xSize; ++xx)
        {
            if (dem[xx][yy] == noData)
                continue;

            const std::size_t cell = srcDiagCellIndex(xx, yy);
            const float oldDsc = srcDiagBeforeDsc[cell];
            const float newDsc =
                (dsc[xx][yy] > 0.0f && std::isfinite(dsc[xx][yy]))
                ? dsc[xx][yy] : 0.0f;

            srcDiagSynchroniseCell(xx, yy, oldDsc);

            if (newDsc > oldDsc + 1.0e-7f)
            {
                const float added = newDsc - oldDsc;
                srcDiagTracer[srcDiagTraceIndex(xx, yy, stageSource)] += added;
                srcDiagDailyAddedM3[stageSource] += added * cellAreaToM3;
                srcDiagAnnualLocalAdded[
                    cell * static_cast<std::size_t>(srcDiagNSources)
                    + static_cast<std::size_t>(stageSource)
                ] += added;
            }
            else if (newDsc + 1.0e-7f < oldDsc)
            {
                const float scale = (oldDsc > 0.0f) ? newDsc / oldDsc : 0.0f;

                for (int source = 0; source < srcDiagNSources; ++source)
                {
                    const std::size_t ti = srcDiagTraceIndex(xx, yy, source);
                    const float before = srcDiagTracer[ti];
                    const float after = before * scale;
                    srcDiagTracer[ti] = after;
                    srcDiagDailyRemovedM3[source] +=
                        static_cast<double>(before - after) * cellAreaToM3;
                }
            }

            srcDiagSynchroniseCell(xx, yy, newDsc);
        }
    }
};
