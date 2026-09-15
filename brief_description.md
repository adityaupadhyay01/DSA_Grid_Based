# Brief Description

## Grid-Based Drone Path Planning Simulation

**Student:** Aditya Upadhyay
**Roll No.:** 2501330100038 · **Section:** CSE-A · **Semester:** III
**Course:** Data Structures and Algorithms II (CCSE0301)
**Faculty:** Mr. Shamshad Ali
**Institute:** NIET Greater Noida
**Domain:** Robotics and Autonomous Systems
**SDG Alignment:** SDG 9, Industry, Innovation and Infrastructure
**Review Stage:** Review 1 · Month 1 · **Progress: 25%**

---

## 1. Overview

This project simulates how a drone decides its route across a map divided into square cells. Some cells are open, some are blocked by obstacles. The drone starts at one cell and has to reach a goal cell without passing through anything blocked.

Underneath that, the project studies the data structures a planner depends on: where the map and obstacles are stored, and what holds the cells still waiting to be examined. These choices decide whether a planner is usable at the sizes real maps come in, and the project sets out to measure how much difference they make.

---

## 2. Background and Motivation

Drones are used for delivery, mapping, inspection, agriculture and rescue work. All of these come down to the same requirement: get from one point to another without hitting a building, a pole, a tree or a restricted zone. Working the route out in advance is called path planning.

The obvious approach is to examine every possible route and keep the shortest. It works on a small example and fails on a real one. A grid of a hundred rows and a hundred columns contains ten thousand cells, and the number of distinct routes through them is far too large to enumerate. The problem is worse on a drone than on a laptop, because the machine runs on a small onboard processor with a battery draining while it thinks. The answer has to arrive quickly and without using much memory.

Two difficulties follow, and they need different tools.

Storing the map. If every cell is kept as an individual record, memory use climbs sharply as the grid grows, and wide empty stretches get stored cell by cell even though nothing is in them. The map needs a structure that finds blocked and open regions quickly.

Representing movement. The planner repeatedly asks which cells it can reach from the current one and at what cost. Answering by scanning the whole map each time is unusable. The moves need a structure built for that query.

The first is a tree problem, the second a graph problem. That is what makes grid-based drone path planning a reasonable fit for DSA-II.

---

## 3. Why This Topic

Most examples in a data structures course involve sorting arrays or searching lists. The concepts are right, but at the sizes used in class every alternative finishes instantly, so the cost of a bad choice never shows.

Path planning behaves differently. Keeping the frontier in the wrong structure slows the search by orders of magnitude. Storing the map badly exhausts memory on a grid that is not even large. Here the structure decides whether the program finishes.

---

## 4. Research Work Completed

Month 1 was spent reading rather than building. Three papers were studied closely.

### 4.1 Base paper

Gao, W., Li, L., & Pang, D. (2026). *Urban low-altitude UAV path planning by fusing an enhanced A\* algorithm with an adaptive artificial potential field method.* Scientific Reports, 16, Article 18275.
https://doi.org/10.1038/s41598-026-45160-6

The authors propose A\*-APF, a multi-phase planner for drones in dense urban airspace. The city is divided into a uniform three-dimensional grid of cubes. The flight is split into take-off, cruise and landing phases, each with its own constraints. A\* searches the grid with a cost function extended beyond f = g + h to include obstacle-repulsion and altitude penalties. Two safety radii are drawn around the drone, controlling step size and deciding when the potential field takes over steering. Candidate neighbours are pruned by a field-of-view cone and by kinematic limits, and the route is smoothed with B-splines so it can actually be flown.

They report a 46.2 per cent reduction in computation time and 33 per cent fewer expanded nodes against the competing hybrid methods they reproduced. The full method completes in 0.53 seconds against 3.58 seconds for improved A\* alone.

### 4.2 Supporting papers

Meng, W., Zhang, X., Zhou, L., Guo, H., & Hu, X. (2025). *Advances in UAV Path Planning: A Comprehensive Review of Methods, Challenges, and Future Directions.* Drones, 9(5), 376. https://doi.org/10.3390/drones9050376

A survey covering grid-based, sampling-based and intelligent planners. It confirmed that A\* and Dijkstra are still the standard node-search methods for grid maps, which is why this project builds on A\* rather than something less established.

Dradoum, A., Khelassi, A., & Lachekhab, F. (2025). *Intelligent path planning algorithms for UAVs: Classification, complexity analysis, hybrid ablation insights, and future directions.* https://doi.org/10.1177/16878132251355020

Sorts planners into classes and works through their complexity. It treats complexity at the algorithm level and never goes down to the data-structure level.

