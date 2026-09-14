# Evidence — Sources Consulted

**Project:** Grid-Based Drone Path Planning Simulation
**Student:** Aditya Upadhyay · 2501330100038 · B.Tech CSE-A, Semester III
**Course:** Data Structures and Algorithms II (CCSE0301) · Faculty: Mr. Shamshad Ali
**Review:** 1 · Month 1

---

## ⚠️ Read this before you submit anything from this file

I need to be straight with you about where these came from, because your report carries a signed declaration.

**Verified by me (links opened and checked):** every research paper DOI, the Red Blob Games pages, the Stanford A* pages, the Abdul Bari channel, and the MovingAI benchmark reference.

**NOT verified — and this is the important one:** I wrote "Abdul Bari's algorithms playlist" and "Jenny's Lectures for AVL rotations" into Field 6 of your report as *plausible* sources for an Indian CSE student. **I do not know whether you actually watched them.** I have now found the real links so you can check.

**What to do:** go through the list below and delete anything you did not actually use. Then tell me and I will remove it from the report too. A source you cannot discuss in a viva is worse than no source at all — it is the one thing that turns a good report into an academic integrity problem.

---

## 1. Base Paper

This is the paper your research gap comes from.

| | |
|---|---|
| **Citation** | Gao, W., Li, L., & Pang, D. (2026). Urban low-altitude UAV path planning by fusing an enhanced A* algorithm with an adaptive artificial potential field method. *Scientific Reports, 16*, Article 18275. |
| **Link** | https://doi.org/10.1038/s41598-026-45160-6 |
| **Direct PDF** | https://www.nature.com/articles/s41598-026-45160-6.pdf |
| **Access** | Open Access — free, no login |
| **Status** | ✅ Verified. Full text read. |
| **Used for** | The research gap: the paper reports all results as computation times but never states what data structure holds its OPEN list. |

**Read at minimum:** the Abstract, the Introduction, and the subsection headed "A* Algorithm". That subsection is where the OPEN list is described in words only — that is your evidence.

---

## 2. Research Papers

| # | Paper | Link | Status |
|---|---|---|---|
| 1 | Meng, W., Zhang, X., Zhou, L., Guo, H., & Hu, X. (2025). Advances in UAV Path Planning: A Comprehensive Review. *Drones, 9*(5), 376. | https://doi.org/10.3390/drones9050376 | ✅ Open Access |
| 2 | Hart, P. E., Nilsson, N. J., & Raphael, B. (1968). A Formal Basis for the Heuristic Determination of Minimum Cost Paths. *IEEE Trans. SSC, 4*(2), 100–107. | https://doi.org/10.1109/TSSC.1968.300136 | ✅ DOI verified (paywalled) |
| 3 | Elfes, A. (1989). Using Occupancy Grids for Mobile Robot Perception and Navigation. *Computer, 22*(6), 46–57. | https://doi.org/10.1109/2.30720 | ✅ DOI verified (paywalled) |
| 4 | Sturtevant, N. R. (2012). Benchmarks for Grid-Based Pathfinding. *IEEE TCIAIG, 4*(2), 144–148. | https://doi.org/10.1109/TCIAIG.2012.2197681 | ✅ DOI verified |
| 5 | Hornung, A., Wurm, K. M., Bennewitz, M., Stachniss, C., & Burgard, W. (2013). OctoMap. *Autonomous Robots, 34*(3), 189–206. | https://doi.org/10.1007/s10514-012-9321-0 | ✅ DOI verified |
| 6 | Harabor, D., & Grastien, A. (2011). Online Graph Pruning for Pathfinding on Grid Maps. *AAAI, 25*, 1114–1119. | https://doi.org/10.1609/aaai.v25i1.7994 | ✅ Free PDF from AAAI |
| 7 | **Gao, Li & Pang (2026)** — see Section 1 | https://doi.org/10.3390/... see above | ✅ **BASE PAPER** |

**Free to read right now:** #1 (MDPI), #6 (AAAI hosts the PDF), and the base paper.
**Likely need institutional login:** #2, #3, #4, #5 — check whether NIET has IEEE/Springer access. Abstracts are public regardless.

---

## 3. Books

| Book | Used for | Status |
|---|---|---|
| Cormen, T. H., Leiserson, C. E., Rivest, R. L., & Stein, C. (2022). *Introduction to Algorithms* (4th ed.). MIT Press. | Heap chapter; the proof that BUILD-HEAP costs O(n), not O(n log n) | ⚠️ Confirm you actually opened this |
| NIET DSA-II course material, Unit 1 (Trees) and Unit 2 (Graphs) | All base definitions | ✅ You definitely used this |

