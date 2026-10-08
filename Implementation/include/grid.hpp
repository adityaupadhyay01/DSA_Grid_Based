#pragma once
#include <cstdint>
#include <vector>
#include <random>

// Movement costs are integers scaled by 1000 so that the bucket queue is
// usable. A straight step is 1.000, a diagonal step is sqrt(2) = 1.414.
constexpr int COST_STRAIGHT = 1000;
constexpr int COST_DIAGONAL = 1414;
constexpr int COST_MAX      = COST_DIAGONAL;   // C, the bucket-queue window width

// The occupancy grid stays a flat array on purpose.
// "Is cell (x,y) blocked?" is the hottest query in the planner (~8 per
// expansion). A flat array answers it in O(1) by index arithmetic. A tree
// would answer in O(log W) and add pointer chasing for no benefit.
class Grid {
public:
    Grid(int w, int h) : w_(w), h_(h), blocked_(static_cast<size_t>(w) * h, 0) {}

    int width()  const { return w_; }
    int height() const { return h_; }
    int cells()  const { return w_ * h_; }

    inline int  index(int x, int y) const { return y * w_ + x; }
    inline int  x_of(int id)        const { return id % w_; }
    inline int  y_of(int id)        const { return id / w_; }

    inline bool in_bounds(int x, int y) const {
        return x >= 0 && y >= 0 && x < w_ && y < h_;
    }
    inline bool blocked(int id)         const { return blocked_[id] != 0; }
    inline bool blocked(int x, int y)   const { return blocked_[index(x, y)] != 0; }
    inline void set_blocked(int x, int y, bool b) { blocked_[index(x, y)] = b ? 1 : 0; }

    // Random obstacles at a given density, leaving start and goal free.
    void randomise(double density, uint32_t seed, int start_id, int goal_id) {
        std::mt19937 rng(seed);
        std::uniform_real_distribution<double> u(0.0, 1.0);
        for (int i = 0; i < cells(); ++i)
            blocked_[i] = (u(rng) < density) ? 1 : 0;
        blocked_[start_id] = 0;
        blocked_[goal_id]  = 0;
    }

    // Octile distance, admissible and consistent on a uniform 8-connected grid.
    // Never overestimates the true remaining cost, which is what guarantees
    // that A* returns an optimal route.
    inline int heuristic(int id, int goal) const {
        int dx = std::abs(x_of(id) - x_of(goal));
        int dy = std::abs(y_of(id) - y_of(goal));
        int lo = dx < dy ? dx : dy;
        int hi = dx < dy ? dy : dx;
        return COST_DIAGONAL * lo + COST_STRAIGHT * (hi - lo);
    }

private:
    int w_, h_;
    std::vector<uint8_t> blocked_;
};