### 4.3 The gap identified

All three papers optimise the search strategy and report the improvement in seconds. None of them reports the structures underneath the search.

This is clearest in the base paper, which explains the OPEN and CLOSED lists in words:

> the algorithm operates iteratively by selecting the node with the lowest cost from the OPEN list for expansion

The paper never states how that selection is implemented. There is no mention of a heap, a priority queue, a sorted array or a hash set anywhere in the algorithm description. The CLOSED-set membership test is also unspecified. Every headline claim in the paper is a wall-clock timing, and timing depends on this unstated choice.

Selecting the lowest-cost node has several implementations, and they are not close to each other:

| OPEN list implementation | Cost per extract-min | Total search cost |
|---|---|---|
| Unsorted array with linear scan | O(n) | O(n²) |
| Binary min-heap | O(log n) | O(n log n) |
| Bucket (Dial) queue | O(1) amortised | O(n) |

If a linear scan was used, part of the reported baseline comes from the data structure rather than the algorithm, and the comparison against competing methods is confounded. If a heap was used, it was not reported, and the result cannot be reproduced independently. Both cases leave something unaccounted for.

Amit Patel's A\* implementation notes at Stanford contain a section called *Set representation* listing these same options: unsorted arrays, sorted arrays, binary heaps, indexed arrays, hash tables and bucketing. The OPEN-list choice is therefore a recognised design decision with known consequences.

---

## 5. Proposed Work

The project holds the algorithm fixed and varies only the structure beneath it.

A single grid-based A\* planner will be built, with the OPEN list placed behind a common interface so structures can be substituted without altering the search logic. The same planner, on the same maps, with the same heuristic, will then run with each structure in turn, recording runtime and node expansions.

The claim being tested is that data-structure choice is an unreported but measurable factor in published path-planning results, and that its size can be quantified.

This project does not claim a better planner than the base paper. That work is three-dimensional, models flight dynamics, and applies potential fields and spline smoothing, all of which sit outside this course. This project works in two dimensions at fixed altitude, models no kinematics, and isolates one variable the base paper left undefined. The narrow scope is what makes the result possible to validate.

---

## 6. System Model

The workspace is a two-dimensional occupancy grid of W × H square cells at fixed altitude. Each cell holds an occupancy state, open or blocked, and its coordinates.

Movement is eight-connected. A straight step costs 1 and a diagonal step costs √2, approximately 1.414, since a diagonal covers more ground than a cardinal move. Treating them as equal produces wrong routes.

The search is A\* with the octile-distance heuristic, which is admissible and consistent on a uniform grid. The cost function is f(v) = g(v) + h(v), where g is the cost from the start and h the estimate to the goal.

For path recovery, every examined cell records the cell it was reached from. Each cell has at most one such parent, so the examined set forms a tree rooted at the start, and the route is recovered by walking parent links backwards from the goal.

Workflow:

1. Build the grid and mark blocked cells
2. Insert the start cell into the OPEN list
3. Remove the lowest-f cell from OPEN
4. If it is the goal, recover the path and stop
5. Otherwise generate its reachable neighbours, compute costs, insert or update them in OPEN
6. Repeat from step 3
7. Draw the grid, the obstacles and the resulting route

Step 3 is the operation this project measures. It runs once per expansion, and its cost is set by the structure holding OPEN.

---

## 7. Data Structures Identified

| DSA Topic | Best case | Worst case | Useful? | Role in this project |
|---|---|---|---|---|
| Unsorted Array | O(1) insert | O(n) extract-min | Baseline | Simplest OPEN list; the O(n) scan is what the heap is measured against |
| Min Heap | O(1) get min | O(log n) | Yes | Core structure: selects the lowest f = g + h cell from OPEN |
| AVL Tree | O(log n) | O(log n) | Yes | Balanced index of obstacle and visited cells |
| BST | O(log n) | O(n) | Limited | Same role, but degenerates to a linked list when cells are inserted in sorted grid order |
| Binary Tree | O(n) | O(n) | No | Recursive subdivision of the grid; no ordering, so search stays linear |
| Max Heap | O(1) get max | O(log n) | No | Highest-cost-first is not what shortest-path search needs |
| Threaded Binary Tree | O(n) traversal | O(n) traversal | No | Stack-free traversal of waypoint lists; useful but not core |
| Tree Traversal | O(n) | O(n) | Support | Rebuilding the final path from parent links |
| Graph | O(V+E)* | O(V+E)* | Yes | Cells as vertices, legal moves as weighted edges |
| Adjacency List | O(V+E)* | O(V+E)* | Yes | The grid graph is sparse, at most 8 neighbours per cell |
| Adjacency Matrix | O(1) edge check | O(1) edge check | No | O(V²) space is impossible: a 256×256 grid would need 65,536² entries |

