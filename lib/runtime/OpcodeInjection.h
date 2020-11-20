//
// Created by BNunnally on 11/18/2020.
//

#ifndef SHARP_OPCODEINJECTION_H
#define SHARP_OPCODEINJECTION_H

#define update_pc(addr) this_fiber->pc = addr;

#define inj_op_nop

#define inj_op_int(signal) \
    VirtualMachine::sysInterrupt(signal); \
    if(vm.state == VM_TERMINATED) return;

#define inj_op_movi(reg, value) \
    registers[reg]=value;

#define inj_op_ret(err_state) \
    if(err_state == ERR_STATE && this_fiber->exceptionObject.object == NULL) { \
        this_fiber->exceptionObject \
            = (this_fiber->sp--)->object.object; \
    } \
     \
    returnMethod(this); \
     \
    if(err_state == ERR_STATE) { \
        GUARD(thread->mutex); \
        sendSignal(thread->signal, tsig_except, 1); \
    } \
    return;

#define inj_op_hlt \
       sendSignal(thread->signal, tsig_kill, 1);

#define inj_op_newarray(sizeReg, type) \
    (++this_fiber->sp)->object = \
        gc.newObject(registers[sizeReg], type);

#define inj_op_cast(objReg) \
    this_fiber->ptr->castObject(registers[objReg]);

#define inj_op_varcast(type, isArray) \
    if(!(TYPE(this_fiber->ptr->object->info) == _stype_var && this_fiber->ptr->object->ntype == type)) { \
        throw Exception(vm.IllStateExcept, \
        getVarCastExceptionMsg((DataType)type, isArray, this_fiber->ptr->object)); \
    }

#define inj_op_mov8(resultReg, castReg) \
    registers[resultReg]=(int8_t)registers[castReg];

#define inj_op_mov16(resultReg, castReg) \
    registers[resultReg]=(int16_t)registers[castReg];

#define inj_op_mov32(resultReg, castReg) \
    registers[resultReg]=(int32_t)registers[castReg];

#define inj_op_mov64(resultReg, castReg) \
    registers[resultReg]=(int64_t)registers[castReg];

#define inj_op_movu8(resultReg, castReg) \
    registers[resultReg]=(uint8_t)registers[castReg];

#define inj_op_movu16(resultReg, castReg) \
    registers[resultReg]=(uint16_t)registers[castReg];

#define inj_op_movu32(resultReg, castReg) \
    registers[resultReg]=(uint32_t)registers[castReg];

#define inj_op_movu64(resultReg, castReg) \
    registers[resultReg]=(uint64_t)registers[castReg];

#define inj_op_rstore(regToSave) \
    (++this_fiber->sp)->var = registers[regToSave];

#define inj_op_add(resultReg, left, right) \
    registers[resultReg]=registers[left]+registers[right];

#define inj_op_sub(resultReg, left, right) \
    registers[resultReg]=registers[left]-registers[right];

#define inj_op_mul(resultReg, left, right) \
    registers[resultReg]=registers[left]*registers[right];

#define inj_op_div(resultReg, left, right) \
    registers[resultReg]=registers[left]/registers[right];

#define inj_op_mod(resultReg, left, right) \
    registers[resultReg]=(Int)registers[left]%(Int)registers[right];

#define inj_op_iadd(resultReg, value) \
    registers[resultReg]+=value;

#define inj_op_isub(resultReg, value) \
    registers[resultReg]-=value;

#define inj_op_imul(resultReg, value) \
    registers[resultReg]*=value;

#define inj_op_idiv(resultReg, value) \
    registers[resultReg]/=value;

#define inj_op_imod(resultReg, value) \
    registers[resultReg]=(Int)registers[resultReg] % value;

#define inj_op_pop \
    this_fiber->sp->object = (SharpObject*)NULL; \
    --this_fiber->sp;

#define inj_op_inc(regToInc) \
    registers[regToInc]++;

#define inj_op_dec(regToInc) \
    registers[regToInc]--;

#define inj_op_movr(resultReg, right) \
    registers[resultReg]=registers[right];

#define inj_op_brh(resultReg, ctx_check) \
    this_fiber->pc=(Int)registers[ADX]; \
    ctx_check \
    goto *opcode_table[this_fiber->pc];

#define inj_op_ife(ctx_check) \
    if(registers[CMT]) { \
        this_fiber->pc=(Int)registers[ADX]; \
        ctx_check \
        goto *opcode_table[this_fiber->pc]; \
    }

#define inj_op_ifne(ctx_check) \
    if(registers[CMT] == 0) { \
        this_fiber->pc=(Int)registers[ADX]; \
        ctx_check \
        goto *opcode_table[this_fiber->pc]; \
    }

#define inj_op_lt(left, right) \
    registers[CMT]=registers[left]<registers[right];

#define inj_op_lt(left, right) \
    registers[CMT]=registers[left]<registers[right];

#define inj_op_gt(left, right) \
    registers[CMT]=registers[left]>registers[right];

#define inj_op_lte(left, right) \
    registers[CMT]=registers[left]<=registers[right];

#define inj_op_gte(left, right) \
    registers[CMT]=registers[left]>=registers[right];

#define inj_op_movl(frameAddr) \
    this_fiber->ptr = &(this_fiber->fp+frameAddr)->object;

#define inj_op_popl(frameAddr) \
   (this_fiber->fp+frameAddr)->object \
        = (this_fiber->sp--)->object.object;

