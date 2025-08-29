//
// Created by bknun on 7/9/2023.
//

#include "single_set_variable_processor.h"
#include "fragment_extractor.h"

void process_single_set_variables(List<write_variable*> &variables, code_fragment *origin) {

    sharp_field *duplicateWriteVar = NULL;
    List<code_fragment*> writes;
    extract_fragments(variable_write, origin, writes, true);


    for(Int i = 0; i < writes.size(); i++) {
        write_variable *var = (write_variable*)writes[i];
        
        for(Int j = 0; j < writes.size(); j++) {
            if(j == i) continue;
            write_variable *vwrite = (write_variable*)writes[j];
            
            if(vwrite->variable == var->variable) {
                // remove all occurrences of variable "?" writes
                duplicateWriteVar = vwrite->variable;
                for(Int g = 0; g < writes.size(); g++) {
                    if(((write_variable*)writes[g])->variable == duplicateWriteVar) {
                        writes.removeAt(g);
                        g = 0;
                    }
                }

                duplicateWriteVar = NULL;
                i = 0;
                break;
            }
        }
    }

    for(Int i = 0; i < writes.size(); i++) {
        variables.add((write_variable *) writes[i]);
    }
}