//
// System interrupts for math and random numbers.
//
// Moved verbatim from the old switch in virtual_machine.cpp (OP_MATH and the
// OP_RANDOM_* family). The string-to-number parsers (strtod, strtol) live in
// console_io.cpp. See interrupt_internal.h for what a handler is.
//
#include "interrupt_internal.h"

//
// math(x, which [, y]): one math function (sin, sqrt, pow, atan2 ...). `which`
// (ECX) picks the function; the operand(s) are decimals in EBX (and EGX for the
// two-operand functions). The decimal result goes in CMT.
//
void op_math(Int interrupt) {
    registers[CMT].d = __cmath(registers[EBX].d, registers[EGX].d, (int)registers[ECX].i);
}

// random_int(): any whole number. Result in EBX.
void op_random_int(Int interrupt) {
    registers[EBX].i = randInt();
}

// random_double(): a decimal in [0, 1). Result in EBX.
void op_random_double(Int interrupt) {
    registers[EBX].d = randDouble();
}

// random_int_ranged(lo, hi): a whole number in [lo, hi]. lo in EBX, hi in EGX; result in EBX.
void op_random_int_ranged(Int interrupt) {
    registers[EBX].i = randInt((Int)registers[EBX].i, (Int)registers[EGX].i);
}

// random_double_ranged(lo, hi): a decimal in [lo, hi). Result in EBX.
void op_random_double_ranged(Int interrupt) {
    registers[EBX].d = randDouble(registers[EBX].d, registers[EGX].d);
}

// random_seed(seed): fix the sequence of the random numbers (ADX), for repeatable runs.
void op_random_seed(Int interrupt) {
    setSeed(registers[ADX].i);
}
