//
// Created by BraxtonN on 2/2/2018.
//

#ifndef SHARP_FIELDTYPE_H
#define SHARP_FIELDTYPE_H

enum FieldType
{
    VAR,
    OBJECT,
    CLASS,
    TYPEVOID,
    CLASSFIELD,
    TYPEGENERIC,
    UNDEFINED
};

enum StorageLocality
{
    stl_local,
    stl_thread
};

#endif //SHARP_FIELDTYPE_H
