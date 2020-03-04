//
// Created by BraxtonN on 10/22/2019.
//

#ifndef SHARP_DATAENTITY_H
#define SHARP_DATAENTITY_H

#include "../../../../stdimports.h"
#include "../../DataType.h"
#include "../../List.h"
#include "../../AccessFlag.h"

class ClassObject;
class Ast;

struct Meta {
    Meta()
    :
        line(""),
        file(""),
        ln(0),
        col(0)
    {
    }

    Meta(string l, string f, long ln, long col)
    :
        line(l),
        file(f),
        ln(ln),
        col(col)
    {
    }

    string line;
    string file;
    long ln,col;

    void copy(Meta &meta) {
        file = meta.file;
        ln = meta.ln;
        col = meta.col;
        line = meta.line;
    }
};

extern long invalidAddr;

/**
 * This class represents any subset of data such as:
 * methods, fields, functions, classes, function pointers etc.
 *
 * All pieces of information all hold data either indirectly or directly
 * This class will hold all the common information each data subset holds
 * i.e name, fullname, type, etc.
 */
class DataEntity {
public:
    DataEntity()
        :
        type(UNTYPED),
        name(""),
        fullName(""),
        meta(),
        address(invalidAddr),
        guid(-109913),
        owner(NULL),
        ast(NULL),
        module("")
    {
        flags.init();
    }

    DataEntity(DataType type)
            :
            type(type),
            name(""),
            fullName(""),
            meta(),
            address(invalidAddr),
            guid(-109913),
            owner(NULL),
            ast(NULL),
            module("")
    {
        flags.init();
    }

    void release() { flags.free(); }
    bool isVar() { return type <= VAR; }
    void copy(DataEntity *de) {
        type = de->type;
        name = de->name;
        fullName = de->fullName;
        meta.copy(de->meta);
        module = de->module;
        flags = de->flags;
        owner = de->owner;
        type = de->type;
        type = de->type;
        flags.addAll(de->flags);
        owner = de->owner;
        ast = de->ast;
    }

    DataType type;
    string name;
    string fullName;
    Meta meta;
    string module;
    long address;
    long guid;
    List<AccessFlag> flags;
    ClassObject *owner;
    Ast *ast;
};


#endif //SHARP_DATAENTITY_H