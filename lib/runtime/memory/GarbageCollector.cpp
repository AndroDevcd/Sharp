//
// Created by BraxtonN on 2/11/2018.
//

#include "GarbageCollector.h"

void GarbageCollector::initilize() {
    self=(GarbageCollector*)malloc(sizeof(GarbageCollector)*1);
    self->mutex = Mutex();
    self->threadMutex = Mutex();
    self->gc_alloc_heap=(Object*)malloc(sizeof(Object)*gc_max_heap_size);

    self->allocptr=0;
}
