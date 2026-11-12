// Persistent lake-origin tracer for water held in dsc [mm].
// Include once after xSize/ySize and the main arrays are available,
// but before the daily timestep loop.
std::vector<std::vector<float> > dscLakeOrigin(
    xSize,
    std::vector<float>(ySize, 0.0f)
);

// Outgoing lake-origin flux at each cell for the current day [mm].
// Useful for source-resolved hydrograph diagnostics.
std::vector<std::vector<float> > flaccLakeOrigin(
    xSize,
    std::vector<float>(ySize, 0.0f)
);

// Gross lake-origin water crossing from a terrestrial cell into a channel
// during the current day [m3/day]. This is a diagnostic internal transfer,
// not necessarily catchment-outlet discharge.
float Q_lakeRouted = 0.0f;
