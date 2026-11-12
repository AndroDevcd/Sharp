//
// Pass: box elimination   (-O3, works across functions)
// =====================================================
// Removes number objects that never leave the function ("escape analysis" /
// "scalar replacement" in optimizing compilers).
//
// WHY IT MATTERS
//   Every number that crosses a function boundary in Sharp is wrapped in an
//   object: `def square(x: var): double` allocates a std#double, runs a chain
//   of three constructors to store one number in it, and the caller then reads
//   that number straight back out with `.value`. Once square() is inlined, the
//   object is created and thrown away inside one function, which makes it
//   pointless:
//
//       k : var = square(4).value;
//
//       before (square already inlined)        after
//       ------------------------------------   --------------------------------
//         new_class std#double                   istore 16
//         dup                                    ipopl fp+B         ; B = value of the box
//         istore 16                              loadl egx, fp+B
//         call std#double.double                 smovr_2 egx, fp+1  ; k
//         popobj2
//         smovr_3 fp+3      ; tmp = the box
//         pushl 3
//         popobj2
//         getf egx, slot 0  ; tmp.value
//         smovr_2 egx, fp+1 ; k
//
//   No allocation, no constructor calls, no garbage for the collector. The
//   other passes then shrink what is left further.
//
// HOW IT DECIDES
//   Every `new_class` is a candidate. The pass follows each new object through
//   the function (operand stack, object locals, ptr) with a forward data flow
//   analysis and keeps the object only if it can prove EVERYTHING that ever
//   happens to it is one of:
//
//     - being copied around: dup, popl / pushl, popobj2, smovr_3, pushobj, pop
//     - one call that "stores a number into one number field and does nothing else"
//       (analysis/box_constructors.h works that out from the callee's bytecode,
//       so nothing about the std number classes is hard coded here)
//     - calls that do nothing at all with it (the empty init<T>() functions)
//     - reading that field back: getf r, slot
//     - writing that field: putf r, slot
//       (what `n += 2` on an `int` compiles to once the operator is inlined)
//     - a cast to the class it was created as (always succeeds)
//
//   Anything else - returned, passed to a function, stored in a field or an
//   array, compared, locked, type-tested, thrown, reachable through two
//   different paths with different contents - makes the object "escape", and an
//   escaping object is left exactly as the generator emitted it.
//
//   For an object that qualifies, the instructions that only moved it around
//   are deleted, the constructor call becomes "store the number in a new local
//   slot" (with the constructor's casts) and each read or write of the field
//   becomes a read or write of that slot. A field of an object and a local are
//   the same 64-bit cell, so the value is the same bit for bit.
//
// DETAILS THAT KEEP IT HONEST
//   - One slot per `new_class` instruction. If the same `new_class` can run
//     again while an older object from it is still referenced anywhere, the two
//     would share the slot, so that site is given up. A stale reference in a
//     local that is never read again does not count (object-local liveness).
//   - When the tracked picture of the stack or of ptr is lost (delegate calls,
//     interrupts, sp-relative access, paths that disagree), every object that
//     is visible at that point escapes. The analysis never guesses.
//   - Functions with try/catch/finally are skipped.
//
// VISIBLE DIFFERENCES
//   - The removed constructors do not run, so they cannot appear in a stack
//     trace, and an out-of-memory error can no longer come from these objects.
//   - A local that used to be overwritten with the box keeps its previous
//     object a little longer (until the local is written again or the function
//     returns).
//

#include "passes.h"
#include "../analysis/control_flow.h"
#include "../analysis/liveness.h"
#include <bitset>

/* more candidates than this in one function: the rest are simply not tracked */
#define MAX_SITES 511
#define MAX_FRAME_SLOTS 250

