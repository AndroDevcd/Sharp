//
// Sharp bytecode optimizer - control flow graph
// See control_flow.h.
//

#include "control_flow.h"
#include <algorithm>

/** index in ir.code a reference currently means, or -1 for "end of function" / none */
static int index_of_reference(function_ir &ir, int reference) {
    if(reference == IR_NONE) return -1;
    int id = resolve_reference(ir, reference);
    return id == IR_END ? -1 : index_of(ir, id);
}

static void add_unique(std::vector<int> &list, int value) {
    if(value >= 0 && std::find(list.begin(), list.end(), value) == list.end())
        list.push_back(value);
}

void build_control_flow(function_ir &ir, control_flow &cfg) {
    cfg.blocks.clear();
    cfg.blockOf.assign(ir.code.size(), -1);

    int count = (int)ir.code.size();
    if(count == 0) return;

    // ---- 1. leaders: instructions that start a block
    std::vector<bool> leader(count, false);
    std::vector<int> indirectTargets;   // where a brh may land
    leader[0] = true;

    auto mark = [&](int index) { if(index >= 0 && index < count) leader[index] = true; };

    for(int i = 0; i < count; i++) {
        ir_instruction &instr = ir.code[i];
        const instruction_info &info = instruction_info_of(instr.op);

        if(instr.target != IR_NONE) {
            int target = index_of_reference(ir, instr.target);
            mark(target);
            if(instr.pcConstant) add_unique(indirectTargets, target);
        }

        // whatever follows a jump, a conditional jump or a terminator starts a new block
        if(info.flags & (f_jump | f_cond_jump | f_indirect_jump | f_terminator))
            mark(i + 1);
    }

    for(ir_try_entry &entry : ir.tries) {
        // range boundaries are leaders so that a block is either inside or outside a range
        mark(index_of_reference(ir, entry.tryStart));
        mark(index_of_reference(ir, entry.tryEnd));
        mark(index_of_reference(ir, entry.blockStart));
        mark(index_of_reference(ir, entry.blockEnd));
        mark(index_of_reference(ir, entry.finallyStart));
        for(int handler : entry.handlers) mark(index_of_reference(ir, handler));
    }

    // ---- 2. blocks
    for(int i = 0; i < count; i++) {
        if(leader[i]) {
            basic_block block;
            block.first = i;
            block.last = i;
            block.reachable = false;
            block.isHandler = false;
            cfg.blocks.push_back(block);
        }

        cfg.blocks.back().last = i;
        cfg.blockOf[i] = (int)cfg.blocks.size() - 1;
    }

    auto block_at = [&](int index) { return index >= 0 && index < count ? cfg.blockOf[index] : -1; };

    // ---- 3. normal successors
    for(size_t b = 0; b < cfg.blocks.size(); b++) {
        basic_block &block = cfg.blocks[b];
        ir_instruction &last = ir.code[block.last];
        const instruction_info &info = instruction_info_of(last.op);
        bool fallsThrough = !(info.flags & f_terminator);

        if((info.flags & (f_jump | f_cond_jump)) && !(info.flags & f_indirect_jump))
            add_unique(block.successors, block_at(index_of_reference(ir, last.target)));

        if(info.flags & f_indirect_jump) {
            for(int target : indirectTargets) add_unique(block.successors, block_at(target));
        }

        if(fallsThrough)
            add_unique(block.successors, block_at(block.last + 1));
    }

    // ---- 4. exception successors
    for(ir_try_entry &entry : ir.tries) {
        // The runtime tests "start <= pc <= end" for both ranges (catches use the
        // block range, finally uses the try range), so take the union and treat
        // the end as inclusive. A reference to the end of the function means
        // "up to the last instruction".
        int starts[2] = { index_of_reference(ir, entry.blockStart), index_of_reference(ir, entry.tryStart) };
        int ends[2]   = { index_of_reference(ir, entry.blockEnd),   index_of_reference(ir, entry.tryEnd) };
        bool endAtFunctionEnd[2] = {
            entry.blockEnd != IR_NONE && resolve_reference(ir, entry.blockEnd) == IR_END,
            entry.tryEnd != IR_NONE && resolve_reference(ir, entry.tryEnd) == IR_END
        };

        std::vector<int> entries;
        for(int handler : entry.handlers) add_unique(entries, block_at(index_of_reference(ir, handler)));
        if(entry.hasFinally) add_unique(entries, block_at(index_of_reference(ir, entry.finallyStart)));

        for(int handlerBlock : entries) cfg.blocks[handlerBlock].isHandler = true;

        for(basic_block &block : cfg.blocks) {
            bool covered = false;
            for(int r = 0; r < 2 && !covered; r++) {
                if(starts[r] < 0) continue;
                int end = endAtFunctionEnd[r] ? count - 1 : ends[r];
                if(end < 0) continue;
                covered = block.first >= starts[r] && block.first <= end;
            }

            if(covered) {
                for(int handlerBlock : entries) add_unique(block.handlers, handlerBlock);
            }
        }
    }

    // ---- 5. predecessors and reachability
    for(size_t b = 0; b < cfg.blocks.size(); b++) {
        for(int next : cfg.blocks[b].successors) add_unique(cfg.blocks[next].predecessors, (int)b);
        for(int next : cfg.blocks[b].handlers) add_unique(cfg.blocks[next].predecessors, (int)b);
    }

    std::vector<int> work;
    work.push_back(0);
    cfg.blocks[0].reachable = true;

    while(!work.empty()) {
        int b = work.back();
        work.pop_back();

        auto visit = [&](int next) {
            if(!cfg.blocks[next].reachable) {
                cfg.blocks[next].reachable = true;
                work.push_back(next);
            }
        };

        for(int next : cfg.blocks[b].successors) visit(next);
        for(int next : cfg.blocks[b].handlers) visit(next);
    }
}
