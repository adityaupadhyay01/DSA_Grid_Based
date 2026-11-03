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

// ---------------------------------------------------------------------------
// MODULE 5 — the heaps the course covers after the binary heap.
//
// Unit 1 stops at the binary heap. Unit 5 asks what the mergeable heaps buy,
// and the honest answer for this workload is "less than the bound suggests",
// which is only visible by measuring rather than by reading the table of
// asymptotic costs.
//
// All three below satisfy the same OpenList contract, so A* cannot tell them
// apart and the comparison stays valid.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// 4. d-ary (4-ary) heap.
//
// Same algorithm as the binary heap with a wider fan-out. The tree is shallower
// by a factor of log2(4) = 2, so sift-up walks half as far; sift-down pays for
// it by comparing four children instead of two. A* inserts more often than it
// extracts, so a structure that makes insertion cheaper at the cost of
// extraction should win. The measured insert-to-extract ratio is 1.20:1, so
// the effect is real but small.
// ---------------------------------------------------------------------------
class FourAryHeapOpenList : public OpenList {
public:
    void insert(int cell, int f) override {
        a_.push_back({f, cell});
        ++inserts;
        if (a_.size() > peak_size) peak_size = a_.size();
        size_t i = a_.size() - 1;
        while (i > 0) {                                   // sift-up
            size_t parent = (i - 1) / 4;
            if (a_[parent].f <= a_[i].f) break;
            std::swap(a_[parent], a_[i]);
            i = parent;
        }
    }

    int extract_min() override {
        const int cell = a_[0].cell;
        a_[0] = a_.back();
        a_.pop_back();
        ++extracts;
        const size_t n = a_.size();
        size_t i = 0;
        while (true) {                                    // sift-down
            size_t first = 4 * i + 1;
            if (first >= n) break;
            size_t best = first;
            const size_t last = first + 4 < n ? first + 4 : n;
            for (size_t c = first + 1; c < last; ++c)
                if (a_[c].f < a_[best].f) best = c;
            if (a_[i].f <= a_[best].f) break;
            std::swap(a_[i], a_[best]);
            i = best;
        }
        return cell;
    }

    bool empty() const override { return a_.empty(); }
    const char* name() const override { return "four_ary_heap"; }

private:
    struct Node { int f; int cell; };
    std::vector<Node> a_;
};

// ---------------------------------------------------------------------------
// 5. Binomial heap.
//
// A forest of binomial trees, at most one of each order, so the root list holds
// O(log n) trees. Insert is a merge of a single-node heap, which is O(1)
// amortised; extract-min finds the smallest root in O(log n) and then merges
// that root's children back in.
//
// The asymptotic promise is better than the binary heap on insertion. The cost
// is that every node is a separate allocation with three pointers, so the
// frontier stops being contiguous and the cache stops helping. That trade is
// exactly what this module measures.
// ---------------------------------------------------------------------------
class BinomialHeapOpenList : public OpenList {
public:
    ~BinomialHeapOpenList() override { for (Node* n : pool_) delete n; }

    void insert(int cell, int f) override {
        Node* n = new Node{f, cell, 0, nullptr, nullptr, nullptr};
        pool_.push_back(n);
        head_ = merge(head_, n);
        ++inserts; ++count_;
        if (count_ > peak_size) peak_size = count_;
    }

    int extract_min() override {
        // find the minimum root
        Node* best = head_; Node* best_prev = nullptr;
        Node* prev = nullptr; Node* cur = head_;
        while (cur) {
            if (cur->f < best->f) { best = cur; best_prev = prev; }
            prev = cur; cur = cur->sibling;
        }
        // unlink it
        if (best_prev) best_prev->sibling = best->sibling;
        else           head_ = best->sibling;

        // its children, reversed, form a valid binomial heap
        Node* child = best->child; Node* rev = nullptr;
        while (child) {
            Node* next = child->sibling;
            child->sibling = rev; child->parent = nullptr;
            rev = child; child = next;
        }
        head_ = merge(head_, rev);

        const int cell = best->cell;
        best->dead = true;                 // freed in the destructor
        ++extracts; --count_;
        return cell;
    }

    bool empty() const override { return count_ == 0; }
    const char* name() const override { return "binomial_heap"; }

private:
    struct Node {
        int f; int cell; int degree;
        Node* parent; Node* child; Node* sibling;
        bool dead = false;
    };

    static Node* link(Node* a, Node* b) {      // b becomes a child of a
        b->parent = a;
        b->sibling = a->child;
        a->child = b;
        a->degree++;
        return a;
    }

