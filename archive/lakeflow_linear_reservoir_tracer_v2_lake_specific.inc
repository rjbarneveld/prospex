// Daily lake water balance and delayed outflow for prospex-Hydro/Sedi.
//
// State convention:
//   lakeStats[i][0] : lake surface area [m2]
//   lakeStats[i][1] : outlet raster x index
//   lakeStats[i][2] : outlet raster y index
//   lakeStats[i][3] : lake/outlet elevation or active-lake flag (>0)
//   lakeStats[i][4] : routed inflow ledger [m3/day]
//   lakeStats[i][5] : daily outflow [m3/day]
//   lakeStats[i][6] : active storage above outlet [m3]
//   lakeStats[i][7] : water level relative to outlet [m]
//   lakeStats[i][8] : lake ID
//   lakeStats[i][9] : lake-specific residence time [days]
//   lakeStats[i][10]: lake-specific daily release fraction [-]
//
// Intended call order inside each day:
//   1. soil and groundwater routines
//   2. this lakeflow routine
//   3. flacc_DAG_v2_lake_storage_tracer_v2.inc
//
// Inflows collected by flacc_DAG today are stored and become available for
// release on the next day. This gives every lake at least one daily timestep
// of detention and prevents order-dependent same-day lake cascades.

const int lakeDiagnostic1 = 51;
const int lakeDiagnostic2 = 67;
const int lakeDiagnostic3 = 105;

// Fallback residence time [days], used only when a lake has neither a valid
// residence time nor a valid release fraction in lakeStats.
const float defaultLakeResidenceTimeDays = 1.0f;

// Direct open-water evaporation correction applied to Oudin PET.
const float lakeEvapFactor = 1.10f;

// Lakes explicitly representing export out of the model domain. Ordinary
// in-domain channel outlets are NOT terminal: their outflow is inserted into
// dsc and routed through the channel network to the hydrograph point.
auto isTerminalLakeID = [](int lakeID) -> bool
{
    return lakeID == 21 || lakeID == 22 || lakeID == 98 ||
           lakeID == 9  || lakeID == 82 || lakeID == 83;
};

auto lakeMakeDateString = [](int yy, int mm, int dd) -> std::string
{
    std::ostringstream oss;
    oss << yy << "-"
        << std::setw(2) << std::setfill('0') << mm << "-"
        << std::setw(2) << std::setfill('0') << dd;
    return oss.str();
};

auto lakeIsLeapYear = [](int yy) -> bool
{
    return (yy % 4 == 0 && yy % 100 != 0) || (yy % 400 == 0);
};

auto lakeDayOfYear = [&](const std::string& date) -> int
{
    const int yy = std::stoi(date.substr(0, 4));
    const int mm = std::stoi(date.substr(5, 2));
    const int dd = std::stoi(date.substr(8, 2));

    int daysInMonth[12] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31
    };

    if (lakeIsLeapYear(yy))
        daysInMonth[1] = 29;

    int doy = dd;
    for (int m = 0; m < mm - 1; ++m)
        doy += daysInMonth[m];

    return doy;
};

const std::string lakeDate =
    lakeMakeDateString(year[index], month[index], day[index]);
const float lakeYearDay =
    static_cast<float>(lakeDayOfYear(lakeDate));

// FAO-56 extraterrestrial radiation [MJ m-2 day-1] at 60 degrees N.
const float lakeLatitudeRad =
    60.0f * 3.14159265358979323846f / 180.0f;
const float lakeGsc = 0.0820f;
const float lakeDr =
    1.0f + 0.033f * cosf(
        2.0f * 3.14159265358979323846f * lakeYearDay / 365.0f
    );
const float lakeDelta =
    0.409f * sinf(
        2.0f * 3.14159265358979323846f * lakeYearDay / 365.0f
        - 1.39f
    );
float lakeAcosArg =
    -tanf(lakeLatitudeRad) * tanf(lakeDelta);
lakeAcosArg = std::max(-1.0f, std::min(1.0f, lakeAcosArg));
const float lakeOmega = acosf(lakeAcosArg);
float lakeRadiation =
    (24.0f * 60.0f / 3.14159265358979323846f) *
    lakeGsc * lakeDr *
    (lakeOmega * sinf(lakeLatitudeRad) * sinf(lakeDelta) +
     cosf(lakeLatitudeRad) * cosf(lakeDelta) * sinf(lakeOmega));
lakeRadiation = std::max(0.0f, lakeRadiation);

float lakeEvapMM = lakeEvapFactor * std::max(
    0.0f,
    (lakeRadiation / 2.45f) * ((tm[index] + 5.0f) / 100.0f)
);
if (tm[index] < 0.1f)
    lakeEvapMM = 0.0f;

// Transfers to downstream lakes are applied only after all source lakes have
// calculated today's release. This removes dependence on lakeStats row order.
std::vector<float> pendingLakeTransferM3(nrLakes, 0.0f);
std::vector<float> routedInflowForLogM3(nrLakes, 0.0f);

