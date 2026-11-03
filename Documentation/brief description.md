# Brief Description

## Grid-Based Drone Path Planning Simulation

**Student:** Aditya Upadhyay
**Roll No.:** 2501330100038 · **Section:** CSE-A · **Semester:** III
**Course:** Data Structures and Algorithms II (CCSE0301)
**Faculty:** Mr. Shamshad Ali
**Institute:** NIET Greater Noida
**Domain:** Robotics and Autonomous Systems
**SDG Alignment:** SDG 9, Industry, Innovation and Infrastructure
**Review Stage:** Final · Month 3 · **Progress: 100%**

**Repository:** https://github.com/adityaupadhyay01/DSA_Grid_Based
**Live demo:** https://drone-path-planning-ruby.vercel.app/

---

## 1. Overview

This project simulates how a drone decides its route across a map divided into square cells. Some cells are open, some are blocked by obstacles. The drone starts at one cell and has to reach a goal cell without passing through anything blocked.

Underneath that, the project studies the data structures a planner depends on: where the map and obstacles are stored, and what holds the cells still waiting to be examined. These choices decide whether a planner is usable at the sizes real maps come in, and the project set out to measure how much difference they make.

It is now complete. A\* was held fixed and six different OPEN-list structures were run behind one interface on identical maps. The fastest is thirteen times quicker than the slowest, and all six return the same route. The project also ran the knapsack and tour-ordering work from Units 3 and 4 on top of that planner, and compared the binary heap against the advanced heaps from Unit 5.

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

That expectation turned out to be correct, and the measured gap is reported in section 8.

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

The base paper was re-read at Review 2 specifically to check whether the OPEN-list structure is stated anywhere outside the algorithm description. It is not. The gap held.

---

## 5. Work Done

The project held the algorithm fixed and varied only the structure beneath it.

A single grid-based A\* planner was built in C++17, with the OPEN list placed behind an abstract interface of three operations: `insert`, `extract_min` and `empty`. The search calls nothing else, so swapping the structure changes no line of the search. Six implementations sit behind that interface:

1. Unsorted array with a linear scan, as the control
2. Binary min-heap
3. Bucket (Dial) queue with a 2C+1 window
4. 4-ary heap
5. Binomial heap
6. Fibonacci heap

The same planner, on the same seeded maps, with the same heuristic, was run with each structure in turn, recording runtime, node expansions, insertions and extractions. Correctness is checked before any timing: a ten-test suite runs on every build, and the load-bearing test plans the same fifty random maps with every structure and requires them to return an identical route cost.

The claim being tested was that data-structure choice is an unreported but measurable factor in published path-planning results, and that its size can be quantified. It is: on a 128 × 128 grid at 20% obstacle density the gap between the fastest and the slowest structure is a factor of thirteen, with the route unchanged.

This project does not claim a better planner than the base paper. That work is three-dimensional, models flight dynamics, and applies potential fields and spline smoothing, all of which sit outside this course. This project works in two dimensions at fixed altitude, models no kinematics, and isolates one variable the base paper left undefined. The narrow scope is what made the result possible to validate.

---

## 6. System Model

The workspace is a two-dimensional occupancy grid of W × H square cells at fixed altitude. Each cell holds an occupancy state, open or blocked, and its coordinates.

Movement is eight-connected. Costs are integers scaled by 1000: a straight step costs 1000 and a diagonal step 1414, approximately √2, since a diagonal covers more ground than a cardinal move. Treating them as equal produces wrong routes. Integers were chosen over floating point for two reasons: comparisons stay exact, so two routes can be compared without a tolerance, and the bucket queue needs integer keys to index its buckets at all.

The search is A\* with the octile-distance heuristic, which is admissible and consistent on a uniform grid. The cost function is f(v) = g(v) + h(v), where g is the cost from the start and h the estimate to the goal.

For path recovery, every examined cell records the cell it was reached from. Each cell has at most one such parent, so the examined set forms a tree rooted at the start, and the route is recovered by walking parent links backwards from the goal.

