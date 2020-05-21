//
// Created by bknun on 7/15/2018.
//

#include "clist.h"
#include "../../runtime/Thread.h"
#include "../../runtime/register.h"

int RESIZE_LIMIT = 35;

void clist(int proc) {
    switch(proc) {
        case 0x000:
            __clist_add();
            return;
        case 0x001:
            __clist_shrink();
            return;
        case 0x002:
            __clist_insert();
            return;
        case 0x003:
            __clist_remove();
            return;
    }
}

void __clist_expand(SharpObject* lst) {
    SharpObject* size = lst->node[0].object;
    SharpObject* data = lst->node[1].object;
    register int64_t sz = data->size;

    if(size->HEAD[0] >= sz) {
        size_t capacity = sz+
                          (sz >> 2) + (sz > 1000000 ? 10 : RESIZE_LIMIT);

        if(data->node != NULL) {
            GarbageCollector::self->reallocObject(data, capacity);
        } else
            GarbageCollector::self->realloc(data, capacity);
    }
}

void __shrink(SharpObject *lst) {
    double* size = lst->node[0].object->HEAD;
    SharpObject* data = lst->node[1].object;

    if(*size <= data->size && *size > 0) {
        (*size)--;
        size_t capacity = data->size-1;

        if(data->node != NULL) {
            GarbageCollector::self->reallocObject(data, capacity);
        } else
            GarbageCollector::self->realloc(data, capacity);
    }
}

void __clist_shrink() {
    Thread *self = thread_self;
    SharpObject *lst = (self->sp--)->object.object;

    if(lst != NULL && IS_CLASS(lst->info) && lst->size >= 2) {
        if(lst->node[0].object != NULL && TYPE(lst->node[0].object->info) == _stype_var
           && lst->node[1].object != NULL) {

            __shrink(lst);
        }
    }
}

void __clist_add() {
    Thread *self = thread_self;
    SharpObject *lst = (self->sp--)->object.object;

    if(lst != NULL && IS_CLASS(lst->info) && lst->size >= 2) {
        if (lst->node[0].object != NULL && TYPE(lst->node[0].object->info) == _stype_var
            && lst->node[1].object != NULL) {
            double* size = lst->node[0].object->HEAD;
            SharpObject* data = lst->node[1].object;


            if(*size >= data->size)
                __clist_expand(lst);

            if(data->HEAD != NULL) {
                data->HEAD[(size_t)(*size)++]
                        = registers[EBX];
            } else {
                data->node[(size_t)(*size)++] =
                        (self->sp--)->object.object;
            }
        }
    }
}

void __clist_insert() {
    Thread *self = thread_self;
    SharpObject *lst = (self->sp--)->object.object;

    if(lst != NULL && IS_CLASS(lst->info) && lst->size >= 2) {
        if (lst->node[0].object != NULL && lst->node[0].object->HEAD != NULL
            && lst->node[1].object != NULL) {
            SharpObject* size = lst->node[0].object;
            SharpObject* data = lst->node[1].object;
            size_t pos = (size_t)registers[EBX];

            if(size->HEAD[0] >= data->size)
                __clist_expand(lst);

            if(data->HEAD != NULL) {
                double value = registers[EGX];
                for(size_t i = data->size-1; i > pos; i--)
                    data->HEAD[i] = data->HEAD[i-1];
                data->HEAD[pos] = value;
            } else {
                for(size_t i = data->size-1; i > pos; i--)
                    data->node[i] = data->node[i-1];
                data->node[pos] =
                        (self->sp--)->object.object;
            }

        }
    }
}

void __clist_remove() {
    Thread *self = thread_self;
    SharpObject *lst = (self->sp--)->object.object;

    if(lst != NULL && IS_CLASS(lst->info) && lst->size >= 2) {
        if (lst->node[0].object != NULL && lst->node[0].object->HEAD != NULL
            && lst->node[1].object != NULL) {
            SharpObject* size = lst->node[0].object;
            SharpObject* data = lst->node[1].object;
            size_t pos = (size_t)registers[EBX];

            if(size->HEAD[0] == 1) {
                size->HEAD[0]--;
                return;
            } else if(size->HEAD[0] == 2) {
                if(pos == 0) {
                    if(data->HEAD != NULL)
                        data->HEAD[0] = data->HEAD[1];
                    else
                        data->node[0] =
                                data->node[1];
                }

                __shrink(lst);
                return;
            }

            size_t newLen = size->HEAD[0]-1;
            if(data->HEAD != NULL) {
                for(size_t i = pos; i < newLen; i++)
                    data->HEAD[i] = data->HEAD[i+1];
            } else {
                for(size_t i = pos; i < newLen; i++)
                    data->node[i] = data->node[i+1];
            }

            __shrink(lst);
        }
    }
}