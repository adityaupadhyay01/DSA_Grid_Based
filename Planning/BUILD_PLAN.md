# Build Plan — Phase by Phase

**Project:** Grid-Based Drone Path Planning Simulation
**Repository:** `drone-path-planning`

## How to use this

Each numbered item is one commit. Write the code, check it compiles, commit, move on. The point is that every commit leaves the repository in a working state — you should be able to check out any commit and run `make`.

Do not write everything and then split it into fake commits afterwards, and do not backdate anything. A history where each commit is a real step you took is worth more than a tidy-looking fake one, and it is obvious which is which when someone reads the diffs.

**Commit message style:** `type: what changed`, present tense, under 60 characters.
Types used here: `feat`, `fix`, `test`, `docs`, `chore`, `perf`, `refactor`.

**Verify before every commit:**
```bash
make && make test
```

---

## Phase 0 — Repository setup (6 commits)

| # | Commit message | What to do |
|---|---|---|
| 1 | `chore: initialise repository` | `git init`, empty commit or a one-line README |
| 2 | `chore: add gitignore for build artefacts` | Ignore `planner`, `tests`, `*.o`, `results.csv`, `*.png`, `__pycache__/` |
| 3 | `docs: add project README skeleton` | Title, one-paragraph description, nothing else yet |
| 4 | `chore: add directory structure` | Create `include/`, `src/`, `scripts/`, `docs/` with `.gitkeep` |
| 5 | `chore: add Makefile with C++17 and -O2` | Targets: `all`, `clean`. No sources yet, so it will not build — that is fine |
| 6 | `docs: add research papers to docs folder` | The three PDFs, or a `papers.md` listing DOIs if you cannot redistribute |

---

## Phase 1 — Grid and cost model (9 commits)

| # | Commit message | What to do |
|---|---|---|
| 7 | `feat: add Grid class with width and height` | Just the constructor, `width()`, `height()`, `cells()` |
| 8 | `feat: add flat-array cell storage` | `std::vector<uint8_t> blocked_`, sized `w*h` |
| 9 | `feat: add O(1) index arithmetic` | `index(x,y)`, `x_of(id)`, `y_of(id)` |
| 10 | `feat: add bounds checking and blocked queries` | `in_bounds`, `blocked(id)`, `blocked(x,y)`, `set_blocked` |
| 11 | `feat: add integer-scaled movement costs` | `COST_STRAIGHT = 1000`, `COST_DIAGONAL = 1414`, with a comment on why scaling matters |
| 12 | `feat: add octile distance heuristic` | `heuristic(id, goal)` using the two constants |
| 13 | `feat: add seeded random obstacle generation` | `randomise(density, seed, start, goal)`, keeping start and goal free |
| 14 | `test: add grid unit tests` | Index round-trip, bounds, blocked set/get, heuristic on a known pair |
| 15 | `docs: explain why the map stays a flat array` | README section: Q1 runs ~10⁵ times per plan, O(1) beats O(log W) |

**Checkpoint.** `make test` passes. The data model exists with no search.

---

## Phase 2 — The OpenList interface (4 commits)

| # | Commit message | What to do |
|---|---|---|
| 16 | `feat: declare OpenList interface` | Pure virtual `insert`, `extract_min`, `empty`, `name` |
| 17 | `feat: add instrumentation counters to OpenList` | `inserts`, `extracts`, `peak_size` |
| 18 | `docs: explain why the interface comes first` | It is what holds the search fixed so the comparison is valid |
| 19 | `test: add a compile-only interface check` | A trivial subclass that confirms the contract compiles |

**Checkpoint.** Interface fixed before any structure exists. This ordering is the point.

---

## Phase 3 — Unsorted array baseline (5 commits)

| # | Commit message | What to do |
|---|---|---|
| 20 | `feat: add UnsortedArrayOpenList skeleton` | Class with the vector, `empty()`, `name()` |
| 21 | `feat: implement O(1) insert for unsorted array` | `push_back` plus counter updates |
| 22 | `feat: implement O(n) extract-min by linear scan` | The scan, swap-with-back, `pop_back` |
| 23 | `test: verify unsorted array returns keys in order` | Insert 12, 9, 14, 7, 11, 8 — expect 7, 8, 9, 11, 12, 14 |
| 24 | `docs: record the baseline and its O(n²) total` | Why the baseline exists at all |

---

## Phase 4 — A\* search (8 commits)

| # | Commit message | What to do |
|---|---|---|
| 25 | `feat: add SearchResult struct` | `found`, `path_cost`, `expansions`, `path`, counters |
| 26 | `feat: add 8-connected neighbour offsets` | The `DX`, `DY` arrays |
| 27 | `feat: add g-score and parent arrays` | Implicit search tree, one parent index per cell |
| 28 | `feat: implement the A* main loop` | Extract, closed check, goal check, neighbour expansion |
| 29 | `feat: add lazy deletion via the closed set` | `if (closed[u]) continue;` with a comment on why it is correct |
| 30 | `feat: add path reconstruction from parent links` | Walk goal to start, reverse |
| 31 | `test: verify optimal cost on an empty grid` | 10×10 corner to corner = 9 × 1414 = 12726 |
| 32 | `test: verify walls block and gaps route around` | Solid wall = no path; wall with a gap = path, no blocked cell on it |

**Checkpoint.** A working planner using the slowest possible structure. It finds optimal routes. That is the correctness baseline everything else is checked against.

---

## Phase 5 — Binary heap (7 commits)

