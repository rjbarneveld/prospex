// ==========================================================
// BUILD TOPOLOGICAL ORDER FOR FLOW ROUTING (MFD)
// v4: baseline + dam-aware topology with virtual dam links
// ==========================================================
//
// PURPOSE
// -------
// simulateDams == 0:
//   Build the ordinary MFD DAG.  The dam raster is ignored completely.
//
// simulateDams == 1:
//   Dam raster cells are storage cells and therefore are NOT ordinary DAG
//   cells.  However, flow entering a dam may leave through its pipe/spillway
//   during the same model day.  To guarantee that the dam outlet is processed
//   only after all contributing cells have been processed, every MFD edge
//   terminating at a dam is represented in the TOPOLOGY by a virtual edge to
//   that dam's first downstream non-dam routing cell.
//
//   The virtual edge carries NO WATER.  It only establishes processing order.
//   flacc_DAG must perform the actual dam water balance and insert immediate
//   pipe/spill flow into dsc at the dam outlet during the single normal DAG pass.
//
//   If dams discharge directly into other dams, the virtual dependency follows
//   the dam chain until the first ordinary routing cell.  A chain ending in a
//   lake or outside the active domain needs no ordinary-cell dependency.
//
// IMPORTANT
// ---------
// The same receiver-resolution routine is used both when incrementing and when
// decrementing inDegree.  This symmetry is essential for Kahn's algorithm.
// ==========================================================

#include <queue>
#include <vector>
#include <cmath>
#include <iostream>

