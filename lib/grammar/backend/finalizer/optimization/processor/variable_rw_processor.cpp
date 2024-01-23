#include "variable_rw_processor.h"

fragment_type first_field_access_fragment(sharp_field *field, code_fragment *origin) {
    switch(origin->type) {
        case unsupported_fragment: {
            auto frag = (unsupported*)origin;
            for(Int i = 0; i < frag->nodes.size(); i++) {
                auto result = first_field_access_fragment(field, frag->nodes.get(i));

                if(result == variable_write || result == variable_read) 
                    return result;
            }
            break;
        }
        case code_block_fragment: {
            auto frag = (code_block*)origin;
            for(Int i = 0; i < frag->nodes.size(); i++) {
                auto result = first_field_access_fragment(field, frag->nodes.get(i));

                if(result == variable_write || result == variable_read) 
                    return result;
            }
            break;
        }
        case while_block_fragment: {
            auto frag = (while_loop*)origin;

            auto result = first_field_access_fragment(field, frag->conditionCheck);
            if(result == variable_write || result == variable_read) 
                return result;

            result = first_field_access_fragment(field, frag->block);
            if(result == variable_write || result == variable_read) 
                return result;
            break;
        }
        case if_block_fragment: {
            auto frag = (if_blk*)origin;

            auto result = first_field_access_fragment(field, frag->conditionCheck);
            if(result == variable_write || result == variable_read) 
                return result;

            result = first_field_access_fragment(field, frag->block);
            if(result == variable_write || result == variable_read) 
                return result;
            break;
        }
        case data_nil: {
            return no_fragment;
        }
        case debug_info: {
            return no_fragment;
        }
        case variable_write: {
            auto frag = (write_variable*)origin;
            if(frag->variable == field)
                return variable_write;

            auto result = first_field_access_fragment(field, frag->assignmentData);
            if(result == variable_write || result == variable_read) 
                return result;
            break;
        }
        case variable_read: {
            auto frag = (read_variable*)origin;
            if(frag->variable == field)
                return variable_read;

            for(Int i = 0; i < frag->actions.size(); i++) {
                auto result = first_field_access_fragment(field, frag->actions.get(i));

                if(result == variable_write || result == variable_read) 
                    return result;
            }
            break;
        }
        case new_class_fragment: {
            auto frag = (new_class*)origin;
            for(Int i = 0; i < frag->params.size(); i++) {
                auto result = first_field_access_fragment(field, frag->params.get(i));

                if(result == variable_write || result == variable_read) 
                    return result;
            }
            break;
        }
        case get_constant: {
            return no_fragment;
        }
        case break_fragment: {
            return no_fragment;
        }
        case function_call_fragment: {
            auto frag = (function_call*)origin;
            for(Int i = 0; i < frag->actions.size(); i++) {
                auto result = first_field_access_fragment(field, frag->actions.get(i));

                if(result == variable_write || result == variable_read) 
                    return result;
            }
            break;
        }
    }

    return no_fragment;
}