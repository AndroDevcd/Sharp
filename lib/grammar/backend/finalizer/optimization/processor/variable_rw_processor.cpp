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
            if(frag->variable == field) {
                auto result = first_field_access_fragment(field, frag->assignmentData);
                if(result == no_fragment)
                    return variable_write;
                else return no_fragment;
            }

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
        case no_fragment:
            break;
        case negated_value_fragment: {
            auto frag = (negated_value*)origin;

            auto result = first_field_access_fragment(field, frag->value);
            if(result == variable_write || result == variable_read)
                return result;
            break;
        }
        case incremented_value_fragment: {
            auto frag = (incremented_value*)origin;

            auto result = first_field_access_fragment(field, frag->value);
            if(result == variable_write || result == variable_read)
                return result;
            break;
        }
        case decremented_value_fragment: {
            auto frag = (decremented_value*)origin;

            auto result = first_field_access_fragment(field, frag->value);
            if(result == variable_write || result == variable_read)
                return result;
            break;
        }
        case not_value_fragment: {
            auto frag = (not_value*)origin;

            auto result = first_field_access_fragment(field, frag->value);
            if(result == variable_write || result == variable_read)
                return result;
            break;
        }
    }

    return no_fragment;
}