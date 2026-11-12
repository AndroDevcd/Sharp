//
// Compiler stage timings   (sharpc -timings)
// ==========================================
// Measures how long each stage of a compile takes and prints a table when the
// compile is done, so that decisions about speeding the compiler up (caching,
// compiling fewer files, ...) are based on numbers.
//
//     void parse_file() {
//         TIME_STAGE("parse");        // counts until the end of this block
//         ...
//     }
//
// A stage name may be timed from many places and many times; the table shows
// the total and how often it ran. Stages can be nested (a parent's time then
// includes its children - the table is not meant to add up to 100%).
//
// Costs nothing worth mentioning when -timings is not given: one branch per
// TIME_STAGE.
//

#ifndef SHARP_COMPILER_TIMINGS_H
#define SHARP_COMPILER_TIMINGS_H

#include <chrono>

/** Add `nanoseconds` to the stage called `name` (a string literal). Thread safe. */
void add_stage_time(const char *name, long long nanoseconds);

/** Print the table. Called once at the end of a compile when -timings was given. */
void print_stage_timings();

/** Is -timings on? */
bool stage_timings_enabled();

/** Times from its construction to the end of the enclosing block. */
struct stage_timer {
    const char *name;
    std::chrono::steady_clock::time_point start;
    bool active;

    explicit stage_timer(const char *stage)
            : name(stage), active(stage_timings_enabled()) {
        if(active) start = std::chrono::steady_clock::now();
    }

    ~stage_timer() {
        if(active) {
            add_stage_time(name, std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now() - start).count());
        }
    }
};

#define TIME_STAGE_JOIN2(a, b) a##b
#define TIME_STAGE_JOIN(a, b) TIME_STAGE_JOIN2(a, b)
#define TIME_STAGE(name) stage_timer TIME_STAGE_JOIN(stageTimer, __LINE__)(name)

#endif //SHARP_COMPILER_TIMINGS_H