| # | Commit message | What to do |
|---|---|---|
| 33 | `feat: add BinaryHeapOpenList skeleton` | Vector of entries, `empty()`, `name()` |
| 34 | `feat: add heap index arithmetic helpers` | `parent(i)`, `left(i)`, `right(i)` with a comment on no pointers |
| 35 | `feat: implement sift-up and heap insert` | Hole-based sift-up, one assignment per level |
| 36 | `feat: implement sift-down and extract-min` | Compare both children, swap with the smaller |
| 37 | `test: verify heap invariant under mixed operations` | Same 12, 9, 14, 7, 11, 8 sequence |
| 38 | `test: verify heap and array agree on 50 random maps` | **The cross-structure test.** Same cost, both structures |
| 39 | `docs: add the heap array layout diagram` | Tree and array side by side, index arithmetic annotated |

**Checkpoint.** Two structures, both correct, agreeing on every map.

---

## Phase 6 — Bucket queue, including the bug (7 commits)

Do this honestly. Write the C+1 version first, let the test fail, then fix it. The failing commit is the most valuable one in the repository.

| # | Commit message | What to do |
|---|---|---|
| 40 | `feat: add BucketQueueOpenList with C+1 buckets` | The circular buffer, scan pointer, count |
| 41 | `feat: implement O(1) bucket insert and extract` | Modulo indexing, scan advance |
| 42 | `test: extend agreement test to the bucket queue` | **This fails.** Commit the failing test |
| 43 | `fix: size bucket window at 2C+1 not C+1` | The fix, with the derivation in the comment |
| 44 | `docs: record the bucket window derivation` | `f_v = f_u − h_u + c + h_v`, and `h_v − h_u ≤ c` so `f_v ≤ f_u + 2c` |
| 45 | `test: report tie-breaking difference between structures` | Heap 62 vs bucket 32 expansions, same cost. Assert cost only |
| 46 | `docs: note LIFO tie-breaking as a separate factor` | Why it is a finding and not a defect |

**Checkpoint.** Three structures, all correct. A real bug found, diagnosed and fixed with the maths written down.

---

## Phase 7 — Benchmark harness (7 commits)

| # | Commit message | What to do |
|---|---|---|
| 47 | `feat: add benchmark runner skeleton` | `main.cpp`, argument parsing for density and trials |
| 48 | `feat: add steady_clock timing around the search` | `std::chrono::steady_clock`, milliseconds |
| 49 | `feat: discard the cold run and report medians` | Repetitions loop, drop run 0, median helper |
| 50 | `feat: emit results as CSV` | Header row and one line per configuration |
| 51 | `feat: add grid-size sweep across 64, 128, 256` | The outer loop |
| 52 | `perf: skip the O(n²) baseline above 128x128` | With a comment explaining why, not silently |
| 53 | `chore: add make bench target` | Writes `results.csv` |

---

## Phase 8 — Results and charts (6 commits)

| # | Commit message | What to do |
|---|---|---|
| 54 | `feat: add plotting script reading results.csv` | matplotlib, Agg backend |
| 55 | `feat: add runtime chart with error bars` | Log scale both axes, min/max as error bars |
| 56 | `feat: add expansion-count chart` | Grouped bars, deterministic metric |
| 57 | `docs: commit first measured results` | The CSV from a real run |
| 58 | `docs: add results tables to README` | Runtime, speedup and scaling tables |
| 59 | `docs: record build environment and flags` | g++ version, `-O2`, OS, CPU |

---

## Phase 9 — Documentation and corrections (6 commits)

| # | Commit message | What to do |
|---|---|---|
| 60 | `docs: add base paper and research gap section` | Gao et al. 2026, what the paper omits |
| 61 | `docs: explain why C++ and not Python` | Boxed ints, cache locality, interpreter overhead |
| 62 | `fix: correct insert-to-extract ratio claim` | 8:1 was reasoned, 1.20:1 is measured. Correct it, do not delete it |
| 63 | `docs: add findings that corrected the design` | The three findings from Phase 6 and 8 |
| 64 | `docs: add scope and limitations` | 2D, no kinematics, static maps, structures not planners |
| 65 | `docs: link evidence.md and the paper list` | Tie the repo to the report |

---

## Summary

| Phase | Commits | Leaves you with |
|---|---|---|
| 0 Setup | 6 | Repository, build system |
| 1 Grid | 9 | Data model, tested |
| 2 Interface | 4 | The contract, fixed before implementations |
| 3 Array | 5 | Working baseline |
| 4 A\* | 8 | A planner that finds optimal routes |
| 5 Heap | 7 | Two structures agreeing |
| 6 Bucket | 7 | Three structures, one bug found and fixed |
| 7 Harness | 7 | Reproducible measurement |
| 8 Results | 6 | Charts and numbers |
| 9 Docs | 6 | The written argument |
| **Total** | **65** | |

That is 65 commits, comfortably above the 40–50 you wanted, and each one is a real step rather than a slice of finished code.

---

## Two things that make the history worth reading

**Commit 42 should fail.** Write the `C+1` bucket queue, extend the agreement test, and commit it failing. Then fix it in 43. Anyone reading the log sees a hypothesis, a test that caught it, and a corrected derivation. That is a better story than code that worked first time, and it is what actually happened.

**Commit 62 corrects an earlier claim of yours.** The 8:1 insert ratio was reasoned from the branching factor and was wrong; the measured figure is 1.20:1. Correcting it in a later commit, rather than editing the earlier one, shows the work being revised by evidence.

## Spread it over time

Sixty-five commits in one evening looks exactly like what it is. Aim for a handful per sitting across the weeks between reviews. Phases 0 to 4 are one week's work at a reasonable pace, 5 to 6 another, 7 to 9 another.

## Useful commands

```bash
git add -p                  # stage selectively, keeps commits focused
git commit -m "feat: ..."   # one logical change per commit
git log --oneline --stat    # check the history reads sensibly
git diff HEAD~1             # confirm a commit does only what it claims
```
