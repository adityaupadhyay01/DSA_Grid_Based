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

## 7. The DSA Concepts, Explained

This section works through each concept from Unit 1 and Unit 2, what it does, and whether it belongs in this project. The examples are small enough to check by hand.

### 7.1 The unsorted array, and why it is the baseline

The simplest possible OPEN list is a plain array. Adding a cell means putting it at the end, which costs O(1). Finding the cheapest cell means comparing every element, which costs O(n).

With 5,000 cells in OPEN, one extraction costs 5,000 comparisons. A\* extracts once per expansion, so across 10,000 expansions that is roughly 50 million comparisons spent only on deciding what to look at next.

The array is in this project as the control. Everything else gets measured against it.

### 7.2 The binary heap, and why it is the main structure

A binary heap is a complete binary tree with one rule: **every parent is smaller than or equal to both its children.** That rule alone guarantees the smallest element sits at the root, where it can be read in O(1).

It is stored as a plain array with no pointers at all. For an element at index i:

```
parent(i) = (i - 1) / 2
left(i)   = 2i + 1
right(i)  = 2i + 2
```

**Insertion.** Put the new element at the end of the array, then compare it with its parent and swap upward until the rule holds again. This is called sift-up. Inserting f-values 12, 9, 14, 7, 11, 8 in order:

| Step | Action | Array after |
|---|---|---|
| 1 | insert 12 | `[12]` |
| 2 | insert 9, swap with 12 | `[9, 12]` |
| 3 | insert 14, no swap | `[9, 12, 14]` |
| 4 | insert 7, swaps twice up to root | `[7, 9, 14, 12]` |
| 5 | insert 11, no swap (11 > 9) | `[7, 9, 14, 12, 11]` |
| 6 | insert 8, swaps once with 14 | `[7, 9, 8, 12, 11, 14]` |

As a tree:

```
            7
          /   \
         9     8
        / \   /
      12  11 14
```

**Extract-min.** Take the root, move the last element into its place, then compare it with its children and swap downward until the rule holds. This is sift-down. Removing 7: the last element 14 goes to the root, compares against children 9 and 8, swaps with the smaller one, 8. Result `[8, 9, 14, 12, 11]`, new minimum 8. Two comparisons and one swap.

**Why it is O(log n).** A complete binary tree with n nodes has height ⌊log₂ n⌋. Sifting up or down travels at most one level per step, so both operations touch at most log₂ n elements. With 5,000 cells in OPEN, log₂ 5000 is about 12. Compare that with the array's 5,000.

**Build-heap is O(n), not O(n log n).** Building a heap from n unsorted elements by sifting down from the middle of the array costs Θ(n), because most nodes sit near the leaves where sift-down has almost nowhere to travel. Only the root can travel the full height.

### 7.3 Binary search tree, and why it breaks here

A BST keeps everything ordered: for any node, the left subtree is smaller and the right subtree is larger. Searching means comparing and going left or right, halving the search space each time. That is O(log n) when the tree is balanced.

The problem is that a BST does not balance itself. Inserting keys **in sorted order** produces a tree with no branching at all. Inserting 1 through 7 in order:

```
1
 \
  2
   \
    3
     \
      4
       \
        5
         \
          6
           \
            7
```

Height 6 instead of 2. Searching for 7 takes 7 comparisons instead of 3. The structure has become a linked list with extra pointers.

This matters here because cell records are naturally generated by scanning the grid row by row, which produces exactly sorted order. The BST's worst case is this project's normal case.

### 7.4 AVL tree, and the fix

An AVL tree is a BST that rebalances itself. It tracks, for every node, the height difference between its left and right subtrees. If that difference ever reaches 2, the tree performs a rotation to fix it.

Inserting the same keys 1 through 7 into an AVL tree:

- Insert 1, 2. Insert 3 causes an imbalance at 1, so rotate left. Root becomes 2.
- Insert 4, 5. Imbalance at 3, rotate left. Subtree becomes 4 with children 3 and 5.
- Insert 6. Imbalance at 2, rotate left. Root becomes 4.
- Insert 7. Imbalance at 5, rotate left. Subtree becomes 6 with children 5 and 7.

Final tree:

```
        4
      /   \
     2     6
    / \   / \
   1   3 5   7
```

Height 2. Searching for 7 now takes 3 comparisons instead of 7. For 100,000 obstacle cells the difference is 100,000 comparisons against about 17.

A rotation is cheap. It relinks three pointers and takes constant time, and at most two rotations are needed after any single insertion.

### 7.5 Max heap, and why it is not used

A max heap is the same structure with the rule reversed, so the largest element sits at the root. It is the right tool when you repeatedly want the biggest item, for example in heap sort or in scheduling the highest-priority job first.

A\* always wants the **cheapest** unexplored cell, never the most expensive. A max heap would answer the wrong question at every step.

### 7.6 Binary tree, plain

A binary tree with no ordering rule is just a shape: each node has at most two children. It is useful for recursively subdividing the grid into regions, since a region can be split and split again.

Without an ordering rule there is nothing to guide a search, so finding a particular cell means visiting nodes until it turns up, which is O(n). It stays in the analysis as the starting point the other structures improve on.

