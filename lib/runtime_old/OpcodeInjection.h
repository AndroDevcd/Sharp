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
    returnMethod(thread); \
     \
    if(err_state == ERR_STATE) { \
        GUARD(thread->mutex); \
        sendSignal(thread->signal, tsig_except, 1); \
    } \
    this_fiber->pc++; \
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

#define inj_op_brh(ctx_check) \
    this_fiber->pc=(Int)registers[ADX]; \
    ctx_check \
    goto *label_table[this_fiber->pc];

#define inj_op_ife(ctx_check) \
    if(registers[CMT]) { \
        this_fiber->pc=(Int)registers[ADX]; \
        ctx_check \
        goto *label_table[this_fiber->pc]; \
    }

#define inj_op_ifne(ctx_check) \
    if(registers[CMT] == 0) { \
        this_fiber->pc=(Int)registers[ADX]; \
        ctx_check \
        goto *label_table[this_fiber->pc]; \
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
    goto *label_table[addr]; \

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
    registers[resultReg]=!registers[notReg];

#define inj_op_skip(pcAddr) \
    this_fiber->pc = pcAddr; \
    goto *label_table[this_fiber->pc]; \

#define inj_op_loadval(resultReg) \
    registers[resultReg]=(this_fiber->sp--)->var;

#define inj_op_shl(resultReg, left, right) \
    registers[resultReg]=(int64_t)registers[left]<<(int64_t)registers[right];

#define inj_op_shr(resultReg, left, right) \
    registers[resultReg]=(int64_t)registers[left]>>(int64_t)registers[right];

#define inj_op_skpe(condReg, pcAddr) \
    if(((Int)registers[condReg]) != 0) { \
        this_fiber->pc = pcAddr; \
        goto *label_table[this_fiber->pc]; \
    }

#define inj_op_skpne(condReg, pcAddr) \
    if(((Int)registers[condReg]) == 0) { \
        this_fiber->pc = pcAddr; \
        goto *label_table[this_fiber->pc]; \
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

#define inj_op_throw(pcIndex) \
    this_fiber->pc = pcIndex; \
    this_fiber->exceptionObject = (this_fiber->sp--)->object; \
    sendSignal(thread->signal, tsig_except, 1); \
    goto exception_catch;

#define inj_op_checknull(resultReg) \
    registers[resultReg]=this_fiber->ptr == NULL || this_fiber->ptr->object==NULL;

#define inj_op_return_obj \
    this_fiber->fp->object=this_fiber->ptr;

#define inj_op_newClassArray(sizeReg, classAddr) \
    (++this_fiber->sp)->object = gc.newObjectArray(registers[sizeReg], &vm.classes[classAddr]);

#define inj_op_newString(stringAddr) \
    gc.createStringArray(&(++this_fiber->sp)->object, vm.strings[stringAddr]);

#define inj_op_addl(frameAddr, reg) \
   (this_fiber->fp+frameAddr)->var+=registers[reg];

#define inj_op_subl(frameAddr, reg) \
   (this_fiber->fp+frameAddr)->var-=registers[reg];

#define inj_op_mull(frameAddr, reg) \
   (this_fiber->fp+frameAddr)->var*=registers[reg];

#define inj_op_divl(frameAddr, reg) \
   (this_fiber->fp+frameAddr)->var/=registers[reg];

#define inj_op_modl(frameAddr, reg) \
   (this_fiber->fp+frameAddr)->var = (Int)(this_fiber->fp+frameAddr)->var % (Int)registers[reg];

#define inj_op_iaddl(frameAddr, value) \
   (this_fiber->fp+frameAddr)->var+=value;

#define inj_op_isubl(frameAddr, value) \
   (this_fiber->fp+frameAddr)->var-=value;

#define inj_op_imull(frameAddr, value) \
   (this_fiber->fp+frameAddr)->var*=value;

#define inj_op_idivl(frameAddr, value) \
   (this_fiber->fp+frameAddr)->var/=value;

#define inj_op_imodl(frameAddr, value) \
   (this_fiber->fp+frameAddr)->var = (Int)(this_fiber->fp+frameAddr)->var % value;

#define inj_op_loadl(resultReg, frameAddr) \
   registers[resultReg]=(this_fiber->fp+frameAddr)->var;

#define inj_op_iaload(resultReg, indexReg) \
    registers[resultReg] = this_fiber->ptr->object->HEAD[(Int)registers[indexReg]];

#define inj_op_popObj \
    *this_fiber->ptr = (this_fiber->sp--)->object;

#define inj_op_smovr(stackAddr, valueReg) \
    (this_fiber->sp+stackAddr)->var=registers[valueReg];

#define inj_op_smovr2(frameAddr, valueReg) \
    (this_fiber->fp+frameAddr)->var=registers[valueReg];

#define inj_op_smovr3(frameAddr) \
    (this_fiber->fp+frameAddr)->object=this_fiber->ptr;

#define inj_op_andl(frameAddr, valueReg) \
    (this_fiber->fp+frameAddr)->var = (Int)(this_fiber->fp+frameAddr)->var & (Int)registers[valueReg];

#define inj_op_orl(frameAddr, valueReg) \
    (this_fiber->fp+frameAddr)->var = (Int)(this_fiber->fp+frameAddr)->var | (Int)registers[valueReg];

#define inj_op_xorl(frameAddr, valueReg) \
    (this_fiber->fp+frameAddr)->var = (Int)(this_fiber->fp+frameAddr)->var ^ (Int)registers[valueReg];

#define inj_op_rmov(indexReg, valueReg) \
    this_fiber->ptr->object->HEAD[(Int)registers[indexReg]] = \
        registers[valueReg];

#define inj_op_neg(resultReg, valueReg) \
    registers[resultReg]=-registers[valueReg];

#define inj_op_smov(resultReg, stackAddr) \
    registers[resultReg]=(this_fiber->sp+stackAddr)->var;

#define inj_op_returnVal(valueReg) \
    (this_fiber->fp)->var=registers[valueReg];

#define inj_op_istore(value) \
    (++this_fiber->sp)->var = value;

#define inj_op_istorel(frameAddr, value) \
    (this_fiber->fp+frameAddr)->var=value;

#define inj_op_pushnull \
    gc.releaseObject(&(++this_fiber->sp)->object);

#define inj_op_ipushl(frameAddr) \
    (++this_fiber->sp)->var = (this_fiber->fp+frameAddr)->var;

#define inj_op_pushl(frameAddr) \
    (++this_fiber->sp)->object = (this_fiber->fp+frameAddr)->object;

#define inj_op_itest(resultReg) \
    tmpPtr = &(this_fiber->sp--)->object; \
    registers[resultReg] = tmpPtr->object == (this_fiber->sp--)->object.object;

#define inj_op_invokeDelegate(address, args, thread, isStatic, injit) \
    invokeDelegate(address, args, thread, isStatic, false);

#define inj_op_isadd(stackAddr, value) \
    (this_fiber->sp+stackAddr)->var+=value;

#define inj_op_je(pcIndex, ctx_check) \
    if(registers[CMT]) { \
        this_fiber->pc=pcIndex; \
        ctx_check \
        goto *label_table[pcIndex]; \
    }

#define inj_op_jne(pcVal, ctx_check) \
    if(registers[CMT] == 0) { \
        this_fiber->pc=pcVal; \
        ctx_check \
        goto *label_table[pcVal]; \
    }

#define inj_op_tls_movl(stackAddr)  \
    this_fiber->ptr = &(this_fiber->dataStack+stackAddr)->object;

#define inj_op_dup \
    tmpPtr = &this_fiber->sp->object; \
    (++this_fiber->sp)->object = tmpPtr;

#define inj_op_popObj2 \
    this_fiber->ptr = &(this_fiber->sp--)->object;

#define inj_op_swap \
    if((this_fiber->sp-this_fiber->dataStack) >= 2) { \
        SharpObject *swappedObj = this_fiber->sp->object.object; \
        (this_fiber->sp)->object = (this_fiber->sp-1)->object; \
        (this_fiber->sp-1)->object = swappedObj; \
        double swappedVar = this_fiber->sp->var; \
        (this_fiber->sp)->var = (this_fiber->sp-1)->var; \
        (this_fiber->sp-1)->var = swappedVar; \
    } else { \
        stringstream ss; \
        ss << "Illegal stack swap while sp is ( " << (Int )(this_fiber->sp - this_fiber->dataStack) << ") "; \
        throw Exception(vm.ThreadStackExcept, ss.str()); \
    }

#define inj_op_exp(resultReg, baseReg, exponentReg) \
    registers[resultReg]=pow(registers[baseReg], registers[exponentReg]);

#define inj_op_ldc(resultReg, constantAddr) \
    registers[resultReg]=vm.constants[constantAddr];

#define inj_op_is(resultReg, type) \
    registers[resultReg] = VirtualMachine::isType(this_fiber->ptr, type);



#endif //SHARP_OPCODEINJECTION_H