A cell can sit in OPEN more than once, because a cheaper route to it can be found after it was first inserted. Rather than searching OPEN to update the old entry, the planner inserts the better one and discards the stale copy when it surfaces. This is lazy deletion, and it is sound here because the heuristic is consistent: the first time a cell is popped it is already at its cheapest. The decision matters later, in section 7.15.

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

This section works through each concept from the syllabus, what it does, and whether it belongs in this project. The examples are small enough to check by hand.

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

This was the predicted winner. It is not: see 7.15.

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

The AVL tree was studied and analysed but is not in the final planner. The occupancy grid is a flat array, where "is this cell blocked?" is answered in O(1) by index arithmetic. A balanced tree would answer the same question in O(log W) and add pointer chasing for no benefit. The rejection is reported here because working out *why* it loses took as much effort as implementing it would have.

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

Edge weights are 1000 for a straight move and 1414 for a diagonal one.

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

In the finished planner the adjacency list is not even materialised. Neighbours are generated on demand by index arithmetic over the flat occupancy array, so the graph exists implicitly and costs nothing beyond the grid itself.

The grid graph is sparse, meaning it has far fewer edges than a complete graph would. Adjacency lists are built for sparse graphs, which is why the matrix is rejected here despite its faster single-edge lookup.

### 7.11 BFS, DFS and A\*

**Breadth-first search** explores outward level by level using a queue. It finds the route with the fewest steps, but treats every move as equally expensive, so it cannot tell a cheap straight step from a costlier diagonal.

**Depth-first search** follows one branch as deep as it goes before backtracking, using a stack. It finds *a* route but gives no guarantee it is short.

**A\*** is the one used here. It keeps a priority queue of cells ordered by f = g + h, where g is the known cost from the start and h is an estimate of the cost remaining. The estimate is what makes it faster than Dijkstra: it pushes the search toward the goal instead of spreading evenly in all directions. As long as h never overestimates the true remaining cost, the route found is optimal.

The priority queue in that description is the OPEN list, and it is the structure the whole project is about.

BFS and DFS were implemented as reference traversals and A\* was checked against them.

### 7.12 The bucket queue, and the bug it produced

A bucket or Dial queue exploits the fact that f-values are bounded integers. Instead of comparing keys, it keeps an array of buckets indexed by key and walks a cursor forward. Insertion is O(1) and extraction is O(1) amortised.

The question is how wide the window has to be. Dijkstra's bound says C + 1 buckets suffice, where C is the largest edge cost. That bound was used first, and it was wrong. On a 128 × 128 map the bucket queue returned a route of cost **43,006** where the binary heap returned **41,592**. Distant keys were aliasing onto occupied buckets and cells were coming out in the wrong order.

The correct span for A\* with a consistent heuristic is 2C + 1. The derivation:

```
f(v) = g(v) + h(v) = g(u) + c + h(v) = f(u) - h(u) + c + h(v)
consistency gives  h(v) - h(u) <= c
therefore          f(v) <= f(u) + 2c
```

So a newly inserted key can exceed the current cursor by up to 2C, and the window must be 2C + 1 = 2,829 buckets. With that correction all six structures agree on every map.

This bug is worth recording because of how it was caught. No single-structure test could have found it: the planner was internally consistent and simply returned a worse route. Only the test that plans the same map with every structure and demands an identical cost exposed it.

### 7.13 Dynamic programming: 0/1 Knapsack (Unit 3)

A drone has a battery, and the waypoints it could serve cost more than the battery allows. Each waypoint carries a delivery priority and a battery cost, and the choice is which subset to serve.

This is 0/1 Knapsack rather than its fractional form, because a waypoint cannot be half visited. The greedy value-per-unit argument that solves the fractional version does not apply.

The table is filled with

```
dp[i][w] = max(dp[i-1][w], dp[i-1][w - cost(i)] + value(i))
```

in O(n × W), and the chosen subset is recovered by walking the final column upward, taking item i wherever the stored value changed between consecutive rows.

One detail matters: the battery cost of each waypoint is **not** assumed. It is the length of the A\* route that actually reaches it, so this module sits on top of the grid and the search rather than beside them.

