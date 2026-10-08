# Grid-Based Drone Path Planning Simulation

### Project Documentation — Review 1

---

| | |
|---|---|
| **Student** | Aditya Upadhyay |
| **ERP ID / Roll No.** | [ERP ID] / 2501330100038 |
| **Programme** | B.Tech Computer Science and Engineering |
| **Section / Semester** | CSE-A / III |
| **Course** | Data Structures and Algorithms II |
| **Course Code** | CCSE0301 |
| **Faculty** | Mr. Shamshad Ali |
| **Institute** | NIET Greater Noida |
| **Domain** | Robotics and Autonomous Systems |
| **SDG Alignment** | SDG 9 — Industry, Innovation and Infrastructure |
| **Assignment Type** | Individual Assignment |
| **Review Stage** | Review 2, Month 2 |
| **Progress** | 70% |
| **Repository** | `https://github.com/adityaupadhyay01/<repo-name>` |

---

## Contents

1. [Abstract](#1-abstract)
2. [Problem and Motivation](#2-problem-and-motivation)
3. [Base Paper and Research Gap](#3-base-paper-and-research-gap)
4. [Objectives](#4-objectives)
5. [Target Users](#5-target-users)
6. [System Model](#6-system-model)
7. [Backend Architecture](#7-backend-architecture)
8. [The Search Loop](#8-the-search-loop)
9. [Unit 1 — Trees and Heaps](#9-unit-1--trees-and-heaps)
10. [Unit 2 — Graphs](#10-unit-2--graphs)
11. [Why These Structures and Not the Others](#11-why-these-structures-and-not-the-others)
12. [The Swappable Interface](#12-the-swappable-interface)
13. [Complexity Summary](#13-complexity-summary)
14. [Measurement Plan](#14-measurement-plan)
15. [Progress and Remaining Work](#15-progress-and-remaining-work)
16. [Scope and Limitations](#16-scope-and-limitations)
17. [References](#17-references)
18. [Submission Checklist](#18-submission-checklist)

---

## 1. Abstract

This project simulates a drone finding a collision-free route across a map divided into square cells. The route itself is the visible output. The subject of study is one level below it: the data structures a planner runs on, specifically the structure that holds the frontier of cells waiting to be explored.

The work is anchored on a gap in a 2026 paper. Gao, Li and Pang report a 46.2 per cent reduction in computation time for their A\*-APF planner, but describe their OPEN list only in words and never state what structure holds it. Since selecting a minimum costs O(n), O(log n) or O(1) depending on that choice, and since every result in the paper is a timing, the omission is material.

This project holds the algorithm fixed and varies only the OPEN-list structure, measuring how much of a planner's reported speed comes from its data structures rather than its heuristics.

At Review 2 the planner is implemented in C++17 with three interchangeable OPEN-list structures behind a common interface, a correctness suite of six tests, and a first round of measurements. The result is that on a 128 x 128 grid at 20 per cent obstacle density the bucket queue completes a plan **20 times faster** than the unsorted-array baseline and 30 per cent faster than a binary heap, on identical maps with an identical search. The gap the base paper leaves open is therefore measurable, and its size on this workload is now known.

---

## 2. Problem and Motivation

Drones are used for delivery, mapping, inspection, agriculture and rescue work. Every one of those tasks reduces to the same requirement: travel from one point to another without hitting a building, a pole, a tree or a restricted zone. Deciding that route in advance is path planning.

The obvious method is to examine every possible route and keep the shortest. It works on a small example and fails on a real one. A grid of a hundred rows and a hundred columns holds ten thousand cells, and the number of distinct routes through them is far too large to enumerate. The problem is harder on a drone than on a laptop, because the machine runs a small onboard processor with a battery draining while it computes. The answer has to arrive quickly and without consuming much memory.

Two separate difficulties follow, and they need different tools.

**Storing the map.** If every cell is an individual record, memory use climbs sharply as the grid grows, and wide empty stretches get stored cell by cell even though nothing is in them. The map needs a structure that finds blocked and open regions quickly.

**Representing movement.** The planner repeatedly asks which cells it can reach from the current one, and at what cost. Answering by scanning the whole map each time is unusable. The moves need a structure built for that query.

The first is a tree problem, the second a graph problem. That is what makes this topic a fit for DSA-II.

### 2.1 Why this topic rather than another

Most examples in a data structures course involve sorting arrays or searching lists. The concepts are correct, but at the sizes used in class every alternative finishes instantly, so the cost of a bad choice never shows.

Path planning behaves differently. Keeping the frontier in the wrong structure slows the search by orders of magnitude. Storing the map badly exhausts memory on a grid that is not even large. Here the structure decides whether the program finishes at all.

---

## 3. Base Paper and Research Gap

### 3.1 The base paper

**Gao, W., Li, L., & Pang, D. (2026).** Urban low-altitude UAV path planning by fusing an enhanced A\* algorithm with an adaptive artificial potential field method. *Scientific Reports*, 16, Article 18275.
https://doi.org/10.1038/s41598-026-45160-6 · Published 20 April 2026 · Open Access

The authors propose A\*-APF, a multi-phase planner for drones in dense urban airspace:

- The city is divided into a uniform three-dimensional grid of cubes
- The flight is split into take-off, cruise and landing phases by a state machine, each phase with its own constraints
- A\* searches the grid with a cost function extended beyond f = g + h to include obstacle-repulsion and altitude penalties
- Two safety radii around the drone control step size and decide when the potential field takes over steering
- Candidate neighbours are pruned by a field-of-view cone and by kinematic limits
- The final route is smoothed with cubic and quartic B-splines so it can be flown

**Reported results:** 46.2 per cent less computation time and 33 per cent fewer expanded nodes than the competing hybrid methods they reproduced. The full method completes in 0.53 seconds against 3.58 seconds for improved A\* alone. Tested in MATLAB 2023a on an Intel i7-9750H.

### 3.2 Supporting papers

**Meng, W., Zhang, X., Zhou, L., Guo, H., & Hu, X. (2025).** Advances in UAV Path Planning: A Comprehensive Review. *Drones*, 9(5), 376. https://doi.org/10.3390/drones9050376

A survey of grid-based, sampling-based and intelligent planners. It confirmed that A\* and Dijkstra remain the standard node-search methods for grid maps, which is why this project builds on A\* rather than something less established.

**Dradoum, A., Khelassi, A., & Lachekhab, F. (2025).** Intelligent path planning algorithms for UAVs: Classification, complexity analysis, hybrid ablation insights, and future directions. https://doi.org/10.1177/16878132251355020

Classifies planners and works through their complexity. It treats complexity at the algorithm level and never descends to the data-structure level.

### 3.3 The gap

All three papers optimise the search strategy and report the improvement in seconds. None reports the structures underneath the search.

This is sharpest in the base paper, which explains the OPEN and CLOSED lists in words:

> the algorithm operates iteratively by selecting the node with the lowest cost from the OPEN list for expansion

The paper never states how that selection is implemented. There is no mention of a heap, a priority queue, a sorted array or a hash set anywhere in the algorithm description. The CLOSED-set membership test is likewise unspecified.

Selecting the lowest-cost node has several implementations, and they are not close to each other:

| OPEN list implementation | Cost per extract-min | Total search cost |
|---|---|---|
| Unsorted array with linear scan | O(n) | **O(n²)** |
| Binary min-heap | O(log n) | O(n log n) |
| Bucket (Dial) queue | O(1) amortised | **O(n)** |

If a linear scan was used, part of the reported baseline comes from the data structure rather than the algorithm, and the comparison against competing methods is confounded. If a heap was used, it was not reported, and the result cannot be reproduced independently. Both cases leave something unaccounted for.

Amit Patel's A\* implementation notes at Stanford contain a section titled *Set representation* listing these same options: unsorted arrays, sorted arrays, binary heaps, indexed arrays, hash tables and bucketing. The OPEN-list choice is a recognised design decision with known consequences.

### 3.4 What this project claims, and what it does not

**The claim.** Data-structure choice is an unreported but measurable factor in published path-planning results, and its size can be quantified.

**Not claimed.** This is not a better planner than the base paper. That work is three-dimensional, models flight dynamics, and applies potential fields and spline smoothing, all outside this course. This project works in two dimensions at fixed altitude, models no kinematics, and isolates one variable the base paper left undefined. The narrow scope is what makes the result possible to validate.

---

## 4. Objectives

1. Understand grid path planning and why it becomes expensive on large maps
2. Read existing UAV path planning work and identify a gap worth taking up
3. Study DSA-II Unit 1 (Trees) and Unit 2 (Graphs) and determine what this project needs
4. Examine how the map and obstacle data can be stored without wasting memory on empty space
5. Examine how graphs can represent the drone's available moves
6. Determine how a heap decides which cell the search examines next
7. Build one A\* planner with the OPEN list behind a swappable interface
8. Measure runtime and node expansions across OPEN-list structures on identical inputs

---

## 5. Target Users

| User | Benefit |
|---|---|
| Drone pilots and operators | They fly the route the planner produces, so it has to be safe |
| Delivery and logistics firms | A shorter route computed faster means less flight time and less battery |
| Survey, mapping and inspection teams | Full coverage of an area while staying clear of obstacles and restricted zones |
| Rescue and emergency services | Planning speed matters most when reaching an affected site |

---

## 6. System Model

### 6.1 Notation

| Symbol | Meaning | Value used |
|---|---|---|
| W, H | grid width and height | 128² to 1024² |
| n | total cells, W × H | 16,384 to 1,048,576 |
| k | blocked cells | 5–40% of n |
| g(v) | cost from start to cell v | — |
| h(v) | octile-distance estimate to goal | admissible, consistent |
| f(v) | g(v) + h(v), the priority key | — |

### 6.2 Cell record

```
struct Cell {
    uint16  x, y;        // grid coordinates
    uint8   state;       // 0 = FREE, 1 = BLOCKED
    float   g, f;        // search costs
    int32   parent;      // index of the cell this was reached from
    uint8   flags;       // IN_OPEN, IN_CLOSED bits
}
```

### 6.3 Movement and cost

Movement is 8-connected. A straight step costs 1. A diagonal step costs √2 ≈ 1.414, because a diagonal covers more ground than a cardinal move. Treating them as equal produces routes that look shorter in step count but are longer in distance.

The heuristic is octile distance:

```
oct(u, v) = √2 · min(Δx, Δy) + |Δx − Δy|
```

It is admissible, meaning it never overestimates the true remaining cost, which is what guarantees A\* returns an optimal route.

### 6.4 The four questions the system asks

Every structural decision in this project is judged against these, with their approximate frequency on a 256 × 256 grid:

| # | Question | Frequency per plan |
|---|---|---|
| Q1 | Is cell (x, y) blocked? | ~10⁵ |
| Q2 | What is the cheapest unexplored cell? | ~10⁴ |
| Q3 | Have I already expanded this cell? | ~10⁵ |
| Q4 | Which cells can I reach from here? | ~10⁵ |

---

## 7. Backend Architecture

Five stores, because the system asks four different questions and no single structure answers all of them well.

```
┌──────────────────────── Planner backend ────────────────────────┐
│                                                                 │
│  MAP STORAGE                                                    │
│  ┌───────────────────────────┐  ┌───────────────────────────┐    │
│  │  Occupancy grid           │  │  Obstacle index           │    │
│  │  2D array                 │  │  AVL tree                 │    │
│  │  O(1) cell lookup         │  │  O(log n) search          │    │
│  └───────────────────────────┘  └───────────────────────────┘    │
│                                                                 │
│  SEARCH STATE                                                   │
│  ┌───────────────────────────┐  ┌───────────────────────────┐    │
│  │  OPEN list      ◄── SWAP  │  │  CLOSED set               │    │
│  │  Min heap                 │  │  Hash table               │    │
│  │  O(1) peek, O(log n) pop  │  │  O(1) membership          │    │
│  └───────────────────────────┘  └───────────────────────────┘    │
│                                                                 │
│  RESULT                                                         │
│            ┌───────────────────────────────────┐                │
│            │  Search tree                      │                │
│            │  Implicit: one parent index/cell  │                │
│            └───────────────────────────────────┘                │
└─────────────────────────────────────────────────────────────────┘
```

### 7.1 Why each one

**Occupancy grid stays a flat array.** This is the one place a tree would be wrong. Q1 is asked up to 8 times per expansion, around 10⁵ times per plan, making it the hottest operation in the system. A flat array answers it in O(1) through direct index arithmetic `grid[y * W + x]`. A quadtree would answer in O(log W) and add pointer chasing. Admitting that the simple structure wins here is a stronger position than forcing a tree in.

**Obstacle index is AVL, not a plain BST.** Obstacle cells are inserted in scan order, which is sorted order. Sorted insertion turns a plain BST into a chain of height n−1. AVL's rotations cap height at 1.44 log₂ n. This is the structure that stops the worst case from being the normal case.

**CLOSED set is a hash table.** Q3 is a pure membership test with no ordering requirement. O(1) expected beats O(log n) when it runs once per neighbour. An AVL variant is kept as a fallback where ordered iteration is useful for debugging.

**OPEN list is the min heap, and it is the component under study.** Q2 asks only for the minimum. The heap answers in O(1) and removes in O(log n), with zero pointer overhead.

**Search tree is implicit.** No nodes and no allocation. One `parent` integer per cell, four bytes. The tree exists inside the cell array, and the route is recovered by walking parent links backwards from the goal.

---

## 8. The Search Loop

```
        ┌──────────────────────────────────────┐
        │  Pop cheapest cell                   │  ← heap extract-min, O(log n)
        │  OPEN.extract_min()                  │
        └──────────────────┬───────────────────┘
                           ▼
        ┌──────────────────────────────────────┐
        │  Goal reached?                       │  → if yes, walk parent links
        │  compare cell id                     │     back and return the route
        └──────────────────┬───────────────────┘
                           ▼
        ┌──────────────────────────────────────┐
        │  Generate neighbours                 │  O(1), at most 8
        │  cost 1 or √2                        │
        └──────────────────┬───────────────────┘
                           ▼
        ┌──────────────────────────────────────┐
        │  Filter blocked and closed           │  grid O(1), hash O(1)
        │  grid lookup + CLOSED membership      │
        └──────────────────┬───────────────────┘
                           ▼
        ┌──────────────────────────────────────┐
        │  Push new cells to OPEN              │  ← heap insert, O(log n)
        │  OPEN.insert(cell, f)                │
        └──────────────────┬───────────────────┘
                           │
                    ↻ returns to step 1 until OPEN is empty
```

### 8.1 Where the cost actually accumulates

The middle three steps are all O(1). They do not scale with how much is in the queue. The first and last steps carry every logarithm in the system.

Total cost is O(|E| log |V|), which on a grid with |V| = n and |E| ≈ 4n is **O(n log n)**. Every `log` comes from the two heap operations.

- Replace the heap with a plain array: those become O(n), total rises to **O(n²)**
- Replace it with a bucket queue: those become O(1), total falls to **O(n)**

### 8.2 The insert-to-extract ratio

Each expansion pops one cell and pushes up to eight neighbours, which suggests insertions should heavily outnumber extractions.

Measured on a 256 × 256 grid, the ratio is **1.20 to 1**, not the 8 to 1 that the branching factor implies. Most neighbours never reach the OPEN list: they are already closed, blocked, or fail to improve a known `g` value, so they are discarded before insertion.

This weakens, without eliminating, the argument for a 4-ary heap. Sift-up costs O(log₄ n) while sift-down costs O(4 log₄ n), so a 4-ary heap still favours the cheaper operation, but with insertions only 20 per cent more frequent than extractions the expected gain is modest. Whether it materialises is an open question for the final review.

This correction is recorded rather than quietly removed, because the original estimate was reasoned from the branching factor and was wrong. Measuring it was the only way to find out.

---

## 9. Unit 1 — Trees and Heaps

### 9.1 Unsorted array, the baseline

The simplest OPEN list is a plain array. Insertion is O(1) at the end. Finding the cheapest cell means comparing every element, which is O(n).

With 5,000 cells in OPEN, one extraction costs 5,000 comparisons. Across 10,000 expansions that is around 50 million comparisons spent only on deciding what to look at next. This is the control that everything else is measured against.

### 9.2 Binary min heap, the core structure

A complete binary tree with one rule: **every parent is less than or equal to both its children.** The smallest element is therefore always at the root, readable in O(1).

It is stored as a plain array with no pointers:

```
parent(i) = (i − 1) / 2
left(i)   = 2i + 1
right(i)  = 2i + 2
```

**Insertion (sift-up).** Place the new element at the end, then compare with its parent and swap while it is smaller. Inserting f-values 12, 9, 14, 7, 11, 8:

| Step | Action | Array after |
|---|---|---|
| 1 | insert 12 | `[12]` |
| 2 | insert 9, swap with 12 | `[9, 12]` |
| 3 | insert 14, no swap | `[9, 12, 14]` |
| 4 | insert 7, swaps twice to the root | `[7, 9, 14, 12]` |
| 5 | insert 11, no swap (11 > 9) | `[7, 9, 14, 12, 11]` |
| 6 | insert 8, swaps once with 14 | `[7, 9, 8, 12, 11, 14]` |

As a tree, and as the array that actually stores it:

```
            7                 index:  0   1   2   3   4   5
          /   \               array: [7]-[9]-[8]-[12]-[11]-[14]
         9     8
        / \   /               node 9 at index 1
      12  11 14               children at 2(1)+1 = 3 and 2(1)+2 = 4
                              which hold 12 and 11 ✓
```

**Extract-min (sift-down).** Take the root, move the last element into its place, then swap downward while a child is smaller. Removing 7: element 14 moves to the root, compares against children 9 and 8, swaps with the smaller one, 8. Result `[8, 9, 14, 12, 11]`, new minimum 8. Two comparisons, one swap.

**Why O(log n).** A complete binary tree of n nodes has height ⌊log₂ n⌋. Sift-up and sift-down move one level per step, so both touch at most log₂ n elements. With 5,000 cells in OPEN that is about 12 comparisons against the array's 5,000.

**Build-heap is O(n), not O(n log n).** Building from n unsorted elements by sifting down from the middle of the array costs Θ(n), because most nodes sit near the leaves where sift-down has almost nowhere to travel. Only the root can travel the full height. The sum Σ i/2ⁱ converges to 2, giving O(2n) = Θ(n).

> **Correction worth noting.** Binary heap insertion is O(log n) worst case and O(1) *average* case for randomly ordered keys. It is **not** O(1) amortised. Inserting keys in decreasing order forces every element to the root, giving Θ(n log n) for n insertions with no amortisation available. Genuine O(1) amortised insertion belongs to the mergeable heaps: binomial, Fibonacci and pairing.

### 9.3 Binary search tree, and where it breaks

A BST keeps records ordered: left subtree smaller, right subtree larger. Searching halves the space each comparison, giving O(log n) when balanced.

A BST does not balance itself. Inserting keys in sorted order produces a chain. Inserting 1 through 7 in order:

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

Height 6 instead of 2. Searching for 7 takes 7 comparisons instead of 3. The structure is a linked list carrying extra pointers.

Constructing it is worse than the search. Inserting n sorted keys costs Σi = n(n−1)/2 = **Θ(n²)**. For 10⁵ obstacle cells that is around 5 × 10⁹ comparisons.

This matters here because cell records are generated by scanning the grid row by row, which is sorted order. **The BST's worst case is this project's expected case.**

### 9.4 AVL tree, the fix

An AVL tree is a BST that rebalances itself. Every node tracks the height difference between its subtrees, and when that difference reaches 2 a rotation restores balance.

Inserting the same keys 1 through 7:

- Insert 1, 2. Inserting 3 unbalances 1 (RR case), rotate left. Root becomes 2.
- Insert 4, 5. Imbalance at 3 (RR), rotate left. Subtree becomes 4 with children 3 and 5.
- Insert 6. Imbalance at 2 (RR), rotate left. Root becomes 4.
- Insert 7. Imbalance at 5 (RR), rotate left. Subtree becomes 6 with children 5 and 7.

```
        4
      /   \
     2     6
    / \   / \
   1   3 5   7
```

Height 2. Search for 7 is 3 comparisons. At 10⁵ records the difference is 10⁵ comparisons against about 17.

**Height bound.** Solving the recurrence N(h) = N(h−1) + N(h−2) + 1 gives h ≤ 1.4404 log₂(n+2) − 0.328. For n = 10⁵ that is h ≤ 23, against 17 for a perfectly balanced tree. A 39 per cent height penalty is the price of allowing balance factor ±1 instead of 0.

**Rotation cost.** A rotation relinks three pointers in constant time. At most **two** rotations restore balance after any single insertion. Deletion is worse: rotations can cascade to the root, O(log n) of them.

**The four cases.** LL and RR need one rotation each. LR and RL need two, because the inner child has to be rotated into position first.

### 9.5 Max heap

The same structure with the rule reversed, so the largest element sits at the root. Correct when you repeatedly want the biggest item, as in heap sort or highest-priority-first scheduling.

A\* always wants the cheapest unexplored cell. A max heap would answer the wrong question at every step.

### 9.6 Threaded binary tree

An ordinary binary tree with n nodes has n+1 empty child pointers going nowhere. A threaded tree reuses them: an empty right pointer stores the inorder successor.

Inorder traversal then needs no recursion and no stack, running in O(n) time with **O(1) auxiliary space**. On a flight controller with a bounded stack, removing O(h) recursion depth from a routine that runs every telemetry tick is a real benefit.

It is not part of the core search, so it is analysed rather than used.

### 9.7 Traversals, and what each one is for

Using the AVL tree from 9.4:

| Traversal | Order | Result | Role here |
|---|---|---|---|
| Inorder (L, N, R) | sorted | 1, 2, 3, 4, 5, 6, 7 | Ordered cell scan; feeds the Unit 2 adjacency build |
| Preorder (N, L, R) | root first | 4, 2, 1, 3, 6, 5, 7 | Serialising the stored map for checkpointing |
| Postorder (L, R, N) | children first | 1, 3, 2, 5, 7, 6, 4 | Merging or freeing child regions before the parent |
| Level order | by depth | 4, 2, 6, 1, 3, 5, 7 | Coarse-to-fine inspection of the structure |

The traversal this project actually depends on is none of the four. Path recovery walks parent links from the goal back to the start, which is a leaf-to-root walk on the search tree. That is why a parent pointer, rather than child pointers, is the right representation.

### 9.8 Heap sort

Two legitimate uses here: sorting the flight log by timestamp for telemetry export, and ordering waypoints by ETA before dispatch.

Sorting ETAs `[12, 9, 14, 7, 11, 8]` with a max heap. BUILD-MAX-HEAP from index ⌊n/2⌋−1 = 2 downward:

| i | Subtree root | Children | Action | Array |
|---|---|---|---|---|
| 2 | 14 | 8 | none | `[12, 9, 14, 7, 11, 8]` |
| 1 | 9 | 7, 11 | swap 9 ↔ 11 | `[12, 11, 14, 7, 9, 8]` |
| 0 | 12 | 11, 14 | swap 12 ↔ 14 | `[14, 11, 12, 7, 9, 8]` |

Then repeatedly swap root with the last unsorted element and re-heapify:

| Step | Swap | Array (sorted region after `|`) |
|---|---|---|
| 1 | 14 ↔ 8 | `[12, 11, 8, 7, 9 \| 14]` |
| 2 | 12 ↔ 9 | `[11, 9, 8, 7 \| 12, 14]` |
| 3 | 11 ↔ 7 | `[9, 7, 8 \| 11, 12, 14]` |
| 4 | 9 ↔ 8 | `[8, 7 \| 9, 11, 12, 14]` |
| 5 | 8 ↔ 7 | `[7 \| 8, 9, 11, 12, 14]` |

**Result: `[7, 8, 9, 11, 12, 14]`**

**Why heap sort specifically on a drone:**

| Algorithm | Worst case | Auxiliary space | Stable | Adaptive |
|---|---|---|---|---|
| **Heap sort** | **O(n log n)** | **O(1)** | no | no |
| Quicksort | O(n²) | O(log n) stack | no | no |
| Merge sort | O(n log n) | **O(n)** | yes | no |
| Insertion sort | O(n²) | O(1) | yes | **yes** |

Heap sort is the only one with both an O(n log n) worst-case guarantee and O(1) auxiliary space. On an embedded controller with a hard deadline and no memory headroom, quicksort's O(n²) tail risk and merge sort's buffer are both disqualifying. Merge sort's stability is irrelevant since ETA keys are unique.

---

## 10. Unit 2 — Graphs

### 10.1 The grid as a graph

Every open cell is a vertex. An edge connects two cells if the drone can move directly between them. Edge weights are movement costs, 1 or √2.

In an 8-connected grid a cell has at most 8 neighbours. In a 3 × 3 grid numbered:

```
0 1 2
3 4 5
6 7 8
```

- Corner cell 0 has 3 neighbours: 1, 3, 4
- Edge cell 1 has 5 neighbours: 0, 2, 3, 4, 5
- Centre cell 4 has all 8

For a W × H grid: |V| = W · H and |E| ≈ 4 · W · H, since each of the 8 directed edges is shared between two cells.

### 10.2 Adjacency list against adjacency matrix

**Adjacency matrix.** A V × V table where entry [i][j] records whether an edge exists. Edge lookup is O(1). Space is O(V²) regardless of how many edges exist.

**Adjacency list.** Each vertex stores a list of its real neighbours. Space is O(V + E).

The 3 × 3 example: 9 vertices means an 81-entry table holding 20 undirected edges. Most of it is empty. Scaling up:

| Grid | Vertices | Matrix entries | Matrix memory (1 B/entry) | List entries |
|---|---|---|---|---|
| 128 × 128 | 16,384 | 268,435,456 | 268 MB | ~131,000 |
| 256 × 256 | 65,536 | 4,294,967,296 | **4.29 GB** | ~524,000 (**0.5 MB**) |
| 512 × 512 | 262,144 | 6.87 × 10¹⁰ | 68.7 GB | ~2,100,000 |

The grid graph is **sparse**: at most 8 neighbours out of 65,535 possible. The matrix reserves space for every conceivable edge and stores almost nothing in it.

The deeper reason for rejecting it is the access pattern. Its O(1) advantage answers "is there an edge between these two arbitrary cells?", a question A\* never asks. A\* asks "list my neighbours", which the list answers directly.

### 10.3 BFS, DFS and A\*

All three explore a graph; they differ in the order and in what they guarantee.

**Breadth-first search.** A queue, level by level. Finds the route with the fewest steps. O(V + E) with an adjacency list. Unusable here on its own because it treats every move as equal cost, so it would return a route with fewer steps that is longer in distance.

**Depth-first search.** A stack, deep before backtracking. Finds *a* route with no shortness guarantee. Useful for connectivity and component analysis, not for optimal routing.

**A\*.** A priority queue ordered by f = g + h. The heuristic pushes the search toward the goal instead of spreading evenly, so it expands far fewer nodes than Dijkstra. Dijkstra is exactly A\* with h = 0.

```
A_STAR(start, goal):
    OPEN ← empty min-heap keyed on f
    g[start] ← 0;  f[start] ← h(start)
    OPEN.insert(start, f[start])
    while OPEN not empty:
        u ← OPEN.extract_min()                     // O(log n)
        if u = goal: return walk_parents(u)
        CLOSED.add(u)
        for each v in neighbours(u):               // at most 8
            if grid.blocked(v) or v in CLOSED: continue
            tentative ← g[u] + cost(u, v)
            if tentative < g[v]:
                g[v] ← tentative
                f[v] ← tentative + h(v)
                parent[v] ← u
                OPEN.insert(v, f[v])               // O(log n)
    return NO_PATH
```

**Admissibility.** h never overestimates the true remaining cost. Octile distance satisfies this on a uniform grid, which is what guarantees the route returned is optimal.

---

## 11. Why These Structures and Not the Others

The governing principle: **pick the structure that answers your question cheaply, not the most powerful one available.**

The min heap is the weakest structure in the analysis. It cannot search by key at all. It is also the core of the project, because the only question A\* asks the frontier is "what is cheapest", and that is the one thing a heap does in O(1).

| Question | Frequency | Chosen | Rejected | Why the rejected one loses |
|---|---|---|---|---|
| Q1 Is this cell blocked? | ~10⁵ | Flat array, O(1) | Binary tree, O(n) | No ordering to guide a descent |
| Q2 What is cheapest? | ~10⁴ | Min heap, O(1) peek | Unsorted array, O(n) | Rescans the whole frontier per pop |
| Q3 Already expanded? | ~10⁵ | Hash O(1) / AVL O(log n) | Plain BST, O(n) | Degenerates on sorted insertion |
| Q4 My neighbours? | ~10⁵ | Adjacency list, O(degree) | Adjacency matrix, O(V²) space | 4.29 GB to hold 0.5 MB of edges |

### 11.1 Each rejection, for a different reason

**Binary tree — no ordering rule.** It can subdivide the grid recursively, but without an ordering rule nothing tells a search which way to descend. Finding a cell means visiting nodes until it turns up: O(n), worse than the flat array's O(1). It stays in the analysis as the baseline the ordered structures improve on.

**Max heap — right structure, wrong question.** Nothing is wrong with max heaps. A\* wants minimum f, and a max heap would hand back the most expensive cell every step. In a different domain the verdict flips: an alert-triage system wants the riskiest item first, and there a max heap is correct.

**Threaded binary tree — solves a real problem this project does not have.** Stack-free O(1)-space traversal is genuinely valuable on constrained hardware, but the waypoint list is 10 to 200 items read occasionally, not on the critical path. Optimising it saves nothing measurable. Rejecting something for being irrelevant rather than bad is a distinction worth stating.

**Adjacency matrix — the access pattern does not match.** The space figure alone is disqualifying, and its one advantage answers a question the algorithm never asks.

**BST is Limited, not No.** It earns its place by being the thing AVL fixes. The rotation overhead of AVL cannot be justified without first showing that sorted insertion produces a height-6 chain. It is in the analysis as evidence, not as a candidate.

**Unsorted array is Baseline, not No.** Its O(n²) total is the number this project measures against. Removing it would remove the comparison.

**Tree traversal is Support** because it is not a container at all. It is an operation, so it cannot be chosen against alternatives the way the others can.

**Single-threaded is Yes for a methodological reason.** With parallel expansion, a speedup could no longer be attributed to the data structure rather than the thread count. Keeping it sequential is a controlled-experiment requirement, not a performance choice.

### 11.2 A point that could be challenged

The Binary Tree row describes recursive subdivision of the grid, which is a quadtree, and a quadtree genuinely is good for map storage. Its node count scales with obstacle perimeter rather than area, and OctoMap applies exactly this idea in 3D.

It is excluded because it is not on the Unit 1 syllabus, which covers Binary Trees, BST, AVL, Threaded Trees and Heaps. Claiming a structure the course has not covered would be overreaching at this stage. The honest position is that subdivision would help and was left out deliberately.

---

## 12. The Swappable Interface

The interface is the deliverable that makes the whole comparison valid.

```
interface OpenList:
    insert(cell_id, f_value)     // add a cell to the frontier
    extract_min() -> cell_id     // remove and return the cheapest
    is_empty() -> bool
```

A\* calls these three operations and knows nothing about what sits behind them. Three implementations satisfy the same interface:

**1. UnsortedArrayOpenList** — append on insert; linear scan on extract. O(1) / O(n).

**2. BinaryHeapOpenList** — sift-up on insert; sift-down on extract. O(log n) / O(log n).

**3. BucketQueueOpenList** — an array of buckets indexed by `f mod (C+1)`, used as a circular buffer. O(1) / O(1) amortised.

The bucket queue works here because of a structural property of grids that general graph algorithms cannot assume. On an 8-connected uniform-cost grid, edge weights take exactly **two** values, 1 and √2. Scaled by 1000 they become integers {1000, 1414}, so all f-values lie on a bounded integer lattice with maximum edge cost C = 1414. Under a consistent heuristic, A\* expands nodes in non-decreasing f order, so live keys always lie within a window of width C above the current minimum. Those are precisely the preconditions for Dial's bucket queue.

Likhachev and Koenig replaced D\* Lite's binary heap with buckets and reported a **factor of two** runtime reduction, which is direct empirical support from this exact domain.

**Why the interface matters for the viva.** Without it, swapping the structure would mean editing the search itself, and the comparison would be between two different programs rather than between two structures. The interface is what holds the algorithm fixed.

---

## 13. Complexity Summary

| Operation | Flat array | Binary tree | BST (sorted input) | AVL | Min heap | 4-ary heap | Bucket queue |
|---|---|---|---|---|---|---|---|
| Build from n items | O(n) | O(n) | **O(n²)** | O(n log n) | **O(n)** | O(n) | O(n) |
| Search by key | **O(1)** | O(n) | O(n) | O(log n) | O(n) | O(n) | O(n) |
| Insert | O(1) | O(n) | O(n) | O(log n) | O(log n) | O(log₄ n) | **O(1)** |
| Find minimum | O(n) | O(n) | O(n) | O(log n) | **O(1)** | **O(1)** | **O(1)** |
| Extract minimum | O(n) | O(n) | O(n) | O(log n) | O(log n) | O(4 log₄ n) | **O(1)** am. |
| Space | Θ(n) | Θ(n) + 16 B/node | Θ(n) + 16 B/node | Θ(n) + 24 B/node | Θ(n), no overhead | Θ(n), no overhead | Θ(n + C) |

**Whole-search cost:**

| OPEN list | Total A\* cost |
|---|---|
| Unsorted array | O(n²) |
| Binary heap | O(n log n) |
| Bucket queue | O(n) |

---

## 14. Measurement Plan

The first round of results is reported in 15.1.1. This section states the protocol, including the parts not yet applied.

### 14.1 Factors

**Independent:** grid size {128², 256², 512², 1024²}; obstacle density {5, 10, 20, 30, 40}%; OPEN-list structure {unsorted array, binary heap, 4-ary heap, bucket queue}; tie-breaking {none, prefer-larger-g}.

**Dependent:** wall-clock planning time; node expansions; peak OPEN size; total heap operations; peak memory; path length ÷ known optimum.

### 14.2 Results table

| Metric | 128² | 256² | 512² | 1024² | Predicted |
|---|---|---|---|---|---|
| Node expansions | | | | | ~Θ(n) worst, far less with good h |
| Peak OPEN size | | | | | ~O(√n) on open maps |
| Runtime, unsorted array (ms) | | | | | O(n²), should blow up visibly |
| Runtime, binary heap (ms) | | | | | baseline |
| Runtime, 4-ary heap (ms) | | | | | 10–30% faster than binary |
| Runtime, bucket queue (ms) | | | | | fastest; log factor removed |
| Expansions with tie-breaking | | | | | markedly fewer on open maps |

### 14.3 Statistical discipline

- Use the **MovingAI 2D pathfinding benchmarks** (Sturtevant, 2012), which provide standard grid maps with known optimal costs. Normalising path length by the known optimum makes results directly comparable to published work.
- ≥ 30 problem instances per design cell, drawn from the published problem sets rather than hand-picked
- ≥ 10 timing repetitions per instance; discard the first (cold cache) run
- Report median and interquartile range, not the mean alone; timing distributions are right-skewed
- Report 95% confidence intervals; a bar chart without error bars is not evidence
- Separate **algorithmic** metrics (expansions, heap operations — deterministic, exactly reproducible) from **implementation** metrics (wall-clock, memory — machine-dependent). Report both and state which is which.
- Pin the environment: CPU model, cache sizes, RAM, compiler and flags, OS
- Record the seed for every generated map

### 14.4 Threats to validity

| Threat | Type | Mitigation |
|---|---|---|
| Implementation quality confounds the structural comparison | Internal | Share allocator, comparison functions and instrumentation across all three variants; report operation counts alongside times |
| Language or runtime overhead masks the effect | Internal | Use a compiled language or array-backed structures; report GC-excluded timings |
| Benchmark maps are game maps, not aerial environments | External | Add a synthetic sparse-airspace stratum; report per-stratum, never pooled |
| 2D fixed altitude does not generalise to 3D flight | External | State the limitation; cite OctoMap for the octree extension |
| Cache effects vary by machine | Construct | Report on ≥ 2 machines with different cache sizes if available |

---

## 15. Progress and Remaining Work

### 15.1 Review 2 — 70%

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
| 10 | Grid, cost model and octile heuristic implemented | Complete |
| 11 | `OpenList` interface defined | Complete |
| 12 | Unsorted array, binary heap, bucket queue implemented | Complete |
| 13 | A\* search depending only on the interface | Complete |
| 14 | Correctness suite, six tests | Complete |
| 15 | First timing comparison across three structures | Complete |
| 16 | Pairwise waypoint distance matrix from repeated A\* runs | Complete |
| 17 | Dynamic programming: 0/1 Knapsack waypoint selector with traceback | Complete |
| 18 | Backtracking and branch and bound tour solvers | Complete |
| 19 | Brute-force verifiers for the selector and both solvers | Complete |
| 20 | 4-ary heap variant | Not started |
| 21 | MovingAI benchmark maps in place of random maps | Not started |
| 22 | Binomial and Fibonacci heap comparison | Not started |
| 23 | Final analysis and report | Not started |

The 70% covers the completed research from Review 1, a working planner with a
passing test suite and a first round of measurements, and the mission layer
built on top of it: the dynamic programming selector and the branch and bound
tour solver. Nineteen of twenty-three activities are complete.

**Environment.** g++ 13.3.0, C++17, `-O2 -Wall -Wextra`, Ubuntu 24.04.
Timing uses `std::chrono::steady_clock`. Ten maps per configuration, five
repetitions each, the cold run discarded, medians reported.

### 15.1.1 Measured results

20% obstacle density, random maps, corner to corner.

| Structure | 64x64 | 128x128 | 256x256 |
|---|---|---|---|
| Unsorted array, O(n^2) | 0.303 ms | 3.421 ms | not run |
| Binary heap, O(n log n) | 0.042 ms | 0.244 ms | 1.085 ms |
| Bucket queue, O(n) | 0.028 ms | 0.144 ms | 0.568 ms |

**Speedup over the array baseline:** about 7x at 64x64, rising to 14x for the heap
and 24x for the bucket queue at 128x128. The gap widens with grid size, which
is what O(n^2) against O(n log n) predicts.

**Scaling when the grid doubles**, that is four times the cells:

| Structure | 64 to 128 | 128 to 256 | Expected |
|---|---|---|---|
| Unsorted array | 11.3x | not run | super-linear |
| Binary heap | 5.8x | 4.4x | about 4x plus a log factor |
| Bucket queue | 5.0x | 4.0x | about 4x, linear |

The 64 to 128 column is noisy: a 64x64 plan finishes in tens of microseconds,
so scheduling jitter is a large share of the measurement. The 128 to 256 column
is the one the argument rests on.

The bucket queue tracks 4x almost exactly. That is the log factor
disappearing, visible in measurement rather than argued from the bound.

**Bucket queue against binary heap:** 33% faster at 64x64, 41% at 128x128,
48% at 256x256.

### 15.1.2 The mission layer, measured

Both run on waypoint costs taken from repeated A\* calls, so the mission layer
sits directly on the search layer rather than beside it.

**0/1 Knapsack against exhaustive subset search.** The DP table costs
O(n x W) regardless of n, so at small n the exhaustive search wins outright.
The crossover is at 14 items. By 22 items the DP runs in 0.034 ms against
201.42 ms for the exhaustive search, and both return the same selection.

**Branch and bound against plain backtracking.** At 11 stops the bound opens
70,590 nodes where backtracking opens 9,864,101, a cut of 99.3%, and the tour
returned is identical. Below about 7 stops the bound costs more to compute than
it saves, so backtracking is faster there. Both crossovers are reported as
measured rather than chosen for a favourable figure.


### 15.1.3 Findings that corrected the design

**A bug the cross-structure test caught.** The first bucket queue used C+1
buckets, taking the Dijkstra bound where a successor key exceeds the current
minimum by at most one edge cost. It returned suboptimal routes: 43006 against
the heap's 41592 on the same map. For A\* with a consistent heuristic the bound
is twice that, since

    f_v = g_u + c + h_v = f_u - h_u + c + h_v,  and  h_v - h_u <= c
    therefore f_v <= f_u + 2c

At C+1 the distant keys aliased onto occupied buckets and cells came out in the
wrong order. Corrected to 2C+1. The defect was found only because the test
suite checks that all three structures return the same cost across 50 random
maps; no single-structure test would have exposed it.

**The insert-to-extract ratio was overestimated.** Section 8.2 of this document
originally argued that insertions outnumber extractions roughly 8 to 1, since
each expansion generates up to eight neighbours, and used that to favour
structures with cheap insertion. Measured on a 256x256 grid the ratio is
**1.20 to 1**. Most neighbours are rejected before insertion because they are
already closed, blocked, or do not improve a known g value. The argument for
favouring cheap insertion is therefore much weaker than first stated, and
Section 8.2 is corrected accordingly.

**Tie-breaking has a larger effect than expected.** On an identical map the
binary heap expanded 62 nodes and the bucket queue 32, a 48% difference, with
identical route cost. Both are correct. They differ because the bucket queue
pops LIFO within a bucket, which biases exploration toward recently discovered
cells and therefore toward the goal. Tie-breaking deserves treatment as a
factor in its own right rather than as an implementation detail.

### 15.2 Remaining

**Review 2 — delivered.** The planner is built with the `OpenList` interface in place, all three structures are implemented and tested, and the first timing comparison is reported in 15.1.1. The mission layer is also in place: the 0/1 Knapsack selector chooses which delivery waypoints fit the battery budget, and the branch and bound solver orders the chosen stops into a minimum-cost tour. Both are measured in 15.1.2.

**Review 3, to approximately 85%.** Replace the random maps with MovingAI benchmark maps, which carry known optimal costs and allow direct comparison. Add the 4-ary heap variant and test it against the measured 1.20 to 1 insert ratio. Sweep obstacle density from 5% to 40% and report per density with confidence intervals.

**Final, 100%.** Compare the binary heap against the binomial and Fibonacci heaps, and report the measured result rather than the asymptotic expectation. Produce the full complexity analysis, the charts, the stated limitations and the final report, and prepare the live demonstration.

---

## 16. Scope and Limitations

- Two dimensions at fixed altitude; the base paper works in three
- No flight dynamics, turning radius, potential fields or spline smoothing
- Static maps with known obstacles; no sensing or replanning mid-flight
- Simulation only; no hardware
- Single-threaded by design, so speedups remain attributable to the structure
- The comparison is between data structures, not between planners

---

## 17. References

1. Gao, W., Li, L., & Pang, D. (2026). Urban low-altitude UAV path planning by fusing an enhanced A\* algorithm with an adaptive artificial potential field method. *Scientific Reports, 16*, Article 18275. https://doi.org/10.1038/s41598-026-45160-6 **[Base paper]**
2. Meng, W., Zhang, X., Zhou, L., Guo, H., & Hu, X. (2025). Advances in UAV Path Planning: A Comprehensive Review of Methods, Challenges, and Future Directions. *Drones, 9*(5), 376. https://doi.org/10.3390/drones9050376
3. Dradoum, A., Khelassi, A., & Lachekhab, F. (2025). Intelligent path planning algorithms for UAVs: Classification, complexity analysis, hybrid ablation insights, and future directions. https://doi.org/10.1177/16878132251355020
4. Hart, P. E., Nilsson, N. J., & Raphael, B. (1968). A Formal Basis for the Heuristic Determination of Minimum Cost Paths. *IEEE Transactions on Systems Science and Cybernetics, 4*(2), 100–107. https://doi.org/10.1109/TSSC.1968.300136
5. Elfes, A. (1989). Using Occupancy Grids for Mobile Robot Perception and Navigation. *Computer, 22*(6), 46–57. https://doi.org/10.1109/2.30720
6. Sturtevant, N. R. (2012). Benchmarks for Grid-Based Pathfinding. *IEEE Transactions on Computational Intelligence and AI in Games, 4*(2), 144–148. https://doi.org/10.1109/TCIAIG.2012.2197681
7. Hornung, A., Wurm, K. M., Bennewitz, M., Stachniss, C., & Burgard, W. (2013). OctoMap: An Efficient Probabilistic 3D Mapping Framework Based on Octrees. *Autonomous Robots, 34*(3), 189–206. https://doi.org/10.1007/s10514-012-9321-0
8. Harabor, D., & Grastien, A. (2011). Online Graph Pruning for Pathfinding on Grid Maps. *Proceedings of the AAAI Conference on Artificial Intelligence, 25*, 1114–1119. https://doi.org/10.1609/aaai.v25i1.7994
9. Dial, R. B. (1969). Algorithm 360: Shortest-path forest with topological ordering. *Communications of the ACM, 12*(11), 632–633.
10. Likhachev, M., & Koenig, S. (2006). Incremental heuristic search in games: The quest for speed. *Proceedings of AAAI AIIDE, 2*(1), 118–120.
11. Larkin, D. H., Sen, S., & Tarjan, R. E. (2014). A back-to-basics empirical study of priority queues. *Proceedings of ALENEX*. arXiv:1403.0252
12. Cormen, T. H., Leiserson, C. E., Rivest, R. L., & Stein, C. (2022). *Introduction to Algorithms* (4th ed.). MIT Press.
13. Patel, A. *Amit's A\* Pages, Implementation Notes.* Stanford University. https://theory.stanford.edu/~amitp/GameProgramming/index.html

The complete source list, including books and video lectures, is in `evidence.md`.

---

## 18. Submission Checklist

- [ ] Replace `[ERP ID]` in the header table
- [ ] Fill the GitHub repository URL in the header and in the progress report
- [ ] Verify reference #3's author initials, journal name, volume and pages
- [ ] Open the base paper and confirm the OPEN-list gap first-hand: search the PDF for "heap", "priority queue" and "sorted"
- [ ] Push `include/`, `src/`, `Makefile`, `results.csv` and the charts to the repository
- [ ] Record the machine used for timings (CPU model, RAM) in the results section
- [ ] Re-run `make test` on the submission machine and confirm all six tests pass
- [ ] Fill the **AI Tool Usage Declaration** (section F of the assignment brief)
- [ ] Read the document through once and change anything that does not sound like your own voice

---

*Review 2 · Month 2 · This document is updated at the end of each review stage.*