    // Merge two root lists, combining trees of equal order, exactly as binary
    // addition carries.
    static Node* merge(Node* x, Node* y) {
        if (!x) return y;
        if (!y) return x;

        Node* head = nullptr; Node** tail = &head;
        while (x && y) {
            if (x->degree <= y->degree) { *tail = x; x = x->sibling; }
            else                        { *tail = y; y = y->sibling; }
            tail = &(*tail)->sibling;
        }
        *tail = x ? x : y;

        Node* prev = nullptr; Node* cur = head; Node* next = cur->sibling;
        while (next) {
            if (cur->degree != next->degree ||
                (next->sibling && next->sibling->degree == cur->degree)) {
                prev = cur; cur = next;
            } else if (cur->f <= next->f) {
                cur->sibling = next->sibling;
                link(cur, next);
            } else {
                if (prev) prev->sibling = next; else head = next;
                link(next, cur);
                cur = next;
            }
            next = cur->sibling;
        }
        return head;
    }

    Node* head_ = nullptr;
    size_t count_ = 0;
    std::vector<Node*> pool_;      // every node, so nothing leaks
};

// ---------------------------------------------------------------------------
// 6. Fibonacci heap.
//
// Insert is genuinely O(1): the new node is dropped into the root list and
// nothing is tidied. All the work is deferred to extract-min, which
// consolidates the root list so that no two trees share a degree.
//
// Its famous advantage is O(1) amortised decrease-key. This planner has no
// decrease-key: a better g re-inserts the cell and the stale copy is skipped
// when it surfaces, which is sound because the heuristic is consistent. So the
// one operation the Fibonacci heap is built for is never called, and what is
// left is a structure with larger constants than the binary heap.
// ---------------------------------------------------------------------------
class FibonacciHeapOpenList : public OpenList {
public:
    ~FibonacciHeapOpenList() override { for (Node* n : pool_) delete n; }

    void insert(int cell, int f) override {
        Node* n = new Node{f, cell, 0, nullptr, nullptr, nullptr, nullptr};
        pool_.push_back(n);
        n->left = n; n->right = n;
        splice(n);
        if (!min_ || f < min_->f) min_ = n;
        ++inserts; ++count_;
        if (count_ > peak_size) peak_size = count_;
    }

    int extract_min() override {
        Node* z = min_;
        // promote the children of the minimum to the root list
        if (z->child) {
            Node* c = z->child;
            do {
                Node* next = c->right;
                c->parent = nullptr;
                c->left = c; c->right = c;
                splice(c);
                c = next;
            } while (c != z->child);
        }
        // unlink z
        z->left->right = z->right;
        z->right->left = z->left;
        if (z == z->right) { min_ = nullptr; root_ = nullptr; }
        else { if (root_ == z) root_ = z->right; min_ = z->right; consolidate(); }

        const int cell = z->cell;
        ++extracts; --count_;
        return cell;
    }

    bool empty() const override { return count_ == 0; }
    const char* name() const override { return "fibonacci_heap"; }

private:
    struct Node {
        int f; int cell; int degree;
        Node* parent; Node* child; Node* left; Node* right;
    };

    void splice(Node* n) {                 // insert n into the root list
        if (!root_) { root_ = n; n->left = n; n->right = n; return; }
        n->right = root_->right;
        n->left = root_;
        root_->right->left = n;
        root_->right = n;
    }

    // The deferred work: combine roots until every degree is unique, then find
    // the new minimum. This is where a Fibonacci heap pays for its cheap insert.
    void consolidate() {
        if (!root_) { min_ = nullptr; return; }
        std::vector<Node*> by_degree(64, nullptr);

        std::vector<Node*> roots;
        Node* cur = root_;
        do { roots.push_back(cur); cur = cur->right; } while (cur != root_);

        for (Node* w : roots) {
            Node* x = w;
            int d = x->degree;
            while (by_degree[d]) {
                Node* y = by_degree[d];
                if (y == x) break;
                if (y->f < x->f) std::swap(x, y);
                // y becomes a child of x
                y->left->right = y->right;
                y->right->left = y->left;
                y->parent = x;
                if (!x->child) { x->child = y; y->left = y; y->right = y; }
                else {
                    y->right = x->child->right;
                    y->left = x->child;
                    x->child->right->left = y;
                    x->child->right = y;
                }
                x->degree++;
                by_degree[d] = nullptr;
                ++d;
            }
            by_degree[d] = x;
        }

        root_ = nullptr; min_ = nullptr;
        for (Node* n : by_degree) {
            if (!n) continue;
            n->left = n; n->right = n; n->parent = nullptr;
            splice(n);
            if (!min_ || n->f < min_->f) min_ = n;
        }
    }

    Node* root_ = nullptr;
    Node* min_  = nullptr;
    size_t count_ = 0;
    std::vector<Node*> pool_;
};
