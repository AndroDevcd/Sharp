//
// Created by BraxtonN on 10/22/2019.
//

#ifndef SHARP_DATAENTITY_H
#define SHARP_DATAENTITY_H

#include "../../../stdimports.h"
#include "../DataType.h"
#include "../List.h"
#include "../AccessFlag.h"

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

protected:
    DataEntity()
        :
        type(UNDEFINED),
        name(""),
        fullName(""),
        meta(),
        address(-1),
        guid(-1),
        owner(NULL),
        ast(NULL),
        module("")
    {
        flags.init();
    }

    void release() { flags.free(); }

public:
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
