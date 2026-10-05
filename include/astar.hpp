#pragma once
#include <vector>
#include <limits>
#include "grid.hpp"
#include "open_list.hpp"

struct SearchResult {
    bool   found      = false;
    int    path_cost  = 0;       // scaled by 1000
    size_t expansions = 0;       // deterministic: reproduces on any machine
    size_t path_len   = 0;
    size_t peak_open  = 0;
    size_t inserts    = 0;
    size_t extracts   = 0;
    std::vector<int> path;
};

// The search touches the OPEN list only through insert / extract_min / empty.
// Nothing below knows which structure is in use.
inline SearchResult astar(const Grid& grid, int start, int goal, OpenList& open) {
    const int INF = std::numeric_limits<int>::max();
    const int n   = grid.cells();

    // The search tree is implicit: one parent index per cell, 4 bytes each.
    // No nodes, no allocation. The route is recovered by walking these links
    // backwards from the goal, which is a leaf-to-root walk.
    std::vector<int>     g(n, INF);
    std::vector<int>     parent(n, -1);
    std::vector<uint8_t> closed(n, 0);      // O(1) membership, no ordering needed

    static const int DX[8] = { 1, -1,  0,  0,  1,  1, -1, -1 };
    static const int DY[8] = { 0,  0,  1, -1,  1, -1,  1, -1 };

    SearchResult res;
    g[start] = 0;
    open.insert(start, grid.heuristic(start, goal));

    while (!open.empty()) {
        int u = open.extract_min();

        // Lazy deletion: a cell can sit in OPEN more than once. With a
        // consistent heuristic the first pop is the cheapest, so later
        // duplicates are simply skipped.
        if (closed[u]) continue;
        closed[u] = 1;
        ++res.expansions;

        if (u == goal) {
            res.found     = true;
            res.path_cost = g[u];
            for (int c = goal; c != -1; c = parent[c]) res.path.push_back(c);
            res.path_len = res.path.size();
            break;
        }

        int ux = grid.x_of(u), uy = grid.y_of(u);
        for (int d = 0; d < 8; ++d) {
            int vx = ux + DX[d], vy = uy + DY[d];
            if (!grid.in_bounds(vx, vy)) continue;
            if (grid.blocked(vx, vy))    continue;       // flat array, O(1)

            int v = grid.index(vx, vy);
            if (closed[v]) continue;

            // Diagonals cost more because they cover more ground. Treating
            // them as equal would produce routes with fewer steps but greater
            // distance.
            int step = (DX[d] != 0 && DY[d] != 0) ? COST_DIAGONAL : COST_STRAIGHT;
            int tentative = g[u] + step;

            if (tentative < g[v]) {
                g[v]      = tentative;
                parent[v] = u;
                open.insert(v, tentative + grid.heuristic(v, goal));
            }
        }
    }

    res.peak_open = open.peak_size;
    res.inserts   = open.inserts;
    res.extracts  = open.extracts;
    return res;
}
