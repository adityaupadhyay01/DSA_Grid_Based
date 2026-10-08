# Research papers

| File | Citation | Role in this project |
|---|---|---|
| `s41598-026-45160-6.pdf` | Gao, W., Li, L., & Pang, D. (2026). Urban low-altitude UAV path planning by fusing an enhanced A* algorithm with an adaptive artificial potential field method. *Scientific Reports, 16*, 18275. | **Base paper.** Reports every result as a computation time but never states which structure holds its OPEN list. That omission is the gap this project works in. |
| `drones-09-00376-v2.pdf` | Meng, W., Zhang, X., Zhou, L., Guo, H., & Hu, X. (2025). Advances in UAV Path Planning: A Comprehensive Review. *Drones, 9*(5), 376. | Survey used to place the project in context and to confirm that OPEN-list choice is rarely reported. |

To check the gap yourself, open the base paper and search the PDF for "heap",
"priority queue" and "sorted". None of them appears in its algorithm description.

The full source list, including books, course material and online references, is in
`../Documentation/evidence.md`.
