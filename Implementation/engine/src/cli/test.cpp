#include <cstdio>
#include <memory>
#include <vector>
#include <cmath>
#include "grid.hpp"
#include "astar.hpp"
#include "open_list.hpp"
#include "scenario.hpp"

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

    std::printf("\nTest 7: the Module 5 heaps agree with the binary heap\n");
    {
        // Four more structures behind the same interface. If any of them
        // returns cells out of order the route cost changes, so one cross-check
        // covers correctness for all of them at once.
        int cost_mismatch = 0, reach_mismatch = 0;
        for (int t = 0; t < 30; ++t) {
            Grid g(50, 50);
            int s = g.index(0, 0), gl = g.index(49, 49);
            g.randomise(0.22, 9100 + t, s, gl);

            BinaryHeapOpenList    ref;   auto rr = astar(g, s, gl, ref);
            FourAryHeapOpenList   four;  auto r4 = astar(g, s, gl, four);
            BinomialHeapOpenList  binom; auto rb = astar(g, s, gl, binom);
            FibonacciHeapOpenList fib;   auto rf = astar(g, s, gl, fib);

            if (rr.found != r4.found || rr.found != rb.found || rr.found != rf.found)
                ++reach_mismatch;
            if (rr.found && (rr.path_cost != r4.path_cost ||
                             rr.path_cost != rb.path_cost ||
                             rr.path_cost != rf.path_cost))
                ++cost_mismatch;
        }
        check(reach_mismatch == 0, "4-ary, binomial and Fibonacci agree on reachability");
        check(cost_mismatch == 0,  "all four heaps return the same optimal cost");
    }

    std::printf("\nTest 8: each Module 5 heap drains in sorted order\n");
    {
        const int keys[] = {41, 7, 93, 18, 55, 2, 77, 30, 64, 11};
        const int want[] = {2, 7, 11, 18, 30, 41, 55, 64, 77, 93};

        FourAryHeapOpenList   four;
        BinomialHeapOpenList  binom;
        FibonacciHeapOpenList fib;
        for (int i = 0; i < 10; ++i) {
            four.insert(keys[i], keys[i]);
            binom.insert(keys[i], keys[i]);
            fib.insert(keys[i], keys[i]);
        }
        bool ok4 = true, okb = true, okf = true;
        for (int i = 0; i < 10; ++i) {
            if (four.extract_min()  != want[i]) ok4 = false;
            if (binom.extract_min() != want[i]) okb = false;
            if (fib.extract_min()   != want[i]) okf = false;
        }
        check(ok4, "4-ary heap returns keys in order");
        check(okb, "binomial heap returns keys in order");
        check(okf, "Fibonacci heap returns keys in order");
    }

    std::printf("\nTest 9: every demo route is reachable and all six structures agree\n");
    {
        // The three hand-built maps that the demo server serves. A demonstration
        // that cannot find a route in front of an audience is worse than no
        // demonstration, so every start/end pair is checked on every build.
        int unreachable = 0, disagreements = 0, pairs = 0;
        for (const auto& sc : scenario::all()) {
            const Grid g = sc.to_grid();
            for (const auto& st : sc.starts) {
                for (const auto& en : sc.ends) {
                    ++pairs;
                    const int s_id = sc.index(st.x, st.y);
                    const int g_id = sc.index(en.x, en.y);

                    int agreed = -1;
                    for (int w = 0; w < 6; ++w) {
                        std::unique_ptr<OpenList> open;
                        switch (w) {
                            case 0: open = std::make_unique<UnsortedArrayOpenList>(); break;
                            case 1: open = std::make_unique<BinaryHeapOpenList>();    break;
                            case 2: open = std::make_unique<BucketQueueOpenList>();   break;
                            case 3: open = std::make_unique<FourAryHeapOpenList>();   break;
                            case 4: open = std::make_unique<BinomialHeapOpenList>();  break;
                            default: open = std::make_unique<FibonacciHeapOpenList>();
                        }
                        const SearchResult r = astar(g, s_id, g_id, *open);
                        if (!r.found) { ++unreachable; break; }
                        if (agreed < 0) agreed = r.path_cost;
                        else if (agreed != r.path_cost) { ++disagreements; break; }
                    }
                }
            }
        }
        std::printf("       %d start/end pairs over %zu maps\n", pairs, scenario::all().size());
        check(unreachable == 0,   "every demo start reaches every demo delivery point");
        check(disagreements == 0, "all six structures return the same cost on every demo route");
    }

    std::printf("\nTest 10: a demo start or delivery point is never inside an obstacle\n");
    {
        int buried = 0;
        for (const auto& sc : scenario::all()) {
            for (const auto& p : sc.starts)
                if (sc.kind[sc.index(p.x, p.y)] != scenario::FREE) ++buried;
            for (const auto& p : sc.ends)
                if (sc.kind[sc.index(p.x, p.y)] != scenario::FREE) ++buried;
        }
        check(buried == 0, "all 18 demo points stand on open ground");
    }

    std::printf("\n%s (%d failure%s)\n",
                failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED",
                failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
