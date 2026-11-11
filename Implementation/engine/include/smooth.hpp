#pragma once
// ---------------------------------------------------------------------------
// Turning a grid route into a flight line.
//
// A* on an 8-connected grid returns a staircase: every cell it passed through.
// That route is the shortest one *on the grid*, but a drone does not have to
// turn at every cell. If it can see a later cell in a straight line without
// touching an obstacle, it can fly there directly.
//
// This is greedy line-of-sight smoothing: from the current waypoint, jump to
// the farthest later cell on the route that is visible, and repeat.
//
// What it does and does not claim:
//   * the result never crosses an obstacle and never clips a corner
//   * it is never longer than the route it started from (triangle inequality)
//   * it is NOT claimed to be the shortest any-angle path that exists. That
//     would need Theta* or a visibility graph. This is a cheap, safe shortcut.
// ---------------------------------------------------------------------------
#include <cmath>
#include <cstdlib>
#include <vector>

#include "grid.hpp"

namespace route {

/**
 * True when a straight flight from the centre of cell `a` to the centre of
 * cell `b` touches only open cells.
 *
 * This visits every cell the segment passes through (a grid traversal, not
 * plain Bresenham, which picks one cell per column and can miss a cell the line
 * clips). The next border the line meets is decided in exact integer
 * arithmetic. When the line goes precisely through a cell corner it touches
 * four cells at one point; both side cells must be open, because a drone that
 * slips between two diagonal obstacles would clip them.
 */
inline bool line_clear(const Grid& g, int a, int b) {
    int x = g.x_of(a), y = g.y_of(a);
    const int x1 = g.x_of(b), y1 = g.y_of(b);
    const int dx = std::abs(x1 - x), dy = std::abs(y1 - y);
    const int sx = x < x1 ? 1 : -1, sy = y < y1 ? 1 : -1;

    if (g.blocked(x, y)) return false;

    int nx = 0, ny = 0;   // vertical and horizontal borders crossed so far
    while (nx < dx || ny < dy) {
        // The line reaches the next vertical border at t = (nx + 1/2) / dx and
        // the next horizontal one at t = (ny + 1/2) / dy. Cross-multiplying
        // compares them without a division.
        int move = 0;   // 1 = step in x, 2 = step in y, 3 = through the corner
        if (nx >= dx) {
            move = 2;
        } else if (ny >= dy) {
            move = 1;
        } else {
            const long long tx = (2LL * nx + 1) * dy;
            const long long ty = (2LL * ny + 1) * dx;
            move = tx < ty ? 1 : (tx > ty ? 2 : 3);
        }

        if (move == 3 && (g.blocked(x + sx, y) || g.blocked(x, y + sy))) return false;
        if (move & 1) { x += sx; ++nx; }
        if (move & 2) { y += sy; ++ny; }
        if (g.blocked(x, y)) return false;
    }
    return true;
}

/**
 * `path` runs start to goal. Consecutive cells of the original route are
 * always accepted as a fallback, even where the strict corner rule above would
 * refuse them, so smoothing can only ever shorten a route and never fail one.
 */
inline std::vector<int> smooth(const Grid& g, const std::vector<int>& path) {
    if (path.size() <= 2) return path;

    std::vector<int> out;
    out.push_back(path.front());

    size_t anchor = 0;
    while (anchor + 1 < path.size()) {
        size_t far = anchor + 1;
        for (size_t j = path.size() - 1; j > anchor + 1; --j) {
            if (line_clear(g, path[anchor], path[j])) { far = j; break; }
        }
        out.push_back(path[far]);
        anchor = far;
    }
    return out;
}

/**
 * Straight-line length of a polyline through cell centres, in the same
 * 1000-per-cell units as the search cost. Measuring the grid route and the
 * smoothed route with this one function keeps the comparison like for like:
 * the search charges 1414 for a diagonal where the true figure is 1414.2.
 */
inline int length_scaled(const Grid& g, const std::vector<int>& pts) {
    double sum = 0.0;
    for (size_t i = 1; i < pts.size(); ++i) {
        const double dx = g.x_of(pts[i]) - g.x_of(pts[i - 1]);
        const double dy = g.y_of(pts[i]) - g.y_of(pts[i - 1]);
        sum += std::sqrt(dx * dx + dy * dy);
    }
    return static_cast<int>(std::lround(sum * COST_STRAIGHT));
}

}  // namespace route
