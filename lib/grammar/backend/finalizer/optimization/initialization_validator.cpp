//
// Created by bknun on 1/9/2024.
//

#include "initialization_validator.h"
#include "../generation/generator.h"
#include "../../../../core/access_flag.h"
#include "../../../compiler_info.h"
#include "field_injector.h"
#include "processor/variable_rw_processor.h"
#include "analyzer/code_analyzer.h"

bool validationPassed = true;
bool requires_initialization(sharp_type &type) {
    return type.isArray || type == type_class
        || type == type_object || type == type_function_ptr;
}

void validate_global_field_initialization() {
    for(Int i = 0; i < compressedCompilationClasses.size(); i++) {
        auto sc = compressedCompilationClasses.get(i);

        if(check_flag(sc->flags, flag_global)) {
            for(Int j = 0; j < sc->fields.size(); j++) {
                auto sf = sc->fields.get(j);

                if(requires_initialization(sf->type) && check_flag(sf->flags, flag_static)
                    && sf->scheme == NULL && !sf->type.nullable && !check_flag(sf->flags, flag_excuse)) {
                    if(sf->preInitScheme != NULL) {
                        if(sf->used) {
                            sf->scheme = sf->preInitScheme;
                            sf->preInitScheme = NULL;
                            inject_field_initialization(sf->owner, sf->ast, sf);
                        }
                    } else {
                        validationPassed = false;
                        create_impl_error(GENERIC, sf->implLocation, "field `" + sf->name + "` must be initialized");
                    }
                }
            }
        }
    }
}

void validate_instance_class_field_initialization(sharp_class *sc) {
    for(Int i = 0; i < sc->fields.size(); i++) {
        auto sf = sc->fields.get(i);
        
        if(requires_initialization(sf->type) && !check_flag(sf->flags, flag_static)
            && sf->scheme == NULL && !sf->type.nullable && !check_flag(sf->flags, flag_excuse)) {
            if(sf->preInitScheme != NULL) {
                if(sf->used) {
                    sf->scheme = sf->preInitScheme;
                    sf->preInitScheme = NULL;
                    inject_field_initialization(sf->owner, sf->ast, sf);
                }
            } else {
                // check if field is assigned before read in all constructors
                if(sf->fullName == "std.io.fiber#fiber.main") {
                    int iol = 0;
                }
                bool fullCoverageInit = true;
                for(Int j = 0; j < sc->functions.size(); j++) {
                    auto func = sc->functions.get(j);
                    if(func->used && func->type == constructor_function) {
                        if(first_field_access_fragment(
                                sf,
                                require_non_null(analyze_code(func->scheme))) != variable_write) {
                            fullCoverageInit = false;
                            break;
                        }
                    }
                }

                // override if we have initialization in init
                if(!fullCoverageInit) {
                    string initializerFunc = instance_init_name(sc->name);
                    for(Int j = 0; j < sc->functions.size(); j++) {
                        auto func = sc->functions.get(j);
                        if(func->name ==  initializerFunc && func->used && func->parameters.empty()) {
                            if(first_field_access_fragment(
                                    sf,
                                    require_non_null(analyze_code(func->scheme))) == variable_write) {
                                fullCoverageInit = true;
                            }
                            break;
                        }
                    }
                }

                if(!fullCoverageInit) {
                    validationPassed = false;
                    create_impl_error(GENERIC, sf->implLocation, "field `" + sf->name + "` must be initialized");
                }
            }
        }
    }
}

void validate_static_class_field_initialization(sharp_class *sc) {
    for(Int i = 0; i < sc->fields.size(); i++) {
        auto sf = sc->fields.get(i);
        
        if(requires_initialization(sf->type) && check_flag(sf->flags, flag_static)
            && sf->scheme == NULL && !sf->type.nullable && !check_flag(sf->flags, flag_excuse)) {
            if(sf->preInitScheme != NULL) {
                if(sf->used) {
                    sf->scheme = sf->preInitScheme;
                    sf->preInitScheme = NULL;
                    inject_field_initialization(sf->owner, sf->ast, sf);
                }
            } else {
                bool fullCoverageInit = false;
                string initializerFunc = static_init_name(sc->name);
                for(Int j = 0; j < sc->functions.size(); j++) {
                    auto func = sc->functions.get(j);
                    if(func->name ==  initializerFunc && func->used && func->parameters.empty()) {
                        if(first_field_access_fragment(
                                sf,
                                require_non_null(analyze_code(func->scheme))) == variable_write) {
                            fullCoverageInit = true;
                        }
                        break;
                    }
                }

                if(!fullCoverageInit) {
                    validationPassed = false;
                    create_impl_error(GENERIC, sf->implLocation, "field `" + sf->name + "` must be initialized");
                }
            }
        }
    }
}

void validate_class_field_initialization() {
    for(Int i = 0; i < compressedCompilationClasses.size(); i++) {
        auto sc = compressedCompilationClasses.get(i);

        if(!check_flag(sc->flags, flag_global)) {
            validate_static_class_field_initialization(sc);
            validate_instance_class_field_initialization(sc);
        }
    }
}

void validateNonNullableInitializations() {
     validate_global_field_initialization();
     validate_class_field_initialization();

     if(!validationPassed) {
        exit(1);
     }
}
