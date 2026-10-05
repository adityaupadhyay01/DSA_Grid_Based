// Units 3 and 4 demonstration.
//
// A* (Units 1-2) supplies the distances. Knapsack (Unit 3) chooses which
// stops are worth serving on the available battery. Branch and bound (Unit 4)
// decides the order to serve them in.
#include <cstdio>
#include <chrono>
#include "grid.hpp"
#include "mission.hpp"

using Clock = std::chrono::steady_clock;
static double ms(Clock::time_point a, Clock::time_point b) {
    return std::chrono::duration<double, std::milli>(b - a).count();
}

int main() {
    const int W = 60, H = 40;
    Grid g(W, H);
    g.randomise(0.15, 31415, 0, W * H - 1);

    std::vector<Waypoint> wps = {
        {g.index( 2,  2),  0,   0},   // W0, depot
        {g.index(55,  5), 60, 180},
        {g.index(30, 35), 85, 240},
        {g.index(50, 30), 45, 120},
        {g.index(10, 25), 70, 200},
        {g.index(40, 10), 95, 260},
        {g.index(20,  8), 30,  90},
        {g.index(58, 38), 55, 170},
        {g.index( 5, 38), 40, 130},
        {g.index(35, 20), 75, 210},
    };
    for (auto& w : wps) g.set_blocked(g.x_of(w.cell), g.y_of(w.cell), false);

    std::printf("================================================\n");
    std::printf(" Mission planning: Units 1-2 feed Units 3 and 4\n");
    std::printf("================================================\n\n");

    std::printf("UNITS 1-2  A* builds the distance matrix\n");
    auto t0 = Clock::now();
    auto dist = build_distance_matrix(g, wps);
    auto t1 = Clock::now();
    std::printf("  %zu waypoints, %zu A* runs, %.1f ms\n\n",
                wps.size(), wps.size() * (wps.size() - 1) / 2, ms(t0, t1));

    std::printf("UNIT 3  0/1 Knapsack: which stops to serve\n");
    const int budget = 900;
    auto ks = knapsack_select(wps, budget);
    std::printf("  battery budget  : %d units\n", budget);
    std::printf("  selected        : ");
    for (int i : ks.chosen) std::printf("W%d ", i);
    std::printf("\n  total value     : %d\n", ks.total_value);
    std::printf("  battery used    : %d of %d\n", ks.total_cost, budget);
    std::printf("  DP table cells  : %zu\n", ks.table_cells);

    size_t subsets = 0;
    auto t2 = Clock::now();
    int bf = knapsack_bruteforce(wps, budget, subsets);
    auto t3 = Clock::now();
    std::printf("  brute force     : value %d over %zu subsets, %.2f ms\n",
                bf, subsets, ms(t2, t3));
    std::printf("  agreement       : %s\n",
                bf == ks.total_value ? "YES, DP matches the exhaustive answer"
                                     : "NO -- BUG");
    std::printf("  work ratio      : %.1fx fewer operations than brute force\n\n",
                (double)subsets / (double)ks.table_cells);

    std::vector<int> tour_idx;
    tour_idx.push_back(0);
    for (int i : ks.chosen) if (i != 0) tour_idx.push_back(i);
    const int m = (int)tour_idx.size();
    std::vector<std::vector<int>> sub(m, std::vector<int>(m, 0));
    for (int i = 0; i < m; ++i)
        for (int j = 0; j < m; ++j)
            sub[i][j] = dist[tour_idx[i]][tour_idx[j]];

    std::printf("UNIT 4  TSP over the %d selected stops\n", m);
    auto t4 = Clock::now();
    BacktrackingTSP   bt(sub);  auto rbt = bt.solve(0);
    auto t5 = Clock::now();
    BranchAndBoundTSP bb(sub);  auto rbb = bb.solve(0);
    auto t6 = Clock::now();

    std::printf("  backtracking    : cost %7.3f   nodes %8zu   %.2f ms\n",
                rbt.cost / 1000.0, rbt.nodes_explored, ms(t4, t5));
    std::printf("  branch & bound  : cost %7.3f   nodes %8zu   %.2f ms\n",
                rbb.cost / 1000.0, rbb.nodes_explored, ms(t5, t6));
    std::printf("  same optimum    : %s\n",
                rbt.cost == rbb.cost ? "YES, pruning is safe" : "NO -- BUG");
    std::printf("  nodes avoided   : %.1f%%\n",
                100.0 * (double)(rbt.nodes_explored - rbb.nodes_explored)
                      / (double)rbt.nodes_explored);
    std::printf("  branches pruned : %zu\n", rbb.nodes_pruned);
    std::printf("  speedup         : %.1fx\n", ms(t4, t5) / ms(t5, t6));
    std::printf("  visiting order  : ");
    for (int i : rbb.order) std::printf("W%d ", tour_idx[i]);
    std::printf("-> W%d (return to depot)\n", tour_idx[rbb.order[0]]);
    return 0;
}
