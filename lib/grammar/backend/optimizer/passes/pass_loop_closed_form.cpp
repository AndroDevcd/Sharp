//
// Pass: loops replaced by a formula   (-O2, run before the other loop passes)
// ==========================================================================
// A counting loop that only adds things up is replaced by the formula for the
// sum, so it takes the same (tiny) time for a million trips as for ten.
// Compilers call this "induction variable final value replacement".
//
//     total : var = 0;
//     for(i : var = 0; i < n; i++) { total += i * 2 + 1; }
//
//     becomes, in effect
//
//     T = number of trips (worked out from n)
//     if(T is in the range where the formula is exact) {
//         total = total + T * T;          // sum of (2i + 1) for i = 0 .. T-1
//         i = T;
//     } else {
//         ... the original loop ...
//     }
//
// WHAT IS RECOGNIZED
//   The loop is run SYMBOLICALLY for one trip (see `polynomial` below): every
//   register and local holds an expression in the locals' values at the start
//   of the trip. After the trip, each local the loop writes must be one of:
//     - the counter:        i  = i + c            c a whole number, not 0
//     - an accumulator:     s  = s + f(i)         f a polynomial up to i*i with
//                                                 whole-number coefficients
//     - a scratch variable that nothing reads after the loop
//   and the loop must be exactly:  test of `i` against a bound that does not
//   change (`<`, `<=` counting up, `>`, `>=` counting down), then one straight
//   block of plain arithmetic. No calls, no memory access, no branches in the
//   body. The counter's starting value must be known at compile time.
//
// THE FORMULAS
//   With i = i0 + c*t on trip t (t = 0 .. T-1) and f(i) = a2*i*i + a1*i + a0:
//       f = p2*t*t + p1*t + p0      p2 = a2*c*c
//                                   p1 = 2*a2*i0*c + a1*c
//                                   p0 = a2*i0*i0 + a1*i0 + a0
//       sum over t = 0..T-1  =  p0*T + p1*T*(T-1)/2 + p2*(T-1)*T*(2T-1)/6
//   Multiplied by 6 this is a polynomial in T with whole-number coefficients,
//       6*sum = (2*p2)*T^3 + (3*p1 - 3*p2)*T^2 + (6*p0 - 3*p1 + p2)*T
//   which is what the generated code evaluates (Horner form) and then divides.
//
//   Trips:  counting up,   i < n :  T = ceil((n - i0) / c)
//                          i <= n:  T = floor((n - i0) / c) + 1
//           counting down mirrors this with (i0 - n) / -c.
//
// WHY THE RESULT IS BIT-FOR-BIT WHAT THE LOOP WOULD GIVE
//   Sharp numbers are floating point: adding a million values one at a time
//   and adding their sum in one go round differently in general. They agree
//   exactly when everything involved is a whole number small enough to be
//   exact (below 2^52). The formula is therefore only used when, at run time,
//     - the trip count is between 1 and a limit computed here so that every
//       intermediate value stays below 2^52
//     - an accumulator whose starting value is not known at compile time
//       holds a whole number of moderate size
//   In every other case (including a bound that is NaN) the code jumps to the
//   original loop, which is kept unchanged right behind the formula.
//
// COST
//   The loop stays in the program as the fallback, plus roughly 15-40
//   instructions for the formula.
//

#include "passes.h"
#include "../analysis/control_flow.h"
#include "../analysis/loops.h"
#include "../analysis/liveness.h"
#include "../analysis/known_values.h"
#include <map>
#include <cstdlib>

#define MAX_FUNCTION_INSTRUCTIONS 6000
/* every value the formula computes stays below this, so plain doubles are exact */
#define EXACT_LIMIT ((long double)4503599627370496.0L)      /* 2^52 */
/* coefficients beyond this are not worth reasoning about */
#define MAX_COEFFICIENT ((int64_t)1 << 28)

