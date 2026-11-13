//
// Sharp bytecode optimizer - instruction set description (the table itself)
// See instruction_set.h. The comment beside each row is what the interpreter
// (lib/runtime/virtual_machine.cpp, main_vm_loop) does for that opcode.
//

#include "instruction_set.h"

static instruction_info table[256];
static bool tableReady = false;

// short names so the rows stay readable
#define TH f_throws
#define SE f_side_effect
#define RP f_reads_ptr
#define WP f_writes_ptr
#define RH f_reads_heap
#define SC f_state_check
#define NEW (f_throws | f_allocates | f_state_check)
#define R(reg) REG_BIT(reg)

static void row(uint8_t op, const char *name, operand_layout layout,
                operand_kind a, operand_kind b, operand_kind c,
                register_set implicitReads, register_set implicitWrites,
                int popsNumbers, int popsObjects, int pushesNumbers, int pushesObjects,
                uint32_t flags) {
    instruction_info &i = table[op];
    i.name = name; i.layout = layout;
    i.a = a; i.b = b; i.c = c;
    i.implicitReads = implicitReads; i.implicitWrites = implicitWrites;
    i.popsNumbers = (int8_t)popsNumbers; i.popsObjects = (int8_t)popsObjects;
    i.pushesNumbers = (int8_t)pushesNumbers; i.pushesObjects = (int8_t)pushesObjects;
    i.flags = flags;
}

// the common shapes
#define PLAIN(op, name, layout, a, b, c, flags)               row(Opcode::op, name, layout, a, b, c, 0, 0, 0, 0, 0, 0, flags)
#define IMPLICIT(op, name, layout, a, b, c, reads, writes, flags) row(Opcode::op, name, layout, a, b, c, reads, writes, 0, 0, 0, 0, flags)
#define STACK(op, name, layout, a, b, c, pn, po, un, uo, flags) row(Opcode::op, name, layout, a, b, c, 0, 0, pn, po, un, uo, flags)

