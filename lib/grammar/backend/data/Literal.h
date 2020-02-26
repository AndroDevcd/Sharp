//
// Created by braxtonn on 11/15/2019.
//

#ifndef SHARP_LITERAL_H
#define SHARP_LITERAL_H

#include "../../../../stdimports.h"
#include "DataEntity.h"

enum literal_type
{
    numeric_literal,
    string_literal,
    unknown_literal
};

class Literal : public DataEntity {
public:
    Literal()
    :
        DataEntity(),
        literalType(unknown_literal),
        stringData(""),
        numericData(0)
    {
    }

    Literal(string &data, long long address)
            :
            DataEntity(),
            literalType(string_literal),
            stringData(data),
            numericData(address)
    {
        type = _INT8;
    }

    explicit Literal(double data)
            :
            DataEntity(),
            literalType(numeric_literal),
            stringData(""),
            numericData(data)
    {
        type = VAR;
    }
    void free();

    literal_type literalType;
    string stringData;
    double numericData;
};

#endif //SHARP_LITERAL_H
