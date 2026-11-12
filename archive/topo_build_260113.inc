// ==========================================================
// BUILD TOPOLOGICAL ORDER FOR FLOW ROUTING (MFD)
// Simple version - all cells treated equally
// ==========================================================

#include <queue>
std::queue<Cell> Q;

// ----------------------------------------------------------
// Macros
// ----------------------------------------------------------
#define IS_ACTIVE_CELL(x,y) \
    (dem[x][y] != noData dem[x][y] > 0 && lakes[x][y] == noData && dams[x][y] == noData)

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
            if (X == 0 && Y == 0)
            {
                continue;  // Skip center, don't increment wIndex
            }

            if (d[x][y][wIndex] > 0)
            {
                int nx = x + X;
                int ny = y + Y;

                if (!IS_ACTIVE_CELL(nx, ny))
                {
                    wIndex++;
                    continue;
                }

                inDegree[nx][ny]++;
            }

            wIndex++;
        }
    }
}

// ----------------------------------------------------------
// 3. Push source cells
// ----------------------------------------------------------
for (int x = 1; x < xSize - 1; x++)
for (int y = 1; y < ySize - 1; y++)
{
    if (!IS_ACTIVE_CELL(x, y)) continue;

    if (inDegree[x][y] == 0)
        Q.push({x, y});
}

// ----------------------------------------------------------
// 4. Kahn's algorithm
// ----------------------------------------------------------
topoOrder.clear();

while (!Q.empty())
{
    Cell c = Q.front();
    Q.pop();
    topoOrder.push_back(c);

    int x = c.x;
    int y = c.y;

    if (!IS_ACTIVE_CELL(x, y))
        continue;

    int wIndex = 0;
    for (int X = -1; X <= 1; X++)
    {
        for (int Y = -1; Y <= 1; Y++)
        {
            if (X == 0 && Y == 0)
            {
                continue;  // Skip center
            }

            if (d[x][y][wIndex] > 0)
            {
                int nx = x + X;
                int ny = y + Y;

                if (!IS_ACTIVE_CELL(nx, ny))
                {
                    wIndex++;
                    continue;
                }

                inDegree[nx][ny]--;
                if (inDegree[nx][ny] == 0)
                    Q.push({nx, ny});
            }

            wIndex++;
        }
    }
}

std::cout << "TopoOrder size: " << topoOrder.size() << std::endl;

// ----------------------------------------------------------
// Cleanup macros
// ----------------------------------------------------------
#undef IS_ACTIVE_CELL