### 7.7 Threaded binary tree

In an ordinary binary tree with n nodes, there are n + 1 empty child pointers going nowhere. A threaded tree reuses them: an empty right pointer stores the inorder successor instead of null.

The benefit is that inorder traversal needs no recursion and no stack, running in O(n) time with O(1) extra space. On a flight controller with a small stack that is a real advantage for walking a waypoint list.

It is not part of the core search, so it is noted rather than used.

### 7.8 Tree traversals, and what each one is for

Take the AVL tree from 7.4:

| Traversal | Order visited | Result | Use here |
|---|---|---|---|
| Inorder (L, N, R) | left, node, right | 1, 2, 3, 4, 5, 6, 7 | Produces sorted output, useful for listing cells in order |
| Preorder (N, L, R) | node, left, right | 4, 2, 1, 3, 6, 5, 7 | Saving or transmitting the tree structure |
| Postorder (L, R, N) | left, right, node | 1, 3, 2, 5, 7, 6, 4 | Merging or freeing child regions before the parent |
| Level order | by depth | 4, 2, 6, 1, 3, 5, 7 | Examining the map coarse to fine |

The one this project actually depends on is a different kind of walk. Path recovery follows parent links from the goal back to the start, which is a leaf-to-root traversal of the search tree rather than one of the four standard orders.

### 7.9 Graph, and how the grid becomes one

A graph is a set of vertices and edges. Here every open cell is a vertex, and an edge connects two cells if the drone can move directly between them. Edges carry weights, which are the movement costs.

In an 8-connected grid a cell has at most 8 neighbours. Cells on the edge of the map have fewer. In a 3 × 3 grid numbered:

```
0 1 2
3 4 5
6 7 8
```

- Corner cell 0 has 3 neighbours: 1, 3, 4
- Edge cell 1 has 5 neighbours: 0, 2, 3, 4, 5
- Centre cell 4 has all 8

Edge weights are 1 for a straight move and √2 for a diagonal one.

### 7.10 Adjacency list against adjacency matrix

These are the two standard ways to store a graph, and the choice here is decided by arithmetic rather than preference.

**Adjacency matrix.** A V × V table where entry [i][j] says whether an edge exists. Checking one edge is O(1), which is its advantage. Space is O(V²) regardless of how many edges actually exist.

For the 3 × 3 grid above: 9 vertices means a 9 × 9 table, 81 entries, holding only 20 undirected edges. Most of the table is empty.

Scaling to a 256 × 256 grid:

- Vertices: 256 × 256 = 65,536
- Matrix entries: 65,536² = 4,294,967,296
- At one byte per entry, roughly **4.29 GB**

That is not usable on a drone or on a laptop.

**Adjacency list.** Each vertex stores a list of its actual neighbours. Space is O(V + E).

For the same grid: 65,536 cells with at most 8 neighbours each is about 524,288 entries, which is roughly **0.5 MB**. Around eight thousand times smaller.

The grid graph is sparse, meaning it has far fewer edges than a complete graph would. Adjacency lists are built for sparse graphs, which is why the matrix is rejected here despite its faster single-edge lookup.

### 7.11 BFS, DFS and A\*

**Breadth-first search** explores outward level by level using a queue. It finds the route with the fewest steps, but treats every move as equally expensive, so it cannot tell a cheap straight step from a costlier diagonal.

**Depth-first search** follows one branch as deep as it goes before backtracking, using a stack. It finds *a* route but gives no guarantee it is short.

**A\*** is the one used here. It keeps a priority queue of cells ordered by f = g + h, where g is the known cost from the start and h is an estimate of the cost remaining. The estimate is what makes it faster than Dijkstra: it pushes the search toward the goal instead of spreading evenly in all directions. As long as h never overestimates the true remaining cost, the route found is optimal.

The priority queue in that description is the OPEN list, and it is the structure the whole project is about.

### 7.12 Summary

| DSA Topic | Best case | Worst case | Useful? | Role in this project |
|---|---|---|---|---|
| Unsorted Array | O(1) insert | O(n) extract-min | Baseline | Simplest OPEN list; what the heap is measured against |
| Min Heap | O(1) get min | O(log n) | Yes | Core structure: selects the lowest f = g + h cell |
| AVL Tree | O(log n) | O(log n) | Yes | Balanced index of obstacle and visited cells |
| BST | O(log n) | O(n) | Limited | Degenerates to a linked list on sorted insertion |
| Binary Tree | O(n) | O(n) | No | Recursive subdivision; no ordering, so search stays linear |
| Max Heap | O(1) get max | O(log n) | No | Answers the wrong question for shortest-path search |
| Threaded Binary Tree | O(n) traversal | O(n) traversal | No | Stack-free traversal of waypoint lists; not core |
| Tree Traversal | O(n) | O(n) | Support | Rebuilding the path from parent links |
| Graph | O(V+E)* | O(V+E)* | Yes | Cells as vertices, moves as weighted edges |
| Adjacency List | O(V+E)* | O(V+E)* | Yes | Sparse grid, at most 8 neighbours per cell |
| Adjacency Matrix | O(1) edge check | O(1) edge check | No | 4.29 GB for a 256 × 256 grid |

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
