//
// Created by BraxtonN on 6/19/2018.
//

#include <iomanip>
#include "profiler.h"

#ifdef SHARP_PROF_
#include "../util/time.h"
#include "Exe.h"
#include "Environment.h"


void Profiler::hit(Method *func) {
    lastHit = func->address;
    totalHits++;
    past = Clock::realTimeInNSecs();
    functions.get(lastHit).hits++;
}

void Profiler::profile() {
    if(lastHit != -1) {
        now = Clock::realTimeInNSecs();
        funcProf &prof = functions.get(lastHit);
        prof.time += NANO_TOMILL(now-past);
        prof.avgtm = prof.time/prof.hits;
        lastHit = -1;
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

    ss << "total time : avg time     :     calls    :               function               :                file" << endl;
    for(i = functions.size()-1; i > 0; i--) {
        if(iter-- <= 0)
            break;

        funcProf &prof = functions.get(i);

        string source = env->sourceFiles[prof.func->sourceFile].str();
        ss << std::setw(11) << prof.time << setw(13) << prof.avgtm << setw(13) << prof.hits << setw(37) << prof.func->fullName.str() << "   " << source;
        ss << endl;

        if(prof.hits==0)
            break;
    }

    cout << ss.str();
    cout << "\n total time = " << NANO_TOMILL(endtm-starttm) << endl;
}

void Profiler::free() {
    for(size_t i = 0; i < functions.size(); i++) {
        functions.get(i).free();
    }
}

#endif