Measured against exhaustive subset search, the table does more work below fourteen items and wins decisively above it. At twenty-two items it finished in 0.034 ms against 201.42 ms. Both methods returned the same selection at every size tested. The crossover is reported because reporting only the large sizes would have hidden it.

### 7.14 Backtracking and branch and bound (Unit 4)

The selected waypoints still have to be visited in some order. Plain backtracking walks every permutation, so the tree holds O(n!) nodes.

Branch and bound walks the same tree but computes, at each partial tour, a lower bound on the cheapest completion: the cost committed so far plus the cheapest outgoing edge of every unvisited stop. Any real completion must leave each unvisited stop along some edge, and none is cheaper than that stop's cheapest, so the bound can never overstate the true cost.

That direction is the whole safety argument. A bound that could overstate would prune the optimum and return a worse tour with no error reported.

Plain backtracking is kept in the code as a control, and both returned the identical tour at every size:

| Stops | Backtracking nodes | Branch and bound nodes | Avoided |
|---|---|---|---|
| 9 | 109,601 | 3,366 | 96.9% |
| 10 | 986,410 | 7,854 | 99.2% |
| 11 | 9,864,101 | 70,590 | 99.3% |

Below about seven stops the bound costs more to compute than it saves.

### 7.15 Binomial and Fibonacci heaps, and the result that contradicts the theory (Unit 5)

Unit 5 reaches the heaps that improve on the binary heap asymptotically. A 4-ary heap, a binomial heap and a Fibonacci heap were implemented behind the same interface and measured on identical maps.

The ranking contradicts the asymptotic one. On a 128 × 128 grid the binary heap took 0.354 ms, the 4-ary heap 0.363 ms, the binomial heap 0.762 ms and the **Fibonacci heap 1.434 ms** — the structure with the best theoretical insertion was the slowest of the four, by a factor of four.

Two reasons account for it, and both are the point of the project.

A Fibonacci heap exists to make decrease-key cheap. This planner never calls decrease-key. As section 6 explains, a better g value reinserts the cell and the stale copy is discarded when it surfaces, which is sound because the heuristic is consistent. The operation the structure is built to optimise is never used.

Second, both mergeable heaps allocate a pointer-linked node per entry. The frontier stops being a contiguous array and the cache stops helping. The binary heap's advantage is that it has no pointers at all.

### 7.16 Summary

| DSA Topic | Best case | Worst case | Useful? | Role in this project |
|---|---|---|---|---|
| Unsorted Array | O(1) insert | O(n) extract-min | Baseline | Simplest OPEN list; what everything is measured against |
| Min Heap | O(1) get min | O(log n) | Yes | Fastest general-purpose OPEN list measured |
| Bucket (Dial) Queue | O(1) insert | O(1) amortised | Yes | Fastest overall; needed a 2C+1 window, not C+1 |
| 4-ary Heap | O(1) get min | O(log₄ n) | Yes | Shallower tree, fewer levels per sift; matches the binary heap |
| Binomial Heap | O(1) insert amortised | O(log n) | Measured, not chosen | Pointer-linked nodes cost more than the bound saves |
| Fibonacci Heap | O(1) insert | O(log n) amortised | Measured, not chosen | Slowest of the six: decrease-key is never called |
| AVL Tree | O(log n) | O(log n) | No | Flat array answers the same query in O(1) |
| BST | O(log n) | O(n) | Limited | Degenerates to a linked list on sorted insertion |
| Binary Tree | O(n) | O(n) | No | Recursive subdivision; no ordering, so search stays linear |
| Max Heap | O(1) get max | O(log n) | No | Answers the wrong question for shortest-path search |
| Threaded Binary Tree | O(n) traversal | O(n) traversal | No | Stack-free traversal of waypoint lists; not core |
| Tree Traversal | O(n) | O(n) | Support | Rebuilding the path from parent links |
| Graph | O(V+E)* | O(V+E)* | Yes | Cells as vertices, moves as weighted edges |
| Adjacency List | O(V+E)* | O(V+E)* | Yes | Sparse grid, at most 8 neighbours per cell |
| Adjacency Matrix | O(1) edge check | O(1) edge check | No | 4.29 GB for a 256 × 256 grid |
| 0/1 Knapsack DP | O(n × W) | O(n × W) | Yes | Which waypoints the battery allows |
| Backtracking | O(n!) | O(n!) | Control | Proves the bound did not change the answer |
| Branch and Bound | O(n!) worst | O(n!) worst | Yes | Cuts 99.3% of the tree at 11 stops |

