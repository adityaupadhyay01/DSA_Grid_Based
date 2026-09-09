# Grid-Based Drone Path Planning Simulation

DSA-II individual project. B.Tech CSE, 3rd semester, NIET Greater Noida.

The idea is simple to say and annoying to actually do. Take a map, cut it into square cells, block some of them off, then make a drone find its way from one cell to another without flying into anything.

## The problem

You could just try every route and keep the shortest one. That works fine on paper. It falls apart the moment the map gets big.

A 100x100 grid is ten thousand cells. Nobody is checking every path through that. And a real drone is worse off than a laptop is, because it has a battery draining and a small chip doing the thinking, so the answer needs to show up fast.

So the map has to be stored in a way that doesn't waste space on empty air, and the drone's possible moves have to be written down in a way that answers "where can I go from here" without scanning everything. First part is a tree problem. Second part is a graph problem. That's the whole reason this topic works for DSA-II.

## Why I chose it

Half the examples in this subject are about sorting arrays. Nothing wrong with that, but you never really feel why the data structure mattered.

Here you do. Store the frontier in the wrong thing and the search takes forever. Store the map badly and you run out of memory on a grid that isn't even that large. The structure isn't decoration in this problem, it decides whether the thing runs at all.

## Where this is right now

Nothing is coded yet. Review 1 was reading and understanding, not building.

Month 1 went into working out what the problem actually is and getting through Unit 1 (Trees) and Unit 2 (Graphs) from the course material. Code starts in Review 2. I made this repo now so everything after that has somewhere to go instead of sitting in a folder on my laptop.

Roughly:

- **Review 1** - problem understanding, Units 1 and 2. Done.
- **Review 2** - grid storage, the graph model, first search that actually finds a path. Next up.
- **Review 3** - dynamic programming, backtracking, branch and bound.
- **Final** - full simulation, comparisons, report.

## The plan

Nothing here is locked in. This is what I think it looks like:

Build the grid and mark the blocked cells. Store the map in a tree so a big empty region sits in one node instead of a few thousand. Treat every cell as a vertex and every legal move as an edge, straight moves costing 1 and diagonal ones costing around 1.414 since a diagonal covers more ground. Run a search that keeps grabbing the cheapest unexplored cell, with a heap deciding which one that is. Once the goal is reached, walk the parent links backwards to get the actual route. Then draw it, because a path you can't see is hard to check.

## What goes where

| Structure | Doing what |
|---|---|
| Quadtree / binary tree | Splitting the grid into open and blocked regions |
| AVL tree | Keeping obstacle and visited-cell lookups fast as the grid grows |
| Min heap | Pulling out the cell with the lowest cost |
| Priority queue | Holding cells waiting to be explored, cheapest first |
| Tree traversal | Going through stored regions and rebuilding the path |
| Graph | Cells as vertices, moves as weighted edges |
| Adjacency list / matrix | Which neighbours are reachable from a given cell |

## Setup

Python. Probably NumPy for the grid and Matplotlib or Pygame for drawing it. Not fixed, since there's no code to be wrong about yet.

```
drone-path-planning/
├── src/
├── maps/
├── docs/
├── results/
└── README.md
```

## Running it

Nothing to run yet. This section gets filled in once there is.

## Me

Aditya Upadhyay
Roll no. 2501330100038, CSE-A, 3rd sem, NIET Greater Noida
Faculty: Mr. Shamshad Ali
SDG 9 - Industry, Innovation and Infrastructure

Updated at the end of each review.
