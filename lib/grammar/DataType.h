//
// Created by BraxtonN on 2/2/2018.
//

#ifndef SHARP_FIELDTYPE_H
#define SHARP_FIELDTYPE_H

enum DataType
{
    _INT8=0,
    _INT16=1,
    _INT32=2,
    _INT64=3,
    _UINT8=4,
    _UINT16=5,
    _UINT32=6,
    _UINT64=7,
    FNPTR=8,
    VAR=9,
    OBJECT=10,
    CLASS=11,
    METHOD=12,
    _VOID = 13,
    UNTYPED=14,
    UNDEFINED=15
};

enum StorageLocality
{
    stl_stack,
    stl_thread
};

#endif //SHARP_FIELDTYPE_H
