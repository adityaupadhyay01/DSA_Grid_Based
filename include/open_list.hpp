#pragma once
#include <vector>
#include <string>
#include <cstddef>
#include "grid.hpp"

// ---------------------------------------------------------------------------
// The interface is the deliverable that makes the comparison valid.
//
// A* calls exactly these three operations and knows nothing about what sits
// behind them. Swapping the implementation changes no line of the search, so
// any difference in runtime is attributable to the structure rather than to
// two different programs.
// ---------------------------------------------------------------------------
class OpenList {
public:
    virtual ~OpenList() = default;

    virtual void insert(int cell, int f) = 0;
    virtual int  extract_min()           = 0;   // returns the cell id
    virtual bool empty() const           = 0;
    virtual const char* name() const     = 0;

    // Instrumentation. These counts are deterministic: they reproduce exactly
    // on any machine, unlike wall-clock time.
    size_t inserts   = 0;
    size_t extracts  = 0;
    size_t peak_size = 0;
};

// ---------------------------------------------------------------------------
// 1. Unsorted array. The baseline.
//    insert O(1), extract-min O(n)  =>  whole search O(n^2)
// ---------------------------------------------------------------------------
class UnsortedArrayOpenList : public OpenList {
public:
    void insert(int cell, int f) override {
        items_.push_back({cell, f});
        ++inserts;
        if (items_.size() > peak_size) peak_size = items_.size();
    }

    int extract_min() override {
        size_t best = 0;
        for (size_t i = 1; i < items_.size(); ++i)      // the linear scan
            if (items_[i].f < items_[best].f) best = i;
        int cell = items_[best].cell;
        items_[best] = items_.back();
        items_.pop_back();
        ++extracts;
        return cell;
    }

    bool empty() const override { return items_.empty(); }
    const char* name() const override { return "unsorted_array"; }

private:
    struct Entry { int cell; int f; };
    std::vector<Entry> items_;
};

// ---------------------------------------------------------------------------
// 2. Binary min heap. The main structure.
//    insert O(log n), extract-min O(log n)  =>  whole search O(n log n)
//
//    Stored as a flat array with no pointers at all:
//      parent(i) = (i-1)/2   left(i) = 2i+1   right(i) = 2i+2
//    The top levels of the tree fit inside a single cache line, which is the
//    real reason it beats a pointer-linked structure with the same asymptotics.
// ---------------------------------------------------------------------------
class BinaryHeapOpenList : public OpenList {
public:
    void insert(int cell, int f) override {
        heap_.push_back({cell, f});
        sift_up(heap_.size() - 1);
        ++inserts;
        if (heap_.size() > peak_size) peak_size = heap_.size();
    }

    int extract_min() override {
        int cell = heap_[0].cell;
        heap_[0] = heap_.back();
        heap_.pop_back();
        if (!heap_.empty()) sift_down(0);
        ++extracts;
        return cell;
    }

    bool empty() const override { return heap_.empty(); }
    const char* name() const override { return "binary_heap"; }

private:
    struct Entry { int cell; int f; };
    std::vector<Entry> heap_;

    void sift_up(size_t i) {
        Entry e = heap_[i];
        while (i > 0) {
            size_t p = (i - 1) / 2;
            if (heap_[p].f <= e.f) break;
            heap_[i] = heap_[p];
            i = p;
        }
        heap_[i] = e;
    }

    void sift_down(size_t i) {
        Entry e = heap_[i];
        size_t n = heap_.size();
        while (true) {
            size_t l = 2 * i + 1;
            if (l >= n) break;
            size_t smallest = l;
            size_t r = l + 1;
            if (r < n && heap_[r].f < heap_[l].f) smallest = r;
            if (heap_[smallest].f >= e.f) break;
            heap_[i] = heap_[smallest];
            i = smallest;
        }
        heap_[i] = e;
    }
};

// ---------------------------------------------------------------------------
// 3. Bucket (Dial) queue.
//    insert O(1), extract-min O(1) amortised  =>  whole search O(n)
//
//    Valid here because of a property of grids that general graph algorithms
//    cannot assume: edge weights take exactly two values, 1000 and 1414. All
//    f-values therefore lie on a bounded integer lattice, and under a
//    consistent heuristic A* expands in non-decreasing f order, so every live
//    key sits within a window of width C above the current minimum.
//    Those are exactly Dial's preconditions.
// ---------------------------------------------------------------------------
class BucketQueueOpenList : public OpenList {
public:
    // Window width is 2C+1, not C+1.
    //
    // For Dijkstra a successor key exceeds the current minimum by at most one
    // edge cost. For A* with a consistent heuristic the bound is twice that:
    //     f_v = g_u + c + h_v = f_u - h_u + c + h_v
    // and consistency gives h_v - h_u <= c, so f_v <= f_u + 2c.
    // Sizing the buffer at C+1 makes distant keys alias onto occupied buckets
    // and the queue then returns cells out of order, producing suboptimal
    // paths. Caught by the cross-structure agreement test.
    static constexpr int SPAN = 2 * COST_MAX + 1;

    BucketQueueOpenList() : buckets_(SPAN), scan_(-1), count_(0) {}

    void insert(int cell, int f) override {
        int b = f % SPAN;
        buckets_[b].push_back(cell);
        if (scan_ < 0) scan_ = b;          // first insert sets the scan origin
        ++count_;
        ++inserts;
        if (static_cast<size_t>(count_) > peak_size) peak_size = count_;
    }

    int extract_min() override {
        while (buckets_[scan_].empty())
            scan_ = (scan_ + 1) % SPAN;
        int cell = buckets_[scan_].back();
        buckets_[scan_].pop_back();
        --count_;
        ++extracts;
        return cell;
    }

    bool empty() const override { return count_ == 0; }
    const char* name() const override { return "bucket_queue"; }

private:
    std::vector<std::vector<int>> buckets_;
    int scan_;
    int count_;
};
