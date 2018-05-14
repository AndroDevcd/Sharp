//
// Created by bknun on 5/13/2018.
//

#include "Optimizer.h"
#include "../runtime/Opcode.h"

void Optimizer::readjustAddresses(unsigned int stopAddr) {
    // TODO: do line_table as well
    for(unsigned int i = 0; i < func->exceptions.size(); i++) {
        ExceptionTable &et = func->exceptions.get(i);

        if(stopAddr <= et.start_pc)
            et.start_pc--;
        if(stopAddr <= et.end_pc)
            et.end_pc--;
        if(stopAddr <= et.handler_pc)
            et.handler_pc--;
    }
    for(unsigned int i = 0; i < func->finallyBlocks.size(); i++) {
        FinallyTable &ft = func->finallyBlocks.get(i);

        if(stopAddr <= ft.start_pc)
            ft.start_pc--;
        if(stopAddr <= ft.end_pc)
            ft.end_pc--;
    }

    int64_t x64, op, addr;
    for(unsigned int i = 0; i < stopAddr; i++) {
        x64 = assembler->__asm64.get(i);

        switch (op=GET_OP(x64)) {
            case op_SKPE:
            case op_SKNE:
            case op_GOTO:
                addr=GET_Da(x64);

                /*
                 * We only want to update data which is referencing data below us
                 */
                if(addr > stopAddr)
                {
                    // update address
                    assembler->__asm64.replace(i, SET_Di(x64, op, --addr));
                }
                break;
            case op_MOVI:
                if(unique_addr_lst.find(i)) {
                    addr=GET_Da(x64);
                    unique_addr_lst.replace(unique_addr_lst.indexof(i), i-1);

                    /*
                     * We only want to update data which is referencing data below us
                     */
                    if(addr <= stopAddr)
                        assembler->__asm64.replace(i, SET_Di(x64, op_MOVI, addr));
                    else {
                        assembler->__asm64.replace(i, SET_Di(x64, op_MOVI, addr-1));
                    }

                    i++;
                }
                break;
        }
    }

    for(long long i = stopAddr; i < assembler->__asm64.size(); i++) {
        x64 = assembler->__asm64.get(i);

        switch (op=GET_OP(x64)) {
            case op_SKPE:
            case op_SKNE:
            case op_GOTO:
                addr=GET_Da(x64);

                /*
                 * We only want to update data which is referencing data below us
                 */
                if(addr > stopAddr)
                {
                    // update address
                    assembler->__asm64.replace(i, SET_Di(x64, op, addr-1));
                }
                break;
            case op_MOVI:
                if(unique_addr_lst.find(i+1)) {
                    addr=GET_Da(x64);
                    unique_addr_lst.replace(unique_addr_lst.indexof(i+1), i);

                    /*
                     * We only want to update data which is referencing data below us
                     */
                    if(addr <= stopAddr) {
                        assembler->__asm64.replace(i, SET_Di(x64, op_MOVI, addr - 2));
                    } else
                        assembler->__asm64.replace(i, SET_Di(x64, op_MOVI, addr-1));
                    i++;
                }
                break;
        }
    }
}

/**
 * [0x2] 2:	movl 2
 * [0x4] 4:	popobj
 */
void Optimizer::optimizeLocalPops() {
    int64_t x64, local;
    readjust:
    for(unsigned int i = 0; i < assembler->size(); i++) {
        x64 = assembler->__asm64.get(i);

        switch (GET_OP(x64)) {
            case op_MOVL:
                i++;

                if(GET_OP(assembler->__asm64.get(i)) == op_POPOBJ) {
                    local = GET_Da(x64);

                    assembler->__asm64.remove(i);
                    readjustAddresses(i+1);
                    assembler->__asm64.replace(--i, SET_Di(x64, op_POPL, local));

                    optimizedOpcodes++;
                    goto readjust;
                }
                break;
        }
    }
}

void Optimizer::optimize(Method *method) {
    this->assembler = &method->code;
    this->unique_addr_lst.addAll(method->unique_address_table);
    this->func=method;
    optimizedOpcodes=0;

    if(method->code.size()==0)
        return;

    optimizeLocalPops();
    optimizeRedundantMovICall();
}

/**
 * [0x2] 2:	movi #10000000, ebx
 * [0x4] 4:	movr egx, ebx
 */
void Optimizer::optimizeRedundantMovICall() {
    int64_t x64, reg1, reg2, reg3;
    readjust:
    for(unsigned int i = 0; i < assembler->size(); i++) {
        x64 = assembler->__asm64.get(i);

        switch (GET_OP(x64)) {
            case op_MOVI:
                reg1 = assembler->__asm64.get(++i);

                if(GET_OP(assembler->__asm64.get(++i)) == op_MOVR) {
                    reg2 = GET_Ca(assembler->__asm64.get(i));
                    reg3 = GET_Cb(assembler->__asm64.get(i));

                    if(reg3 == reg1) {
                        assembler->__asm64.remove(i);
                        readjustAddresses(i);
                        assembler->__asm64.replace(--i, reg2); // remember register are left outside the instruction
                        optimizedOpcodes++;
                    }

                    goto readjust;
                }
                break;
        }
    }
}