namespace {

// ---------------------------------------------------------------------------------
// expressions: polynomials of degree <= 2 in the loop's locals, whole-number coefficients
// ---------------------------------------------------------------------------------
struct polynomial {
    int64_t constant = 0;
    std::map<int, int64_t> linear;                       // local -> coefficient
    std::map<std::pair<int, int>, int64_t> quadratic;    // (local, local), first <= second -> coefficient

    void tidy() {
        for(auto it = linear.begin(); it != linear.end(); ) it = it->second == 0 ? linear.erase(it) : std::next(it);
        for(auto it = quadratic.begin(); it != quadratic.end(); ) it = it->second == 0 ? quadratic.erase(it) : std::next(it);
    }
    bool is_constant() const { return linear.empty() && quadratic.empty(); }
    bool too_big() const {
        if(std::llabs(constant) > MAX_COEFFICIENT) return true;
        for(auto &term : linear) if(std::llabs(term.second) > MAX_COEFFICIENT) return true;
        for(auto &term : quadratic) if(std::llabs(term.second) > MAX_COEFFICIENT) return true;
        return false;
    }
};

enum value_kind : uint8_t { v_unknown, v_polynomial, v_invariant };

/** What a register or local holds during the symbolic trip */
struct symbolic {
    value_kind kind = v_unknown;
    polynomial p;           // v_polynomial
    int local = -1;         // v_invariant: the value of this local, which the loop never writes
};

symbolic unknown() { return symbolic(); }
symbolic number(int64_t value) { symbolic s; s.kind = v_polynomial; s.p.constant = value; return s; }
symbolic variable(int local) { symbolic s; s.kind = v_polynomial; s.p.linear[local] = 1; return s; }

symbolic checked(symbolic s) {
    s.p.tidy();
    return s.p.too_big() ? unknown() : s;
}

symbolic add(const symbolic &a, const symbolic &b, int sign) {
    if(a.kind != v_polynomial || b.kind != v_polynomial) return unknown();
    symbolic r = a;
    r.p.constant += sign * b.p.constant;
    for(auto &term : b.p.linear) r.p.linear[term.first] += sign * term.second;
    for(auto &term : b.p.quadratic) r.p.quadratic[term.first] += sign * term.second;
    return checked(r);
}

symbolic multiply(const symbolic &a, const symbolic &b) {
    if(a.kind != v_polynomial || b.kind != v_polynomial) return unknown();
    if(!a.p.quadratic.empty() && !b.p.is_constant()) return unknown();     // degree 3 and up
    if(!b.p.quadratic.empty() && !a.p.is_constant()) return unknown();

    // A product of two whole numbers that comes out as zero is -0 in floating
    // point when one factor is negative. That only matters if the zero itself
    // is observed; sums are not affected by it (x + -0 == x), and the pass only
    // ever uses these values inside sums.
    symbolic r;
    r.kind = v_polynomial;
    r.p.constant = a.p.constant * b.p.constant;
    for(auto &term : a.p.linear) r.p.linear[term.first] += term.second * b.p.constant;
    for(auto &term : b.p.linear) r.p.linear[term.first] += term.second * a.p.constant;
    for(auto &term : a.p.quadratic) r.p.quadratic[term.first] += term.second * b.p.constant;
    for(auto &term : b.p.quadratic) r.p.quadratic[term.first] += term.second * a.p.constant;
    for(auto &x : a.p.linear) {
        for(auto &y : b.p.linear) {
            std::pair<int, int> key(std::min(x.first, y.first), std::max(x.first, y.first));
            r.p.quadratic[key] += x.second * y.second;
        }
    }
    return checked(r);
}

/** The state of the symbolic trip */
struct trip_state {
    symbolic registers[10];
    std::vector<symbolic> locals;

