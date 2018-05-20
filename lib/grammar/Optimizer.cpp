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

        if(stopAddr >= ft.try_start_pc && stopAddr <= ft.try_end_pc)
            ft.try_end_pc--;
        if(stopAddr <= ft.try_start_pc) {
            ft.try_start_pc--;
            ft.try_end_pc--;
        }
    }

    for(unsigned int i = 0; i < func->line_table.size(); i++) {
        KeyPair<int64_t, long> &lt = func->line_table.get(i);

        if(stopAddr < lt.key && lt.key > 0)
            lt.key--;
    }

    int64_t x64, op, addr;
    for(unsigned int i = 0; i < stopAddr; i++) {
        if(i >= assembler->__asm64.size())
            break;

        x64 = assembler->__asm64.get(i);

        switch (op=GET_OP(x64)) {
            case op_SKPE:
            case op_SKNE:
            case op_GOTO:
                addr=GET_Da(x64);

                /*
                 * We only want to update data which is referencing data below us
                 */
                if(addr >= stopAddr)
                {
                    // update address
                    assembler->__asm64.replace(i, SET_Di(x64, op, --addr));
                }
                break;
            case op_MOVI:
                if(unique_addr_lst.find(i)) {
                    addr=GET_Da(x64);
                    //unique_addr_lst.replace(unique_addr_lst.indexof(i), i-1);

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

               // cout << " address " << addr << " stop addr " << stopAddr << " in " << func->getFullName() << endl;
                /*
                 * We only want to update data which is referencing data below us
                 */
                if(addr >= stopAddr)
                {
                    // update address
                    assembler->__asm64.replace(i, SET_Di(x64, op, --addr));
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
                        assembler->__asm64.replace(i, SET_Di(x64, op_MOVI, addr));
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
 *
 * to -> [0x4] 4: popl 2
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
    optimizeRedundantSelfInitilization();
    optimizeLocalPush();
    optimizeRedundantMovICall2();
    optimizeObjectPush();
}

/**
 * [0x2] 2:	movi #10000000, ebx
 * [0x4] 4:	movr egx, ebx
 *
 * to -> [0x2] 2:	movi #10000000, egx
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
                        assembler->__asm64.replace(--i, reg2); // remember registers are left outside the instruction
                        optimizedOpcodes++;

                        goto readjust;
                    }

                }
                break;
        }
    }
}

/**
 *
 * [0x7] 7:	movr ebx, cmt
 * [0x8] 8:	movr cmt, ebx
 *
 * to -> (removed)
 */
void Optimizer::optimizeRedundantSelfInitilization() {
    int64_t x64, reg1, reg2, reg3, reg4;
    readjust:
    for(unsigned int i = 0; i < assembler->size(); i++) {
        x64 = assembler->__asm64.get(i);

        switch (GET_OP(x64)) {
            case op_MOVR:
                reg1 = GET_Ca(x64);
                reg2 = GET_Cb(x64);

                if(GET_OP(assembler->__asm64.get(++i)) == op_MOVR) {
                    reg3 = GET_Ca(assembler->__asm64.get(i));
                    reg4 = GET_Cb(assembler->__asm64.get(i));

                    if(reg2 == reg3 && reg1 == reg4) {
                        assembler->__asm64.remove(i);
                        readjustAddresses(i);

                        i--;
                        assembler->__asm64.remove(i);
                        readjustAddresses(i);

                        optimizedOpcodes+=2;
                        goto readjust;
                    }

                }
                break;
        }
    }
}

/**
 *
 * [0x0] 0:	loadl ebx, fp+0
 * [0x1] 1:	rstore ebx
 *
 * to -> [0x1] 1: ipushl #0
 */
void Optimizer::optimizeLocalPush() {
    int64_t x64, reg1, reg2, idx;
    readjust:
    for(unsigned int i = 0; i < assembler->size(); i++) {
        x64 = assembler->__asm64.get(i);

        switch (GET_OP(x64)) {
            case op_LOADL:
                reg1 = GET_Ca(x64);
                idx = GET_Cb(x64);

                if(GET_OP(assembler->__asm64.get(++i)) == op_RSTORE) {
                    reg2 = GET_Da(assembler->__asm64.get(i));

                    if(reg1 == reg2) {
                        assembler->__asm64.remove(i);
                        readjustAddresses(i);
                        assembler->__asm64.replace(--i, SET_Di(x64, op_IPUSHL, idx)); // remember registers are left outside the instruction
                        optimizedOpcodes++;

                        goto readjust;
                    }
                }
                break;
        }
    }
}

/**
 *
 * [0x5] 5:	movi #2, ebx
 * [0x7] 7:	rstore ebx
 *
 * to -> [0x5] 5: istore #2
 */
void Optimizer::optimizeRedundantMovICall2() {
    int64_t x64, reg1, reg2, val;
    readjust:
    for(unsigned int i = 0; i < assembler->size(); i++) {
        x64 = assembler->__asm64.get(i);

        switch (GET_OP(x64)) {
            case op_MOVI:
                val = GET_Da(x64);
                reg1 = assembler->__asm64.get(++i);

                if(GET_OP(assembler->__asm64.get(++i)) == op_RSTORE) {
                    reg2 = GET_Da(assembler->__asm64.get(i));

                    if(reg1 == reg2) {
                        assembler->__asm64.remove(i); // remove rstore
                        readjustAddresses(i);
                        i--;

                        assembler->__asm64.remove(i); // remove unused register
                        readjustAddresses(i);

                        assembler->__asm64.replace(--i, SET_Di(x64, op_ISTORE, val)); // remember registers are left outside the instruction
                        optimizedOpcodes+=2;

                        goto readjust;
                    }

                }
                break;
        }
    }
}

/**
 *
 * [0x2] 2:	movl 1
 * [0x3] 3:	pushobj
 *
 * to -> [0x3] 3: pushl 1
 */
void Optimizer::optimizeObjectPush() {
    int64_t x64, idx;
    readjust:
    for(unsigned int i = 0; i < assembler->size(); i++) {
        x64 = assembler->__asm64.get(i);

        switch (GET_OP(x64)) {
            case op_MOVL:
                idx = GET_Da(x64);

                if(GET_OP(assembler->__asm64.get(++i)) == op_PUSHOBJ) {
                    assembler->__asm64.remove(i);
                    readjustAddresses(i);

                    assembler->__asm64.replace(--i, SET_Di(x64, op_PUSHL, idx)); // remember registers are left outside the instruction
                    optimizedOpcodes++;

                    goto readjust;
                }
                break;
        }
    }
}
