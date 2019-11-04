//
// Created by BraxtonN on 2/2/2018.
//

#ifndef SHARP_FIELDTYPE_H
#define SHARP_FIELDTYPE_H

enum DataType
{
    _INT8,
    _INT16,
    _INT32,
    _INT64,
    _UINT8,
    _UINT16,
    _UINT32,
    _UINT64,
    FNPTR,
    VAR,
    OBJECT,
    CLASS,
    METHOD,
    _VOID_,
    _GENERIC,
    UNDEFINED
};

enum StorageLocality
{
    stl_stack,
    stl_thread
};

#endif //SHARP_FIELDTYPE_H