---

## 4. Online Resources

### ⭐ Most important one for your gap

**Amit Patel — Amit's A\* Pages (Stanford / Red Blob Games)**
https://theory.stanford.edu/~amitp/GameProgramming/index.html

Go to the **Implementation Notes → Set representation** section. It lists the exact OPEN-list structures your project compares:

> unsorted arrays or linked lists · sorted arrays · **binary heaps** · sorted skip lists · indexed arrays · hash tables · splay trees · **bucketing**

This is direct, citable support that OPEN-list choice is a recognised design decision — which is precisely why the base paper leaving it unstated is a real gap. **Cite this one.** ✅ Verified.

### Others

| Resource | Link | Used for | Status |
|---|---|---|---|
| Red Blob Games — Introduction to A* | https://www.redblobgames.com/pathfinding/a-star/introduction.html | Interactive grid diagrams; understanding how A* expands across a grid | ✅ Verified, free |
| Red Blob Games — Heuristics (see index above) | https://theory.stanford.edu/~amitp/GameProgramming/index.html | Manhattan / diagonal / Euclidean heuristics, tie-breaking | ✅ Verified |
| MovingAI 2D Pathfinding Benchmarks | Referenced in Sturtevant (2012), paper #4 | Standard grid maps with known optimal path costs, for Review 2 testing | ✅ Referenced |
| GeeksforGeeks — heap operations | https://www.geeksforgeeks.org/ | Code-level reference for heap insert / extract-min | ⚠️ Confirm you used it |

---

## 5. Video Lectures

**⚠️ This is the section I flagged at the top. I put these in your report without knowing whether you watched them. Check each one honestly.**

| Channel | Link | Claimed use in report | Status |
|---|---|---|---|
| Abdul Bari — channel | https://www.youtube.com/@abdul_bari | A* and Dijkstra | ✅ Channel exists · ⚠️ Did you watch it? |
| Abdul Bari — all playlists | https://www.youtube.com/@abdul_bari/playlists | Algorithms playlist (84 videos) covers BFS, DFS, shortest paths, greedy, DP, backtracking | ✅ Verified |
| Abdul Bari — DSA Full Course | https://www.youtube.com/playlist?list=PLsr8vTgyLdy_YndxNcI4WkH5Vorj5qvrv | Trees, AVL, heaps | ✅ Verified |
| Jenny's Lectures | — | AVL rotations | ❌ **I did not verify this. I assumed it.** |

**Decide now:** if you did not watch these, say so and I will strike the "Video lectures" bullet from Field 6. If you watched *something else* — any channel, in Hindi or English — tell me what it was and I will put the real one in. There is nothing wrong with learning AVL rotations from YouTube. There is something wrong with citing a video you never opened.

---

## 6. What Each Source Actually Contributed

A viva-proof summary. If Mr. Ali asks "where did this idea come from", this table is your answer.

| Idea in the report | Came from |
|---|---|
| Grid map as the environment model | Elfes (1989); base paper §Planning Space |
| A* and the f = g + h cost function | Hart, Nilsson & Raphael (1968); Red Blob Games |
| Diagonal step costs √2, not 1 | Red Blob Games heuristics page |
| Heap for choosing the cheapest next cell | Course material Unit 1; Cormen ch. on heaps |
| Quadtree / hierarchical map storage | Hornung et al. (2013) — octree version |
| Cutting search by pruning symmetric paths | Harabor & Grastien (2011) |
| Standard benchmark maps for testing | Sturtevant (2012) |
| **The research gap (unstated OPEN-list structure)** | **My own reading of Gao et al. (2026), supported by Amit Patel's "Set representation" list** |

---

## 7. Declaration Checklist

- [ ] Every source above that I did not personally use has been deleted from this file **and** from Field 6 of the report
- [ ] I have opened the base paper and confirmed the OPEN-list gap with my own eyes
- [ ] I can explain, without notes, what each remaining source contributed
- [ ] The **AI Tool Usage Declaration** (section F of the assignment brief) is filled in
- [ ] GitHub repository link pasted into Field 9
- [ ] ERP ID filled in
- [ ] Report signed

---

*Last updated: Month 1 / Review 1. This file will be extended at each review as new sources are used.*