static void build_table() {
    for(auto &entry : table) entry = instruction_info{ nullptr, layout_none, k_none, k_none, k_none, 0, 0, 0, 0, 0, 0, 0 };

    PLAIN(NOP, "nop", layout_none, k_none, k_none, k_none, f_none);                     // nothing
    PLAIN(INT, "int", layout_d, k_int, k_none, k_none,                                  // exec_interrupt(a): can do anything
          f_opaque | f_call | TH | SE | SC);
    PLAIN(MOVI, "movi", layout_d_raw, k_reg_w, k_int, k_none, f_none);                  // regs[a] = b
    PLAIN(DMOVI, "dmovi", layout_d_raw, k_reg_w, k_int, k_none, f_none);
    PLAIN(RET, "ret", layout_d, k_int, k_none, k_none, f_terminator | SE);              // return (a == 1: with the exception on the stack)
    PLAIN(HLT, "hlt", layout_none, k_none, k_none, k_none, f_terminator | SE);          // stop the thread

    STACK(NEWARRAY, "newarray", layout_c, k_reg_r, k_int, k_none, 0, 0, 0, 1, NEW);     // push new number array [regs[a]] of type b
    PLAIN(CAST, "cast", layout_d, k_int, k_none, k_none, RP | RH | TH);                 // cast_object(ptr, a)
    PLAIN(VARCAST, "varcast", layout_c, k_int, k_int, k_none, RP | RH | TH | SE);       // cast_numeric_array(ptr, a)

    PLAIN(MOV8,  "mov8",  layout_c, k_reg_w, k_reg_r, k_none, f_none);                  // regs[a] = (int8_t)regs[b]
    PLAIN(MOV16, "mov16", layout_c, k_reg_w, k_reg_r, k_none, f_none);
    PLAIN(MOV32, "mov32", layout_c, k_reg_w, k_reg_r, k_none, f_none);
    PLAIN(MOV64, "mov64", layout_c, k_reg_w, k_reg_r, k_none, f_none);
    PLAIN(D2I, "d2i", layout_c, k_reg_w, k_reg_r, k_none, f_none);
    PLAIN(I2D, "i2d", layout_c, k_reg_w, k_reg_r, k_none, f_none);
    PLAIN(MOVU8,  "movu8",  layout_c, k_reg_w, k_reg_r, k_none, f_none);
    PLAIN(MOVU16, "movu16", layout_c, k_reg_w, k_reg_r, k_none, f_none);
    PLAIN(MOVU32, "movu32", layout_c, k_reg_w, k_reg_r, k_none, f_none);
    PLAIN(MOVU64, "movu64", layout_c, k_reg_w, k_reg_r, k_none, f_none);

    STACK(RSTORE, "rstore", layout_d, k_reg_r, k_none, k_none, 0, 0, 1, 0, f_none);     // push regs[a]

    PLAIN(ADD, "add", layout_b, k_reg_r, k_reg_r, k_reg_w, f_none);                     // regs[c] = regs[a] + regs[b]
    PLAIN(DADD, "dadd", layout_b, k_reg_r, k_reg_r, k_reg_w, f_none);
    PLAIN(SUB, "sub", layout_b, k_reg_r, k_reg_r, k_reg_w, f_none);
    PLAIN(DSUB, "dsub", layout_b, k_reg_r, k_reg_r, k_reg_w, f_none);
    PLAIN(MUL, "mul", layout_b, k_reg_r, k_reg_r, k_reg_w, f_none);
    PLAIN(DMUL, "dmul", layout_b, k_reg_r, k_reg_r, k_reg_w, f_none);
    PLAIN(DDIV, "ddiv", layout_b, k_reg_r, k_reg_r, k_reg_w, f_none);
    PLAIN(MOD, "mod", layout_b, k_reg_r, k_reg_r, k_reg_w, TH);                         // integer %: traps on a zero divisor
    PLAIN(IADD, "iadd", layout_d_raw, k_reg_rw, k_int, k_none, f_none);                 // regs[a] += b
    PLAIN(DIADD, "diadd", layout_d_raw, k_reg_rw, k_int, k_none, f_none);
    PLAIN(ISUB, "isub", layout_d_raw, k_reg_rw, k_int, k_none, f_none);
    PLAIN(DISUB, "disub", layout_d_raw, k_reg_rw, k_int, k_none, f_none);
    PLAIN(IMUL, "imul", layout_d_raw, k_reg_rw, k_int, k_none, f_none);
    PLAIN(DIMUL, "dimul", layout_d_raw, k_reg_rw, k_int, k_none, f_none);
    PLAIN(DIDIV, "didiv", layout_d_raw, k_reg_rw, k_int, k_none, f_none);
    PLAIN(IMOD, "imod", layout_d_raw, k_reg_rw, k_int, k_none, TH);

    STACK(POP, "pop", layout_none, k_none, k_none, k_none, 0, 1, 0, 0, f_none);         // pop an object (the slot is cleared)
    PLAIN(INC, "inc", layout_d, k_reg_rw, k_none, k_none, f_none);                      // regs[a]++
    PLAIN(DINC, "dinc", layout_d, k_reg_rw, k_none, k_none, f_none);
    PLAIN(DEC, "dec", layout_d, k_reg_rw, k_none, k_none, f_none);
    PLAIN(DDEC, "ddec", layout_d, k_reg_rw, k_none, k_none, f_none);
    PLAIN(MOVR, "movr", layout_c, k_reg_w, k_reg_r, k_none, f_none);                    // regs[a] = regs[b]

    IMPLICIT(BRH, "brh", layout_none, k_none, k_none, k_none, R(ADX), 0,                // pc = regs[ADX]
             f_indirect_jump | f_terminator | SC);
    IMPLICIT(IFE, "ife", layout_none, k_none, k_none, k_none, R(CMT) | R(ADX), 0,       // if(regs[CMT]) pc = regs[ADX]
             f_indirect_jump | f_cond_jump | SC);
    IMPLICIT(IFNE, "ifne", layout_none, k_none, k_none, k_none, R(CMT) | R(ADX), 0,     // if(!regs[CMT]) pc = regs[ADX]
             f_indirect_jump | f_cond_jump | SC);

    IMPLICIT(LT,  "lt",  layout_c, k_reg_r, k_reg_r, k_none, 0, R(CMT), f_none);        // regs[CMT] = regs[a] < regs[b]
    IMPLICIT(DLT,  "dlt",  layout_c, k_reg_r, k_reg_r, k_none, 0, R(CMT), f_none);
    IMPLICIT(GT,  "gt",  layout_c, k_reg_r, k_reg_r, k_none, 0, R(CMT), f_none);
    IMPLICIT(DGT,  "dgt",  layout_c, k_reg_r, k_reg_r, k_none, 0, R(CMT), f_none);
    IMPLICIT(LTE, "lte", layout_c, k_reg_r, k_reg_r, k_none, 0, R(CMT), f_none);
    IMPLICIT(DLTE, "dlte", layout_c, k_reg_r, k_reg_r, k_none, 0, R(CMT), f_none);
    IMPLICIT(GTE, "gte", layout_c, k_reg_r, k_reg_r, k_none, 0, R(CMT), f_none);
    IMPLICIT(DGTE, "dgte", layout_c, k_reg_r, k_reg_r, k_none, 0, R(CMT), f_none);

    PLAIN(MOVL, "movl", layout_d, k_slot_addr, k_none, k_none, WP);                     // ptr = &(fp+a)->obj
    STACK(POPL, "popl", layout_d, k_obj_w, k_none, k_none, 0, 1, 0, 0, f_none);         // (fp+a)->obj = pop object
    STACK(IPOPL, "ipopl", layout_d, k_var_w, k_none, k_none, 1, 0, 0, 0, f_none);       // (fp+a)->var = pop number
    PLAIN(MOVSL, "movsl", layout_d, k_sp_rel, k_none, k_none, WP);                      // ptr = &(sp+a)->obj
    PLAIN(SIZEOF, "sizeof", layout_d, k_reg_w, k_none, k_none, RP | RH);                // regs[a] = ptr ? ptr->o->size : 0
    PLAIN(PUT, "put", layout_d, k_reg_r, k_none, k_none, SE);                           // print number
    PLAIN(DPUT, "dput", layout_d, k_reg_r, k_none, k_none, SE);
    PLAIN(PUTC, "putc", layout_d, k_reg_r, k_none, k_none, SE);                         // print char
    IMPLICIT(GET, "get", layout_d, k_reg_w, k_none, k_none, R(CMT), 0, SE);             // read a key
    PLAIN(CHECKLEN, "checklen", layout_d, k_reg_r, k_none, k_none, RP | RH | TH);       // bounds check regs[a] against ptr->o->size

    PLAIN(JMP, "jmp", layout_d, k_pc, k_none, k_none, f_jump | f_terminator | SC);      // pc = a
    PLAIN(LOADPC, "loadpc", layout_d, k_reg_w, k_none, k_none, f_reads_pc);             // regs[a] = current pc
    STACK(PUSHOBJ, "pushobj", layout_none, k_none, k_none, k_none, 0, 0, 0, 1, RP | RH);// push *ptr
    PLAIN(DEL, "del", layout_none, k_none, k_none, k_none, RP | SE | TH);               // *ptr = null
    PLAIN(CALL, "call", layout_d, k_method, k_none, k_none, f_call | TH | SE | SC);     // call function a
    PLAIN(CALLD, "calld", layout_d, k_int, k_none, k_none, f_call | TH | SE | SC);      // call the function value under a arguments
    STACK(NEWCLASS, "newclass", layout_raw, k_class, k_none, k_none, 0, 0, 0, 1, NEW);  // push new a()
    PLAIN(MOVN, "movn", layout_raw, k_field, k_none, k_none, RP | WP | RH | TH);        // ptr = &ptr->o->node[a]
    PLAIN(SLEEP, "sleep", layout_d, k_reg_r, k_none, k_none, SE);
    IMPLICIT(TEST, "test", layout_c, k_reg_r, k_reg_r, k_none, 0, R(BMR), f_none);      // regs[BMR] = regs[a] == regs[b]
    IMPLICIT(DTEST, "dtest", layout_c, k_reg_r, k_reg_r, k_none, 0, R(BMR), f_none);
    IMPLICIT(TNE,  "tne",  layout_c, k_reg_r, k_reg_r, k_none, 0, R(BMR), f_none);      // regs[BMR] = regs[a] != regs[b]
    IMPLICIT(DTNE,  "dtne",  layout_c, k_reg_r, k_reg_r, k_none, 0, R(BMR), f_none);
    PLAIN(LOCK, "lock", layout_none, k_none, k_none, k_none, RP | RH | SE | TH | SC);   // lock ptr->o (may yield and retry)
    PLAIN(ULOCK, "ulock", layout_none, k_none, k_none, k_none, RP | RH | SE | TH);
    PLAIN(MOVG, "movg", layout_d, k_static, k_none, k_none, WP);                        // ptr = &staticHeap[a]
    PLAIN(MOVND, "movnd", layout_d, k_reg_r, k_none, k_none, RP | WP | RH | TH);        // ptr = &ptr->o->node[regs[a]]
    STACK(NEWOBJARRAY, "newobjarray", layout_d, k_reg_r, k_none, k_none, 0, 0, 0, 1, NEW);
    PLAIN(NOT, "not", layout_c, k_reg_w, k_reg_r, k_none, f_none);                      // regs[a] = !regs[b]
    PLAIN(DNOT, "dnot", layout_c, k_reg_w, k_reg_r, k_none, f_none);
    PLAIN(SKIP, "skip", layout_d, k_pc_rel, k_none, k_none, f_jump | f_terminator);     // pc += a + 1
    STACK(LOADVAL, "loadval", layout_d, k_reg_w, k_none, k_none, 1, 0, 0, 0, f_none);   // regs[a] = pop number
    PLAIN(SHL, "shl", layout_b, k_reg_r, k_reg_r, k_reg_w, f_none);                     // regs[c] = (Int)regs[a] << (Int)regs[b]
    PLAIN(SHR, "shr", layout_b, k_reg_r, k_reg_r, k_reg_w, f_none);
    PLAIN(SKPE, "skpe", layout_c, k_reg_r, k_pc_rel, k_none, f_cond_jump | SC);         // if(regs[a]) pc += b
    PLAIN(SKNE, "skne", layout_c, k_reg_r, k_pc_rel, k_none, f_cond_jump | SC);         // if(!regs[a]) pc += b
    IMPLICIT(CMP, "cmp", layout_d_raw, k_reg_r, k_int, k_none, 0, R(CMT), f_none);      // regs[CMT] = regs[a] == b
    IMPLICIT(DCMP, "dcmp", layout_d_raw, k_reg_r, k_int, k_none, 0, R(CMT), f_none);
    IMPLICIT(AND,  "and",  layout_c, k_reg_r, k_reg_r, k_none, 0, R(BMR), f_none);      // regs[BMR] = regs[a] && regs[b]
    IMPLICIT(UAND, "uand", layout_c, k_reg_r, k_reg_r, k_none, 0, R(BMR), f_none);      // regs[BMR] = (Int)regs[a] & (Int)regs[b]
    IMPLICIT(OR,   "or",   layout_c, k_reg_r, k_reg_r, k_none, 0, R(BMR), f_none);
    IMPLICIT(XOR,  "xor",  layout_c, k_reg_r, k_reg_r, k_none, 0, R(BMR), f_none);
    STACK(THROW, "throw", layout_none, k_none, k_none, k_none, 0, 1, 0, 0,              // throw the popped object
          f_terminator | TH | SE);
    PLAIN(CHECKNULL, "checknull", layout_d, k_reg_w, k_none, k_none, RP | RH);          // regs[a] = ptr == null || *ptr == null
    PLAIN(RETURNOBJ, "returnobj", layout_none, k_none, k_none, k_none, RP | RH | SE);   // fp->obj = *ptr   (the return slot)
    STACK(NEWCLASSARRAY, "newclassarray", layout_d_raw, k_reg_r, k_class, k_none, 0, 0, 0, 1, NEW);
    STACK(NEWSTRING, "newstring", layout_d, k_string, k_none, k_none, 0, 0, 0, 1, NEW);

    PLAIN(ADDL, "addl", layout_c, k_reg_r, k_var_rw, k_none, f_none);                   // (fp+b)->var += regs[a]
    PLAIN(DADDL, "daddl", layout_c, k_reg_r, k_var_rw, k_none, f_none);
    PLAIN(SUBL, "subl", layout_c, k_reg_r, k_var_rw, k_none, f_none);
    PLAIN(DSUBL, "dsubl", layout_c, k_reg_r, k_var_rw, k_none, f_none);
    PLAIN(MULL, "mull", layout_c, k_reg_r, k_var_rw, k_none, f_none);
    PLAIN(DMULL, "dmull", layout_c, k_reg_r, k_var_rw, k_none, f_none);
    PLAIN(DDIVL, "ddivl", layout_c, k_reg_r, k_var_rw, k_none, f_none);
    PLAIN(MODL, "modl", layout_c, k_reg_r, k_var_rw, k_none, TH);
    PLAIN(IADDL, "iaddl", layout_d_raw, k_var_rw, k_int, k_none, f_none);               // (fp+a)->var += b
    PLAIN(DIADDL, "diaddl", layout_d_raw, k_var_rw, k_int, k_none, f_none);
    PLAIN(ISUBL, "isubl", layout_d_raw, k_var_rw, k_int, k_none, f_none);
    PLAIN(DISUBL, "disubl", layout_d_raw, k_var_rw, k_int, k_none, f_none);
    PLAIN(IMULL, "imull", layout_d_raw, k_var_rw, k_int, k_none, f_none);
    PLAIN(DIMULL, "dimull", layout_d_raw, k_var_rw, k_int, k_none, f_none);
    PLAIN(DIDIVL, "didivl", layout_d_raw, k_var_rw, k_int, k_none, f_none);
    PLAIN(IMODL, "imodl", layout_d_raw, k_var_rw, k_int, k_none, TH);
    PLAIN(LOADL, "loadl", layout_c, k_reg_w, k_var_r, k_none, f_none);                  // regs[a] = (fp+b)->var
    PLAIN(GETF, "getf", layout_c, k_reg_w, k_int, k_none, RP | RH | TH);                  // regs[a] = ptr->o.<number field b>
    PLAIN(ALOAD64, "aload64", layout_c, k_reg_w, k_reg_r, k_none, RP | RH | TH);
    PLAIN(ALOADU32, "aloadu32", layout_c, k_reg_w, k_reg_r, k_none, RP | RH | TH);
    PLAIN(ALOAD32, "aload32", layout_c, k_reg_w, k_reg_r, k_none, RP | RH | TH);
    PLAIN(ALOADU16, "aloadu16", layout_c, k_reg_w, k_reg_r, k_none, RP | RH | TH);
    PLAIN(ALOAD16, "aload16", layout_c, k_reg_w, k_reg_r, k_none, RP | RH | TH);
    PLAIN(ALOADU8, "aloadu8", layout_c, k_reg_w, k_reg_r, k_none, RP | RH | TH);
    PLAIN(ALOAD8, "aload8", layout_c, k_reg_w, k_reg_r, k_none, RP | RH | TH);
    STACK(POPOBJ, "popobj", layout_none, k_none, k_none, k_none, 0, 1, 0, 0, RP | SE | TH); // *ptr = pop object
    PLAIN(SMOVR, "smovr", layout_c, k_reg_r, k_sp_rel, k_none, f_none);                 // (sp+b)->var = regs[a]
    PLAIN(ANDL, "andl", layout_c, k_reg_r, k_var_rw, k_none, f_none);
    PLAIN(ORL,  "orl",  layout_c, k_reg_r, k_var_rw, k_none, f_none);
    PLAIN(XORL, "xorl", layout_c, k_reg_r, k_var_rw, k_none, f_none);
    PLAIN(ASTORE64, "astore64", layout_c, k_reg_r, k_reg_r, k_none, RP | RH | SE | TH);
    PLAIN(ASTORE32, "astore32", layout_c, k_reg_r, k_reg_r, k_none, RP | RH | SE | TH);
    PLAIN(ASTORE16, "astore16", layout_c, k_reg_r, k_reg_r, k_none, RP | RH | SE | TH);
    PLAIN(ASTORE8, "astore8", layout_c, k_reg_r, k_reg_r, k_none, RP | RH | SE | TH);
    PLAIN(SMOV, "smov", layout_c, k_reg_w, k_sp_rel, k_none, f_none);                   // regs[a] = (sp+b)->var
    PLAIN(RETURNVAL, "returnval", layout_d, k_reg_r, k_none, k_none, SE);               // fp->var = regs[a]   (the return slot)
    STACK(ISTORE, "istore", layout_raw, k_int, k_none, k_none, 0, 0, 1, 0, f_none);     // push a
    STACK(DISTORE, "distore", layout_raw, k_int, k_none, k_none, 0, 0, 1, 0, f_none);
    PLAIN(SMOVR_2, "smovr_2", layout_c, k_reg_r, k_var_w, k_none, f_none);              // (fp+b)->var = regs[a]
    PLAIN(PUTF, "putf", layout_c, k_reg_r, k_int, k_none, RP | RH | SE | TH);             // ptr->o.<number field b> = regs[a]
    PLAIN(ISTOREL, "istorel", layout_d_raw, k_var_w, k_int, k_none, f_none);            // (fp+a)->var = b
    PLAIN(DISTOREL, "distorel", layout_d_raw, k_var_w, k_int, k_none, f_none);
    STACK(PUSHNULL, "pushnull", layout_none, k_none, k_none, k_none, 0, 0, 0, 1, f_none);
    STACK(IPUSHL, "ipushl", layout_d, k_var_r, k_none, k_none, 0, 0, 1, 0, f_none);     // push (fp+a)->var
    STACK(PUSHL, "pushl", layout_d, k_obj_r, k_none, k_none, 0, 0, 0, 1, f_none);       // push (fp+a)->obj
    STACK(ITEST, "itest", layout_d, k_reg_w, k_none, k_none, 0, 2, 0, 0, f_none);       // regs[a] = pop object == pop object
    PLAIN(INVOKE_DELEGATE, "invoke_delegate", layout_d_c, k_method, k_int, k_int,       // call delegate a (b = static, c = arg count)
          f_call | TH | SE | SC);
    PLAIN(ISADD, "isadd", layout_d_raw, k_sp_rel, k_int, k_none, f_none);               // (sp+a)->var += b
    PLAIN(DISADD, "disadd", layout_d_raw, k_sp_rel, k_int, k_none, f_none);
    IMPLICIT(JE,  "je",  layout_d, k_pc, k_none, k_none, R(CMT), 0, f_cond_jump | SC);  // if(regs[CMT]) pc = a
    IMPLICIT(JNE, "jne", layout_d, k_pc, k_none, k_none, R(CMT), 0, f_cond_jump | SC);  // if(!regs[CMT]) pc = a
    PLAIN(TLS_MOVL, "tls_movl", layout_d, k_abs_slot, k_none, k_none, WP);              // ptr = &(stack+a)->obj
    IMPLICIT(MOV_ABS, "mov_abs", layout_d, k_abs_slot, k_none, k_none, R(EBX), 0, SE);  // (stack+a)->var = regs[EBX]
    IMPLICIT(LOAD_ABS, "load_abs", layout_d, k_abs_slot, k_none, k_none, 0, R(EBX), RH);// regs[EBX] = (stack+a)->var
    STACK(DUP, "dup", layout_none, k_none, k_none, k_none, 0, 0, 0, 1, f_none);         // push a copy of the top object
    STACK(POPOBJ_2, "popobj_2", layout_none, k_none, k_none, k_none, 0, 1, 0, 0, WP);   // ptr = &popped slot
    PLAIN(SWAP, "swap", layout_none, k_none, k_none, k_none, TH);                       // swap the two top stack items
    PLAIN(LDC, "ldc", layout_c, k_reg_w, k_constant, k_none, f_none);                   // regs[a] = constants[b]
    PLAIN(SMOVR_3, "smovr_3", layout_d, k_obj_w, k_none, k_none, RP | RH);              // (fp+a)->obj = *ptr
    PLAIN(NEG, "neg", layout_c, k_reg_w, k_reg_r, k_none, f_none);                      // regs[a] = -regs[b]
    PLAIN(DNEG, "dneg", layout_c, k_reg_w, k_reg_r, k_none, f_none);
    PLAIN(DEXP, "dexp", layout_b, k_reg_r, k_reg_r, k_reg_w, f_none);                     // regs[c] = pow(regs[a], regs[b])
    PLAIN(IS, "is", layout_d_raw, k_reg_w, k_int, k_none, RP | RH);                     // regs[a] = is_type(ptr, b)
    PLAIN(SMOVR_4, "smovr_4", layout_d_raw, k_var_w, k_var_r, k_none, f_none);          // (fp+a)->var = (fp+b)->var

    tableReady = true;
}

const instruction_info& instruction_info_of(uint8_t opcode) {
    if(!tableReady) build_table();
    return table[opcode];
}
