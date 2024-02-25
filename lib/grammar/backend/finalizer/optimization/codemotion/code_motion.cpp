//
// Created by bknun on 7/2/2023.
//

#include "code_motion.h"
#include "../../../../../../stdimports.h"
#include "../../generation/generator.h"
#include "../optimizer.h"
#include "while_step_runner.h"
#include "../analyzer/code_analyzer.h"
#include "../processor/fragment_extractor.h"
#include "../processor/single_set_variable_processor.h"
#include "../processor/variable_read_processor.h"

void optimize_code_motion() {
    for(Int i = 0; i < compressedCompilationFunctions.size(); i++) {
        auto subject = compressedCompilationFunctions.get(i);
        setup_context(subject);

        if(subject->name == "code_motion") {
            auto code_fragments = analyze_code(subject->scheme);
            List<code_fragment*> while_loops;
            extract_fragments(while_block_fragment, code_fragments, while_loops, false);

            for(Int j = 0; j < while_loops.size(); j++) {
                List<write_variable *> writes;
                process_single_set_variables(writes, while_loops[j]);

                for(Int g = 0; g < writes.size(); g++) {
                    Int reads = count_variable_reads(writes[g], while_loops[j]);
                    int gkd = 0;
                }
                int dkjjsd = 0;
            }
            int gg = 0;
//            run_code_motion_while_steps();
        }
    }
}