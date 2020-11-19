//
// Created by BNunnally on 11/18/2020.
//

#ifndef SHARP_OPCODEINJECTION_H
#define SHARP_OPCODEINJECTION_H

#define update_pc(addr) this_fiber->pc = this_fiber->cache+addr;

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
    if(returnMethod(this)) { \
        if(err_state == ERR_STATE) { \
            GUARD(thread->mutex); \
            sendSignal(thread->signal, tsig_except, 1); \
        } \
         \
        this_fiber->pc++; \
        return; \
    } \
     \
    if(err_state == ERR_STATE) { \
        GUARD(thread->mutex); \
        sendSignal(thread->signal, tsig_except, 1); \
        goto exception_catch; \
    }

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
    (++this_fiber->sp)->var = registers[GET_Da(*this_fiber->pc)];

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
    this_fiber->pc=this_fiber->cache+(Int)registers[ADX]; \
    ctx_check \
    goto *opcode_table[this_fiber->pc-this_fiber->cache];

#define inj_op_ife(ctx_check) \
    if(registers[CMT]) { \
        this_fiber->pc=this_fiber->cache+(int64_t)registers[ADX]; \
        ctx_check \
        goto *opcode_table[this_fiber->pc-this_fiber->cache]; \
    }

#define inj_op_ifne(ctx_check) \
    if(registers[CMT] == 0) { \
        this_fiber->pc=this_fiber->cache+(int64_t)registers[ADX]; \
        ctx_check \
        goto *opcode_table[this_fiber->pc-this_fiber->cache]; \
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



#endif //SHARP_OPCODEINJECTION_H
