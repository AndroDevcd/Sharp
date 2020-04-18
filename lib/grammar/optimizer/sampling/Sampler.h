//
// Created by BNunnally on 4/18/2020.
//

#ifndef SHARP_SAMPLER_H
#define SHARP_SAMPLER_H

#include "../../List.h"
#include "../../backend/code/CodeHolder.h"

class SampleBuilder;

class Sampler {
public:
    static uint32_t run(SampleBuilder &builder, CodeHolder &code);
};

enum argument_type {
    HARD_ADDRESS,
    ANY
};

struct SampleArg {
    int32_t arg;
    argument_type type;
};

struct SampleIR {
    int8_t instruction;
    SampleArg *args;
};

class SampleBuilder {

public:
    // TODO: add code from OpcodeBuilder and retrofit it to match my needs
    // TODO: create a SampleManager that tkes a list{SampleBuilder, SampleResult} and use math to figure out wether or not the sample passed
    // TODO: based on a percentange threshold from how much code will be affected by optimization

private:
    List<SampleIR> ir;
};


#endif //SHARP_SAMPLER_H