\* O(V+E) refers to BFS/DFS traversal using an adjacency list. Heap costs are per operation, not per search.

The rejections took as much work as the selections. Ruling out the adjacency matrix required a concrete space calculation, ruling out the max heap required working out what the search actually asks for at each step, and ruling out the Fibonacci heap required building it and measuring it.

---

## 8. Results

All figures are from the C++ build at `-O2`, median of repeated runs on seeded maps at 20% obstacle density. The committed `results.csv` is the evidence behind every number here.

### 8.1 The structure comparison

| OPEN list | 64 × 64 | 128 × 128 | 256 × 256 | 128 → 256 |
|---|---|---|---|---|
| Unsorted array | 0.343 ms | 3.321 ms | not run | — |
| **Bucket queue (2C+1)** | **0.058 ms** | **0.250 ms** | **0.866 ms** | 3.5× |
| Binary heap | 0.070 ms | 0.354 ms | 1.424 ms | 4.0× |
| 4-ary heap | 0.075 ms | 0.363 ms | 1.352 ms | 3.7× |
| Binomial heap | 0.152 ms | 0.762 ms | 2.284 ms | 3.0× |
| Fibonacci heap | 0.238 ms | 1.434 ms | 4.386 ms | 3.1× |

Fastest to slowest is a factor of **thirteen** at 128 × 128, and all six return the same route cost. That is the number the project set out to find: on this workload, the unstated OPEN-list choice can account for a thirteen-fold difference in a reported timing.

### 8.2 Two predictions that were wrong

Both are recorded rather than quietly removed, because what they show is more useful than a clean prediction would have been.

**The insert-to-extract ratio.** The design assumed insertions would outnumber extractions roughly eight to one, and the structures were chosen partly for cheap insertion. The measured ratio is **1.20 : 1**. Most neighbours are rejected before they are ever inserted — they are blocked, already closed, or offer no improvement — so insertion is far rarer than expected and the structures optimised for it gain less than predicted.

**The bucket queue would always win.** It does at 128 × 128 and 256 × 256. On the small 40 × 40 demonstration maps it does not, because its window is 2,829 buckets whatever the map size, and a search closing about a thousand cells never amortises that fixed setup. The benchmark maps are where the comparison belongs.

### 8.3 Units 3 and 4

- Dynamic programming overtakes exhaustive subset search at **14 items**; at 22 items, 0.034 ms against 201.42 ms, same answer
- Branch and bound removes **99.3%** of the search tree at 11 stops, same tour as plain backtracking at every size
- Below 14 items and below 7 stops the simpler method is genuinely faster, and both crossovers are reported

### 8.4 Correctness

Ten tests run on every build. The load-bearing one plans fifty random maps with every structure and requires identical route costs; it is what caught the bucket-queue window bug in 7.12. The others cover optimality on a known grid, obstacle handling, heap invariants, and the demonstration maps.

### 8.5 Deliverables

- A working C++17 grid planner with six interchangeable OPEN-list structures
- The measured comparison above, with `results.csv`, charts and a scaling study
- The knapsack selector and both tour solvers from Units 3 and 4
- A browser demonstration at https://drone-path-planning-ruby.vercel.app/ with three fixed maps and a 100 × 100 grid anyone can draw on, which names every algorithm a run used. The hosted page plans in JavaScript, because static hosting cannot run a compiled binary; every measured figure in this document comes from the C++ build
- A design book with wireframes, flowcharts and the algorithm write-ups

---

