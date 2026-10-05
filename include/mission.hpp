#pragma once
#include <vector>
#include <algorithm>
#include <limits>
#include <cstdint>
#include "grid.hpp"
#include "astar.hpp"
#include "open_list.hpp"

// ---------------------------------------------------------------------------
// Mission layer: Units 3 and 4.
//
// The Unit 1/2 planner answers "what is the cheapest route between two cells".
// A real delivery mission asks two further questions that A* cannot answer:
//
//   Unit 3  Which waypoints should be visited at all, given a battery budget?
//           -> 0/1 Knapsack, solved by dynamic programming
//
//   Unit 4  In what order should the chosen ones be visited?
//           -> Travelling Salesman, solved by branch and bound
//
// A* supplies the pairwise distances both layers depend on.
// ---------------------------------------------------------------------------

struct Waypoint {
    int cell;       // grid cell id
    int value;      // delivery priority or payload value
    int cost;       // battery units consumed by servicing this stop
};

// ---------------------------------------------------------------------------
// Distance matrix built by running A* between every pair of waypoints.
// This is where Units 1 and 2 feed Units 3 and 4.
// ---------------------------------------------------------------------------
inline std::vector<std::vector<int>>
build_distance_matrix(const Grid& grid, const std::vector<Waypoint>& wps) {
    const int n = static_cast<int>(wps.size());
    std::vector<std::vector<int>> d(n, std::vector<int>(n, 0));
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            BinaryHeapOpenList open;
            SearchResult r = astar(grid, wps[i].cell, wps[j].cell, open);
            int cost = r.found ? r.path_cost : std::numeric_limits<int>::max() / 4;
            d[i][j] = d[j][i] = cost;      // symmetric on an undirected grid
        }
    }
    return d;
}

// ---------------------------------------------------------------------------
// UNIT 3 — 0/1 Knapsack by dynamic programming.
//
// Choose the subset of waypoints with the greatest total value whose combined
// battery cost fits the budget. Each waypoint is taken once or not at all,
// which is what makes it 0/1 rather than fractional.
//
// Recurrence:
//   dp[i][w] = max( dp[i-1][w],                        skip item i
//                   dp[i-1][w - cost_i] + value_i )    take item i
//
// Time  O(n * W)   Space O(n * W), kept in full so the choice can be traced
// back. A rolling two-row version would be O(W) space but loses the trace.
//
// Brute force over subsets is O(2^n): 20 waypoints is a million combinations,
// 30 is a billion. The DP table for 20 waypoints and a 1000-unit budget is
// 20,000 cells.
// ---------------------------------------------------------------------------
struct KnapsackResult {
    int total_value = 0;
    int total_cost  = 0;
    std::vector<int> chosen;          // indices into the waypoint list
    size_t table_cells = 0;           // work done, for the complexity comparison
};

inline KnapsackResult knapsack_select(const std::vector<Waypoint>& wps, int budget) {
    const int n = static_cast<int>(wps.size());
    std::vector<std::vector<int>> dp(n + 1, std::vector<int>(budget + 1, 0));

    for (int i = 1; i <= n; ++i) {
        const int ci = wps[i - 1].cost, vi = wps[i - 1].value;
        for (int w = 0; w <= budget; ++w) {
            dp[i][w] = dp[i - 1][w];                         // skip
            if (ci <= w) {
                int take = dp[i - 1][w - ci] + vi;            // take
                if (take > dp[i][w]) dp[i][w] = take;
            }
        }
    }

    // Trace back which items were taken.
    KnapsackResult res;
    res.total_value = dp[n][budget];
    res.table_cells = static_cast<size_t>(n + 1) * (budget + 1);
    int w = budget;
    for (int i = n; i > 0; --i) {
        if (dp[i][w] != dp[i - 1][w]) {                       // item i was taken
            res.chosen.push_back(i - 1);
            res.total_cost += wps[i - 1].cost;
            w -= wps[i - 1].cost;
        }
    }
    std::reverse(res.chosen.begin(), res.chosen.end());
    return res;
}

// Brute-force knapsack, kept only to show the gap DP closes.
// Safe to call for n <= 25.
inline int knapsack_bruteforce(const std::vector<Waypoint>& wps, int budget,
                               size_t& subsets_examined) {
    const int n = static_cast<int>(wps.size());
    int best = 0;
    subsets_examined = 0;
    for (uint32_t mask = 0; mask < (1u << n); ++mask) {
        ++subsets_examined;
        int c = 0, v = 0;
        for (int i = 0; i < n; ++i)
            if (mask & (1u << i)) { c += wps[i].cost; v += wps[i].value; }
        if (c <= budget && v > best) best = v;
    }
    return best;
}

