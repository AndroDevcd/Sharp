//
// Created by BraxtonN on 6/19/2018.
//

#include <iomanip>
#include "profiler.h"

#ifdef SHARP_PROF_
#include "../util/time.h"
#include "Exe.h"
#include "Thread.h"
#include "main.h"
#include "VirtualMachine.h"


extern unsigned long irCount, overflow;
void Profiler::hit(Method *func) {
    funcProf prof;
    prof.func = func;
    prof.ir = irCount;
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
        func->ir += irCount-prof.ir;
        calls.pop_back();
    }
}

void Profiler::dump() {
    stringstream ss;
    ss << "\n%%%%%=====================================================%%%%%" << endl;
    ss <<    "                     Sharp Speed Profiler" << endl;
    ss << endl << endl;

    funcProf tmp;
    long long i, j, iter=40, doSort = 0;
    for(i = 0; i < functions.size(); ++i) {

        for(j = i + 1; j < functions.size(); ++j)
        {

            switch(c_options.sortBy) {
                case profilerSort::tm:
                    if(functions.get(i).time > functions.get(j).time)
                        doSort = 1;
                    break;
                case avgt:
                    if(functions.get(i).avgtm > functions.get(j).avgtm)
                        doSort = 1;
                    break;
                case profilerSort::calls:
                    if(functions.get(i).hits > functions.get(j).hits)
                        doSort = 1;
                    break;
                case ir:
                    if(functions.get(i).ir > functions.get(j).ir)
                        doSort = 1;
                    break;
            }

            if(doSort)
            {
                tmp = functions.get(i);
                functions.get(i) = functions.get(j);
                functions.get(j) = tmp;
                doSort = 0;
            }
        }
    }

    ss << "   total time(ms)  :    avg time(ms)    :     calls    :        ir        :       ir/iter       :               function               :                file" << endl;
    for(i = functions.size()-1; i > 0; i--) {
        if(iter-- <= 0)
            break;

        funcProf &prof = functions.get(i);

        string source = vm.metaData.files.get(prof.func->sourceFile).name.str();
        ss << std::setw(18) << NANO_TOMILL(prof.time) << setw(20) << NANO_TOMILL(prof.avgtm) << setw(13) << prof.hits
           << setw(17) << prof.ir << setw(19);
        if(prof.ir == 0)
            ss << 0;
        else {
            if(prof.hits==0)
                ss << prof.ir;
            else
                ss << (prof.ir/prof.hits);
        }
        ss << "   " << setw(39) << prof.func->fullName.str() << "      " << setw(22) << source;
        ss << endl;

        switch(c_options.sortBy) {
            case profilerSort::tm:
                if(prof.time==0)
                    goto print;
                break;
            case avgt:
                if(prof.avgtm==0)
                    goto print;
                break;
            case profilerSort::calls:
                if(prof.hits==0)
                    goto print;
                break;
            case ir:
                if(prof.ir==0)
                    goto print;
                break;
        }
    }

    print:
    cout << ss.str();
    long long tm = NANO_TOMILL(endtm-starttm);
    cout << "\nthread " << thread_self->name.str() << " (" << thread_self->id << ")";
    cout << "\nreal time = " << (tm < 0 ? 0 : tm) << endl;
}

void Profiler::free() {
    for(size_t i = 0; i < functions.size(); i++) {
        functions.get(i).free();
    }
    calls.clear();
}

#endif
