# Grid-Based Drone Path Planning — context for Claude Code

Read this before changing anything.

## Who and what

Aditya Upadhyay, roll 2501330100038, B.Tech CSE-A, Semester III, NIET Greater Noida.
Data Structures and Algorithms II (CCSE0301), faculty Mr. Shamshad Ali. PBL project, SDG 9.

A C++17 grid path planner with a React + TypeScript frontend. The backend is entirely C++:
engine, HTTP server, JSON and static file serving. Node is a build-time dependency only — it
compiles the frontend and is not present in the running system. The subject is **not** A\*: A\* is held
fixed and six OPEN-list structures are swapped behind one interface, so any runtime difference
belongs to the structure. Base paper: Gao, Li and Pang (2026), *Scientific Reports* 16:18275, which
reports timings without ever naming its OPEN-list structure. That is the gap.

## The one rule

**All algorithm work lives in `engine/`. Nothing else plans anything.**

`server/` does HTTP. The browser draws. If a change would put a search, a heap or a heuristic
anywhere but `engine/include`, it is in the wrong place — the C++ test suite cannot reach it
there, and the benchmark numbers stop describing the code that actually runs.

## Build and run

```bash
make                          # server, tests and CLI tools; needs only g++
make test                     # ten C++ tests, no Node, no network
cd web && npm install && npm run build && cd ..
make serve                    # http://127.0.0.1:8080 — API and frontend together
make test-api                 # 49 end-to-end checks
docker build -t drone-planner . && docker run --rm -p 8080:8080 drone-planner
```

Frontend development: `./bin/server` in one terminal, `cd web && npm run dev` in another. Vite
proxies /api and /health to :8080.

## Layout

```
engine/include/
  grid.hpp        occupancy grid, integer costs, octile heuristic
  open_list.hpp   the OpenList interface and all six implementations
  astar.hpp       the search; takes an optional SearchTrace*
  mission.hpp     knapsack selector and both tour solvers
  scenario.hpp    the three hand-built demo maps
  planner.hpp     the public façade: scenarios(), plan(), compare()
server/include/
  http.hpp        sockets, worker pool, request parsing, static files, shutdown
  json.hpp        a writer; the server never parses JSON, input is the query string
server/src/main.cpp   the four endpoints and nothing else
web/src/          React + TypeScript; types.ts is the wire contract
```

## Things that will bite you

**`astar()` takes an optional `SearchTrace*`.** It defaults to `nullptr` and the benchmark path
never passes one. Timing is always taken from an untraced run — recording the search would measure
the recording. Do not make the trace mandatory.

**The bucket queue window is 2C+1, not C+1.** Sized at C+1 following Dijkstra's bound it returned
cost 43,006 where the heap returned 41,592. With a consistent heuristic,
`f(v) = f(u) − h(u) + c + h(v)` and `h(v) − h(u) ≤ c`, so `f(v) ≤ f(u) + 2c`. Do not "simplify"
this back.

**Only a cross-structure test catches that class of bug.** The C++ suite plans the same maps with
every structure and requires identical cost. A single-structure test has nothing to disagree with.
Keep it passing; it is the load-bearing one.

**Expansion counts differ between structures and that is fine.** The bucket queue pops LIFO within
a bucket, so it breaks ties among equal-f cells differently. Only the *cost* must match.

**Nothing in Module 4 is dead code.** The backtracking TSP solver looks redundant beside branch and
bound. It is the control that proves the bound did not change the answer. It was deleted once and
had to be rewritten from scratch.

**The static file server has a traversal guard and it is load-bearing.** `resolve_static()`
resolves the path with `realpath` and refuses anything landing outside the web root. Rejecting
`".."` as text is not enough once percent-encoding exists. Four attack paths are in `make
test-api`; do not delete those checks.

**`web/src/types.ts` is the only description of the wire format the browser has.** If a field name
changes in `server/src/main.cpp`, change it there too. Nothing checks this at compile time — the
API smoke test is what catches it.

**`SIGPIPE` is ignored in `listen_and_serve()`.** Without that, a client disconnecting mid-response
kills the process.

**`engine/results.csv` is committed benchmark evidence, not a build artifact.** `make clean` must not
delete it. It did once.

**On the 40×40 demo maps the bucket queue is not fastest**, unlike on the 128×128 benchmark: its
2C+1 = 2829-bucket window is a fixed setup cost a thousand-expansion search does not amortise. This
is expected, and the README says so.

## The animation must stay honest

`/api/plan` returns `order`, the sequence in which A\* actually closed cells. The browser replays
that array. It must never run its own search and present the result as the engine's.

## Canonical measured numbers

C++ at `-O2`, median of repeated runs, 20% density, 128×128: unsorted array 3.321 ms, bucket queue
0.250, binary heap 0.354, 4-ary 0.363, binomial 0.762, Fibonacci 1.434. Insert:extract 1.20 : 1.
Knapsack DP overtakes exhaustive search at 14 items. Branch and bound cuts 99.3% of the tree at 11
stops.

Re-running `make bench` on different hardware moves the absolute milliseconds; the ranking is
stable and is what the project claims. Do not overwrite report figures with a fresh run from
another machine — if they are updated, every figure must come from one run.

## How this project reports things

The reports deliberately include the cases where a method *loses*: the DP crossover below 14 items,
the bound costing more below 7 stops, the 8:1 prediction that measured 1.20:1. Two wrong
predictions are corrected in place rather than erased. Add results the same way — state where it
fails, not only where it wins, and never quote a number `results.csv` does not support.
