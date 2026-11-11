#pragma once
// ---------------------------------------------------------------------------
// The engine's public API.
//
// Everything above this line is the data-structures work: the grid, the six
// OPEN lists, A*, the knapsack and the tour solvers. Everything below is a
// thin façade over it, shaped so that a caller in another language does not
// have to know any of it.
//
// The HTTP server, the command-line tools and the C++ tests all go through
// this header, so there is one definition of what "plan a route" means and no
// chance of the web layer drifting away from the code that was benchmarked.
//
// The only exception thrown is std::invalid_argument for bad input, which the
// server turns into a 400 response.
// ---------------------------------------------------------------------------
#include <algorithm>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "astar.hpp"
#include "grid.hpp"
#include "open_list.hpp"
#include "scenario.hpp"
#include "smooth.hpp"

namespace planner {

// --------------------------- structures ---------------------------
inline int structure_count() { return 6; }

inline std::unique_ptr<OpenList> make_open_list(int which) {
    switch (which) {
        case 0: return std::make_unique<UnsortedArrayOpenList>();
        case 1: return std::make_unique<BinaryHeapOpenList>();
        case 2: return std::make_unique<BucketQueueOpenList>();
        case 3: return std::make_unique<FourAryHeapOpenList>();
        case 4: return std::make_unique<BinomialHeapOpenList>();
        case 5: return std::make_unique<FibonacciHeapOpenList>();
        default:
            throw std::invalid_argument("structure must be 0..5");
    }
}

inline std::string structure_name(int which) {
    return make_open_list(which)->name();
}

// --------------------------- scenarios ---------------------------
// Built once. They are fixed data, and rebuilding them per request would
// show up in the timings the demo reports.
inline const std::vector<scenario::Scenario>& scenarios() {
    static const std::vector<scenario::Scenario> all = scenario::all();
    return all;
}

inline const scenario::Scenario& find_scenario(const std::string& id) {
    for (const auto& s : scenarios())
        if (s.id == id) return s;
    throw std::invalid_argument("no scenario called '" + id + "'");
}

// --------------------------- results ---------------------------
struct PlanResult {
    std::string scenarioId;
    std::string structure;
    bool   found      = false;
    int    cost       = 0;
    size_t expansions = 0;
    size_t inserts    = 0;
    size_t extracts   = 0;
    size_t stalePops  = 0;
    size_t peakOpen   = 0;
    int    openCells  = 0;
    double ms         = 0.0;       // median of `reps` untraced runs
    int    reps       = 0;
    std::vector<int> path;         // start -> goal
    std::vector<int> order;        // the order A* actually closed cells in