#define inj_op_ipopl(frameAddr) \
   (this_fiber->fp+frameAddr)->var \
            = (this_fiber->sp--)->var;

#define inj_op_movsl(stackAddr) \
    this_fiber->ptr = &((this_fiber->sp+stackAddr)->object);

#define inj_op_sizeof(resultReg) \
    if(this_fiber->ptr==NULL || this_fiber->ptr->object == NULL) \
        registers[resultReg] = 0; \
    else \
        registers[resultReg]=this_fiber->ptr->object->size;

#define inj_op_put(outReg) \
    cout << registers[outReg];

#define inj_op_putc(outReg) \
    cout << (char)registers[outReg];

#define inj_op_get(resultReg) \
     if(registers[CMT])  \
        registers[resultReg] = getche();  \
    else  \
        registers[resultReg] = getch();

#define inj_op_checklen(lenReg) \
    if(registers[lenReg] >= this_fiber->ptr->object->size || registers[lenReg] < 0) { \
        stringstream ss; \
        ss << "Access to Object at: " << registers[lenReg] << " size is " << this_fiber->ptr->object->size; \
        throw Exception(vm.IndexOutOfBoundsExcept, ss.str()); \
    }

#define inj_op_jmp(addr, ctx_check) \
    this_fiber->pc=addr; \
    ctx_check \
    goto *opcode_table[addr]; \

#define  inj_op_loadpc(resutReg, pc) \
    registers[resultReg] = pc;

#define inj_op_pushobj \
    (++this_fiber->sp)->object = this_fiber->ptr;

#define inj_op_del \
    gc.releaseObject(this_fiber->ptr);

#define inj_op_call(funcAddr) \
    executeMethod(funcAddr, thread)(thread);

#define inj_op_calld(addrReg) \
    executeMethod(registers[addrReg], thread)(thread);

#define inj_op_newclass(classAddr) \
    (++this_fiber->sp)->object = gc.newObject(&vm.classes[classAddr]);

#define inj_op_movn(addr) \
    this_fiber->ptr = &this_fiber->ptr->object->node[addr];

#define inj_op_sleep(timeReg) \
    __os_sleep((Int)registers[timeReg]);

#define inj_op_test(left, right) \
   registers[CMT]=registers[left]==registers[right];

#define inj_op_testne(left, right) \
   registers[CMT]=registers[left]!=registers[right];

#define inj_op_lock \
    if(!Object::monitorLock(this_fiber->ptr, thread)) { \
        return; \
    }

#define inj_op_ulock \
    Object::monitorUnLock(this_fiber->ptr);

#define inj_op_movg(classAddr) \
    this_fiber->ptr = vm.staticHeap+classAddr;

#define inj_op_movnd(indexReg) \
    this_fiber->ptr = &this_fiber->ptr->object->node[(Int)registers[indexReg]];

#define inj_op_newobjarray(sizeReg) \
    (++this_fiber->sp)->object = gc.newObjectArray(registers[sizeReg]);

#define inj_op_not(resultReg, notReg) \
    registers[resutReg]=!registers[notReg];

#define inj_op_skip(skippedOpcodes) \
    this_fiber->pc = this_fiber->pc+skippedOpcodes; \
    goto *opcode_table[this_fiber->pc]; \

#define inj_op_loadval(resultReg) \
    registers[resultReg]=(this_fiber->sp--)->var;

#define inj_op_shl(resultReg, left, right) \
    registers[resultReg]=(int64_t)registers[left]<<(int64_t)registers[right];

#define inj_op_shr(resultReg, left, right) \
    registers[resultReg]=(int64_t)registers[left]>>(int64_t)registers[right];

#define inj_op_skpe(condReg, skippedOpcodes) \
    if(registers[condReg] != 0) { \
        this_fiber->pc = this_fiber->pc+skippedOpcodes; \
        goto *opcode_table[this_fiber->pc]; \
    }

#define inj_op_skpne(condReg, skippedOpcodes) \
    if(registers[condReg] == 0) { \
        this_fiber->pc = this_fiber->pc+skippedOpcodes; \
        goto *opcode_table[this_fiber->pc]; \
    }

#define inj_op_cmp(cmpReg, cmpNum) \
    registers[CMT]=registers[cmpReg]==cmpReg;

#define inj_op_and(left, right) \
    registers[CMT]=registers[left]&&registers[right];

#define inj_op_uand(left, right) \
    registers[CMT]=(Int)registers[left]&(Int)registers[right];

#define inj_op_or(left, right) \
    registers[CMT]=(Int)registers[left]|(Int)registers[right];

#define inj_op_xor(left, right) \
    registers[CMT]=(Int)registers[left]^(Int)registers[right];

#define inj_op_throw(pc) \
    this_fiber->pc = pc; \
    this_fiber->exceptionObject = (this_fiber->sp--)->object; \
    sendSignal(signal, tsig_except, 1); \
    goto exception_catch;

#define inj_op_checknull(resultReg) \
    registers[resultReg]=this_fiber->ptr == NULL || this_fiber->ptr->object==NULL;

#define inj_op_return_obj \
    this_fiber->fp->object=this_fiber->ptr;

#define inj_op_newClassArray(sizeReg, classAddr) \
    (++this_fiber->sp)->object = gc.newObjectArray(registers[sizeReg], &vm.classes[classAddr]);

#define inj_op_newString(stringAddr) \
    gc.createStringArray(&(++this_fiber->sp)->object, vm.strings[stringAddr]);

#endif //SHARP_OPCODEINJECTION_H
