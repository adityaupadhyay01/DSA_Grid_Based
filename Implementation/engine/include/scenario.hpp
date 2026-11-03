#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "grid.hpp"

// ---------------------------------------------------------------------------
// The three hand-built demo maps.
//
// The benchmark driver uses random maps, because a comparison between data
// structures needs many independent cases. A demonstration needs the opposite:
// a small number of fixed maps that a person can recognise and reason about.
// These are those maps, and they are fixed so that every run of the demo on
// every machine produces exactly the same route and the same counts.
//
// A cell carries what is in it rather than a plain blocked flag, so the front
// end can draw a building as a building and water as water. Anything that is
// not FREE is impassable; the planner itself only ever asks "blocked or not".
// ---------------------------------------------------------------------------
namespace scenario {

enum Kind : uint8_t { FREE = 0, BUILDING = 1, TREE = 2, WATER = 3, RACK = 4 };

inline const char* kind_name(uint8_t k) {
    switch (k) {
        case BUILDING: return "building";
        case TREE:     return "tree";
        case WATER:    return "water";
        case RACK:     return "warehouse rack";
        default:       return "free";
    }
}

struct Point {
    std::string name;
    int x, y;
};

struct Scenario {
    std::string id, name, blurb;
    int w = 40, h = 40;
    std::vector<uint8_t> kind;          // w*h, one Kind per cell
    std::vector<Point> starts, ends;

    int index(int x, int y) const { return y * w + x; }

    // The planner wants a plain occupancy grid; it has no interest in whether
    // a blocked cell is a wall or a tree.
    Grid to_grid() const {
        Grid g(w, h);
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
                if (kind[index(x, y)] != FREE) g.set_blocked(x, y, true);
        return g;
    }

    int open_cells() const {
        int n = 0;
        for (uint8_t k : kind) if (k == FREE) ++n;
        return n;
    }
    int count_of(uint8_t want) const {
        int n = 0;
        for (uint8_t k : kind) if (k == want) ++n;
        return n;
    }
};

namespace detail {

inline void fill(Scenario& s, int x0, int y0, int w, int h, uint8_t k) {
    for (int y = y0; y < y0 + h; ++y) {
        if (y < 0 || y >= s.h) continue;
        for (int x = x0; x < x0 + w; ++x) {
            if (x < 0 || x >= s.w) continue;
            s.kind[s.index(x, y)] = k;
        }
    }
}

inline Scenario city() {
    Scenario s;
    s.id = "city"; s.name = "City blocks"; s.blurb = "buildings, streets, a park";
    s.kind.assign(s.w * s.h, FREE);

    for (int by = 0; by < 5; ++by) {
        for (int bx = 0; bx < 5; ++bx) {
            const int x = 2 + bx * 8, y = 2 + by * 8;
            if ((bx * 3 + by) % 5 == 2) {
                // a park: trees in a loose pattern, so the drone can weave through
                for (int j = 0; j < 5; ++j)
                    for (int i = 0; i < 5; ++i)
                        if ((i + j) % 2 == 0) s.kind[s.index(x + i, y + j)] = TREE;
            } else {
                fill(s, x, y, 5, 5, BUILDING);
            }
        }
    }
    for (int y = 3; y < s.h; y += 4) s.kind[s.index(16, y)] = TREE;   // street trees

    s.starts = {{"Depot North", 0, 0}, {"Depot East", 39, 1}, {"Depot South", 1, 39}};
    s.ends   = {{"Hospital", 39, 39}, {"School", 24, 8}, {"Market", 8, 24}};
    return s;
}

inline Scenario warehouse() {
    Scenario s;
    s.id = "warehouse"; s.name = "Warehouse"; s.blurb = "racks with narrow gaps";
    s.kind.assign(s.w * s.h, FREE);

    for (int i = 0; i < 6; ++i) {
        const int x = 5 + i * 6;
        fill(s, x, 2, 2, 36, RACK);
        fill(s, x, (i % 2 == 0) ? 5 : 30, 2, 5, FREE);   // one opening per rack
    }
    fill(s, 20, 18, 2, 2, BUILDING);                      // support pillars
    fill(s, 32, 26, 2, 2, BUILDING);

    s.starts = {{"Dock A", 1, 20}, {"Dock B", 1, 3}, {"Dock C", 1, 37}};
    s.ends   = {{"Bay 1", 38, 20}, {"Bay 2", 38, 3}, {"Bay 3", 38, 37}};
    return s;
}

inline Scenario river() {
    Scenario s;
    s.id = "river"; s.name = "River crossing"; s.blurb = "water, forest, two bridges";
    s.kind.assign(s.w * s.h, FREE);

    for (int y = 0; y < s.h; ++y)
        for (int x = 0; x < s.w; ++x) {
            const int d = x + y - 39;
            if (d >= -1 && d <= 1) s.kind[s.index(x, y)] = WATER;
        }
    fill(s, 8, 27, 5, 6, FREE);     // the two bridges
    fill(s, 27, 8, 6, 5, FREE);

    const int forest[5][4] = {{14,4,5,5},{4,14,5,6},{30,28,6,5},{22,30,5,4},{33,18,4,5}};
    for (const auto& f : forest)
        for (int j = 0; j < f[3]; ++j)
            for (int i = 0; i < f[2]; ++i)
                if ((i * 3 + j * 5) % 4 != 0) s.kind[s.index(f[0] + i, f[1] + j)] = TREE;

    fill(s, 24, 22, 3, 3, BUILDING);
    fill(s,  9,  8, 3, 3, BUILDING);

    s.starts = {{"North Pad", 3, 3}, {"East Pad", 36, 1}, {"West Pad", 1, 30}};
    s.ends   = {{"South Site", 36, 36}, {"Hill Site", 20, 37}, {"Ridge Site", 37, 15}};
    return s;
}

}  // namespace detail

inline std::vector<Scenario> all() {
    return { detail::city(), detail::warehouse(), detail::river() };
}

}  // namespace scenario
