//
// Sharp bytecode optimizer - liveness analysis
// See liveness.h.
//
// Classic backward data flow: start from "nothing is needed" at the exits and
// walk the instructions backwards. An instruction that writes a value ends its
// life (above it the old value is not needed), an instruction that reads a
// value starts it. Repeat over the control flow graph until nothing changes.
//

#include "liveness.h"

void register_effects(const ir_instruction &instr, register_set &reads, register_set &writes) {
    const instruction_info &info = instruction_info_of(instr.op);
    reads = info.implicitReads;
    writes = info.implicitWrites;

    const operand_kind kinds[3] = { info.a, info.b, info.c };
    const int32_t values[3] = { instr.a, instr.b, instr.c };

    for(int i = 0; i < 3; i++) {
        if(!is_register_kind(kinds[i]) || values[i] < 0 || values[i] > 9) continue;
        if(reads_register(kinds[i])) reads |= REG_BIT(values[i]);
        if(writes_register(kinds[i])) writes |= REG_BIT(values[i]);
    }

    if(info.flags & f_opaque) {
        reads = ALL_REGISTERS;   // an interrupt may look at anything...
        writes = 0;              // ...and we cannot count on it overwriting anything
    }

    // NOTE: a call is deliberately NOT treated as overwriting the registers.
    // The callee may overwrite any of them, but generated code sometimes keeps
    // a value in a register across a call to a function that happens not to
    // touch it (e.g. `loadval cx; call vm.thread_args; movr egx, cx`). So a
    // register that is read after a call must be considered needed before it.
}

/** local slots (number part) an instruction reads / writes */
static void local_effects(const ir_instruction &instr, int &read, int &written, bool &readsAll) {
    const instruction_info &info = instruction_info_of(instr.op);
    read = written = -1;
    readsAll = (info.flags & f_opaque) != 0;

    const operand_kind kinds[3] = { info.a, info.b, info.c };
    const int32_t values[3] = { instr.a, instr.b, instr.c };

    for(int i = 0; i < 3; i++) {
        switch(kinds[i]) {
            case k_var_r:  read = values[i]; break;
            case k_var_w:  written = values[i]; break;
            case k_var_rw: read = values[i]; break;   // read first: the old value is needed
            default: break;
        }
    }
}

bool is_plain_computation(const ir_instruction &instr) {
    const instruction_info &info = instruction_info_of(instr.op);

    // reading ptr / the heap is harmless as long as it cannot throw
    if(info.flags & ~(uint32_t)(f_reads_ptr | f_reads_heap | f_state_check)) return false;
    if(info.popsNumbers || info.popsObjects || info.pushesNumbers || info.pushesObjects) return false;

    const operand_kind kinds[3] = { info.a, info.b, info.c };
    for(int k = 0; k < 3; k++) {
        // writes to places liveness does not track
        if(kinds[k] == k_sp_rel || kinds[k] == k_abs_slot || kinds[k] == k_obj_w || kinds[k] == k_slot_addr)
            return false;
    }

    return true;
}

void compute_liveness(function_ir &ir, control_flow &cfg, liveness &live, const std::vector<char> *essential) {
    size_t count = ir.code.size();
    size_t blocks = cfg.blocks.size();

    live.registersAfter.assign(count, 0);
    // slot 0 exists even in a function with no locals: it is the return value slot
    live.slotCount = ir.frameSlots > 0 ? ir.frameSlots : 1;
    live.localsKnown = true;

    // ---- do we understand every local this function touches?
    for(ir_instruction &instr : ir.code) {
        const instruction_info &info = instruction_info_of(instr.op);
        const operand_kind kinds[3] = { info.a, info.b, info.c };
        const int32_t values[3] = { instr.a, instr.b, instr.c };

        for(int i = 0; i < 3; i++) {
            if(is_local_kind(kinds[i]) && (values[i] < 0 || values[i] >= live.slotCount))
                live.localsKnown = false;
        }
    }

    slot_set empty;
    empty.resize(live.slotCount);
    live.localsAfter.assign(live.localsKnown ? count : 0, empty);

    // what is needed when the function is left: only the return value slot
    slot_set atExit = empty;
    atExit.add(0);

    std::vector<register_set> regIn(blocks, 0);
    std::vector<slot_set> localIn(blocks, empty);

    // One backward walk over a block. `record` stores the per instruction result.
    auto walk = [&](size_t b, bool record, register_set &regsOut, slot_set &localsOut) {
        basic_block &block = cfg.blocks[b];
        register_set regs = 0;
        slot_set locals = empty;

        // needed after the block = needed at the start of whatever can follow it
        ir_instruction &last = ir.code[block.last];
        const instruction_info &lastInfo = instruction_info_of(last.op);
        bool leavesFunction = (lastInfo.flags & f_terminator) && block.successors.empty();
        bool runsOffEnd = !(lastInfo.flags & f_terminator) && block.last + 1 >= (int)count;

        if(leavesFunction || runsOffEnd) locals = atExit;
        for(int next : block.successors) {
            regs |= regIn[next];
            if(live.localsKnown) locals.merge(localIn[next]);
        }

        // what the exception handlers of this block need
        register_set handlerRegs = 0;
        slot_set handlerLocals = empty;
        for(int handler : block.handlers) {
            handlerRegs |= regIn[handler];
            if(live.localsKnown) handlerLocals.merge(localIn[handler]);
        }

        for(int i = block.last; i >= block.first; i--) {
            ir_instruction &instr = ir.code[i];
            const instruction_info &info = instruction_info_of(instr.op);

            if(record) {
                live.registersAfter[i] = regs;
                if(live.localsKnown) live.localsAfter[i] = locals;
            }

            register_set reads, writes;
            register_effects(instr, reads, writes);

            int read = -1, written = -1; bool readsAll = false;
            if(live.localsKnown) local_effects(instr, read, written, readsAll);

            // "Strong" liveness: an instruction that is not essential and whose
            // results nobody needs is going to be deleted, so the values IT reads
            // are not needed on its account. Without this rule `x += 1` keeps x
            // alive forever just by reading its own previous value.
            if(essential != nullptr && !(*essential)[i]) {
                bool resultNeeded = (writes & regs) != 0
                        || (written >= 0 && live.localsKnown && locals.has(written));
                if(!resultNeeded) continue;
            }

            regs = (register_set)((regs & ~writes) | reads);

            if(live.localsKnown) {
                if(written >= 0) locals.remove(written);
                if(read >= 0) locals.add(read);
                if(readsAll) locals.add_all();
            }

            // it may raise an exception instead of completing
            if(!block.handlers.empty() && (info.flags & (f_throws | f_call | f_opaque))) {
                regs |= handlerRegs;
                if(live.localsKnown) locals.merge(handlerLocals);
            }
        }

        regsOut = regs;
        localsOut = locals;
    };

    // ---- iterate to a fixed point
    bool changed = true;
    while(changed) {
        changed = false;

        for(size_t b = blocks; b-- > 0; ) {
            register_set regs; slot_set locals;
            walk(b, false, regs, locals);

            if(regs != regIn[b]) { regIn[b] = regs; changed = true; }
            if(live.localsKnown && localIn[b].merge(locals)) changed = true;
        }
    }

    // ---- final walk records the answer for every instruction
    for(size_t b = 0; b < blocks; b++) {
        register_set regs; slot_set locals;
        walk(b, true, regs, locals);
    }
}
