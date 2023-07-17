//
// Created by bknun on 7/9/2023.
//

#ifndef SHARP_FRAGMENT_EXTRACTOR_H
#define SHARP_FRAGMENT_EXTRACTOR_H

#include "../analyzer/code_analyzer.h"

void extract_fragments(
        fragment_type type,
        code_fragment *frag,
        List<code_fragment*> &results,
        bool recursiveSearch,
        code_fragment *endPoint = NULL);

#endif //SHARP_FRAGMENT_EXTRACTOR_H
