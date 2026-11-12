// ==========================================================
// BUILD TOPOLOGICAL ORDER FOR FLOW ROUTING (MFD)
// Consistent wIndex handling (CRITICAL FIX)
// ==========================================================

#include <queue>

// ----------------------------------------------------------
// Macros
// ----------------------------------------------------------
#define IS_ACTIVE_CELL(x,y) \
    (dem[x][y] != noData && lakes[x][y] == noData && dams[x][y] == noData)

// ----------------------------------------------------------
// 1. Initialize inDegree
// ----------------------------------------------------------
for (int x = 0; x < xSize; x++)
for (int y = 0; y < ySize; y++)
    inDegree[x][y] = 0;

// ----------------------------------------------------------
// 2. Build DAG: count downstream dependencies
// ----------------------------------------------------------
for (int x = 1; x < xSize - 1; x++)
for (int y = 1; y < ySize - 1; y++)
{
    if (!IS_ACTIVE_CELL(x, y)) continue;

    int wIndex = 0;

    for (int X = -1; X <= 1; X++)
    {
        for (int Y = -1; Y <= 1; Y++)
        {
            // IMPORTANT: wIndex is incremented for ALL (X,Y)
            if (X == 0 && Y == 0)
            {
                wIndex++;
                continue;
            }

            if (d[x][y][wIndex] > 0.0f)
            {
                int nx = x + X;
                int ny = y + Y;

                if (IS_ACTIVE_CELL(nx, ny))
                    inDegree[nx][ny]++;
            }

            wIndex++;
        }
    }
}

// ----------------------------------------------------------
// 3. Push source cells (inDegree == 0)
// ----------------------------------------------------------
std::queue<Cell> Q;

for (int x = 1; x < xSize - 1; x++)
for (int y = 1; y < ySize - 1; y++)
{
    if (!IS_ACTIVE_CELL(x, y)) continue;

    if (inDegree[x][y] == 0)
        Q.push({x, y});
}

// ----------------------------------------------------------
// 4. Kahn's algorithm (topological order)
// ----------------------------------------------------------
topoOrder.clear();

while (!Q.empty())
{
    Cell c = Q.front();
    Q.pop();
    topoOrder.push_back(c);

    int x = c.x;
    int y = c.y;

    if (!IS_ACTIVE_CELL(x, y)) continue;

    int wIndex = 0;

    for (int X = -1; X <= 1; X++)
    {
        for (int Y = -1; Y <= 1; Y++)
        {
            if (X == 0 && Y == 0)
            {
                wIndex++;
                continue;
            }

            if (d[x][y][wIndex] > 0.0f)
            {
                int nx = x + X;
                int ny = y + Y;

                if (IS_ACTIVE_CELL(nx, ny))
                {
                    inDegree[nx][ny]--;
                    if (inDegree[nx][ny] == 0)
                        Q.push({nx, ny});
                }
            }

            wIndex++;
        }
    }
}

std::cout << "TopoOrder size: " << topoOrder.size() << std::endl;
/*
// ----------------------------------------------------------
// DIAGNOSTIC FLOW ACCUMULATION (VALIDATES DAG)
// ----------------------------------------------------------
std::cout << "Calculating diagnostic flow accumulation..." << std::endl;

std::vector<std::vector<float>> flowAccum(
    xSize, std::vector<float>(ySize, 0.0f)
);

// Each active cell contributes 1
for (int x = 0; x < xSize; x++)
for (int y = 0; y < ySize; y++)
{
    if (IS_ACTIVE_CELL(x, y))
        flowAccum[x][y] = 1.0f;
}

// Route flow
for (const Cell& c : topoOrder)
{
    int x = c.x;
    int y = c.y;

    if (!IS_ACTIVE_CELL(x, y)) continue;

    float myFlow = flowAccum[x][y];
    int wIndex = 0;

    for (int X = -1; X <= 1; X++)
    {
        for (int Y = -1; Y <= 1; Y++)
        {
            if (X == 0 && Y == 0)
            {
                wIndex++;
                continue;
            }

            float w = d[x][y][wIndex];
            if (w > 0.0f)
            {
                int nx = x + X;
                int ny = y + Y;

                if (IS_ACTIVE_CELL(nx, ny))
                    flowAccum[nx][ny] += myFlow * w;
            }

            wIndex++;
        }
    }
}

// ----------------------------------------------------------
// Write diagnostic GeoTIFF
// ----------------------------------------------------------
saveGeoTIFF(
    ioFolder + "/diagnostic_flowacc.tif",
    flowAccum,
    xSize,
    ySize,
    adfGeoTransform,
    INDEM->GetProjectionRef()
);

// ----------------------------------------------------------
// Diagnostics
// ----------------------------------------------------------
float maxFlowAccum = 0.0f;
float maxChannelAccum = 0.0f;

for (int x = 0; x < xSize; x++)
for (int y = 0; y < ySize; y++)
{
    if (flowAccum[x][y] > maxFlowAccum)
        maxFlowAccum = flowAccum[x][y];

    if (channel[x][y] == 1 && flowAccum[x][y] > maxChannelAccum)
        maxChannelAccum = flowAccum[x][y];
}

std::cout << "Max flow accumulation: " << maxFlowAccum << std::endl;
std::cout << "Max in channels: " << maxChannelAccum << std::endl;
*/
// ----------------------------------------------------------
// Cleanup
// ----------------------------------------------------------
#undef IS_ACTIVE_CELL