namespace {

typedef std::bitset<MAX_SITES + 1> site_set; // bit k = candidate object k (1..MAX_SITES)
inline site_set bit(int site) { site_set set; set.set((size_t)site); return set; }
inline bool has(const site_set &set, int site) { return set.test((size_t)site); }

/** What ptr points at, as far as this pass cares */
enum pointer_kind : uint8_t {
    p_dead,     // not read again before it is set (or garbage left by a call: never read by generated code)
    p_heap,     // a field / static / thread local: never a tracked object
    p_any,      // unknown: reading or writing through it may touch any slot
    p_local,    // &local[value]
    p_slot      // the stack slot a tracked object (value) was just popped from; `index` = its stack position
};

struct flow_state {
    bool reached = false;
    std::vector<int> stack;     // 0 = a number or an untracked object, k = candidate k
    int base = 0;               // stack entries below `stack` that were popped without being known
    std::vector<int> locals;    // object part of each frame slot
    pointer_kind ptr = p_dead;
    int ptrValue = 0, ptrIndex = 0;
    site_set constructed;   // candidates whose value-storing constructor has run
};

/** What to do with one instruction if its candidate survives */
enum action_kind : uint8_t { a_none, a_delete, a_load, a_store, a_construct };
struct action { action_kind kind = a_none; int site = 0; };

struct analysis {
    function_ir &ir;
    program_ir &program;
    control_flow cfg;

    std::vector<int> siteOf;                // per instruction index: candidate number of a new_class, else 0
    std::vector<int32_t> classOf;           // per site: the class its new_class creates
    int sites = 0;
    site_set escaped;

    std::vector<slot_set> liveObjectsIn;    // per block: object locals that may still be read
    std::vector<char> ptrLiveIn;            // per block: ptr may be read before it is set
    std::vector<char> ptrLiveAfter;         // per instruction: the same, right after it

    std::vector<flow_state> in;             // per block
    std::vector<action> actions;            // per instruction (filled by the final walk)
    std::vector<const box_summary*> constructors; // per site: the summary of its constructor call

    analysis(function_ir &f, program_ir &p) : ir(f), program(p) {}

    // ---- helpers on a state
    void escape(int site) { if(site > 0) escaped |= bit(site); }

    void escape_everything(flow_state &s) {
        for(int v : s.stack) escape(v);
        for(int v : s.locals) escape(v);
        if(s.ptr == p_slot) escape(s.ptrValue);
    }

    bool holds(flow_state &s, int site) {
        for(int v : s.stack) if(v == site) return true;
        for(int v : s.locals) if(v == site) return true;
        return (s.ptr == p_slot) && s.ptrValue == site;
    }

    void push(flow_state &s, int value) {
        // pushing lands on the slot ptr still points at: its old content is gone
        if(s.ptr == p_slot && s.ptrIndex == s.base + (int)s.stack.size() && s.ptrValue != value) {
            escape(s.ptrValue);
            s.ptr = p_any;
        }
        s.stack.push_back(value);
    }

    int pop(flow_state &s) {
        if(s.stack.empty()) { s.base--; return 0; }   // below what we know: not one of ours
        int value = s.stack.back();
        s.stack.pop_back();
        return value;
    }

    /** the tracked object ptr points at (0 = none). Unknown pointers give everything up. */
    int deref(flow_state &s) {
        switch(s.ptr) {
            case p_local: return s.locals[s.ptrValue];
            case p_slot:  return s.ptrValue;
            case p_any:
            case p_dead:  escape_everything(s); return 0;
            default:      return 0;
        }
    }

