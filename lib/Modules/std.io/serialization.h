//
// Created by BNunnally on 9/14/2020.
//

#ifndef SHARP_SERIALIZATION_H
#define SHARP_SERIALIZATION_H

#include "../../runtime/symbols/Object.h"

#define EXPORT_VERS 1
#define EXPORT_SECRET 0x20
#define EXPORT_DATA 0x5F
#define EXPORT_FIELD 0xD
#define EXPORT_CLASS 0x2B
#define EXPORT_END 0x2E
#define EXPORT_EMPTY 0x6C
#define EXPORT_REFERENCE 0x4B
#define DATA_END 0x0
#define DATA_SIZE 0xca

#define STREAM_SIZE_START KB_TO_BYTES(250) // 16Mb limit per object
#define STREAM_SIZE_MAX MB_TO_BYTES(16)
#define STREAM_SIZE_ALLOC_CHUNK KB_TO_BYTES(50)

extern recursive_mutex exportMutex;
string export_obj(SharpObject* obj);
void import_obj(SharpObject* obj);
void cleanup();

#endif //SHARP_SERIALIZATION_H
