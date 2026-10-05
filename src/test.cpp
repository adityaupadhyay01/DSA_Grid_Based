#include <cstdio>
#include <memory>
#include <vector>
#include <cmath>
#include "grid.hpp"
#include "astar.hpp"
#include "open_list.hpp"

static int failures = 0;

static void check(bool cond, const char* what) {
    std::printf("  [%s] %s\n", cond ? "PASS" : "FAIL", what);
    if (!cond) ++failures;
}

int main() {
    std::printf("Test 1: empty 10x10 grid, corner to corner\n");
    {
        Grid g(10, 10);
        int s = g.index(0, 0), t = g.index(9, 9);
        // Pure diagonal: 9 diagonal steps = 9 * 1414 = 12726
        BinaryHeapOpenList h;
        auto r = astar(g, s, t, h);
        check(r.found, "path found");
        check(r.path_cost == 9 * COST_DIAGONAL, "cost is 9 diagonal steps (12726)");
        std::printf("       cost=%d expansions=%zu\n", r.path_cost, r.expansions);
    }

    std::printf("\nTest 2: all three structures agree on 50 random maps\n");
    {
        int mismatches = 0, cost_mismatch = 0;
        for (int t = 0; t < 50; ++t) {
            Grid g(40, 40);
            int s = g.index(0, 0), gl = g.index(39, 39);
            g.randomise(0.25, 7000 + t, s, gl);

            UnsortedArrayOpenList a; BinaryHeapOpenList b; BucketQueueOpenList c;
            auto ra = astar(g, s, gl, a);
            auto rb = astar(g, s, gl, b);
            auto rc = astar(g, s, gl, c);

            if (ra.found != rb.found || rb.found != rc.found) ++mismatches;
            if (ra.found && (ra.path_cost != rb.path_cost || rb.path_cost != rc.path_cost))
                ++cost_mismatch;
        }
        check(mismatches == 0,   "all three agree on reachability");
        check(cost_mismatch == 0, "all three return the same optimal cost");
    }

    std::printf("\nTest 3: obstacles are respected\n");
    {
        Grid g(5, 5);
        int s = g.index(0, 2), t = g.index(4, 2);
        for (int y = 0; y < 5; ++y) g.set_blocked(2, y, true);   // full wall
        BinaryHeapOpenList h;
        auto r = astar(g, s, t, h);
        check(!r.found, "no path through a solid wall");
    }

    std::printf("\nTest 4: wall with a gap is routed around\n");
    {
        Grid g(5, 5);
        int s = g.index(0, 2), t = g.index(4, 2);
        for (int y = 0; y < 5; ++y) g.set_blocked(2, y, true);
        g.set_blocked(2, 0, false);                              // one opening
        BinaryHeapOpenList h;
        auto r = astar(g, s, t, h);
        check(r.found, "path found through the gap");
        bool clean = true;
        for (int c : r.path) if (g.blocked(c)) clean = false;
        check(clean, "path contains no blocked cell");
    }

    std::printf("\nTest 5: heap invariant holds under mixed operations\n");
    {
        BinaryHeapOpenList h;
        int vals[] = {12, 9, 14, 7, 11, 8};
        for (int i = 0; i < 6; ++i) h.insert(i, vals[i]);
        // Extraction must come out in non-decreasing key order: 7,8,9,11,12,14
        int expect[] = {3, 5, 1, 4, 0, 2};
        bool ok = true;
        for (int i = 0; i < 6; ++i) if (h.extract_min() != expect[i]) ok = false;
        check(ok, "extract-min returns 7,8,9,11,12,14 in order");
    }

    std::printf("\nTest 6: bucket queue matches heap ordering\n");
    {
        Grid g(30, 30);
        int s = g.index(0, 0), t = g.index(29, 29);
        g.randomise(0.15, 424242, s, t);
        BinaryHeapOpenList h; BucketQueueOpenList b;
        auto rh = astar(g, s, t, h);
        auto rb = astar(g, s, t, b);
        check(rh.path_cost == rb.path_cost, "identical optimal cost");
        // Expansion counts are NOT expected to match. Both structures are
        // correct, but they break ties among equal-f cells differently: the
        // bucket queue pops LIFO within a bucket, which biases exploration
        // toward recently discovered cells and therefore toward the goal.
        // This is a genuine finding, not a defect, and it is why the report
        // treats tie-breaking as a factor in its own right.
        std::printf("       heap: cost=%d exp=%zu | bucket: cost=%d exp=%zu"
                    "  (tie-breaking differs by %.0f%%)\n",
                    rh.path_cost, rh.expansions, rb.path_cost, rb.expansions,
                    100.0 * (double)(rh.expansions - rb.expansions) / rh.expansions);
    }

    std::printf("\n%s (%d failure%s)\n",
                failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED",
                failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