// ---------------------------------------------------------------------------
// UNIT 4 — Travelling Salesman by branch and bound.
//
// Visit every chosen waypoint exactly once, starting and ending at the depot,
// minimising total distance. Distances come from A*.
//
// Backtracking alone explores every permutation: O(n!). Branch and bound adds
// a lower bound on the best completion of a partial tour, and abandons any
// branch whose bound already exceeds the best complete tour found. The answer
// is still exact; only the search shrinks.
//
// Bound used: cost so far, plus for every unvisited city the cheapest edge
// leaving it. That can never exceed the true remaining cost, so pruning on it
// never discards the optimum.
// ---------------------------------------------------------------------------
struct TourResult {
    int  cost = 0;
    std::vector<int> order;           // indices into the distance matrix
    size_t nodes_explored = 0;        // branch-and-bound search tree nodes
    size_t nodes_pruned   = 0;        // branches cut by the bound
};

class BranchAndBoundTSP {
public:
    explicit BranchAndBoundTSP(const std::vector<std::vector<int>>& d)
        : d_(d), n_(static_cast<int>(d.size())),
          visited_(d.size(), false), best_(std::numeric_limits<int>::max()) {
        min_out_.resize(n_);
        for (int i = 0; i < n_; ++i) {                 // cheapest edge from each city
            int m = std::numeric_limits<int>::max();
            for (int j = 0; j < n_; ++j)
                if (i != j) m = std::min(m, d_[i][j]);
            min_out_[i] = (m == std::numeric_limits<int>::max()) ? 0 : m;
        }
    }

    TourResult solve(int depot = 0) {
        path_.clear();
        path_.push_back(depot);
        visited_[depot] = true;
        recurse(depot, 0, 1);
        TourResult r;
        r.cost = best_;
        r.order = best_path_;
        r.nodes_explored = explored_;
        r.nodes_pruned   = pruned_;
        return r;
    }

private:
    const std::vector<std::vector<int>>& d_;
    int n_;
    std::vector<bool> visited_;
    std::vector<int>  path_, best_path_, min_out_;
    int    best_;
    size_t explored_ = 0, pruned_ = 0;

    // Lower bound on any completion of the current partial tour.
    int bound(int so_far) const {
        int b = so_far;
        for (int i = 0; i < n_; ++i)
            if (!visited_[i]) b += min_out_[i];
        return b;
    }

    void recurse(int city, int so_far, int depth) {
        ++explored_;

        if (depth == n_) {                              // tour complete, close it
            int total = so_far + d_[city][path_[0]];
            if (total < best_) { best_ = total; best_path_ = path_; }
            return;
        }

        for (int next = 0; next < n_; ++next) {
            if (visited_[next]) continue;
            int cost = so_far + d_[city][next];

            visited_[next] = true;
            path_.push_back(next);

            // THE PRUNE. Without this line the search is plain backtracking
            // and explores every one of the (n-1)! permutations.
            if (bound(cost) < best_) {
                recurse(next, cost, depth + 1);
            } else {
                ++pruned_;
            }

            path_.pop_back();
            visited_[next] = false;
        }
    }
};

// Plain backtracking, no bound. Kept to measure what the bound is worth.
class BacktrackingTSP {
public:
    explicit BacktrackingTSP(const std::vector<std::vector<int>>& d)
        : d_(d), n_(static_cast<int>(d.size())),
          visited_(d.size(), false), best_(std::numeric_limits<int>::max()) {}

    TourResult solve(int depot = 0) {
        path_.clear();
        path_.push_back(depot);
        visited_[depot] = true;
        recurse(depot, 0, 1);
        TourResult r;
        r.cost = best_;
        r.order = best_path_;
        r.nodes_explored = explored_;
        r.nodes_pruned   = 0;
        return r;
    }

private:
    const std::vector<std::vector<int>>& d_;
    int n_;
    std::vector<bool> visited_;
    std::vector<int>  path_, best_path_;
    int    best_;
    size_t explored_ = 0;

    void recurse(int city, int so_far, int depth) {
        ++explored_;
        if (depth == n_) {
            int total = so_far + d_[city][path_[0]];
            if (total < best_) { best_ = total; best_path_ = path_; }
            return;
        }
        for (int next = 0; next < n_; ++next) {
            if (visited_[next]) continue;
            visited_[next] = true;
            path_.push_back(next);
            recurse(next, so_far + d_[city][next], depth + 1);
            path_.pop_back();
            visited_[next] = false;
        }
    }
};
