# Evidence

**Project:** Grid-Based Drone Path Planning Simulation
**Student:** Aditya Upadhyay · 2501330100038 · B.Tech CSE-A, Semester III
**Course:** Data Structures and Algorithms II (CCSE0301) · Faculty: Mr. Shamshad Ali
**Review:** 1 · Month 1

---

## 1. Base Paper

This is where the research gap comes from.

| | |
|---|---|
| **Citation** | Gao, W., Li, L., & Pang, D. (2026). Urban low-altitude UAV path planning by fusing an enhanced A* algorithm with an adaptive artificial potential field method. *Scientific Reports, 16*, Article 18275. |
| **Link** | https://doi.org/10.1038/s41598-026-45160-6 |
| **Direct PDF** | https://www.nature.com/articles/s41598-026-45160-6.pdf |
| **Used for** | The gap: the paper reports every result as computation time but never says what structure holds its OPEN list. |

**Read at minimum:** the Abstract, the Introduction, and the subsection headed "A* Algorithm". That subsection is where the OPEN list is described in words only. That is the evidence for the gap.

---

## 2. Research Papers

The first three are the ones cited in Field 6 of the report. The rest are background reading.

| # | Paper | Link | Role |
|---|---|---|---|
| 1 | Gao, W., Li, L., & Pang, D. (2026). Urban low-altitude UAV path planning by fusing an enhanced A* algorithm with an adaptive artificial potential field method. *Scientific Reports, 16*, 18275. | https://doi.org/10.1038/s41598-026-45160-6 | **Base paper** — cited in report |
| 2 | Meng, W., Zhang, X., Zhou, L., Guo, H., & Hu, X. (2025). Advances in UAV Path Planning: A Comprehensive Review of Methods, Challenges, and Future Directions. *Drones, 9*(5), 376. | https://doi.org/10.3390/drones9050376 | Cited in report |
| 3 | Dradoum, A., Khelassi, A., & Lachekhab, F. (2025). Intelligent path planning algorithms for UAVs: Classification, complexity analysis, hybrid ablation insights, and future directions. | https://doi.org/10.1177/16878132251355020 | Cited in report |
| 4 | Hart, P. E., Nilsson, N. J., & Raphael, B. (1968). A Formal Basis for the Heuristic Determination of Minimum Cost Paths. *IEEE Trans. SSC, 4*(2), 100–107. | https://doi.org/10.1109/TSSC.1968.300136 | Background — origin of A* |
| 5 | Elfes, A. (1989). Using Occupancy Grids for Mobile Robot Perception and Navigation. *Computer, 22*(6), 46–57. | https://doi.org/10.1109/2.30720 | Background — occupancy grids |
| 6 | Sturtevant, N. R. (2012). Benchmarks for Grid-Based Pathfinding. *IEEE TCIAIG, 4*(2), 144–148. | https://doi.org/10.1109/TCIAIG.2012.2197681 | Background — test maps for Review 2 |
| 7 | Hornung, A., Wurm, K. M., Bennewitz, M., Stachniss, C., & Burgard, W. (2013). OctoMap. *Autonomous Robots, 34*(3), 189–206. | https://doi.org/10.1007/s10514-012-9321-0 | Background — 3D map storage |
| 8 | Harabor, D., & Grastien, A. (2011). Online Graph Pruning for Pathfinding on Grid Maps. *AAAI, 25*, 1114–1119. | https://doi.org/10.1609/aaai.v25i1.7994 | Background — cutting search cost |

---

## 3. Books

| Book | Used for |
|---|---|
| Cormen, T. H., Leiserson, C. E., Rivest, R. L., & Stein, C. (2022). *Introduction to Algorithms* (4th ed.). MIT Press. | Heap chapter; the proof that BUILD-HEAP costs O(n), not O(n log n) |
| NIET DSA-II course material, Unit 1 (Trees) and Unit 2 (Graphs) | All base definitions |

---

## 4. Online Resources

### The one that matters most for the gap

**Amit Patel — Amit's A\* Pages (Stanford / Red Blob Games)**
https://theory.stanford.edu/~amitp/GameProgramming/index.html

Go to **Implementation Notes → Set representation**. It lists exactly the OPEN-list structures this project compares:

> unsorted arrays or linked lists · sorted arrays · **binary heaps** · sorted skip lists · indexed arrays · hash tables · splay trees · **bucketing**

### Others

| Resource | Link | Used for |
|---|---|---|
| Red Blob Games — Introduction to A* | https://www.redblobgames.com/pathfinding/a-star/introduction.html | Interactive grid diagrams; how A* expands across a grid |
| Red Blob Games — Heuristics | https://theory.stanford.edu/~amitp/GameProgramming/index.html | Manhattan / diagonal / Euclidean heuristics, tie-breaking |
| MovingAI 2D Pathfinding Benchmarks | Referenced in Sturtevant (2012), paper #6 | Standard grid maps with known optimal costs, for Review 2 testing |
| GeeksforGeeks — heap operations | https://www.geeksforgeeks.org/ | Code-level reference for heap insert / extract-min |

---

## 5. Video Lectures

| Source | Link | Used for |
|---|---|---|
| CodeWithHarry | https://www.youtube.com/@CodeWithHarry | C and C++ |
| Abdul Bari | https://www.youtube.com/@abdul_bari/playlists | Algorithms playlist: BFS, DFS, shortest paths, greedy, DP, backtracking |
| Take U Forward | https://takeuforward.org/ | Trees, AVL, heaps |

---

## 6. What Each Source Contributed

If Mr. Ali asks "where did this come from", this table is the answer. Every row matches something actually claimed in the report.

| Idea in the report | Came from |
|---|---|
| Grid map as the environment model | Elfes (1989); base paper, Planning Space section |
| A* and the f = g + h cost function | Hart, Nilsson & Raphael (1968); Red Blob Games |
| Diagonal step costs √2, not 1 | Red Blob Games heuristics page |
| Min Heap for choosing the cheapest next cell | Course material Unit 1; Cormen heap chapter |
| BST degenerates to O(n) on sorted insertion | Course material Unit 1; Cormen |
| Adjacency List preferred over Matrix for a sparse grid | Course material Unit 2 |
| Unsorted array as the O(n) baseline to measure against | Amit Patel, "Set representation" |
| Standard benchmark maps for Review 2 testing | Sturtevant (2012) |
| **The research gap (unstated OPEN-list structure)** | **My own reading of Gao et al. (2026), supported by Amit Patel's "Set representation" list** |

---

## 7. Repository

| | |
|---|---|
| **GitHub** | `https://github.com/adityaupadhyay01/DSA_Grid_Based.git` |
| **Contains** | Research papers folder and README now; simulation code, diagrams and timing tables from Review 2 onwards |

---

*Last updated: Month 1 / Review 1. Extended at each review as new sources are used.*
