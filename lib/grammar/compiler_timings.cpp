//
// Compiler stage timings, see compiler_timings.h
//

#include "compiler_timings.h"
#include "settings/settings.h"
#include <cstring>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <vector>

namespace {

struct stage_total {
    const char *name;
    long long nanoseconds;
    long long count;
};

std::vector<stage_total> totals;    // in the order the stages were first seen
std::mutex totalsLock;

} // namespace

bool stage_timings_enabled() {
    return options.timings;
}

void add_stage_time(const char *name, long long nanoseconds) {
    std::lock_guard<std::mutex> guard(totalsLock);

    for(stage_total &total : totals) {
        if(total.name == name || strcmp(total.name, name) == 0) {
            total.nanoseconds += nanoseconds;
            total.count++;
            return;
        }
    }

    totals.push_back({ name, nanoseconds, 1 });
}

void print_stage_timings() {
    std::lock_guard<std::mutex> guard(totalsLock);

    long long whole = 0;
    for(stage_total &total : totals) {
        if(strcmp(total.name, "TOTAL") == 0) whole = total.nanoseconds;
    }

    std::cout << "\n-- compiler timings --\n";
    std::cout << std::left << std::setw(34) << "stage" << std::right << std::setw(10) << "ms"
              << std::setw(8) << "%" << std::setw(9) << "runs" << "\n";

    for(stage_total &total : totals) {
        double ms = total.nanoseconds / 1000000.0;
        std::cout << std::left << std::setw(34) << total.name
                  << std::right << std::setw(10) << std::fixed << std::setprecision(1) << ms;
        if(whole > 0)
            std::cout << std::setw(7) << std::setprecision(1) << (100.0 * total.nanoseconds / whole) << "%";
        else
            std::cout << std::setw(8) << "";
        std::cout << std::setw(9) << total.count << "\n";
    }
    std::cout << "(indented stages are part of the stage above them)\n" << std::endl;
}
