#include <iostream>
#include "stdimports.h"
#include "lib/util/time.h"

Sharp versions;
#ifdef MAKE_COMPILER
    #include "lib/grammar/main.h"
#endif
#ifndef MAKE_COMPILER
    #include "lib/runtime/main.h"
#include "lib/runtime/VirtualMachine.h"

#endif

int main(int argc, const char* argv[]) {
    uint64_t past= Clock::realTimeInNSecs(),now;
    Int result = 0;

#ifdef MAKE_COMPILER
    _bootstrap( argc, argv );
#endif
#ifndef MAKE_COMPILER
    result = runtimeStart( argc, argv );
#endif
    int declOp = Opcode::POPL;
    opcode_arg declArg1 = -127;
    opcode_arg declArg2 = -127;
    opcode_arg declArg3 = -127;

    int tmpInstr= 0;
    int instr = 0, opcode=0, arg1=0, arg2=0,arg3=0;
    instr = SET_Bi(tmpInstr, declOp, abs(declArg1), NEGATIVE, abs(declArg2), NEGATIVE, abs(declArg3), NEGATIVE);
    opcode = GET_OP(instr);
    arg1 = GET_Ba(instr);
    arg2 = GET_Bb(instr);
    arg3 = GET_Bc(instr);

    if(opcode == declOp) cout << "opcode is correct" << endl;
    if(arg1 == declArg1) cout << "arg1 is correct" << endl;
    if(arg2 == declArg2) cout << "arg2 is correct" << endl;
    if(arg3 == declArg3) cout << "arg3 is correct" << endl;

    instr = 0;

    now= Clock::realTimeInNSecs();
    if(c_options.debugMode)
        cout << endl << "Compiled in " << NANO_TOMICRO(now-past) << "us & "
             << NANO_TOMILL(now-past) << "ms\n";

    if(c_options.debugMode)
        cout << endl << "program exiting..." << endl;
    return result;
}