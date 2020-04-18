//
// Created by BNunnally on 4/18/2020.
//

#ifndef SHARP_SAMPLEMANAGER_H
#define SHARP_SAMPLEMANAGER_H


class SampleManager {
public:

private:
    int8_t threshold;
    double codeSampled;
};

struct SampleResult {
    uint32_t occurences;
    bool passed;
};


#endif //SHARP_SAMPLEMANAGER_H