    void object_liveness();
    int boxed_field(flow_state &s, const ir_instruction &instr);
    void forget_dead(flow_state &s, int block);
    bool merge_into(flow_state &into, flow_state &from);
    void step(size_t index, flow_state &s, action *act);
    bool run();
    bool rewrite();
};

/** May this instruction read through ptr? */
bool reads_pointer(const ir_instruction &instr) {
    const instruction_info &info = instruction_info_of(instr.op);
    return (info.flags & f_reads_ptr) != 0 || instr.op == Opcode::POPOBJ || instr.op == Opcode::DEL;
}

/**
 * Backward data flow, per block entry:
 *   - which object locals may still be read (pushl, movl takes the address, anything opaque)
 *   - whether ptr may be read before it is set again
 */
void analysis::object_liveness() {
    size_t blocks = cfg.blocks.size();
    int slots = (int)in[0].locals.size();

    slot_set empty; empty.resize(slots);
    liveObjectsIn.assign(blocks, empty);
    ptrLiveIn.assign(blocks, 0);

    ptrLiveAfter.assign(ir.code.size(), 0);

    // `record` = false while iterating, true for the last round that stores ptrLiveAfter
    bool changed = true, record = false;
    while(changed || !record) {
        if(!changed) record = true;
        changed = false;

        for(size_t b = blocks; b-- > 0; ) {
            basic_block &block = cfg.blocks[b];
            slot_set live = empty;
            bool ptrLive = false;

            for(int next : block.successors) {
                live.merge(liveObjectsIn[next]);
                ptrLive = ptrLive || ptrLiveIn[next];
            }

            for(int i = block.last; i >= block.first; i--) {
                const ir_instruction &instr = ir.code[i];
                const instruction_info &info = instruction_info_of(instr.op);
                const operand_kind kinds[3] = { info.a, info.b, info.c };
                const int32_t values[3] = { instr.a, instr.b, instr.c };

                if(record) ptrLiveAfter[i] = ptrLive;

                // ptr: a pure write ends its life, a read starts it. (A call is neither:
                // the callee usually overwrites ptr, but nothing guarantees it.)
                if((info.flags & f_writes_ptr) && !reads_pointer(instr)) ptrLive = false;
                if(reads_pointer(instr)) ptrLive = true;

                for(int k = 0; k < 3; k++) {
                    if(values[k] < 0 || values[k] >= slots) continue;
                    if(kinds[k] == k_obj_w) live.remove(values[k]);
                }
                for(int k = 0; k < 3; k++) {
                    if(values[k] < 0 || values[k] >= slots) continue;
                    if(kinds[k] == k_obj_r || kinds[k] == k_slot_addr) live.add(values[k]);
                }
                if(info.flags & f_opaque) live.add_all();
            }

            if(liveObjectsIn[b].merge(live)) changed = true;
            if(ptrLive && !ptrLiveIn[b]) { ptrLiveIn[b] = 1; changed = true; }
        }
    }
}

/**
 * GETF / PUTF: is it the value field of a tracked box whose constructor has
 * run? Returns the box (0 = no: the generic rules apply and the object escapes).
 *     ptr must point at a local or a just-popped stack slot holding the box,
 *     and the number slot must be the one the constructor stored into.
 */
int analysis::boxed_field(flow_state &s, const ir_instruction &instr) {
    if(s.ptr != p_local && s.ptr != p_slot) return 0;

    int value = deref(s);
    if(value <= 0 || !has(s.constructed, value)) return 0;     // not ours, or read before the value was stored
    if(constructors[value] == nullptr || constructors[value]->slot != instr.b) return 0;
    return value;
}

/** On entry to `block`: drop what can never be looked at again, so stale references do not count */
void analysis::forget_dead(flow_state &s, int block) {
    if(!ptrLiveIn[block]) s.ptr = p_dead;
    if(s.ptr == p_any) return;   // an unknown pointer may still reach any local

    for(int n = 0; n < (int)s.locals.size(); n++) {
        if(s.locals[n] == 0 || liveObjectsIn[block].has(n)) continue;
        if(s.ptr == p_local && s.ptrValue == n) continue;
        s.locals[n] = 0;
    }

    // forget "constructed" for objects that are gone, so paths compare equal
    site_set present;
    for(int v : s.stack) if(v > 0) present |= bit(v);
    for(int v : s.locals) if(v > 0) present |= bit(v);
    if(s.ptr == p_slot) present |= bit(s.ptrValue);
    s.constructed &= present;
}

/** Combine the state arriving over one more edge. Returns true if `into` changed. */
bool analysis::merge_into(flow_state &into, flow_state &from) {
    if(!into.reached) { into = from; into.reached = true; return true; }

    bool changed = false;

    // the operand stack must look the same from both sides
    if(into.stack.size() != from.stack.size() || into.base != from.base) {
        for(int v : into.stack) escape(v);
        for(int v : from.stack) escape(v);
        if(!into.stack.empty() || into.base != 0) changed = true;
        into.stack.clear();
        into.base = 0;
    } else {
        for(size_t i = 0; i < into.stack.size(); i++) {
            if(into.stack[i] == from.stack[i]) continue;
            escape(into.stack[i]); escape(from.stack[i]);
            if(into.stack[i] != 0) { into.stack[i] = 0; changed = true; }
        }
    }

    for(size_t n = 0; n < into.locals.size(); n++) {
        if(into.locals[n] == from.locals[n]) continue;
        escape(into.locals[n]); escape(from.locals[n]);
        if(into.locals[n] != 0) { into.locals[n] = 0; changed = true; }
    }

    bool samePtr = into.ptr == from.ptr
            && (into.ptr == p_dead || into.ptr == p_heap || into.ptr == p_any
                || (into.ptrValue == from.ptrValue && (into.ptr != p_slot || into.ptrIndex == from.ptrIndex)));
    if(!samePtr) {
        if(into.ptr == p_slot) escape(into.ptrValue);
        if(from.ptr == p_slot) escape(from.ptrValue);
        if(into.ptr == p_local) escape(into.locals[into.ptrValue]);
        if(from.ptr == p_local) escape(from.locals[from.ptrValue]);
        if(into.ptr != p_any) { into.ptr = p_any; changed = true; }
    }

    // constructed on one path only: we could not tell whether a read is valid
    site_set differs = into.constructed ^ from.constructed;
    if(differs.any()) {
        escaped |= differs;
        into.constructed &= from.constructed;
        changed = true;
    }

    return changed;
}

/** The effect of ir.code[index] on the state. `act` (final walk only) receives what to do with it. */
void analysis::step(size_t index, flow_state &s, action *act) {
    const ir_instruction &instr = ir.code[index];
    const instruction_info &info = instruction_info_of(instr.op);
    int slots = (int)s.locals.size();

    auto set = [&](action_kind kind, int site) { if(act && site > 0) { act->kind = kind; act->site = site; } };
    auto local_ok = [&](int32_t n) { return n >= 0 && n < slots; };

    switch(instr.op) {
        case Opcode::NEWCLASS: {                        // push new a()
            int site = siteOf[index];
            if(site > 0) {
                if(holds(s, site)) escape(site);        // an older object from this very line is still around
                s.constructed &= ~bit(site);
            }
            push(s, site);
            set(a_delete, site);
            return;
        }
        case Opcode::DUP: {                             // push a copy of the top object
            int value = s.stack.empty() ? 0 : s.stack.back();
            push(s, value);
            set(a_delete, value);
            return;
        }
        case Opcode::PUSHL: {                           // push local.obj
            int value = local_ok(instr.a) ? s.locals[instr.a] : 0;
            push(s, value);
            set(a_delete, value);
            return;
        }
        case Opcode::POPL: {                            // local.obj = pop
            int value = pop(s);
            if(local_ok(instr.a)) s.locals[instr.a] = value; else escape(value);
            set(a_delete, value);
            return;
        }
        case Opcode::POP: {
            int value = pop(s);
            set(a_delete, value);
            return;
        }
        case Opcode::PUSHNULL:
            push(s, 0);
            return;
        case Opcode::POPOBJ_2: {                        // ptr = &popped slot
            int value = pop(s);
            if(value > 0) { s.ptr = p_slot; s.ptrValue = value; s.ptrIndex = s.base + (int)s.stack.size(); }
            else s.ptr = p_any;                         // some untracked slot above sp
            set(a_delete, value);
            return;
        }
        case Opcode::SMOVR_3: {                         // local.obj = *ptr
            int value = deref(s);
            if(local_ok(instr.a)) s.locals[instr.a] = value; else escape(value);
            set(a_delete, value);
            return;
        }
        case Opcode::MOVL:                              // ptr = &local.obj
            if(local_ok(instr.a)) { s.ptr = p_local; s.ptrValue = instr.a; }
            else s.ptr = p_any;
            return;
        case Opcode::MOVN: {                            // ptr = &ptr->field[a]: an object field, a box has none
            escape(deref(s));
            s.ptr = p_heap;
            return;
        }
        case Opcode::GETF: {                            // regs[a] = ptr->o.<number slot b>
            int value = boxed_field(s, instr);
            if(value > 0) { set(a_load, value); return; }
            break;                                      // anything else: the generic rules (the object escapes)
        }
        case Opcode::PUTF: {                            // ptr->o.<number slot b> = regs[a]
            // `n += 2` on a boxed local: the new value goes to the value slot instead
            int value = boxed_field(s, instr);
            if(value > 0) { set(a_store, value); return; }
            break;
        }
        case Opcode::CAST: {                            // check that *ptr is of class a
            // a tracked object's class is known: it is the class its new_class created
            if(s.ptr != p_local && s.ptr != p_slot) break;
            int value = deref(s);
            if(value > 0) {
                if(classOf[value] == instr.a) set(a_delete, value);
                else escape(value);
            }
            return;
        }
        case Opcode::PUSHOBJ: {                         // push *ptr
            int value = deref(s);
            push(s, value);
            set(a_delete, value);
            return;
        }
        case Opcode::POPOBJ: {                          // *ptr = pop
            int value = pop(s);
            if(s.ptr == p_local) {
                s.locals[s.ptrValue] = value;
                set(a_delete, value);
            } else {
                escape(value);
                if(s.ptr == p_slot) escape(s.ptrValue);
                if(s.ptr == p_any || s.ptr == p_dead) escape_everything(s);
            }
            return;
        }
        case Opcode::DEL:                               // *ptr = null
            if(s.ptr == p_local) s.locals[s.ptrValue] = 0;
            else if(s.ptr == p_slot) escape(s.ptrValue);
            else if(s.ptr == p_any || s.ptr == p_dead) escape_everything(s);
            return;

        case Opcode::CALL: {
            auto found = program.byAddress.find(instr.a);
            if(found == program.byAddress.end()) break; // unknown callee: the generic rules below give up

            function_ir &callee = *found->second;
            int arguments = callee.ci->fpOffset + 1;
            bool returnsValue = callee.ci->spOffset == callee.ci->fpOffset;
            if(arguments < 0 || (!returnsValue && callee.ci->spOffset != callee.ci->fpOffset + 1)) break;

            size_t depth = s.stack.size();
            int top = depth >= 1 ? s.stack[depth - 1] : 0;
            int below = depth >= 2 ? s.stack[depth - 2] : 0;

            // new C(number): the one call that gives the box its value
            if(arguments == 2 && top == 0 && below > 0 && !has(s.constructed, below)) {
                const box_summary &summary = summarize_box_call(program, instr.a);
                if(summary.effect == box_stores_value) {
                    pop(s); pop(s);
                    s.constructed |= bit(below);
                    constructors[below] = &summary;
                    set(a_construct, below);
                    return;
                }
            }
            // a call that does nothing with the object it is given
            if(arguments == 1 && top > 0 && summarize_box_call(program, instr.a).effect == box_no_effect) {
                pop(s);
                set(a_delete, top);
                return;
            }

            // an ordinary call: whatever it is handed is out of our sight
            // (ptr: the callee normally overwrites it. If the code does read it again
            // it sees either the old value - what we track - or the callee's, which
            // generated code never relies on. A popped stack slot is certainly
            // overwritten by the callee's frame.)
            for(int i = 0; i < arguments; i++) escape(pop(s));
            if(returnsValue) push(s, 0);
            if(s.ptr == p_slot) { escape(s.ptrValue); s.ptr = p_any; }
            return;
        }

        default:
            break;
    }

    // ---- everything else, from the instruction table

    // instructions whose stack effect or reach we cannot describe
    if((info.flags & (f_call | f_opaque | f_indirect_jump)) || instr.op == Opcode::SWAP) {
        escape_everything(s);
        s.stack.clear();
        s.base = 0;
        s.ptr = p_any;
        return;
    }

    const operand_kind kinds[3] = { info.a, info.b, info.c };
    const int32_t values[3] = { instr.a, instr.b, instr.c };
    for(int k = 0; k < 3; k++) {
        switch(kinds[k]) {
            case k_obj_r:
                if(local_ok(values[k])) escape(s.locals[values[k]]);
                break;
            case k_obj_w:
                if(local_ok(values[k])) s.locals[values[k]] = 0;
                break;
            case k_sp_rel:
                // counts stack slots from the top: wrong once a slot below is removed
                for(int v : s.stack) escape(v);
                break;
            default: break;
        }
    }

    if(reads_pointer(instr)) escape(deref(s));

    for(int i = 0; i < info.popsNumbers + info.popsObjects; i++) escape(pop(s));
    for(int i = 0; i < info.pushesNumbers + info.pushesObjects; i++) push(s, 0);

    if(info.flags & f_writes_ptr) {
        // movg / movnd / tls_movl land in the heap; movsl points into the stack
        s.ptr = instr.op == Opcode::MOVSL ? p_any : p_heap;
    }
}

bool analysis::run() {
    build_control_flow(ir, cfg);
    size_t blocks = cfg.blocks.size();
    if(blocks == 0) return false;

    // candidates
    siteOf.assign(ir.code.size(), 0);
    for(size_t i = 0; i < ir.code.size() && sites < MAX_SITES; i++) {
        if(ir.code[i].op == Opcode::NEWCLASS) siteOf[i] = ++sites;
    }
    classOf.assign(sites + 1, -1);
    for(size_t i = 0; i < ir.code.size(); i++) {
        if(siteOf[i] > 0) classOf[siteOf[i]] = ir.code[i].a;
    }
    if(sites == 0) return false;
    constructors.assign(sites + 1, nullptr);

    int slots = ir.frameSlots > 0 ? ir.frameSlots : 1;
    in.assign(blocks, flow_state());
    in[0].reached = true;
    in[0].locals.assign(slots, 0);

    object_liveness();

    // forward data flow until nothing changes
    std::vector<char> queued(blocks, 0);
    std::vector<int> work;
    work.push_back(0); queued[0] = 1;

    while(!work.empty()) {
        int b = work.back();
        work.pop_back();
        queued[b] = 0;

        flow_state s = in[b];
        for(int i = cfg.blocks[b].first; i <= cfg.blocks[b].last; i++) {
            step((size_t)i, s, nullptr);
            if(!ptrLiveAfter[i]) s.ptr = p_dead;
        }

        for(int next : cfg.blocks[b].successors) {
            flow_state arriving = s;
            forget_dead(arriving, next);
            if(merge_into(in[next], arriving) && !queued[next]) {
                queued[next] = 1;
                work.push_back(next);
            }
        }
    }

    // final walk: what would be done with each instruction
    actions.assign(ir.code.size(), action());
    for(size_t b = 0; b < blocks; b++) {
        if(!in[b].reached) continue;
        flow_state s = in[b];
        for(int i = cfg.blocks[b].first; i <= cfg.blocks[b].last; i++) {
            step((size_t)i, s, &actions[i]);
            if(!ptrLiveAfter[i]) s.ptr = p_dead;
        }
    }

    return true;
}

bool analysis::rewrite() {
    // a constructor that casts needs a register nobody is using at that point
    std::vector<int> castRegister(ir.code.size(), -1);
    bool needRegisters = false;
    for(size_t i = 0; i < ir.code.size(); i++) {
        action &act = actions[i];
        if(act.kind == a_construct && !has(escaped, act.site) && !constructors[act.site]->casts.empty())
            needRegisters = true;
    }

    if(needRegisters) {
        liveness live;
        compute_liveness(ir, cfg, live);

        // fixed-role registers (adx, cmt, ebx) are left alone
        static const int candidates[] = { EGX, BMR, ECX, ECF, EDF, EHF, CX };
        for(size_t i = 0; i < ir.code.size(); i++) {
            action &act = actions[i];
            if(act.kind != a_construct || has(escaped, act.site) || constructors[act.site]->casts.empty()) continue;

            for(int candidate : candidates) {
                if(!(live.registersAfter[i] & REG_BIT(candidate))) { castRegister[i] = candidate; break; }
            }
            if(castRegister[i] < 0) escaped |= bit(act.site);   // no free register: keep this box
        }
    }

    // one new local per surviving box: its value
    std::vector<int> valueSlot(sites + 1, -1);
    int frameSlots = ir.frameSlots > 0 ? ir.frameSlots : 1;
    for(size_t i = 0; i < ir.code.size(); i++) {
        int site = siteOf[i];
        if(site == 0 || has(escaped, site)) continue;
        if(frameSlots >= MAX_FRAME_SLOTS) { escaped |= bit(site); continue; }
        valueSlot[site] = frameSlots++;
    }

    bool changed = false;
    for(size_t i = ir.code.size(); i-- > 0; ) {         // back to front: indexes stay valid
        action act = actions[i];
        if(act.kind == a_none || has(escaped, act.site)) continue;
        changed = true;

        ir_instruction &instr = ir.code[i];
        int slot = valueSlot[act.site];

        if(act.kind == a_delete) {
            remove_instruction(ir, i);
        } else if(act.kind == a_store) {
            // box.value = regs[a]   ->   local = regs[a]
            instr.op = Opcode::SMOVR_2;
            instr.b = slot; instr.c = 0;
            instr.modified = true;
        } else if(act.kind == a_load) {
            // regs[a] = box.value   ->   regs[a] = local
            instr.op = Opcode::LOADL;
            instr.b = slot; instr.c = 0;
            instr.modified = true;
        } else {
            // call C.C(number)   ->   local = (casts) pop number
            const std::vector<uint8_t> &casts = constructors[act.site]->casts;
            if(casts.empty()) {
                instr.op = Opcode::IPOPL;
                instr.a = slot; instr.b = instr.c = 0;
                instr.modified = true;
            } else {
                int r = castRegister[i];
                instr.op = Opcode::LOADVAL;
                instr.a = r; instr.b = instr.c = 0;
                instr.modified = true;

                std::vector<ir_instruction> more;
                for(uint8_t cast : casts) more.push_back(make_instruction(ir, cast, r, r));
                more.push_back(make_instruction(ir, Opcode::SMOVR_2, r, slot));
                ir.code.insert(ir.code.begin() + i + 1, more.begin(), more.end());
            }
            program.boxesRemoved++;
        }
    }

    if(changed) {
        ir.frameSlots = frameSlots;
        ir.changed = true;
    }
    return changed;
}

} // namespace

bool pass_box_elimination(function_ir &ir, program_ir &program) {
    if(!ir.failure.empty() || !ir.tries.empty()) return false;

    // anything that can jump to a place only known at run time: leave it alone
    for(ir_instruction &instr : ir.code) {
        if(instr.pcConstant) return false;
    }

    analysis a(ir, program);
    if(!a.run()) return false;

    // every candidate escaped?
    site_set all;
    for(int site = 1; site <= a.sites; site++) all |= bit(site);
    if((a.escaped & all) == all) return false;

    return a.rewrite();
}
