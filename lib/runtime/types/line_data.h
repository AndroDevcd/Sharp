//
// Created by bknun on 9/19/2022.
//

#ifndef SHARP_LINE_DATA_H
#define SHARP_LINE_DATA_H

#include "../../../stdimports.h"

struct line_data {
    line_data()
    :
        pc(0),
        line_number(0)
    {}

    line_data(Int pc, Int line)
    :
        pc(pc),
        line_number(line)
    {}

    Int pc;
    Int line_number;
};


#endif //SHARP_LINE_DATA_H
