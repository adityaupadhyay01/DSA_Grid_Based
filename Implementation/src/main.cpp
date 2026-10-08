#include <chrono>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include "grid.hpp"
#include "astar.hpp"
#include "open_list.hpp"

using Clock = std::chrono::steady_clock;

static std::unique_ptr<OpenList> make_open_list(int which) {
    switch (which) {
        case 0: return std::make_unique<UnsortedArrayOpenList>();
        case 1: return std::make_unique<BinaryHeapOpenList>();
        default: return std::make_unique<BucketQueueOpenList>();
    }
}

static double median(std::vector<double> v) {
    std::sort(v.begin(), v.end());
    size_t n = v.size();
    return n % 2 ? v[n / 2] : 0.5 * (v[n / 2 - 1] + v[n / 2]);
}

int main(int argc, char** argv) {
    // Defaults kept small so a first run finishes quickly. The unsorted array
    // is O(n^2), so raising the size affects it far more than the others.
    std::vector<int> sizes = {64, 128, 256};
    double  density = 0.20;
    int     reps    = 5;
    int     trials  = 10;      // distinct maps per configuration

    if (argc > 1) density = std::stod(argv[1]);
    if (argc > 2) trials  = std::stoi(argv[2]);

    std::printf("structure,grid,density,trial,runtime_ms,expansions,"
                "inserts,extracts,peak_open,path_cost,found\n");

    for (int size : sizes) {
        for (int t = 0; t < trials; ++t) {
            uint32_t seed = 1000u + t;
            Grid grid(size, size);
            int start = grid.index(0, 0);
            int goal  = grid.index(size - 1, size - 1);
            grid.randomise(density, seed, start, goal);

            for (int w = 0; w < 3; ++w) {
                // Skip the O(n^2) baseline on the largest grid: it dominates
                // total runtime and the trend is already clear by 128x128.
                if (w == 0 && size > 128) continue;

                SearchResult last;
                std::vector<double> times;
                for (int r = 0; r < reps; ++r) {
                    auto open = make_open_list(w);
                    auto t0 = Clock::now();
                    last = astar(grid, start, goal, *open);
                    auto t1 = Clock::now();
                    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
                    if (r > 0) times.push_back(ms);     // discard the cold run
                }

                auto probe = make_open_list(w);
                std::printf("%s,%d,%.2f,%d,%.4f,%zu,%zu,%zu,%zu,%d,%d\n",
                            probe->name(), size, density, t,
                            median(times), last.expansions,
                            last.inserts, last.extracts, last.peak_open,
                            last.path_cost, last.found ? 1 : 0);
                std::fflush(stdout);
            }
        }
    }
    return 0;
}