{
    // ------------------------------------------------------
    // Helpers
    // ------------------------------------------------------
    auto topoInBounds = [&](int xx, int yy) -> bool
    {
        return xx >= 0 && xx < xSize && yy >= 0 && yy < ySize;
    };

    // Cells that can participate in the ordinary routing DAG.
    // In the baseline run the dam raster is deliberately ignored.
    auto topoIsActiveCell = [&](int xx, int yy) -> bool
    {
        if (!topoInBounds(xx, yy))
            return false;

        if (dem[xx][yy] == noData)
            return false;

        if (lakes[xx][yy] != noData)
            return false;

        if (simulateDams == 1 && dams[xx][yy] != noData)
            return false;

        return true;
    };

    // topoOrder has historically contained only interior cells, so keep that
    // behaviour unchanged.  Border outlets are treated as exports/endpoints.
    auto topoIsRoutableCell = [&](int xx, int yy) -> bool
    {
        return xx > 0 && xx < xSize - 1 &&
               yy > 0 && yy < ySize - 1 &&
               topoIsActiveCell(xx, yy);
    };

    auto topoDamFindIndexByID = [&](int damID) -> int
    {
        for (int i = 0; i < nrDams; ++i)
        {
            if (static_cast<int>(damStats[i][0]) == damID)
                return i;
        }
        return -1;
    };

    // ------------------------------------------------------
    // 0. Validate dam raster IDs and pre-resolve the ordinary
    //    downstream routing cell for every dam.
    // ------------------------------------------------------
    std::vector<int> topoDamOutletX(nrDams, -1);
    std::vector<int> topoDamOutletY(nrDams, -1);

    long long topoDamRasterCells = 0;
    long long topoInvalidDamRasterCells = 0;
    int topoDamChainsToRoutingCell = 0;
    int topoDamChainsToLakeOrExport = 0;
    int topoDamChainErrors = 0;

    if (simulateDams == 1)
    {
        // Every valid raster dam ID must exist in damStats.
        for (int x = 0; x < xSize; ++x)
        {
            for (int y = 0; y < ySize; ++y)
            {
                if (dem[x][y] == noData || dams[x][y] == noData)
                    continue;

                ++topoDamRasterCells;

                const int damID = static_cast<int>(dams[x][y]);
                if (topoDamFindIndexByID(damID) < 0)
                {
                    if (topoInvalidDamRasterCells < 10)
                    {
                        std::cerr
                            << "ERROR topo_build_v4: dam raster ID " << damID
                            << " at " << x << ":" << y
                            << " has no matching damStats row.\n";
                    }
                    ++topoInvalidDamRasterCells;
                }
            }
        }

        if (topoInvalidDamRasterCells > 0)
        {
            std::cerr
                << "ERROR topo_build_v4: "
                << topoInvalidDamRasterCells
                << " dam raster cells have IDs absent from damStats.\n";
            return 1;
        }

        // Resolve each dam's downstream chain once.  This is topology only:
        // flacc_DAG still has to partition flow through every dam in the chain.
        for (int startDam = 0; startDam < nrDams; ++startDam)
        {
            std::vector<unsigned char> seen(nrDams, 0);
            int currentDam = startDam;
            bool resolved = false;

            for (int hop = 0; hop <= nrDams; ++hop)
            {
                if (currentDam < 0 || currentDam >= nrDams)
                {
                    ++topoDamChainErrors;
                    break;
                }

                if (seen[currentDam])
                {
                    std::cerr
                        << "ERROR topo_build_v4: cycle in dam-to-dam outlet chain "
                        << "starting at dam ID "
                        << static_cast<int>(damStats[startDam][0]) << ".\n";
                    ++topoDamChainErrors;
                    break;
                }
                seen[currentDam] = 1;

                const float outXF = damStats[currentDam][5];
                const float outYF = damStats[currentDam][6];

                if (!std::isfinite(outXF) || !std::isfinite(outYF))
                {
                    std::cerr
                        << "ERROR topo_build_v4: non-finite outlet index for dam ID "
                        << static_cast<int>(damStats[currentDam][0]) << ".\n";
                    ++topoDamChainErrors;
                    break;
                }

                const int outX = static_cast<int>(outXF);
                const int outY = static_cast<int>(outYF);

                // Outside the raster / outside DEM / border = terminal export.
                if (!topoInBounds(outX, outY) ||
                    dem[outX][outY] == noData ||
                    outX == 0 || outX == xSize - 1 ||
                    outY == 0 || outY == ySize - 1)
                {
                    ++topoDamChainsToLakeOrExport;
                    resolved = true;
                    break;
                }

                // A lake is a storage endpoint, not an ordinary DAG cell.
                if (lakes[outX][outY] != noData)
                {
                    ++topoDamChainsToLakeOrExport;
                    resolved = true;
                    break;
                }

                // Direct outlet into another dam: follow the chain.
                if (dams[outX][outY] != noData)
                {
                    const int downstreamDamID =
                        static_cast<int>(dams[outX][outY]);
                    const int downstreamDamIndex =
                        topoDamFindIndexByID(downstreamDamID);

                    if (downstreamDamIndex < 0)
                    {
                        std::cerr
                            << "ERROR topo_build_v4: outlet of dam ID "
                            << static_cast<int>(damStats[currentDam][0])
                            << " enters unknown dam raster ID "
                            << downstreamDamID << ".\n";
                        ++topoDamChainErrors;
                        break;
                    }

                    currentDam = downstreamDamIndex;
                    continue;
                }

                // First ordinary downstream routing cell.
                if (topoIsRoutableCell(outX, outY))
                {
                    topoDamOutletX[startDam] = outX;
                    topoDamOutletY[startDam] = outY;
                    ++topoDamChainsToRoutingCell;
                    resolved = true;
                    break;
                }

                // A valid DEM cell that is neither a routable ordinary cell,
                // lake nor dam is unexpected at this point.
                std::cerr
                    << "ERROR topo_build_v4: could not classify outlet "
                    << outX << ":" << outY
                    << " for dam ID "
                    << static_cast<int>(damStats[currentDam][0]) << ".\n";
                ++topoDamChainErrors;
                break;
            }

            if (!resolved && topoDamChainErrors == 0)
            {
                std::cerr
                    << "ERROR topo_build_v4: dam outlet chain did not resolve for dam ID "
                    << static_cast<int>(damStats[startDam][0]) << ".\n";
                ++topoDamChainErrors;
            }
        }

        if (topoDamChainErrors > 0)
        {
            std::cerr
                << "ERROR topo_build_v4: invalid dam outlet topology; aborting.\n";
            return 1;
        }
    }

    // Resolve one positive MFD edge to the cell whose topological dependency
    // must be counted.  For an ordinary receiver this is the receiver itself.
    // For a dam receiver this is the pre-resolved first ordinary dam outlet.
    // A lake/export endpoint produces no ordinary-cell dependency.
    auto topoResolveTarget = [&](int nx, int ny, int& tx, int& ty) -> bool
    {
        tx = -1;
        ty = -1;

        if (!topoInBounds(nx, ny) || dem[nx][ny] == noData)
            return false;

        if (simulateDams == 1 && dams[nx][ny] != noData)
        {
            const int damID = static_cast<int>(dams[nx][ny]);
            const int damIndex = topoDamFindIndexByID(damID);

            if (damIndex < 0)
                return false; // already validated above

            tx = topoDamOutletX[damIndex];
            ty = topoDamOutletY[damIndex];

            return topoIsRoutableCell(tx, ty);
        }

        if (topoIsRoutableCell(nx, ny))
        {
            tx = nx;
            ty = ny;
            return true;
        }

        return false;
    };

    // ------------------------------------------------------
    // 1. Initialise inDegree
    // ------------------------------------------------------
    for (int x = 0; x < xSize; ++x)
        for (int y = 0; y < ySize; ++y)
            inDegree[x][y] = 0;

    // ------------------------------------------------------
    // 2. Build DAG: count ordinary and virtual dependencies
    // ------------------------------------------------------
    long long topoActiveCells = 0;
    long long topoOrdinaryEdges = 0;
    long long topoVirtualDamEdges = 0;
    long long topoSelfDependencies = 0;

    for (int x = 1; x < xSize - 1; ++x)
    {
        for (int y = 1; y < ySize - 1; ++y)
        {
            if (!topoIsActiveCell(x, y))
                continue;

            ++topoActiveCells;

            int wIndex = 0;
            for (int X = -1; X <= 1; ++X)
            {
                for (int Y = -1; Y <= 1; ++Y)
                {
                    if (X == 0 && Y == 0)
                    {
                        ++wIndex;
                        continue;
                    }

                    const float w = d[x][y][wIndex];
                    ++wIndex;

                    if (w <= 0.0f)
                        continue;

                    const int nx = x + X;
                    const int ny = y + Y;

                    int tx = -1;
                    int ty = -1;
                    if (!topoResolveTarget(nx, ny, tx, ty))
                        continue;

                    if (tx == x && ty == y)
                    {
                        ++topoSelfDependencies;
                        continue;
                    }

                    ++inDegree[tx][ty];

                    if (simulateDams == 1 &&
                        topoInBounds(nx, ny) &&
                        dams[nx][ny] != noData)
                    {
                        ++topoVirtualDamEdges;
                    }
                    else
                    {
                        ++topoOrdinaryEdges;
                    }
                }
            }
        }
    }

    if (topoSelfDependencies > 0)
    {
        std::cerr
            << "ERROR topo_build_v4: " << topoSelfDependencies
            << " virtual/ordinary self-dependencies detected.\n";
        return 1;
    }

    // ------------------------------------------------------
    // 3. Push source cells (inDegree == 0)
    // ------------------------------------------------------
    std::queue<Cell> Q;

    for (int x = 1; x < xSize - 1; ++x)
    {
        for (int y = 1; y < ySize - 1; ++y)
        {
            if (!topoIsActiveCell(x, y))
                continue;

            if (inDegree[x][y] == 0)
                Q.push({x, y});
        }
    }

    // ------------------------------------------------------
    // 4. Kahn's algorithm
    //    IMPORTANT: use exactly the same target resolution as above.
    // ------------------------------------------------------
    topoOrder.clear();
    topoOrder.reserve(static_cast<std::size_t>(topoActiveCells));

    while (!Q.empty())
    {
        const Cell c = Q.front();
        Q.pop();

        const int x = c.x;
        const int y = c.y;

        if (!topoIsActiveCell(x, y))
            continue;

        topoOrder.push_back(c);

        int wIndex = 0;
        for (int X = -1; X <= 1; ++X)
        {
            for (int Y = -1; Y <= 1; ++Y)
            {
                if (X == 0 && Y == 0)
                {
                    ++wIndex;
                    continue;
                }

                const float w = d[x][y][wIndex];
                ++wIndex;

                if (w <= 0.0f)
                    continue;

                const int nx = x + X;
                const int ny = y + Y;

                int tx = -1;
                int ty = -1;
                if (!topoResolveTarget(nx, ny, tx, ty))
                    continue;

                --inDegree[tx][ty];

                if (inDegree[tx][ty] == 0)
                    Q.push({tx, ty});
                else if (inDegree[tx][ty] < 0)
                {
                    std::cerr
                        << "ERROR topo_build_v4: negative inDegree at "
                        << tx << ":" << ty
                        << ". Increment/decrement topology is inconsistent.\n";
                    return 1;
                }
            }
        }
    }

    // ------------------------------------------------------
    // 5. Validation and diagnostics
    // ------------------------------------------------------
    std::cout
        << "Topo mode: "
        << (simulateDams == 1 ? "WITH dams (virtual outlet dependencies)"
                              : "WITHOUT dams (ordinary MFD DAG)")
        << std::endl;

    std::cout << "Topo active cells:      " << topoActiveCells << std::endl;
    std::cout << "TopoOrder size:         " << topoOrder.size() << std::endl;
    std::cout << "Ordinary DAG edges:     " << topoOrdinaryEdges << std::endl;

    if (simulateDams == 1)
    {
        std::cout << "Dam raster cells:       " << topoDamRasterCells << std::endl;
        std::cout << "Virtual dam edges:      " << topoVirtualDamEdges << std::endl;
        std::cout << "Dams -> routing cell:   " << topoDamChainsToRoutingCell << std::endl;
        std::cout << "Dams -> lake/export:    " << topoDamChainsToLakeOrExport << std::endl;
    }

    if (static_cast<long long>(topoOrder.size()) != topoActiveCells)
    {
        const long long missing =
            topoActiveCells - static_cast<long long>(topoOrder.size());

        std::cerr
            << "ERROR topo_build_v4: topological sort incomplete. "
            << missing << " active cells were not ordered.\n"
            << "This indicates a routing cycle or an invalid virtual dam outlet "
            << "dependency.\n";
        return 1;
    }
}
