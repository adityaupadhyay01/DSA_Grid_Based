# Base Paper Selection and Research Gap Analysis

**Project:** Grid-Based Drone Path Planning Simulation
**Student:** Aditya Upadhyay · 2501330100038 · B.Tech CSE-A, Semester III
**Course:** Data Structures and Algorithms II · Faculty: Mr. Shamshad Ali

---

## 1. Selected Base Paper (published 2026)

> **Gao, W., Li, L., & Pang, D. (2026).** Urban low-altitude UAV path planning by fusing an enhanced A\* algorithm with an adaptive artificial potential field method. *Scientific Reports*, **16**, Article 18275.
> https://doi.org/10.1038/s41598-026-45160-6
> Published 20 April 2026 · **Open Access** · Nature Portfolio

**Why this paper and not another:**

| Criterion | Status |
|---|---|
| Published 2026 | ✓ 20 April 2026 |
| Peer-reviewed, indexed journal | ✓ Scientific Reports (Nature Portfolio) |
| Free full text | ✓ Open Access — you can read every section |
| Grid-based and A\*-based | ✓ Directly your topic |
| Leaves a gap you can actually work on | ✓ See §3 |
| Already cited | ✓ 3 citations, 2427 accesses |

The alternative 2026 candidate (Hu et al., *Improved A\* with Collision Probability Model*, Springer LNEE 1576, doi:10.1007/978-981-95-7676-0_30) is paywalled. Don't pick a paper you can't read.

---

## 2. What the Base Paper Does

The authors propose **A\*-APF**, a multi-phase planner for drones flying through dense urban airspace.

**Their method, in plain terms:**

1. The city is chopped into a uniform 3D grid of cubes (voxels) of fixed size δ.
2. The flight is split into three phases by a state machine: take-off (vertical climb), cruise (level flight), landing (vertical descent). Each phase has its own sub-goal and its own constraints.
3. A\* searches the voxel grid. The cost function is extended beyond f = g + h to include an obstacle-repulsion penalty and two altitude penalties, weighted w₁ to w₄.
4. Two spheres are drawn around the drone: an inner safety radius R_safe and an outer avoidance radius R_avoid. Outside both, the search takes big steps. Between them, it takes small steps and lets the potential field steer. Inside R_safe, the drone hovers.
5. Candidate neighbours are pruned by a "field of view" cone and by kinematic limits (max yaw, max pitch, minimum turning radius).
6. The final path is smoothed with cubic and quartic B-splines so the curve is flyable.

**What they report:** 46.2% less computation time and 33% fewer expanded nodes than the two competing hybrid methods they reproduced. Full method 0.53 s versus 3.58 s for plain improved-A\* alone. Tested in MATLAB 2023a on an Intel i7-9750H.

---

## 3. Research Gap

I looked for gaps that are **real**, **verifiable from the paper's own text**, and **inside your syllabus**. Three candidates, ranked.

### Gap A — The paper never states which data structure holds the OPEN list ⭐ RECOMMENDED

This is the strongest gap for your project, and it is hiding in plain sight.

The paper explains the OPEN and CLOSE lists in words:

> *"The OPEN list contains nodes that have been discovered but not yet expanded... The algorithm operates iteratively by selecting the node with the lowest cost from the OPEN list for expansion."*

**But nowhere in the paper is it stated how that selection is implemented.** There is no mention of a heap, a priority queue, a sorted array, or a hash set. The CLOSED-set membership test is likewise unspecified. Yet the paper's headline claims are *absolute wall-clock timings* (0.53 s, 3.58 s) and *node counts*, both of which depend entirely on this unstated choice.

**Why this matters:** "select the node with the lowest cost" can be implemented three ways, and they are not close to each other.

| OPEN list implementation | Cost per extract-min | Total search cost |
|---|---|---|
| Unsorted array + linear scan (MATLAB `min()` — the common default) | O(n) | **O(n²)** |
| Binary min-heap | O(log n) | O(n log n) |
| Bucket / Dial queue (valid here — grid edge costs are bounded) | O(1) amortised | **O(n)** |

If the authors used a linear scan, a large share of their 3.58 s baseline is a data-structure artefact rather than an algorithmic property, and their comparison against competing methods is confounded. If they used a heap, they simply did not report it, and the result is not reproducible. **Either way it is a gap**, and it is precisely the kind of gap a Data Structures course exists to expose.

Be honest about the uncertainty. You are not claiming they did it badly. You are claiming the paper does not say, and that this omission is material because the entire contribution is measured in time.

### Gap B — Uniform voxel grid, no hierarchical compression

The paper discretises the airspace uniformly: every cube is δ on a side, everywhere, whether it contains a skyscraper or empty sky. Memory is therefore Θ(Nx · Ny · Nz) regardless of how much of the airspace is actually empty — and urban airspace is mostly empty.

They cite no hierarchical alternative. OctoMap (Hornung et al., 2013) already does octree compression for exactly this problem, and quadtree compression makes node count scale with obstacle **perimeter** rather than **area**. This is straightforwardly Unit 1 material.

### Gap C — Replanning is explicitly one-directional