## 9. Progress at Final, 100%

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
| 10 | Implementation of the planner and the OPEN-list interface | Complete |
| 11 | Units 3 and 4: knapsack selection and tour ordering | Complete |
| 12 | Unit 5: binomial, Fibonacci and 4-ary heaps | Complete |
| 13 | Correctness suite, ten tests | Complete |
| 14 | Measurement and comparison across map sizes | Complete |
| 15 | Browser demonstration and deployment | Complete |
| 16 | Final analysis and report | Complete |

---

## 10. What Each Review Delivered

**Review 1, 25%.** Research, gap identification, structure selection and conceptual design. Nothing implemented.

**Review 2, 55%.** Re-checked the base paper for any statement of its OPEN-list structure; it has none. Built the planner with the OPEN list behind a common interface, with the unsorted array, binary heap and bucket queue behind it. Ran the first timing comparison and found the 2C+1 window bug.

**Review 3, 80%.** Extended into Units 3 and 4: the 0/1 Knapsack selector over battery budget, and the tour ordering with backtracking kept as a control against branch and bound. Found both crossovers.

**Final, 100%.** Added the 4-ary, binomial and Fibonacci heaps from Unit 5 and completed the comparison across map sizes. Corrected the insert-to-extract prediction against the measurement. Built and deployed the browser demonstration. Produced the complexity analysis, charts and final report.

### Future work

- Replace the random maps with the MovingAI benchmark set, which carries known optimal costs
- Sweep obstacle density from 5% to 40% and report per density rather than at one value
- Extend the grid to three dimensions with an octree
- Add a concurrent frontier, where the mergeable heaps rejected here may finally earn their bounds

---

## 11. Scope and Limitations

- Two dimensions at fixed altitude; the base paper works in three
- No flight dynamics, turning radius, potential fields or spline smoothing
- Static maps with known obstacles; no sensing or replanning mid-flight
- Simulation only; no hardware
- The comparison is between data structures, not between planners
- Timings are from one machine; the ranking between structures is stable, the absolute milliseconds are not
- The hosted demonstration plans in JavaScript and is several times slower than the C++ it mirrors

---

## 12. References

1. Gao, W., Li, L., & Pang, D. (2026). Urban low-altitude UAV path planning by fusing an enhanced A\* algorithm with an adaptive artificial potential field method. *Scientific Reports, 16*, 18275. https://doi.org/10.1038/s41598-026-45160-6
2. Meng, W., Zhang, X., Zhou, L., Guo, H., & Hu, X. (2025). Advances in UAV Path Planning: A Comprehensive Review of Methods, Challenges, and Future Directions. *Drones, 9*(5), 376. https://doi.org/10.3390/drones9050376
3. Dradoum, A., Khelassi, A., & Lachekhab, F. (2025). Intelligent path planning algorithms for UAVs: Classification, complexity analysis, hybrid ablation insights, and future directions. https://doi.org/10.1177/16878132251355020
4. Hart, P. E., Nilsson, N. J., & Raphael, B. (1968). A Formal Basis for the Heuristic Determination of Minimum Cost Paths. *IEEE Transactions on Systems Science and Cybernetics, 4*(2), 100–107. https://doi.org/10.1109/TSSC.1968.300136
5. Cormen, T. H., Leiserson, C. E., Rivest, R. L., & Stein, C. (2022). *Introduction to Algorithms* (4th ed.). MIT Press.
6. Patel, A. *Amit's A\* Pages, Implementation Notes.* Stanford University. https://theory.stanford.edu/~amitp/GameProgramming/index.html
7. Dial, R. B. (1969). Algorithm 360: Shortest-path forest with topological ordering. *Communications of the ACM, 12*(11), 632–633. https://doi.org/10.1145/363269.363610
8. Fredman, M. L., & Tarjan, R. E. (1987). Fibonacci heaps and their uses in improved network optimization algorithms. *Journal of the ACM, 34*(3), 596–615. https://doi.org/10.1145/28869.28874

The complete source list, including background papers, books and video lectures, is in `evidence.md`.

---

*Final · Month 3 · Updated at the end of each review stage.*