    // the last comparison seen, as long as nothing overwrote cmt since
    bool hasTest = false;
    uint8_t testOp = 0;
    symbolic testLeft, testRight;
};

/** Run one instruction symbolically. False = the loop is not of the simple kind. */
bool run(const ir_instruction &instr, trip_state &s) {
    const instruction_info &info = instruction_info_of(instr.op);

    // only plain arithmetic on registers and number locals
    if(!is_plain_computation(instr)) return false;
    if(info.flags & (f_reads_ptr | f_reads_heap | f_throws | f_writes_ptr)) return false;
    if(instr.pcConstant) return false;

    symbolic *reg = s.registers;
    std::vector<symbolic> &local = s.locals;
    auto imm = [&](int32_t v) { return number(v); };

    register_set reads, writes;
    register_effects(instr, reads, writes);
    if(writes & REG_BIT(CMT)) s.hasTest = false;

    switch(instr.op) {
        case Opcode::NOP: return true;
        case Opcode::MOVI: reg[instr.a] = imm(instr.b); return true;
        case Opcode::MOVR: reg[instr.a] = reg[instr.b]; return true;
        case Opcode::ADD: reg[instr.c] = add(reg[instr.a], reg[instr.b], 1); return true;
        case Opcode::SUB: reg[instr.c] = add(reg[instr.a], reg[instr.b], -1); return true;
        case Opcode::MUL: reg[instr.c] = multiply(reg[instr.a], reg[instr.b]); return true;
        case Opcode::IADD: reg[instr.a] = add(reg[instr.a], imm(instr.b), 1); return true;
        case Opcode::ISUB: reg[instr.a] = add(reg[instr.a], imm(instr.b), -1); return true;
        case Opcode::IMUL: reg[instr.a] = multiply(reg[instr.a], imm(instr.b)); return true;
        case Opcode::INC: reg[instr.a] = add(reg[instr.a], imm(1), 1); return true;
        case Opcode::DEC: reg[instr.a] = add(reg[instr.a], imm(1), -1); return true;

        case Opcode::LOADL: reg[instr.a] = local[instr.b]; return true;
        case Opcode::SMOVR_2: local[instr.b] = reg[instr.a]; return true;
        case Opcode::SMOVR_4: local[instr.a] = local[instr.b]; return true;
        case Opcode::ISTOREL: local[instr.a] = imm(instr.b); return true;
        case Opcode::ADDL: local[instr.b] = add(local[instr.b], reg[instr.a], 1); return true;
        case Opcode::SUBL: local[instr.b] = add(local[instr.b], reg[instr.a], -1); return true;
        case Opcode::MULL: local[instr.b] = multiply(local[instr.b], reg[instr.a]); return true;
        case Opcode::IADDL: local[instr.a] = add(local[instr.a], imm(instr.b), 1); return true;
        case Opcode::ISUBL: local[instr.a] = add(local[instr.a], imm(instr.b), -1); return true;
        case Opcode::IMULL: local[instr.a] = multiply(local[instr.a], imm(instr.b)); return true;

        case Opcode::LT: case Opcode::LTE: case Opcode::GT: case Opcode::GTE:
            s.hasTest = true;
            s.testOp = instr.op;
            s.testLeft = reg[instr.a];
            s.testRight = reg[instr.b];
            reg[CMT] = unknown();
            return true;

        default:
            break;
    }

    // some other plain computation (division, casts, ...): its results are simply not known
    for(int r = 0; r < 10; r++) {
        if((writes >> r) & 1u) reg[r] = unknown();
    }
    const operand_kind kinds[3] = { info.a, info.b, info.c };
    const int32_t values[3] = { instr.a, instr.b, instr.c };
    for(int k = 0; k < 3; k++) {
        if(kinds[k] == k_var_w || kinds[k] == k_var_rw) local[values[k]] = unknown();
    }
    return true;
}

/** One local's value after the loop: (c3*T^3 + c2*T^2 + c1*T + c0) / divisor [+ its value before the loop] */
struct final_value {
    int local;
    int64_t c[4];
    int64_t divisor;
    bool addStart;      // the starting value is only known at run time
};

int64_t gcd(int64_t a, int64_t b) { a = std::llabs(a); b = std::llabs(b); while(b) { int64_t t = a % b; a = b; b = t; } return a; }

/** largest T (at most `cap`) for which sum |c[k]| * T^k stays within `limit` */
int64_t largest_trip_count(const int64_t c[4], long double limit, int64_t cap) {
    auto fits = [&](int64_t T) {
        long double t = (long double)T, total = 0, power = 1;
        for(int k = 0; k < 4; k++) { total += (long double)std::llabs(c[k]) * power; power *= t; }
        return total <= limit;
    };
    if(!fits(1)) return 0;
    int64_t low = 1, high = cap;
    while(low < high) {
        int64_t middle = low + (high - low + 1) / 2;
        if(fits(middle)) low = middle; else high = middle - 1;
    }
    return low;
}

bool fits_immediate(int64_t v) { return v >= INT32_MIN && v <= INT32_MAX; }

/** Replace one loop. Returns true if the function was changed (indexes are stale afterwards). */
bool replace_one(function_ir &ir, std::vector<int> &triedHeaders) {
    control_flow cfg;
    loop_forest forest;
    build_control_flow(ir, cfg);
    find_loops(ir, cfg, forest);
    if(!forest.valid) return false;

    int slots = ir.frameSlots > 0 ? ir.frameSlots : 1;
    std::vector<machine_state> known;
    liveness live;
    bool analyzed = false;

    for(loop_info &loop : forest.loops) {
        basic_block &header = cfg.blocks[loop.header];
        int headerId = ir.code[header.first].id;

        bool tried = false;
        for(int id : triedHeaders) tried = tried || id == headerId;
        if(tried) continue;
        triedHeaders.push_back(headerId);

        // ---- shape: [header: ... test ; jne exit] [body: ... ; jmp header]
        if(loop.blocks.size() != 2 || loop.latches.size() != 1) continue;
        int bodyBlock = loop.blocks[0] == loop.header ? loop.blocks[1] : loop.blocks[0];
        basic_block &body = cfg.blocks[bodyBlock];
        if(body.first != header.last + 1) continue;

        const ir_instruction &leave = ir.code[header.last];
        const ir_instruction &back = ir.code[body.last];
        if(leave.op != Opcode::JNE || back.op != Opcode::JMP) continue;

        int exitId = resolve_reference(ir, leave.target);
        int exitIndex = exitId >= 0 ? index_of(ir, exitId) : -1;
        if(exitIndex < 0 || loop.contains[cfg.blockOf[exitIndex]]) continue;

        // ---- entered at one place, by running into the header
        if(loop.entries.size() != 1 || header.first == 0) continue;
        basic_block &entry = cfg.blocks[loop.entries[0]];
        if(entry.last + 1 != header.first) continue;
        {
            const ir_instruction &last = ir.code[entry.last];
            if(instruction_info_of(last.op).flags & f_terminator) continue;
            if(last.target != IR_NONE && resolve_reference(ir, last.target) == headerId) continue;
        }

        if(!analyzed) {
            if(!analyze_known_values(ir, cfg, known)) return false;
            compute_liveness(ir, cfg, live);
            if(!live.localsKnown) return false;
            analyzed = true;
        }

        // ---- what is known on entry
        machine_state atEntry = known[loop.entries[0]];
        for(int i = entry.first; i <= entry.last; i++)
            apply_instruction(ir.code[i], atEntry);

        // ---- which locals does the loop write, which registers
        std::vector<char> written(slots, 0);
        register_set loopWrites = 0;
        bool plain = true, headerWritesLocal = false;

        for(int b : loop.blocks) {
            for(int i = cfg.blocks[b].first; i <= cfg.blocks[b].last; i++) {
                if(i == header.last || i == body.last) continue;    // the two jumps
                const ir_instruction &instr = ir.code[i];
                const instruction_info &info = instruction_info_of(instr.op);

                register_set reads, writes;
                register_effects(instr, reads, writes);
                loopWrites |= writes;

                const operand_kind kinds[3] = { info.a, info.b, info.c };
                const int32_t values[3] = { instr.a, instr.b, instr.c };
                for(int k = 0; k < 3; k++) {
                    if(kinds[k] == k_var_w || kinds[k] == k_var_rw) {
                        if(values[k] < 0 || values[k] >= slots) { plain = false; break; }
                        written[values[k]] = 1;
                        if(b == loop.header) headerWritesLocal = true;
                    }
                    if(kinds[k] == k_var_r && (values[k] < 0 || values[k] >= slots)) plain = false;
                }
            }
        }
        // (the header runs one more time than the body: it must only compute the test)
        if(!plain || headerWritesLocal) continue;

        // nothing after the loop, and nothing in the body, may rely on a register the loop computed
        register_set afterTest = live.registersAfter[header.last];
        if(afterTest & loopWrites) continue;

        // ---- one symbolic trip
        trip_state state;
        state.locals.resize(slots);
        for(int n = 0; n < slots; n++) {
            if(written[n]) state.locals[n] = variable(n);
            else if(is_known(atEntry.locals[n])) state.locals[n] = number(atEntry.locals[n].value);
            else { state.locals[n].kind = v_invariant; state.locals[n].local = n; }
        }

        bool ok = true;
        for(int i = header.first; i < header.last && ok; i++) ok = run(ir.code[i], state);
        if(!ok || !state.hasTest) continue;

        uint8_t testOp = state.testOp;
        symbolic left = state.testLeft, bound = state.testRight;

        for(int i = body.first; i < body.last && ok; i++) ok = run(ir.code[i], state);
        if(!ok) continue;

        // ---- the counter: the test compares exactly it with something that does not change
        if(left.kind != v_polynomial || left.p.constant != 0 || !left.p.quadratic.empty()
           || left.p.linear.size() != 1 || left.p.linear.begin()->second != 1) continue;
        int counter = left.p.linear.begin()->first;

        bool boundIsConstant = bound.kind == v_polynomial && bound.p.is_constant();
        if(!boundIsConstant && bound.kind != v_invariant) continue;
        if(boundIsConstant && !fits_immediate(bound.p.constant)) continue;

        if(!is_known(atEntry.locals[counter])) continue;
        int64_t start = atEntry.locals[counter].value;

        int64_t step = 0;
        {
            const symbolic &next = state.locals[counter];
            if(next.kind != v_polynomial || !next.p.quadratic.empty() || next.p.linear.size() != 1
               || next.p.linear.count(counter) == 0 || next.p.linear.at(counter) != 1) continue;
            step = next.p.constant;
        }
        bool countsUp = testOp == Opcode::LT || testOp == Opcode::LTE;
        if(step == 0 || (countsUp && step < 0) || (!countsUp && step > 0)) continue;
        int64_t stride = std::llabs(step);
        if(!fits_immediate(start) || !fits_immediate(stride)) continue;

        // ---- every other local the loop writes: an accumulator, or dead afterwards
        std::vector<final_value> finals;
        {
            final_value last;
            last.local = counter; last.divisor = 1; last.addStart = false;
            last.c[3] = last.c[2] = 0; last.c[1] = step; last.c[0] = start;        // i0 + c*T
            finals.push_back(last);
        }

        for(int n = 0; n < slots && ok; n++) {
            if(!written[n] || n == counter) continue;

            const symbolic &next = state.locals[n];
            bool accumulates = next.kind == v_polynomial && next.p.linear.count(n) && next.p.linear.at(n) == 1;

            // f = next - n must be a polynomial in the counter alone
            int64_t a0 = 0, a1 = 0, a2 = 0;
            if(accumulates) {
                a0 = next.p.constant;
                for(auto &term : next.p.linear) {
                    if(term.first == n) continue;
                    if(term.first == counter) a1 = term.second; else accumulates = false;
                }
                for(auto &term : next.p.quadratic) {
                    if(term.first.first == counter && term.first.second == counter) a2 = term.second;
                    else accumulates = false;
                }
            }

            if(!accumulates) {
                // not a sum: fine only if nobody looks at it after the loop
                // (inside the loop it was not read before being written, or the
                // accumulators that read it would have failed the test above)
                if(live.local_live_after(header.last, n)) ok = false;
                continue;
            }

            // coefficients of f on trip t, then of 6 * (sum over the trips), see the header comment
            // (long double: anything too large to be exact in it is far outside
            // the range accepted below, so it is rejected either way)
            long double A2 = (long double)a2, A1 = (long double)a1, A0 = (long double)a0;
            long double S = (long double)start, C = (long double)step;
            long double p2 = A2 * C * C;
            long double p1 = 2 * A2 * S * C + A1 * C;
            long double p0 = A2 * S * S + A1 * S + A0;
            long double c3 = 2 * p2, c2 = 3 * p1 - 3 * p2, c1 = 6 * p0 - 3 * p1 + p2, c0 = 0;
            int64_t divisor = 6;

            final_value sum;
            sum.local = n;
            sum.addStart = !is_known(atEntry.locals[n]);
            if(!sum.addStart) c0 = 6 * (long double)atEntry.locals[n].value;

            long double all[4] = { c0, c1, c2, c3 };
            for(int k = 0; k < 4 && ok; k++) {
                if(all[k] > (long double)INT32_MAX || all[k] < (long double)INT32_MIN) ok = false;
                else sum.c[k] = (int64_t)all[k];
            }
            if(!ok) break;

            // 6 is rarely needed in full: drop the factors every coefficient shares with it
            int64_t common = gcd(gcd(gcd(sum.c[0], sum.c[1]), gcd(sum.c[2], sum.c[3])), divisor);
            if(common > 1) { for(int k = 0; k < 4; k++) sum.c[k] /= common; divisor /= common; }
            sum.divisor = divisor;
            finals.push_back(sum);
        }
        if(!ok) continue;

        // ---- how many trips the formula is exact for
        bool runtimeStart = false;
        int64_t limit = INT32_MAX - 1;
        for(final_value &value : finals) {
            runtimeStart = runtimeStart || value.addStart;
            // (half the range when a run time starting value, itself limited to 2^50, is added on top)
            long double room = value.addStart ? EXACT_LIMIT / 4 : EXACT_LIMIT;
            limit = std::min(limit, largest_trip_count(value.c, room, INT32_MAX - 1));
        }
        if(limit < 16) continue;

        // ---- registers for the formula: free where the loop is entered and where it is left
        register_set busy = live.registersAfter[entry.last] | afterTest;
        if(busy & (REG_BIT(CMT) | REG_BIT(BMR))) continue;

        int scratch[3], found = 0;
        static const int order[] = { ECF, EDF, EHF, ECX, CX, EGX, EBX, ADX };
        for(int r : order) {
            if(found < 3 && !(busy & REG_BIT(r))) scratch[found++] = r;
        }
        if(found < 3) continue;
        int T = scratch[0], A = scratch[1], X = scratch[2];

        // ---- generate
        std::vector<ir_instruction> code;
        auto emit = [&](uint8_t op, int32_t a = 0, int32_t b = 0, int32_t c = 0) {
            code.push_back(make_instruction(ir, op, a, b, c));
        };
        auto bail_if_true = [&]() {                 // if(cmt) goto the original loop
            ir_instruction jump = make_instruction(ir, Opcode::JE, 0);
            jump.target = headerId;
            code.push_back(jump);
        };

        // accumulators with a run time starting value: it must be a whole number within +-2^50
        for(final_value &value : finals) {
            if(!value.addStart) continue;
            emit(Opcode::LOADL, X, value.local);
            emit(Opcode::MOV64, A, X);              // A = (int64) X
            emit(Opcode::TEST, A, X);               // bmr = (A == X)
            emit(Opcode::CMP, BMR, 0);              // cmt = (bmr == 0): it has a fraction (or is NaN / huge)
            bail_if_true();
            emit(Opcode::MOVI, A, 1 << 25);
            emit(Opcode::MUL, A, A, A);             // A = 2^50
            emit(Opcode::GT, X, A);
            bail_if_true();
            emit(Opcode::IMUL, A, -1);              // A = -2^50
            emit(Opcode::LT, X, A);
            bail_if_true();
        }

        // X = distance to the bound, in strides
        if(boundIsConstant) emit(Opcode::MOVI, X, (int32_t)bound.p.constant);
        else                emit(Opcode::LOADL, X, bound.local);
        if(countsUp) {
            if(start != 0) emit(Opcode::ISUB, X, (int32_t)start);               // X = n - i0
        } else {
            emit(Opcode::MOVI, A, (int32_t)start);
            emit(Opcode::SUB, A, X, X);                                         // X = i0 - n
        }
        if(stride != 1) emit(Opcode::IDIV, X, (int32_t)stride);

        // T = number of trips
        if(testOp == Opcode::LT || testOp == Opcode::GT) {
            emit(Opcode::MOV64, T, X);              // T = X without its fraction
            emit(Opcode::LT, T, X);                 // cmt = 1 if there was a fraction (X > 0 here, or T < 1 below)
            emit(Opcode::ADD, T, CMT, T);           // T = ceil(X)
        } else {
            emit(Opcode::MOVI, A, 0);
            emit(Opcode::LT, X, A);                 // already past the bound: no trips, let the loop find out
            bail_if_true();
            emit(Opcode::MOV64, T, X);
            emit(Opcode::IADD, T, 1);               // T = floor(X) + 1
        }

        // 1 <= T <= limit, else the original loop (this also catches a NaN bound)
        emit(Opcode::MOVI, A, 1);
        emit(Opcode::LT, T, A);
        bail_if_true();
        emit(Opcode::MOVI, A, (int32_t)limit);
        emit(Opcode::GT, T, A);
        bail_if_true();

        // Double-check T with the loop's own comparison, on exact whole numbers:
        // the counter after T trips must fail the test, the one before must pass.
        // (T above came out of a subtraction and a division, which can round when
        // the bound has a fraction; this makes the trip count right regardless.)
        emit(Opcode::MOVI, A, (int32_t)step);
        emit(Opcode::MUL, A, T, A);
        if(start != 0) emit(Opcode::IADD, A, (int32_t)start);                  // A = counter after T trips
        if(boundIsConstant) emit(Opcode::MOVI, X, (int32_t)bound.p.constant);
        else                emit(Opcode::LOADL, X, bound.local);
        emit(testOp, A, X);
        bail_if_true();                                                         // would go round again
        emit(Opcode::ISUB, A, (int32_t)step);                                   // A = counter on the last trip
        emit(testOp, A, X);
        {
            ir_instruction jump = make_instruction(ir, Opcode::JNE, 0);         // the last trip does not happen
            jump.target = headerId;
            code.push_back(jump);
        }

        // the values after the loop, each as a polynomial in T (Horner form)
        for(final_value &value : finals) {
            int top = 3;
            while(top > 0 && value.c[top] == 0) top--;

            emit(Opcode::MOVI, A, (int32_t)value.c[top]);
            for(int k = top - 1; k >= 0; k--) {
                emit(Opcode::MUL, A, T, A);
                if(value.c[k] != 0) emit(Opcode::IADD, A, (int32_t)value.c[k]);
            }
            if(value.divisor != 1) emit(Opcode::IDIV, A, (int32_t)value.divisor);
            if(value.addStart) {
                emit(Opcode::LOADL, X, value.local);
                emit(Opcode::ADD, X, A, A);
            }
            emit(Opcode::SMOVR_2, A, value.local);
        }

        ir_instruction done = make_instruction(ir, Opcode::JMP, 0);
        done.target = exitId;
        code.push_back(done);

        // In front of the loop. The loop's own jump back still targets its header,
        // which is now behind this code: the formula is tried once, on entry.
        ir.code.insert(ir.code.begin() + header.first, code.begin(), code.end());
        ir.changed = true;
        return true;
    }

    return false;
}

} // namespace

bool pass_loop_closed_form(function_ir &ir) {
    if(!ir.failure.empty() || !ir.tries.empty()) return false;

    bool changed = false;
    std::vector<int> triedHeaders;
    // one loop per call: the blocks move every time
    for(int round = 0; round < 32 && ir.code.size() < MAX_FUNCTION_INSTRUCTIONS && replace_one(ir, triedHeaders); round++)
        changed = true;

    return changed;
}
