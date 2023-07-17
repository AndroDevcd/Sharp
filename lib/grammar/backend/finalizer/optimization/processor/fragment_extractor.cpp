//
// Created by bknun on 7/9/2023.
//

#include "fragment_extractor.h"

bool search_frag(
        fragment_type type,
        code_fragment *frag,
        List<code_fragment*> &results,
        bool recursiveSearch,
        code_fragment *endPoint
);

bool search_block_fragment(
        fragment_type type,
        code_fragment *frag,
        List<code_fragment*> &results,
        bool recursiveSearch,
        code_fragment *endPoint
) {
    if(type == code_block_fragment) {
        results.add(frag);
        if(!recursiveSearch) return false;
    }

    code_block *cblk = (code_block*)frag;
    for(Int i = 0; i < cblk->nodes.size(); i++) {
        if(search_frag(type, cblk->nodes[i], results, true, endPoint))
            return true;
    }

    return false;
}

bool search_while_block_fragment(
        fragment_type type,
        code_fragment *frag,
        List<code_fragment*> &results,
        bool recursiveSearch,
        code_fragment *endPoint
) {
    if(type == while_block_fragment) {
        results.add(frag);
        if(!recursiveSearch) return false;
    }

    while_loop *wblk = (while_loop*)frag;
    if(search_frag(type, wblk->conditionCheck, results, recursiveSearch, endPoint)
        || search_frag(type, wblk->block, results, recursiveSearch, endPoint))
        return true;

    return false;
}

bool search_if_block_fragment(
        fragment_type type,
        code_fragment *frag,
        List<code_fragment*> &results,
        bool recursiveSearch,
        code_fragment *endPoint
) {
    if(type == if_block_fragment) {
        results.add(frag);
        if(!recursiveSearch) return false;
    }

    if_blk *iblk = (if_blk*)frag;
    if(search_frag(type, iblk->conditionCheck, results, recursiveSearch, endPoint)
        || search_frag(type, iblk->block, results, recursiveSearch, endPoint))
        return true;

    return false;
}

bool search_debug_info_fragment(
        fragment_type type,
        code_fragment *frag,
        List<code_fragment*> &results,
        bool recursiveSearch
) {
    if(type == debug_info) {
        results.add(frag);
        if(!recursiveSearch) return false;
    }

    return false;
}

bool search_write_variable_fragment(
        fragment_type type,
        code_fragment *frag,
        List<code_fragment*> &results,
        bool recursiveSearch,
        code_fragment *endPoint
) {
    if(type == variable_write) {
        results.add(frag);
        if(!recursiveSearch) return false;
    }

    write_variable *wvar = (write_variable*)frag;
    if(search_frag(type, wvar->assignmentData, results, recursiveSearch, endPoint))
        return true;

    return false;
}

bool search_read_variable_fragment(
        fragment_type type,
        code_fragment *frag,
        List<code_fragment*> &results,
        bool recursiveSearch,
        code_fragment *endPoint
) {
    if(type == variable_read) {
        results.add(frag);
        if(!recursiveSearch) return false;
    }

    read_variable *rvar = (read_variable*)frag;
    for(Int i = 0; i < rvar->actions.size(); i++) {
        if(search_frag(type, rvar->actions[i], results, true, endPoint))
            return true;
    }

    return false;
}

bool search_new_class_fragment(
        fragment_type type,
        code_fragment *frag,
        List<code_fragment*> &results,
        bool recursiveSearch,
        code_fragment *endPoint
) {
    if(type == new_class_fragment) {
        results.add(frag);
        if(!recursiveSearch) return false;
    }

    new_class *nclass = (new_class*)frag;
    for(Int i = 0; i < nclass->params.size(); i++) {
        if(search_frag(type, nclass->params[i], results, true, endPoint))
            return true;
    }

    return false;
}

bool search_get_constant_fragment(
        fragment_type type,
        code_fragment *frag,
        List<code_fragment*> &results,
        bool recursiveSearch
) {
    if(type == get_constant) {
        results.add(frag);
        if(!recursiveSearch) return false;
    }

    return false;
}

bool search_break_fragment(
        fragment_type type,
        code_fragment *frag,
        List<code_fragment*> &results,
        bool recursiveSearch
) {
    if(type == break_fragment) {
        results.add(frag);
        if(!recursiveSearch) return false;
    }

    return false;
}

bool search_function_call_fragment(
        fragment_type type,
        code_fragment *frag,
        List<code_fragment*> &results,
        bool recursiveSearch,
        code_fragment *endPoint
) {
    if(type == function_call_fragment) {
        results.add(frag);
        if(!recursiveSearch) return false;
    }

    function_call *fcall = (function_call*)frag;
    for(Int i = 0; i < fcall->actions.size(); i++) {
        if(search_frag(type, fcall->actions[i], results, true, endPoint))
            return true;
    }

    return false;
}


bool search_frag(
        fragment_type type,
        code_fragment *frag,
        List<code_fragment*> &results,
        bool recursiveSearch,
        code_fragment *endPoint
) {
    if(frag == endPoint) {
        return true;
    }

    bool cancel = false;
    switch(frag->type) {
        case code_block_fragment:
            cancel = search_block_fragment(type, frag, results, recursiveSearch, endPoint);
            break;
        case while_block_fragment:
            cancel = search_while_block_fragment(type, frag, results, recursiveSearch, endPoint);
            break;
        case if_block_fragment:
            cancel = search_if_block_fragment(type, frag, results, recursiveSearch, endPoint);
            break;
        case return_data:
            break;
        case data_nil:
            break;
        case debug_info:
            cancel = search_debug_info_fragment(type, frag, results, recursiveSearch);
            break;
        case variable_write:
            cancel = search_write_variable_fragment(type, frag, results, recursiveSearch, endPoint);
            break;
        case variable_read:
            cancel = search_read_variable_fragment(type, frag, results, recursiveSearch, endPoint);
            break;
        case new_class_fragment:
            cancel = search_new_class_fragment(type, frag, results, recursiveSearch, endPoint);
            break;
        case get_constant:
            cancel = search_get_constant_fragment(type, frag, results, recursiveSearch);
            break;
        case break_fragment:
            cancel = search_break_fragment(type, frag, results, recursiveSearch);
            break;
        case function_call_fragment:
            cancel = search_function_call_fragment(type, frag, results, recursiveSearch, endPoint);
            break;
    }

    return cancel;
}

void extract_fragments(
        fragment_type type,
        code_fragment *frag,
        List<code_fragment*> &results,
        bool recursiveSearch,
        code_fragment *endPoint
) {
    search_frag(type, frag, results, recursiveSearch, endPoint);
}
