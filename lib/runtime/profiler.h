//
// Created by BraxtonN on 6/19/2018.
//

#ifndef SHARP_PROFILER_H
#define SHARP_PROFILER_H

#include "../../stdimports.h"
#include "oo/Method.h"

#ifdef SHARP_PROF_

struct funcProf {
    Method *func;
    size_t hits;  // total hits for function
    float time;  // time used per call
    float avgtm; // average time user for call
    int64_t past, ir;

    funcProf(Method *f) {
        hits=0;
        time=0;
        avgtm=0;
        ir=0;
        func = f;
    }

    funcProf() {
        hits=0;
        time=0;
        avgtm=0;
        ir=0;
        func = NULL;
    }

    void init() {
        hits=0;
        time=0;
        avgtm=0;
        ir=0;
        func = NULL;
    }

    ~funcProf() {
        hits=0;
        time=0;
        avgtm=0;
        ir=0;
        func = NULL;
    }

    void operator=(funcProf prof) {
        func=prof.func;
        hits=prof.hits;
        time = prof.time;
        ir=prof.ir;
        avgtm = prof.avgtm;
    }

    void free() {
        func = NULL;
    }
};

class Profiler {
public:
    Profiler(std::string &out)
    :
        functions(),
        totalHits(0),
        starttm(0),
        endtm(0)
    {
    }

    Profiler()
            :
            functions(),
            totalHits(0),
            starttm(0),
            endtm(0)
    {
    }

    void init(size_t stack) {
        new (&calls) std::vector<funcProf>();
        calls.reserve(stack/2);
        functions.init();
        totalHits=0;
        starttm=0;
        endtm=0;
    }
    void hit(Method *func);
    void profile();

    std::vector<funcProf> calls;
    List<funcProf> functions;
    size_t totalHits; // total ammount of function calls
    int64_t starttm, endtm;
    size_t lastHit;

    void dump();

    void free();
};


#endif

#endif //SHARP_PROFILER_H
