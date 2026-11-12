// Include immediately after detachment.inc, once per simulated day.
// detachment.inc has already deposited excess local detachment and deducted
// the mobile local load from TC. TC is therefore the budget for incoming load.
{
    ERO_overland = 0;
    ERO_gully = 0;

    auto sedimentTrap = [&](int x, int y)
    {
        return lakes[x][y] != noData ||
               (simulateDams == 1 && dams[x][y] != noData);
    };

    // Prepare daily state, including cells that the water DAG may omit.
    for (int x = 0; x < xSize; ++x)
    {
        for (int y = 0; y < ySize; ++y)
        {
            if (dem[x][y] == noData)
                continue;

            const bool detachedToday =
                tm[index] > -2 && landuse[x][y] > 20 && landuse[x][y] < 31;
            if (!detachedToday)
            {
                toDistribute_sedi[x][y] = 0;
                TC[x][y] = 0;
            }
            if (sedimentTrap(x, y) || isChannelCell(x, y))
            {
                // Account for local detachment on masked cells. Channel
                // delivery totals count only sediment arriving from hillslopes.
                if (toDistribute_sedi[x][y] > 0)
                    depoMFD[x][y] += toDistribute_sedi[x][y];
                toDistribute_sedi[x][y] = 0;
            }
        }
    }

    double overlandDelivery = 0;
    double gullyDelivery = 0;
    for (const Cell& cell : topoOrder)
    {
        const int x = cell.x;
        const int y = cell.y;
        if (dem[x][y] == noData || sedimentTrap(x, y) || isChannelCell(x, y))
            continue;

        // Classify this day's delivery using cumulative detachment to date.
        if (eroMFD[x][y] > gullyThreshold)
            gully[x][y] = true;

        const double load = toDistribute_sedi[x][y];
        toDistribute_sedi[x][y] = 0;
        if (!std::isfinite(load) || load <= 0)
            continue;

        struct SedimentEdge
        {
            int x;
            int y;
            double weight;
        };
        SedimentEdge edges[8];
        int edgeCount = 0;
        double weightSum = 0;

        for (int X = -1; X <= 1; ++X)
        {
            for (int Y = -1; Y <= 1; ++Y)
            {
                if (X == 0 && Y == 0)
                    continue;

                // Same nine-slot stencil as flacc_DAG_v8, with slot 4 unused.
                const double weight = d[x][y][(X + 1) * 3 + (Y + 1)];
                const int nx = x + X;
                const int ny = y + Y;
                if (!std::isfinite(weight) || weight <= 0 ||
                    nx < 0 || nx >= xSize || ny < 0 || ny >= ySize ||
                    dem[nx][ny] == noData)
                    continue;

                edges[edgeCount++] = {nx, ny, weight};
                weightSum += weight;
            }
        }

        // Keep any unroutable fraction at the source. Guard against weights
        // summing slightly above one because the raster weights are floats.
        const double divisor = weightSum > 1 ? weightSum : 1;
        depoMFD[x][y] += load * (1 - weightSum / divisor);

        for (int i = 0; i < edgeCount; ++i)
        {
            const SedimentEdge& edge = edges[i];
            const int nx = edge.x;
            const int ny = edge.y;
            const double incoming = load * (edge.weight / divisor);
            if (sedimentTrap(nx, ny))
            {
                depoMFD[nx][ny] += incoming;
            }
            else if (isChannelCell(nx, ny))
            {
                if (gully[x][y])
                    gullyDelivery += incoming;
                else
                    overlandDelivery += incoming;
            }
            else
            {
                // Local mobile sediment is already in the receiver buffer.
                // Charge only this incoming contribution to its remaining TC.
                const double capacity = TC[nx][ny];
                const double accepted =
                    std::isfinite(capacity) && capacity > 0
                    ? (incoming < capacity ? incoming : capacity)
                    : 0;
                TC[nx][ny] = capacity > 0 && std::isfinite(capacity)
                    ? capacity - accepted : 0;
                toDistribute_sedi[nx][ny] += accepted;
                depoMFD[nx][ny] += incoming - accepted;
            }
        }
    }
    ERO_overland = overlandDelivery;
    ERO_gully = gullyDelivery;
}
