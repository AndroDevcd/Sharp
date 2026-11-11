//
// Sharp bytecode optimizer - post-dominators and control dependence
// See post_dominators.h.
//
// Post-dominators are computed with the standard iterative data flow
// formulation, run backwards over the control flow graph:
//
//     postdom(exit block) = { itself }
//     postdom(X)          = { X }  +  intersection of postdom(S) for every successor S of X
//
// repeated until nothing changes. Functions are small, so plain bit sets are fine.
//

#include "post_dominators.h"
#include <algorithm>

typedef std::vector<uint64_t> block_set;

static bool has(const block_set &set, int block) { return (set[block >> 6] >> (block & 63)) & 1u; }
static void add(block_set &set, int block) { set[block >> 6] |= (uint64_t)1 << (block & 63); }

void compute_post_dominators(function_ir &ir, control_flow &cfg, post_dominators &pdom) {
    int count = (int)cfg.blocks.size();
    pdom.valid = false;
    pdom.immediate.assign(count, PDOM_EXIT);
    pdom.dependsOn.assign(count, std::vector<int>());
    if(count == 0) return;

    // ---- only plain control flow is handled
    for(basic_block &block : cfg.blocks) {
        if(!block.reachable) return;              // run the unreachable code pass first
        if(!block.handlers.empty() || block.isHandler) return;
        if(instruction_info_of(ir.code[block.last].op).flags & f_indirect_jump) return;
    }

    // ---- blocks that leave the function, and "can every block get there?"
    std::vector<bool> exits(count, false), reachesExit(count, false);
    std::vector<int> work;
    for(int b = 0; b < count; b++) {
        if(cfg.blocks[b].successors.empty()) {
            exits[b] = true;
            reachesExit[b] = true;
            work.push_back(b);
        }
    }

    while(!work.empty()) {
        int b = work.back();
        work.pop_back();
        for(int pred : cfg.blocks[b].predecessors) {
            if(!reachesExit[pred]) { reachesExit[pred] = true; work.push_back(pred); }
        }
    }

    for(int b = 0; b < count; b++) {
        if(!reachesExit[b]) return; // an endless loop: there is no "after" to reason about
    }

    // ---- post-dominator sets
    size_t words = (count + 63) / 64;
    block_set everything(words, ~(uint64_t)0);
    std::vector<block_set> sets(count, everything);

    for(int b = 0; b < count; b++) {
        if(exits[b]) {
            sets[b].assign(words, 0);
            add(sets[b], b);
        }
    }

    bool changed = true;
    while(changed) {
        changed = false;

        for(int b = count - 1; b >= 0; b--) {
            if(exits[b]) continue;

            block_set merged = everything;
            for(int next : cfg.blocks[b].successors) {
                for(size_t w = 0; w < words; w++) merged[w] &= sets[next][w];
            }
            add(merged, b);

            if(merged != sets[b]) {
                sets[b] = merged;
                changed = true;
            }
        }
    }

    // ---- immediate post-dominator: the strict post-dominator that every other
    // strict post-dominator of the block also post-dominates (the closest one)
    for(int b = 0; b < count; b++) {
        int best = PDOM_EXIT;
        size_t bestSize = 0;

        for(int candidate = 0; candidate < count; candidate++) {
            if(candidate == b || !has(sets[b], candidate)) continue;

            // the closest one is the one with the LARGEST post-dominator set of its own
            size_t size = 0;
            for(int other = 0; other < count; other++) size += has(sets[candidate], other) ? 1 : 0;

            if(best == PDOM_EXIT || size > bestSize) {
                best = candidate;
                bestSize = size;
            }
        }

        pdom.immediate[b] = best;
    }

    // ---- control dependence: for a branching block X and each successor S,
    // every block from S up the post-dominator tree, stopping before X's own
    // immediate post-dominator, runs or not depending on X's branch.
    for(int x = 0; x < count; x++) {
        if(cfg.blocks[x].successors.size() < 2) continue;

        for(int successor : cfg.blocks[x].successors) {
            int runner = successor;
            int guard = 0;

            while(runner != PDOM_EXIT && runner != pdom.immediate[x] && guard++ <= count) {
                std::vector<int> &list = pdom.dependsOn[runner];
                if(std::find(list.begin(), list.end(), x) == list.end()) list.push_back(x);
                runner = pdom.immediate[runner];
            }
        }
    }

    pdom.valid = true;
}
