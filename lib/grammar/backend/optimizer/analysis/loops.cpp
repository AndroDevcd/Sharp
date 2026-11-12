//
// Sharp bytecode optimizer - finding the loops of a function
// See loops.h.
//
// Dominators use the standard iterative data flow formulation:
//
//     dom(entry) = { entry }
//     dom(B)     = { B }  +  intersection of dom(P) for every predecessor P of B
//
// repeated until nothing changes. Functions are small, so plain bit sets are fine.
//

#include "loops.h"
#include <algorithm>

typedef std::vector<uint64_t> block_set;

static bool has(const block_set &set, int block) { return (set[block >> 6] >> (block & 63)) & 1u; }
static void add(block_set &set, int block) { set[block >> 6] |= (uint64_t)1 << (block & 63); }

void find_loops(function_ir &ir, control_flow &cfg, loop_forest &forest) {
    forest.valid = false;
    forest.loops.clear();

    int count = (int)cfg.blocks.size();
    if(count == 0) return;

    // ---- only plain control flow is handled
    for(basic_block &block : cfg.blocks) {
        if(!block.reachable) return;
        if(!block.handlers.empty() || block.isHandler) return;
        if(instruction_info_of(ir.code[block.last].op).flags & f_indirect_jump) return;
    }

    // ---- dominator sets
    size_t words = (count + 63) / 64;
    block_set everything(words, ~(uint64_t)0);
    std::vector<block_set> dom(count, everything);
    dom[0].assign(words, 0);
    add(dom[0], 0);

    bool changed = true;
    while(changed) {
        changed = false;

        for(int b = 1; b < count; b++) {
            block_set merged = everything;
            for(int pred : cfg.blocks[b].predecessors) {
                for(size_t w = 0; w < words; w++) merged[w] &= dom[pred][w];
            }
            add(merged, b);

            if(merged != dom[b]) {
                dom[b] = merged;
                changed = true;
            }
        }
    }

    // ---- back edges, grouped by header (two `continue` paths make two back
    // edges to the same header: that is still one loop)
    std::vector<std::vector<int>> latchesOf(count);
    for(int b = 0; b < count; b++) {
        for(int next : cfg.blocks[b].successors) {
            if(has(dom[b], next)) latchesOf[next].push_back(b);
        }
    }

    for(int header = 0; header < count; header++) {
        if(latchesOf[header].empty()) continue;

        loop_info loop;
        loop.header = header;
        loop.latches = latchesOf[header];
        loop.contains.assign(count, 0);
        loop.contains[header] = 1;

        // the body: everything that reaches a latch without passing the header
        std::vector<int> work;
        for(int latch : loop.latches) {
            if(!loop.contains[latch]) { loop.contains[latch] = 1; work.push_back(latch); }
        }
        while(!work.empty()) {
            int b = work.back();
            work.pop_back();
            for(int pred : cfg.blocks[b].predecessors) {
                if(!loop.contains[pred]) { loop.contains[pred] = 1; work.push_back(pred); }
            }
        }

        for(int b = 0; b < count; b++) {
            if(!loop.contains[b]) continue;
            loop.blocks.push_back(b);

            for(int next : cfg.blocks[b].successors) {
                if(!loop.contains[next]
                   && std::find(loop.exits.begin(), loop.exits.end(), next) == loop.exits.end())
                    loop.exits.push_back(next);
            }
        }
        for(int pred : cfg.blocks[header].predecessors) {
            if(!loop.contains[pred]) loop.entries.push_back(pred);
        }

        forest.loops.push_back(loop);
    }

    // an inner loop has fewer blocks than the loop around it
    std::stable_sort(forest.loops.begin(), forest.loops.end(),
                     [](const loop_info &a, const loop_info &b) { return a.blocks.size() < b.blocks.size(); });

    forest.valid = true;
}
