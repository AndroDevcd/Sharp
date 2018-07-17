//
// Created by BraxtonN on 6/19/2018.
//

#include <iomanip>
#include "profiler.h"

#ifdef SHARP_PROF_
#include "../util/time.h"
#include "Exe.h"
#include "Environment.h"


extern unsigned long long count, overflow;
void Profiler::hit(Method *func) {
    funcProf prof;
    prof.func = func;
    prof.ir = count;
    prof.past = Clock::realTimeInNSecs();
    functions.get(func->address).hits++;
    calls.push_back(prof);
    totalHits++;
}

void Profiler::profile() {
    if(calls.size() > 0) {
        funcProf &prof = calls.back();
        funcProf *func = &functions.get(prof.func->address);
        func->time += Clock::realTimeInNSecs()-prof.past;
        func->avgtm = func->time/func->hits;
        func->ir += count-prof.ir;
        calls.pop_back();
    }
}

void Profiler::dump() {
    stringstream ss;
    ss << "\n%%%%%============================================%%%%%" << endl;
    ss << "                     Sharp Speed Profiler" << endl;
    ss << endl << endl;

    funcProf tmp;
    size_t i, j, iter=40;
    for(i = 0; i < functions.size(); ++i) {

        for(j = i + 1; j < functions.size(); ++j)
        {
            if(functions.get(i).hits > functions.get(j).hits)
            {
                tmp = functions.get(i);
                functions.get(i) = functions.get(j);
                functions.get(j) = tmp;
            }
        }
    }

    ss << "total time(ms) : avg time(ms)     :     calls    :       ir       :       ir/iter       :               function               :                file" << endl;
    for(i = functions.size()-1; i > 0; i--) {
        if(iter-- <= 0)
            break;

        funcProf &prof = functions.get(i);

        string source = env->sourceFiles[prof.func->sourceFile].str();
        ss << std::setw(11) << NANO_TOMILL(prof.time) << setw(13) << NANO_TOMILL(prof.avgtm) << setw(13) << prof.hits
           << setw(15) << prof.ir << setw(19);
        if(prof.ir == 0)
            ss << 0;
        else {
            if(prof.hits==0)
                ss << prof.ir;
            else
                ss << (prof.ir/prof.hits);
        }
        ss << setw(37) << prof.func->fullName.str() << " " << setw(19) << source;
        ss << endl;

        if(prof.hits==0)
            break;
    }

    cout << ss.str();
    long long tm = NANO_TOMILL(endtm-starttm);
    cout << "\n total time = " << (tm < 0 ? 0 : tm) << endl;
}

void Profiler::free() {
    for(size_t i = 0; i < functions.size(); i++) {
        functions.get(i).free();
    }
    calls.clear();
}

#endif
