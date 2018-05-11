//
// Created by BraxtonN on 5/10/2018.
//

#ifndef SHARP_FASTSTACK_H
#define SHARP_FASTSTACK_H

#include "oo/Method.h"
#include "Environment.h"

class FastStack {

private:
    long long size;
    StackElement *stack;

public:
    long long sp;

    FastStack(long long stack_size)
    {
        stack = new StackElement[stack_size];
        sp = -1;
        size = stack_size;

        for(long long i = 0; i < size; i++) {
            stack[i].object.object = NULL;
        }
    }

    void clear()
    {
        for(unsigned long i = 0; i < size; i++) {
            GarbageCollector::self->freeObject(&stack[i].object);
        }
        sp = -1;
    }

    Object& popObject()
    {
        if( sp >= 0 ) {
            return stack[sp--].object;
        } else
            throw Exception("Stack empty");
    }

    void push(SharpObject *o) {
        if(sp < size) {
            Object *obj = &stack[++sp].object;

            if (obj->object==o) return;
            if(obj->object != nullptr)
            {
                obj->object->refCount--;
                GarbageCollector::self->objs++;
                obj->object = nullptr;
            }

            obj->object = o;
        }
        else
            throw Exception("Stack overflow");
    }

    void push(double n) {
        if(sp < size)
            stack[++sp].var = n;
        else
            throw Exception("Stack overflow");
    }

    double popNumber() {
        if( sp >= 0 ) {
            return stack[sp--].var;
        } else
            throw Exception("Stack empty");
    }

    void drop() {
        sp--;
    }

    void pushArrayAt() {
        long long val = popNumber();
        Object &o = stack[sp--].object;

        if(o.object != NULL) {
            if(val < o.object->size)
                push(o.object->data[val]);
            else
                throw Exception(Environment::IndexOutOfBoundsException, "out of bounds");
        } else
            throw Exception(Environment::NullptrException, "");
    }

    void pushArrayAt(long long index) {
        Object &o = stack[sp--].object;

        if(o.object != NULL) {
            if(index < o.object->size)
                push(o.object->data[index]);
            else
                throw Exception(Environment::IndexOutOfBoundsException, "out of bounds");
        } else
            throw Exception(Environment::NullptrException, "");
    }

    void setArrayAt(long long index, long long data) {
        Object &o = stack[sp].object;

        if(o.object != NULL) {
            if(index < o.object->size)
                o.object->data[index]=data;
            else
                throw Exception(Environment::IndexOutOfBoundsException, "out of bounds");
        } else
            throw Exception(Environment::NullptrException, "");
    }

    void swap() {
        double val = stack[sp-1].var;
        SharpObject* o = stack[sp-1].object.object;

        stack[sp-1].var=stack[sp].var;
        stack[sp-1].object.object=stack[sp].object.object;

        stack[sp].var = val;
        stack[sp].object.object = o;
    }

    void oswap() {
        SharpObject* o = stack[sp-1].object.object;

        stack[sp-1].object.object=stack[sp].object.object;

        stack[sp].object.object = o;
    }

    SharpObject* getObject(long long pos) {
        if(pos <size && pos > 0) {
            return stack[pos].object.object;
        }  else
            throw Exception("illegal argument");

    }

    double getNumber(long long pos) {
        if(pos <size && pos > 0) {
            return stack[pos].var;
        } else
            throw Exception("illegal argument");
    }

    SharpObject* getObject() {
        return stack[sp--].object.object;
    }

    void insert(long long pos, SharpObject *o) {
        Object *obj = &stack[pos].object;

        Object::newRefrence(obj, o);
    }

    void insert(long long pos, double data) {
        stack[pos].var = data;
    }

    void _not_() {
        stack[sp].var = !stack[sp].var;
    }

    void setArrayAt(long long index, SharpObject *o) {
        Object &obj = stack[sp--].object;

        if(obj.object != NULL) {
            if(index < obj.object->size)
                Object::newRefrence(&obj.object->node[index], o);
            else
                throw Exception(Environment::IndexOutOfBoundsException, "out of bounds");
        } else
            throw Exception(Environment::NullptrException, "");
    }

    void dup() {
        Object::newRefrence(&stack[sp+1].object, stack[sp].object.object);
        sp++;

        stack[sp+1].var=stack[sp].var;
        sp++;
    }
};

#endif //SHARP_FASTSTACK_H
