//
// Created by bknun on 7/10/2023.
//

#include "variable_read_processor.h"
#include "fragment_extractor.h"

Int count_variable_reads(code_fragment *variable, code_fragment *origin) {
    sharp_field *duplicateWriteVar = NULL;
    List<code_fragment*> reads;
    extract_fragments(variable_read, origin, reads, true, variable);

    return reads.size();
}