The paper states its phase index is **monotonically non-decreasing** and that *"regression to a previous phase is prohibited."* If the drone gets blocked, it hovers and asks the ground station for a fresh global route. There is no incremental repair of the existing search tree. D\* Lite (Koenig & Likhachev, 2002) solves exactly this by repairing only the locally inconsistent vertices. Good Unit 2 material, but harder to implement than A or B.

### Also worth one line in your report

The paper's Data Availability statement says the datasets are **not publicly available** because of the *"proprietary nature of the simulation models and core algorithm parameters."* Combined with the unspecified data structures, their results cannot be independently reproduced. Noting this is a legitimate and mature observation.

---

## 4. Your Repositioned Project

**Old framing:** "I will simulate a drone finding a path on a grid." (No research anchor. Nothing to compare against.)

**New framing:**

> The base paper improves *what* the search explores — phases, potential fields, pruning cones, kinematic filters — but never specifies *how* the frontier is stored, even though every result it reports is a timing. This project isolates that variable. Holding the algorithm fixed, I implement a grid-based A\* planner with three interchangeable OPEN-list structures (unsorted array, binary min-heap, bucket queue) and measure the effect on runtime and node expansions. The aim is to quantify how much of a planner's reported speed comes from its data structures rather than its heuristics.

**Your one-sentence claim:** *Data-structure choice is an unreported but measurable factor in published UAV path-planning results, and this project quantifies it.*

### Why this scoping is honest

You are in your third semester. You are not going to beat a Nature-portfolio paper on 3D urban trajectory planning, and you should not pretend to. What you *can* do is take one variable the paper left undefined and measure it properly in 2D. That is a real, modest, defensible contribution — and it is worth far more marks than an inflated claim you cannot defend in a viva.

**State these limitations explicitly in your report:**
- Your work is 2D at fixed altitude; theirs is full 3D.
- You do not model kinematics, B-spline smoothing, or potential fields.
- You are not claiming a better planner. You are measuring one factor they left unmeasured.

---

## 5. Mapping to the Five Units

The gap fits the assignment structure without forcing anything.

| Unit | Required content | How the gap drives it |
|---|---|---|
| **1 — Trees** | BST, AVL, heaps, priority queues, heap sort, traversals | **Core of the contribution.** Binary min-heap for OPEN; AVL/Morton index for obstacles; quadtree for the map (Gap B); heap sort for telemetry |
| **2 — Graphs** | Adjacency list/matrix, BFS, DFS, Dijkstra, Bellman-Ford, MST | Grid cells → vertices, moves → weighted edges. A\* against Dijkstra and BFS as baselines. The paper itself uses BFS as its optimality benchmark |
| **3 — Dynamic Programming** | Knapsack, LCS, matrix chain, resource allocation | Battery/payload allocation across waypoints as 0/1 Knapsack |
| **4 — Backtracking & B\&B** | TSP, graph colouring, N-Queens, Hamiltonian cycle | Multi-waypoint visiting order as TSP; branch-and-bound uses the same priority queue, so your Unit 1 work carries straight through |
| **5 — Advanced structures** | Red-Black, B/B+ trees, binomial and Fibonacci heaps | Compare your binary heap against binomial and Fibonacci heaps. Published benchmarks show Fibonacci heaps lose in practice despite better asymptotics — a genuinely interesting result to report |

Note how Unit 1 and Unit 4 share the same priority queue. That's what makes the report read as one argument instead of five disconnected chapters.

---

## 6. What You Do Next

1. **Download and read the paper.** It's free. Read the Abstract, Introduction, the "A\* Algorithm" subsection, and the Conclusions. Skip the equations for now — there are 67 of them and you don't need most.
2. **Find the gap yourself.** Search the PDF for "heap", "priority queue", "sorted", "data structure". Confirm with your own eyes that these words don't appear in the algorithm description. Do not take my word for it — if Mr. Ali asks how you found the gap, "I searched the paper for it" is the right answer.
3. **Write two paragraphs in your own words:** what the paper does, and what it doesn't specify.
4. **Update the Review 1 report** — Field 6 becomes a proper literature review anchored on this base paper, and Field 1 gets the new framing from §4.

---

## 7. Citation for Your Reference List

**IEEE style:**
> W. Gao, L. Li, and D. Pang, "Urban low-altitude UAV path planning by fusing an enhanced A\* algorithm with an adaptive artificial potential field method," *Scientific Reports*, vol. 16, art. no. 18275, Apr. 2026, doi: 10.1038/s41598-026-45160-6.

**APA style:**
> Gao, W., Li, L., & Pang, D. (2026). Urban low-altitude UAV path planning by fusing an enhanced A\* algorithm with an adaptive artificial potential field method. *Scientific Reports, 16*, Article 18275. https://doi.org/10.1038/s41598-026-45160-6

---

## 8. One Caution

Everything in §3 comes from my reading of the paper's full text, which I have gone through. But **you must verify Gap A yourself** before you build a whole project on it. Open the PDF, search for the terms listed in §6.2, and confirm. If you find that they *do* specify a heap somewhere I missed — in a table, a figure caption, or supplementary material — then Gap B (uniform voxel grid, no hierarchical compression) becomes your primary gap instead. It is also real, also on-syllabus, and the project barely changes.

Check first. Build second.
