# Grid-Based Drone Path Planning

A C++ backend with a React + TypeScript frontend.

Aditya Upadhyay · 2501330100038 · B.Tech CSE-A, Semester III · NIET Greater Noida
Data Structures and Algorithms II (CCSE0301) · SDG 9

---

## What this is

A drone path planner for a grid of urban airspace. The subject of the project is not A\* itself:
**A\* is held fixed and six different OPEN-list structures are swapped behind one interface, so
that any difference in runtime belongs to the structure and not to the algorithm above it.**

The base paper — Gao, Li and Pang (2026), *Scientific Reports* 16:18275 — reports all of its
results as timings but never states which structure holds its OPEN list. That omission is the gap
this project fills.

## The stack

| Layer | Technology | What it does |
|---|---|---|
| Everything on the server | **C++17** | The grid, A\*, the six OPEN lists, knapsack, branch and bound, the HTTP server, JSON, static files |
| Frontend | **React 18 + TypeScript** (Vite) | Three steps, one canvas, one dialog explaining which algorithms ran |

There is no Node, no Express and no database in the running system. `bin/server` is one binary
that answers the API and serves the built frontend. Node is a **build-time** dependency only: it
compiles the TypeScript into static files, and then it is not needed again.

Because the engine is linked into the server, a request reaches A\* as a function call. Nothing is
serialised between the HTTP layer and the algorithm, and the timings the API reports are the
engine's own.

## Layout

```
engine/
  include/   grid, open_list, astar, mission, scenario, planner (the public façade)
  src/cli/   benchmark runner, test suite, mission demo, scaling sweep
  results.csv, evidence/   committed benchmark data and recorded output
server/
  include/   http.hpp (sockets, worker pool, static files), json.hpp (writer)
  src/       main.cpp — the four endpoints, nothing else
web/
  src/       React + TypeScript; types.ts is the wire contract
docs/        design book, wireframes, flowcharts, build plan
scripts/     api-smoke.mjs — end-to-end check of every endpoint
```

`engine/include/planner.hpp` is the engine's public API. The server and the command-line tools
both go through it, so there is one definition of "plan a route" and the web layer cannot drift
away from the benchmarked code.

## Running it

### One command, if you have Docker

```bash
docker build -t drone-planner .
docker run --rm -p 8080:8080 drone-planner      # http://localhost:8080
```

The runtime image contains the C++ binary and the static files — no JavaScript runtime at all.

### Locally

```bash
make                         # builds the server, the tests and the CLI tools
cd web && npm install && npm run build && cd ..
make serve                   # http://127.0.0.1:8080
```

### While developing the frontend

```bash
./bin/server                 # terminal 1 — API on :8080
cd web && npm run dev        # terminal 2 — Vite on :5173, proxying to :8080
```

### The engine on its own

```bash
make test        # ten C++ tests; needs nothing but g++
make bench       # writes engine/results.csv
./bin/mission    # one mission end to end: A* distances, selection, ordering
```

## API

| Endpoint | Purpose |
|---|---|
| `GET /api/scenarios` | the three maps, their cells and their start/end points |
| `GET /api/plan?scenarioId=river&startIndex=0&endIndex=2&structure=1&reps=5` | one route, with the search recorded |
| `GET /api/compare?scenarioId=river&startIndex=0&endIndex=2` | the same route with all six structures |
| `GET /health/live` · `GET /health/ready` | process is up · process can do useful work |

Every failure returns `{ "error": { "code", "message", "requestId" } }` and the same id appears in
the access log. `server --healthcheck` probes a running instance and exits 0 or 1, which is what
the container health check uses — there is no curl in the runtime image.

## The animation is not a re-simulation

`/api/plan` returns `order`: the sequence in which A\* actually closed its cells. The browser
replays that array. It does not run its own search and present the result as the engine's, which
would be a demonstration of the browser rather than of the project.

## Results

Measured in C++ at `-O2`, median of repeated runs on seeded maps at 20% obstacle density.

| OPEN list | 64×64 | 128×128 | 256×256 | 128 → 256 |
|---|---|---|---|---|
| unsorted array | 0.343 ms | 3.321 ms | not run | — |
| **bucket queue (2C+1)** | **0.058 ms** | **0.250 ms** | **0.866 ms** | 3.5× |
| binary heap | 0.070 ms | 0.354 ms | 1.424 ms | 4.0× |
| 4-ary heap | 0.075 ms | 0.363 ms | 1.352 ms | 3.7× |
| binomial heap | 0.152 ms | 0.762 ms | 2.284 ms | 3.0× |
| Fibonacci heap | 0.238 ms | 1.434 ms | 4.386 ms | 3.1× |

Fastest to slowest is a factor of thirteen, and all six return the same route cost.

Other measured figures: the insert-to-extract ratio is **1.20 : 1** (the project predicted 8:1 and
was wrong); the knapsack DP overtakes exhaustive subset search at **14 items**; branch and bound
removes **99.3%** of the search tree at 11 stops.

## The finding that matters

The Fibonacci heap has the best asymptotic insertion bound of the six and is **the slowest
measured**. Two reasons:

1. A Fibonacci heap exists to make decrease-key cheap, and this planner never calls decrease-key.
   A better `g` reinserts the cell and the stale copy is discarded when it surfaces. That lazy
   deletion is sound because the octile heuristic is consistent.
2. Both mergeable heaps allocate a pointer-linked node per entry, so the frontier stops being
   contiguous and the cache stops helping.

## Two things worth knowing before anyone asks

**On the 40×40 demo maps the bucket queue is not the fastest**, though it is on the 128×128
benchmark. Its window is 2C+1 = 2829 buckets whatever the map size, and a search of about a
thousand expansions does not amortise that setup. The benchmark maps are where the comparison
belongs; the demo maps exist to be recognised.

**The window is 2C+1, not C+1.** Sized at C+1 following Dijkstra's bound it returned cost 43,006
where the heap returned 41,592. For A\* with a consistent heuristic,
`f(v) = f(u) − h(u) + c + h(v)` and consistency gives `h(v) − h(u) ≤ c`, so `f(v) ≤ f(u) + 2c`.
Only the cross-structure test caught it.

## Tests

```bash
make test        # 10 C++ tests — no Node, no network
make test-api    # 49 end-to-end checks against a running server
cd web && npm run typecheck
```

- **C++** — optimality on a known grid, all six structures agreeing across 50 random maps,
  obstacle handling, heap invariants, every demo route reachable, no demo point inside an
  obstacle.
- **API** — known-good costs for all three maps, all 27 start/end pairs, `order.length ===
  expansions`, validation and status codes, static files, the SPA fallback, and four path
  traversal attempts that must not leak a file.

That last group is there because a static server that can be talked out of its own directory is
the classic way a demo leaks its own source.