    // The grid route shortened by line-of-sight smoothing (see smooth.hpp).
    // Both lengths are straight-line, in 1000-per-cell units, so they compare
    // like for like. Empty / zero when no route was found.
    std::vector<int> smoothPath;   // start -> goal, waypoints only
    int    gridLength   = 0;
    int    smoothLength = 0;
};

struct StructureTiming {
    std::string structure;
    double ms         = 0.0;
    int    cost       = 0;
    size_t expansions = 0;
    size_t inserts    = 0;
    size_t extracts   = 0;
};

struct CompareResult {
    std::string scenarioId;
    int reps = 0;
    std::vector<StructureTiming> results;
};

// --------------------------- helpers ---------------------------
namespace detail {

inline double median(std::vector<double> v) {
    std::sort(v.begin(), v.end());
    return v[v.size() / 2];
}

inline void resolve(const scenario::Scenario& s, int startIndex, int endIndex,
                    int* start, int* goal) {
    if (startIndex < 0 || startIndex >= static_cast<int>(s.starts.size()))
        throw std::invalid_argument("startIndex out of range");
    if (endIndex < 0 || endIndex >= static_cast<int>(s.ends.size()))
        throw std::invalid_argument("endIndex out of range");
    *start = s.index(s.starts[startIndex].x, s.starts[startIndex].y);
    *goal  = s.index(s.ends[endIndex].x,     s.ends[endIndex].y);
}

// One timed sweep: a discarded warm-up, then `reps` runs, median reported.
// A single run measures scheduling noise as much as it measures the code.
inline SearchResult timed(const Grid& grid, int start, int goal, int which,
                          int reps, double* ms_out) {
    { auto warm = make_open_list(which); astar(grid, start, goal, *warm); }

    SearchResult res;
    std::vector<double> times;
    times.reserve(static_cast<size_t>(reps));
    for (int r = 0; r < reps; ++r) {
        auto open = make_open_list(which);
        const auto t0 = std::chrono::steady_clock::now();
        res = astar(grid, start, goal, *open);
        times.push_back(std::chrono::duration<double, std::milli>(
                            std::chrono::steady_clock::now() - t0).count());
    }
    *ms_out = median(std::move(times));
    return res;
}

}  // namespace detail

// --------------------------- operations ---------------------------
// --------------------------- custom maps ---------------------------
constexpr int kCustomMinSide = 2;
constexpr int kCustomMaxSide = 256;   // 65,536 cells: a request cannot ask for an hour of work

/**
 * Builds a one-route scenario from a map somebody drew. `cells` holds one digit
 * per cell, row-major, each a scenario::Kind. Everything about it is checked
 * here, because this is the one place untrusted data becomes a grid.
 */
inline scenario::Scenario custom_scenario(int width, int height, const std::string& cells,
                                          int start, int end) {
    if (width < kCustomMinSide || width > kCustomMaxSide ||
        height < kCustomMinSide || height > kCustomMaxSide)
        throw std::invalid_argument("width and height must each be between 2 and 256");

    const size_t total = static_cast<size_t>(width) * static_cast<size_t>(height);
    if (cells.size() != total)
        throw std::invalid_argument("cells must hold exactly width x height characters");

    scenario::Scenario s;
    s.id = "custom";
    s.name = "Custom map";
    s.blurb = "drawn by the caller";
    s.w = width;
    s.h = height;
    s.kind.resize(total);
    for (size_t i = 0; i < total; ++i) {
        const int k = cells[i] - '0';
        if (k < 0 || k > scenario::BLOCKED)
            throw std::invalid_argument("cells may only contain the digits 0 to 5");
        s.kind[i] = static_cast<uint8_t>(k);
    }

    const int n = static_cast<int>(total);
    if (start < 0 || start >= n) throw std::invalid_argument("start is outside the map");
    if (end < 0 || end >= n)     throw std::invalid_argument("end is outside the map");
    if (start == end)            throw std::invalid_argument("start and end are the same cell");
    if (s.kind[static_cast<size_t>(start)] != scenario::FREE)
        throw std::invalid_argument("start is inside an obstacle");
    if (s.kind[static_cast<size_t>(end)] != scenario::FREE)
        throw std::invalid_argument("end is inside an obstacle");

    s.starts = { {"Start", start % width, start / width} };
    s.ends   = { {"Delivery", end % width, end / width} };
    return s;
}

/** Plans on any scenario: a built-in one, or one from custom_scenario(). */
inline PlanResult plan_on(const scenario::Scenario& s, int startIndex, int endIndex,
                          int structure = 1, int reps = 5) {
    if (reps < 1 || reps > 51) throw std::invalid_argument("reps must be 1..51");

    int start = 0, goal = 0;
    detail::resolve(s, startIndex, endIndex, &start, &goal);

    const Grid grid = s.to_grid();

    double ms = 0.0;
    const SearchResult res = detail::timed(grid, start, goal, structure, reps, &ms);

    // The traced run is the identical search, so its counts match. Only the
    // expansion order is kept, and the timing above excludes the recording.
    SearchTrace trace;
    { auto rec = make_open_list(structure); astar(grid, start, goal, *rec, &trace); }

    PlanResult out;
    out.scenarioId = s.id;
    out.structure  = structure_name(structure);
    out.found      = res.found;
    out.cost       = res.path_cost;
    out.expansions = res.expansions;
    out.inserts    = res.inserts;
    out.extracts   = res.extracts;
    out.stalePops  = res.stale_pops;
    out.peakOpen   = res.peak_open;
    out.openCells  = s.open_cells();
    out.ms         = ms;
    out.reps       = reps;
    out.order      = std::move(trace.expansion_order);
    out.path.assign(res.path.rbegin(), res.path.rend());   // A* walks it backwards

    if (res.found) {
        out.smoothPath   = route::smooth(grid, out.path);
        out.gridLength   = route::length_scaled(grid, out.path);
        out.smoothLength = route::length_scaled(grid, out.smoothPath);
    }
    return out;
}

inline PlanResult plan(const std::string& scenarioId, int startIndex, int endIndex,
                       int structure = 1, int reps = 5) {
    return plan_on(find_scenario(scenarioId), startIndex, endIndex, structure, reps);
}

inline CompareResult compare(const std::string& scenarioId, int startIndex, int endIndex,
                             int reps = 5) {
    if (reps < 1 || reps > 51) throw std::invalid_argument("reps must be 1..51");

    const scenario::Scenario& s = find_scenario(scenarioId);
    int start = 0, goal = 0;
    detail::resolve(s, startIndex, endIndex, &start, &goal);

    const Grid grid = s.to_grid();

    CompareResult out;
    out.scenarioId = s.id;
    out.reps = reps;
    for (int w = 0; w < structure_count(); ++w) {
        double ms = 0.0;
        const SearchResult res = detail::timed(grid, start, goal, w, reps, &ms);
        out.results.push_back({structure_name(w), ms, res.path_cost,
                               res.expansions, res.inserts, res.extracts});
    }
    return out;
}

}  // namespace planner
