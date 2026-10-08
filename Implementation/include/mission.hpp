#pragma once
#include <vector>
#include <algorithm>
#include <limits>
#include <cstdint>
#include "grid.hpp"
#include "astar.hpp"
#include "open_list.hpp"

// ---------------------------------------------------------------------------
// Mission layer: dynamic programming and state-space search on the planner.
//
// The planner answers "what is the cheapest route between two cells".
// A delivery mission asks a further question that A* cannot answer:
//
//   Which waypoints should be visited at all, given a battery budget?
//   -> 0/1 Knapsack, solved by dynamic programming
//
//   In what order should the chosen ones be visited?
//   -> Travelling Salesman, solved by branch and bound
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
// This is where the search layer feeds the mission layer.
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
// 0/1 Knapsack by dynamic programming.
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
// Visiting order: Travelling Salesman by branch and bound.
//
// Once the knapsack has chosen which stops to serve, the remaining question is
// the order to serve them in. Plain backtracking walks every permutation, so
// the tree holds O(n!) nodes. Branch and bound adds a lower bound on the best
// possible completion of a partial tour and abandons any branch whose bound
// already exceeds the best complete tour found so far. The answer is identical;
// only the amount of tree searched changes.
//
// The bound used here is: cost committed so far, plus the cheapest outgoing
// edge of every stop not yet visited. That can never overstate the true
// completion cost, because any real completion must leave each unvisited stop
// along some edge, and no edge is cheaper than that stop's cheapest one. A
// bound that could overstate would prune the optimum and silently return a
// worse tour.
// ---------------------------------------------------------------------------

struct TourResult {
    std::vector<int> order;           // visiting order, starting at the depot
    int    cost           = 0;        // closed tour cost, depot back to depot
    size_t nodes_explored = 0;        // nodes entered in the search tree
    size_t nodes_pruned   = 0;        // branches cut by the bound
};

class BranchAndBoundTSP {
public:
    explicit BranchAndBoundTSP(const std::vector<std::vector<int>>& d)
        : d_(d), n_(static_cast<int>(d.size())) {
        // Cheapest outgoing edge per stop, precomputed once for the bound.
        cheapest_.assign(n_, 0);
        for (int i = 0; i < n_; ++i) {
            int best = std::numeric_limits<int>::max();
            for (int j = 0; j < n_; ++j)
                if (i != j) best = std::min(best, d_[i][j]);
            cheapest_[i] = (best == std::numeric_limits<int>::max()) ? 0 : best;
        }
    }

    TourResult solve(int depot = 0) {
        best_cost_ = std::numeric_limits<int>::max();
        visited_.assign(n_, false);
        path_.clear();
        result_ = TourResult{};

        visited_[depot] = true;
        path_.push_back(depot);
        recurse(depot, depot, 1, 0);

        result_.cost  = best_cost_;
        result_.order = best_order_;
        return result_;
    }

private:
    // Lower bound on any completion of the current partial tour.
    int bound(int cost_so_far) const {
        int b = cost_so_far;
        for (int i = 0; i < n_; ++i)
            if (!visited_[i]) b += cheapest_[i];
        return b;
    }

    void recurse(int depot, int at, int depth, int cost_so_far) {
        ++result_.nodes_explored;

        if (depth == n_) {                              // tour complete, close it
            const int total = cost_so_far + d_[at][depot];
            if (total < best_cost_) { best_cost_ = total; best_order_ = path_; }
            return;
        }

        for (int next = 0; next < n_; ++next) {
            if (visited_[next]) continue;
            const int step = cost_so_far + d_[at][next];

            visited_[next] = true;
            path_.push_back(next);

            // THE PRUNE. Without this line the search is plain backtracking.
            if (bound(step) < best_cost_) {
                recurse(depot, next, depth + 1, step);
            } else {
                ++result_.nodes_pruned;
            }

            path_.pop_back();
            visited_[next] = false;
        }
    }

    const std::vector<std::vector<int>>& d_;
    int n_;
    std::vector<int> cheapest_, path_, best_order_;
    std::vector<bool> visited_;
    int best_cost_ = 0;
    TourResult result_;
};

// Plain backtracking, no bound. Kept to measure what the bound is worth.
class BacktrackingTSP {
public:
    explicit BacktrackingTSP(const std::vector<std::vector<int>>& d)
        : d_(d), n_(static_cast<int>(d.size())) {}

    TourResult solve(int depot = 0) {
        best_cost_ = std::numeric_limits<int>::max();
        visited_.assign(n_, false);
        path_.clear();
        result_ = TourResult{};

        visited_[depot] = true;
        path_.push_back(depot);
        recurse(depot, depot, 1, 0);

        result_.cost  = best_cost_;
        result_.order = best_order_;
        return result_;
    }

private:
    void recurse(int depot, int at, int depth, int cost_so_far) {
        ++result_.nodes_explored;
        if (depth == n_) {
            const int total = cost_so_far + d_[at][depot];
            if (total < best_cost_) { best_cost_ = total; best_order_ = path_; }
            return;
        }
        for (int next = 0; next < n_; ++next) {
            if (visited_[next]) continue;
            visited_[next] = true;
            path_.push_back(next);
            recurse(depot, next, depth + 1, cost_so_far + d_[at][next]);
            path_.pop_back();
            visited_[next] = false;
        }
    }

    const std::vector<std::vector<int>>& d_;
    int n_;
    std::vector<int> path_, best_order_;
    std::vector<bool> visited_;
    int best_cost_ = 0;
    TourResult result_;
};
