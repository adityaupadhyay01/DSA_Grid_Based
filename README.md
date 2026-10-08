# Grid-Based Drone Path Planning Simulation

PBL project for Data Structures and Algorithms II (CCSE0301), Review 2, Month 2.

Aditya Upadhyay · 2501330100038 · B.Tech CSE-A, Semester III · NIET Greater Noida
Faculty: Mr. Shamshad Ali · SDG 9, Industry, Innovation and Infrastructure

A drone crosses a map of square cells and reaches its goal without flying through
anything blocked. What this project measures is one level below that: how much of a
planner's speed comes from the data structure holding its frontier, rather than from
the algorithm on top.

## Where everything is

```
Implementation/     the working code, its recorded outputs and the charts
papers/             the base paper and the supporting review, as PDFs
Documentation/      the two progress reports, the brief description, the evidence file
Planning/           design book, wireframes and flowcharts, build plan, research gap
```

### Implementation

```
include/grid.hpp        occupancy grid, integer costs, octile heuristic
include/open_list.hpp   the OpenList interface and all three implementations
include/astar.hpp       the search, which depends only on the interface
include/mission.hpp     knapsack waypoint selection and both tour solvers
src/main.cpp            benchmark runner, writes results.csv
src/test.cpp            six-test correctness suite
src/mission_demo.cpp    one mission end to end
src/scaling.cpp         sweeps input size to find where DP and the bound pay off
scripts/plot.py         charts from results.csv
```

Build and run:

```bash
cd Implementation
make            # builds planner, tests, mission and scaling
make test       # six tests, all should pass
./mission       # one mission end to end
./scaling       # the crossover tables
make bench      # regenerates results.csv on your machine
python3 scripts/plot.py
```

Requires g++ with C++17. Built with `-O2`, because comparing an unoptimised build
against an optimised one gives a meaningless result.

### Planning

`DESIGN.md` is the design book and the best place to start if you want the logic
without reading source: the four layers, every algorithm in pseudocode, flowcharts
for A\*, the knapsack table and the bound, the terminal screen wireframes, the
complexity table and the test plan. The same document is in
`Drone_Planner_Design_Book.pdf` for printing and `design-book.html` for the browser.

`PROJECT_DOCUMENTATION.md` is the long-form write-up, sixteen sections.
`RESEARCH_GAP.md` sets out the base paper and the gap this project works in.
`BUILD_PLAN.md` is the phased plan the code was committed in.

## The gap this addresses

The base paper is Gao, Li and Pang (2026), *Scientific Reports* 16:18275, in
`papers/`. Every result in it is a time in seconds, but the paper describes its OPEN
list only in words and never says what structure holds it. Picking the minimum costs
O(n) from an array, O(log n) from a heap, or O(1) from a bucket queue, so part of any
reported speed could be coming from the structure, and there is no way to tell from
the paper.

This project holds A\* fixed and swaps only the OPEN list.

## Results so far

At 128×128 with 20% obstacle density, medians over ten maps:

| Structure | Time |
|---|---|
| Unsorted array, O(n²) | 3.421 ms |
| Binary heap, O(log n) | 0.244 ms |
| Bucket queue, O(1) | 0.144 ms |

Going from 128×128 to 256×256 is four times the cells: the heap grows 4.4× and the
bucket queue 4.0×, so the bucket queue tracks the cell count almost exactly and the
log factor can be watched going away rather than argued from the bound.

These are wall-clock times on one machine. Re-run `make bench` on yours and the
absolute milliseconds will differ; the ratios are the part that holds.

## Status

Review 2, about 70% of the project. Still ahead: the 4-ary heap variant, MovingAI
benchmark maps in place of random ones, obstacle-density sweeps, and the comparison
against binomial and Fibonacci heaps.