\* O(V+E) refers to BFS/DFS traversal using an adjacency list. Heap costs are per operation, not per search.

The rejections took as much work as the selections. Ruling out the adjacency matrix required a concrete space calculation, and ruling out the max heap required working out what the search actually asks for at each step.

---

## 8. Expected Outcomes

- A working grid-based A\* planner producing collision-free routes on maps of varying size and obstacle density
- A measured comparison of OPEN-list structures on identical inputs, reporting runtime and node expansions
- A figure for how much of a planner's speed comes from its data structures rather than its heuristics
- Complexity tables, grid diagrams and timing charts supporting the above
- Reporting of the limits: two dimensions, no kinematics, no claim of beating the base paper

---

## 9. Progress at Review 1, 25%

| # | Activity | Status |
|---|---|---|
| 1 | Problem selection and topic approval | Complete |
| 2 | Understanding the path planning problem | Complete |
| 3 | Objectives and stakeholder identification | Complete |
| 4 | Literature review, three papers | Complete |
| 5 | Research gap identification | Complete |
| 6 | DSA-II Unit 1 (Trees) study | Complete |
| 7 | DSA-II Unit 2 (Graphs) study | Complete |
| 8 | Mapping DSA concepts to project operations | Complete |
| 9 | Conceptual design of grid, cost and search model | Complete |
| 10 | Implementation | Not started |
| 11 | Measurement and comparison | Not started |
| 12 | Final analysis and report | Not started |

Nothing has been implemented. The 25% covers completed research, gap identification, structure selection and conceptual design.

---

## 10. Remaining Work

**Review 2, to approximately 55%.** Confirm the gap by re-checking the base paper for any statement of its OPEN-list structure. Settle the structures for map storage, movement modelling and frontier selection. Design the graph linking cells, moves and costs. Build a prototype planner with the OPEN list behind a common interface. Test insertion, search, traversal and path recovery, then run the first timing comparison between a plain array and a binary heap.

**Review 3, to approximately 80%.** Extend into Units 3 and 4. Apply dynamic programming to battery and payload allocation across waypoints. Model multi-waypoint visiting order as a travelling salesman instance and solve it with backtracking and branch-and-bound, reusing the priority queue built in Unit 1.

**Final, 100%.** Complete the structure comparison across map sizes and obstacle densities. Compare the binary heap against the advanced heaps from Unit 5. Produce the full complexity analysis, charts and final report.

---

## 11. Scope and Limitations

- Two dimensions at fixed altitude; the base paper works in three
- No flight dynamics, turning radius, potential fields or spline smoothing
- Static maps with known obstacles; no sensing or replanning mid-flight
- Simulation only; no hardware
- The comparison is between data structures, not between planners

---

## 12. References

1. Gao, W., Li, L., & Pang, D. (2026). Urban low-altitude UAV path planning by fusing an enhanced A\* algorithm with an adaptive artificial potential field method. *Scientific Reports, 16*, 18275. https://doi.org/10.1038/s41598-026-45160-6
2. Meng, W., Zhang, X., Zhou, L., Guo, H., & Hu, X. (2025). Advances in UAV Path Planning: A Comprehensive Review of Methods, Challenges, and Future Directions. *Drones, 9*(5), 376. https://doi.org/10.3390/drones9050376
3. Dradoum, A., Khelassi, A., & Lachekhab, F. (2025). Intelligent path planning algorithms for UAVs: Classification, complexity analysis, hybrid ablation insights, and future directions. https://doi.org/10.1177/16878132251355020
4. Hart, P. E., Nilsson, N. J., & Raphael, B. (1968). A Formal Basis for the Heuristic Determination of Minimum Cost Paths. *IEEE Transactions on Systems Science and Cybernetics, 4*(2), 100–107. https://doi.org/10.1109/TSSC.1968.300136
5. Cormen, T. H., Leiserson, C. E., Rivest, R. L., & Stein, C. (2022). *Introduction to Algorithms* (4th ed.). MIT Press.
6. Patel, A. *Amit's A\* Pages, Implementation Notes.* Stanford University. https://theory.stanford.edu/~amitp/GameProgramming/index.html

The complete source list, including background papers, books and video lectures, is in `evidence.md`.

---

*Review 1 · Month 1 · Updated at the end of each review stage.*