for (int locLake = 0; locLake < nrLakes; ++locLake)
{
    routedInflowForLogM3[locLake] = lakeStats[locLake][4];
    lakeStats[locLake][4] = 0.0f;

    const float lakeAreaM2 = lakeStats[locLake][0];
    if (lakeStats[locLake][3] <= 0.0f || lakeAreaM2 <= 0.0f)
        continue;

    // Lake-specific routing parameters from lakeStats.csv.
    const float storedResidenceTimeDays = lakeStats[locLake][9];
    const float storedReleaseFraction = lakeStats[locLake][10];

    float localLakeResidenceTimeDays = storedResidenceTimeDays;
    float localLakeReleaseFraction = storedReleaseFraction;

    // Prefer the explicitly stored daily release fraction. If it is missing
    // or invalid, reconstruct it from the lake-specific residence time.
    if (!std::isfinite(localLakeReleaseFraction) ||
        localLakeReleaseFraction <= 0.0f ||
        localLakeReleaseFraction > 1.0f)
    {
        if (!std::isfinite(localLakeResidenceTimeDays) ||
            localLakeResidenceTimeDays <= 0.0f)
        {
            localLakeResidenceTimeDays =
                defaultLakeResidenceTimeDays;
        }

        localLakeReleaseFraction =
            1.0f -
            expf(
                -1.0f /
                std::max(0.01f, localLakeResidenceTimeDays)
            );
    }

    // Direct precipitation and evaporation change lake level. Negative level
    // is allowed and represents a water surface below the outlet sill.
    lakeStats[locLake][7] +=
        (rr[index] - lakeEvapMM) / 1000.0f;

    const float positiveLevelM =
        std::max(0.0f, lakeStats[locLake][7]);
    const float availableStorageM3 =
        lakeAreaM2 * positiveLevelM;

    float outflowM3 =
        localLakeReleaseFraction * availableStorageM3;
    if (outflowM3 < 0.0f)
        outflowM3 = 0.0f;
    if (outflowM3 > availableStorageM3)
        outflowM3 = availableStorageM3;

    const int lakeID = static_cast<int>(lakeStats[locLake][8]);
    const int outX = static_cast<int>(lakeStats[locLake][1]);
    const int outY = static_cast<int>(lakeStats[locLake][2]);

    bool validReleaseDestination = false;
    int downstreamLakeIndex = -1;
    bool terminalExport = false;

    if (isTerminalLakeID(lakeID))
    {
        terminalExport = true;
        validReleaseDestination = true;
    }
    else if (outX >= 0 && outX < xSize &&
             outY >= 0 && outY < ySize &&
             dem[outX][outY] != noData)
    {
        if (lakes[outX][outY] != noData)
        {
            // lakes.tif and lakeStats.csv now use the same zero-based index.
            downstreamLakeIndex =
                static_cast<int>(lakes[outX][outY]);

            if (downstreamLakeIndex >= 0 &&
                downstreamLakeIndex < nrLakes &&
                downstreamLakeIndex != locLake &&
                lakeStats[downstreamLakeIndex][0] > 0.0f)
            {
                validReleaseDestination = true;
            }
            else
            {
                cerr << "Invalid/self lake outlet for lake ID "
                     << lakeID << " at " << outX << ":" << outY
                     << "; retaining water in source lake." << endl;
            }
        }
        else
        {
            // Both terrestrial and channel outlet cells are routed through dsc.
            validReleaseDestination = true;
        }
    }
    else
    {
        // An outlet genuinely outside the active raster is treated as export.
        terminalExport = true;
        validReleaseDestination = true;
    }

    if (!validReleaseDestination)
        outflowM3 = 0.0f;

    if (outflowM3 > 0.0f)
    {
        lakeStats[locLake][7] -= outflowM3 / lakeAreaM2;

        if (downstreamLakeIndex >= 0)
        {
            pendingLakeTransferM3[downstreamLakeIndex] += outflowM3;
        }
        else if (terminalExport)
        {
            Q_lake += outflowM3;
        }
        else
        {
            const float outflowMM =
                outflowM3 / (dx * dy) * 1000.0f;

            dsc[outX][outY] += outflowMM;
            dscLakeOrigin[outX][outY] += outflowMM;
        }
    }

    lakeStats[locLake][5] = outflowM3;
    lakeStats[locLake][6] =
        lakeAreaM2 * std::max(0.0f, lakeStats[locLake][7]);

    if (lakeID == lakeDiagnostic1 ||
        lakeID == lakeDiagnostic2 ||
        lakeID == lakeDiagnostic3)
    {
        oLake << year[index] << "-"
              << month[index] << "-"
              << day[index] << " "
              << lakeStats[locLake][7] << " "
              << lakeStats[locLake][6] << " "
              << outflowM3 << " "
              << routedInflowForLogM3[locLake] << " "
              << lakeID << " "
              << localLakeResidenceTimeDays << " "
              << localLakeReleaseFraction << endl;
    }
}

// Apply inter-lake transfers after every source lake has completed its release.
for (int locLake = 0; locLake < nrLakes; ++locLake)
{
    const float transferM3 = pendingLakeTransferM3[locLake];
    const float lakeAreaM2 = lakeStats[locLake][0];

    if (transferM3 <= 0.0f || lakeAreaM2 <= 0.0f)
        continue;

    lakeStats[locLake][4] += transferM3;
    lakeStats[locLake][7] += transferM3 / lakeAreaM2;
}
