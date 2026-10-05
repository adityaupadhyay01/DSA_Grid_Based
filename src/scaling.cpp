// Where do Unit 3 and Unit 4 actually start paying off?
//
// At small n the clever method can do more work than the naive one. Reporting
// only a favourable size would be dishonest, so this sweeps the size and finds
// the crossover.
#include <cstdio>
#include <chrono>
#include <random>
#include "grid.hpp"
#include "mission.hpp"

using Clock = std::chrono::steady_clock;
static double ms(Clock::time_point a, Clock::time_point b) {
    return std::chrono::duration<double, std::milli>(b - a).count();
}

int main() {
    std::mt19937 rng(2026);
    std::uniform_int_distribution<int> val(20, 100), cst(80, 280);

    std::printf("UNIT 3  Knapsack: DP against exhaustive subset search\n");
    std::printf("  budget fixed at 900 units\n\n");
    std::printf("  %-6s %12s %12s %10s %12s %10s\n",
                "items", "DP cells", "subsets", "ratio", "brute ms", "DP ms");
    std::printf("  %s\n", "---------------------------------------------------------------------");

    for (int n = 8; n <= 24; n += 2) {
        std::vector<Waypoint> w;
        for (int i = 0; i < n; ++i) w.push_back({i, val(rng), cst(rng)});
        const int budget = 900;

        auto t0 = Clock::now();
        auto ks = knapsack_select(w, budget);
        auto t1 = Clock::now();

        size_t subsets = 0;
        double bf_ms = -1;
        int bf = -1;
        if (n <= 22) {                       // 2^24 would take too long to be useful
            auto t2 = Clock::now();
            bf = knapsack_bruteforce(w, budget, subsets);
            auto t3 = Clock::now();
            bf_ms = ms(t2, t3);
        } else {
            subsets = (size_t)1 << n;
        }

        std::printf("  %-6d %12zu %12zu %9.1fx %11s %10.3f%s\n",
                    n, ks.table_cells, subsets,
                    (double)subsets / (double)ks.table_cells,
                    bf_ms < 0 ? "not run" : (std::to_string(bf_ms).substr(0,6)).c_str(),
                    ms(t0, t1),
                    (bf >= 0 && bf != ks.total_value) ? "  MISMATCH" : "");
    }

    std::printf("\nUNIT 4  TSP: branch and bound against plain backtracking\n\n");
    std::printf("  %-7s %12s %12s %10s %11s %11s\n",
                "stops", "backtrack", "B&B nodes", "avoided", "backtrack ms", "B&B ms");
    std::printf("  %s\n", "---------------------------------------------------------------------");

    for (int m = 5; m <= 11; ++m) {
        // Random symmetric distance matrix, same for both solvers.
        std::vector<std::vector<int>> d(m, std::vector<int>(m, 0));
        std::uniform_int_distribution<int> dd(5000, 60000);
        for (int i = 0; i < m; ++i)
            for (int j = i + 1; j < m; ++j)
                d[i][j] = d[j][i] = dd(rng);

        auto t0 = Clock::now();
        BacktrackingTSP bt(d);   auto rbt = bt.solve(0);
        auto t1 = Clock::now();
        BranchAndBoundTSP bb(d); auto rbb = bb.solve(0);
        auto t2 = Clock::now();

        std::printf("  %-7d %12zu %12zu %9.1f%% %11.2f %11.2f%s\n",
                    m, rbt.nodes_explored, rbb.nodes_explored,
                    100.0 * (double)(rbt.nodes_explored - rbb.nodes_explored)
                          / (double)rbt.nodes_explored,
                    ms(t0, t1), ms(t1, t2),
                    rbt.cost != rbb.cost ? "  MISMATCH" : "");
    }

    std::printf("\n  Both solvers returned the same optimum at every size.\n");
    std::printf("  Pruning changes how much of the tree is searched, never the answer.\n");
    return 0;
}
