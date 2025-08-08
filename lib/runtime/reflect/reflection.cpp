//
// Created by Dev {Code} on 4/7/25.
//

#include "reflection.h"
#include "../virtual_machine.h"
#include "../exe/manifest.h"
#include "../memory/sharp_object.h"
#include "../memory/garbage_collector.h"
#include "reflect_helpers.h"
#include "../types/sharp_field.h"


sharp_object* create_description_object() {
    auto sc = create_object(vm.reflect_description);
    inc_ref(sc)
    return sc;
}

sharp_object* describe_object(sharp_object *o) {
    if(o == nullptr || o->type > type_class) {
        return nullptr;
    }

    auto description = create_description_object();
    string field_name;

    // data: object
    field_name = "data";
    assign_object_class_field(
            description,
            field_name,
            o
    );

    // is_array: bool
    field_name = "is_array";
    assign_numeric_class_field(
            instantiate_class_field(field_name, description, vm.bool_class)->o,
            o->arrayFlag ? 1 : 0
    );

    // type: data_type;
    field_name = "type";
    auto type = assign_instance_to_class_field(
            field_name,
            description,
            create_object(vm.reflect_data_type)
    );

    // data_type.type: int
    field_name = "type";
    assign_numeric_class_field(
            instantiate_class_field(field_name, type->o, vm.int_class)->o,
            (int)o->type
    );


    if(o->type == type_class) {
        auto sc = vm.classes + CLASS(o->info);
        // data_type.type_class: _class_?;
        field_name = "type_class";
        assign_instance_to_class_field(
                field_name,
                type->o,
                sc->reflect
        );
    } else if(o->type == type_function_ptr) {
        // data_type.type_function: _function_?;
        // todo: we need to look into how to handle this??
    }

    dec_ref(description)
    return description;
}

void create_reflect_object(sharp_class *sc) {
    sc->reflect = create_object(vm.reflect_class);
    SET_GENERATION(sc->reflect->info, gc_perm);
    inc_ref(sc->reflect)
}

sharp_object* build_reflect_function(sharp_function *fun) {
    string field_name;
    auto reflect = create_object(vm.reflect_function);
    SET_GENERATION(reflect->info, gc_perm);
    inc_ref(reflect)

    // name: string
    field_name = "name";
    assign_string_class_field(
            instantiate_class_field(field_name, reflect, vm.string_class)->o,
            fun->name
    );

    // full_name: string
    field_name = "full_name";
    assign_string_class_field(
            instantiate_class_field(field_name, reflect, vm.string_class)->o,
            fun->fullName
    );

    // id: int
    field_name = "id";
    assign_numeric_class_field(
            instantiate_class_field(field_name, reflect, vm.int_class)->o,
            fun->delegateAddress != -1 ? fun->delegateAddress : fun->address
    );

    // flags: int
    field_name = "flags";
    assign_numeric_class_field(
            instantiate_class_field(field_name, reflect, vm.int_class)->o,
            fun->flags
    );

    // owner: _class_?;
    if(fun->owner) {
        field_name = "owner";
        assign_instance_to_class_field(
                field_name,
                reflect,
                fun->owner->reflect
        );
    }

    // type: int;
    field_name = "type";
    assign_numeric_class_field(
            instantiate_class_field(field_name, reflect, vm.int_class)->o,
            (int)fun->fnType
    );

    // return_type: data_type;
    field_name = "return_type";
    auto returnType = assign_instance_to_class_field(
            field_name,
            reflect,
            create_object(vm.reflect_data_type)
    );

    // data_type.type: int
    field_name = "type";
    assign_numeric_class_field(
            instantiate_class_field(field_name, returnType->o, vm.int_class)->o,
            (int)fun->returnType->type
    );

    if(fun->returnType->type == type_class) {
        // data_type.type_class: _class_?;
        field_name = "type_class";
        assign_instance_to_class_field(
                field_name,
                returnType->o,
                fun->returnType->sc->reflect
        );
    } else if(fun->returnType->type == type_function_ptr) {
        // data_type.type_function: _function_?;
        field_name = "type_function";
        assign_instance_to_class_field(
                field_name,
                returnType->o,
                build_reflect_function(fun->returnType->fun)
        );
    }

    // array_return: boolean;
    field_name = "array_return";
    assign_numeric_class_field(
            instantiate_class_field(field_name, reflect, vm.bool_class)->o,
            fun->arrayReturn ? 1 : 0
    );

    // is_delegate: boolean;
    field_name = "is_delegate";
    assign_numeric_class_field(
            instantiate_class_field(field_name, reflect, vm.bool_class)->o,
            fun->delegateAddress != -1 ? 1 : 0
    );

    // params: _function_param_[]?;
    field_name = "params";
    if(fun->paramSize > 0) {
        auto params = instantiate_class_field_array(
                field_name, reflect, vm.reflect_func_params, fun->paramSize
        );

        if(params) {
            for(Int i = 0; i < fun->paramSize; i++) {
                auto param = params->o->node + i;
                auto paramField = fun->params + i;

                copy_object(
                        param,
                        create_object(vm.reflect_func_params)
                );

                // type: data_type;
                field_name = "type";
                auto type = assign_instance_to_class_field(
                        field_name,
                        param->o,
                        create_object(vm.reflect_data_type)
                );

                // data_type.type: int
                field_name = "type";
                assign_numeric_class_field(
                        instantiate_class_field(field_name, type->o, vm.int_class)->o,
                        (int)paramField->type->type
                );

                if(paramField->type->type == type_class) {
                    // data_type.type_class: _class_?;
                    field_name = "type_class";
                    assign_instance_to_class_field(
                            field_name,
                            type->o,
                            paramField->type->sc->reflect
                    );
                } else if(paramField->type->type == type_function_ptr) {
                    // data_type.function_type: _function_?;
                    field_name = "type_function";
                    assign_instance_to_class_field(
                            field_name,
                            type->o,
                            build_reflect_function(paramField->type->fun)
                    );
                }

                // is_array: bool;
                field_name = "is_array";
                assign_numeric_class_field(
                        instantiate_class_field(field_name, param->o, vm.bool_class)->o,
                        paramField->isArray ? 1 : 0
                );
            }
        }
    }

    return reflect;
}

void build_reflect_fields(sharp_field *class_fields, Int count, string reflect_fiend_name, sharp_object *reflect_owner) {
    string field_name;
    auto fields = instantiate_class_field_array(
            reflect_fiend_name, reflect_owner, vm.reflect_field, count
    );

    if(fields) {
        for(Int i = 0; i < count; i++) {
            auto field = fields->o->node + i;
            auto& classField = class_fields[i];

            copy_object(
                    field,
                    create_object(vm.reflect_field)
            );


            // name: string
            field_name = "name";
            assign_string_class_field(
                    instantiate_class_field(field_name, field->o, vm.string_class)->o,
                    classField.name
            );

            // full_name: string
            field_name = "full_name";
            assign_string_class_field(
                    instantiate_class_field(field_name, field->o, vm.string_class)->o,
                    classField.fullName
            );

            // index: int
            field_name = "index";
            assign_numeric_class_field(
                    instantiate_class_field(field_name, field->o, vm.int_class)->o,
                    classField.address
            );

            // flags: int
            field_name = "flags";
            assign_numeric_class_field(
                    instantiate_class_field(field_name, field->o, vm.int_class)->o,
                    classField.flags
            );

            // owner: _class_;
            field_name = "owner";
            assign_instance_to_class_field(
                    field_name,
                    field->o,
                    reflect_owner
            );

            // is_array: boolean
            field_name = "is_array";
            assign_numeric_class_field(
                    instantiate_class_field(field_name, field->o, vm.bool_class)->o,
                    classField.isArray ? 1 : 0
            );

            // type: data_type;
            field_name = "type";
            auto type = assign_instance_to_class_field(
                    field_name,
                    field->o,
                    create_object(vm.reflect_data_type)
            );

            // data_type.type: int
            field_name = "type";
            assign_numeric_class_field(
                    instantiate_class_field(field_name, type->o, vm.int_class)->o,
                    (int)classField.type->type
            );

            if(classField.type->type == type_class) {
                // data_type.type_class: _class_?;
                field_name = "type_class";
                assign_instance_to_class_field(
                        field_name,
                        type->o,
                        classField.type->sc->reflect
                );
            } else if(classField.type->type == type_function_ptr) {
                // data_type.type_function: _function_?;
                field_name = "type_function";
                assign_instance_to_class_field(
                        field_name,
                        type->o,
                        build_reflect_function(classField.type->fun)
                );
            }
        }
    }
}

void build_reflect_object(sharp_class *sc) {
    string field_name;

    // name: string
    field_name = "name";
    assign_string_class_field(
        instantiate_class_field(field_name, sc->reflect, vm.string_class)->o,
        sc->name
    );

    // full_name: string
    field_name = "full_name";
    assign_string_class_field(
        instantiate_class_field(field_name, sc->reflect, vm.string_class)->o,
        sc->fullName
    );

    // id: int
    field_name = "id";
    assign_numeric_class_field(
        instantiate_class_field(field_name, sc->reflect, vm.int_class)->o,
        sc->address
    );

    // hash: int
    field_name = "hash";
    assign_numeric_class_field(
        instantiate_class_field(field_name, sc->reflect, vm.int_class)->o,
        sc->hash
    );

    // flags: int
    field_name = "flags";
    assign_numeric_class_field(
        instantiate_class_field(field_name, sc->reflect, vm.int_class)->o,
        sc->flags
    );

    // base_class: _class_?
    field_name = "base_class";
    if(sc->base) {
        assign_instance_to_class_field(
                field_name,
                sc->reflect,
                sc->base->reflect
        );
    }

    // owner: _class_?
    field_name = "owner";
    if(sc->owner) {
        assign_instance_to_class_field(
                field_name,
                sc->reflect,
                sc->owner->reflect
        );
    }

    // interfaces: _class_[]?;
    field_name = "interfaces";
    if(sc->interfaces && sc->interfaceCount > 0) {
        auto interfaces = instantiate_class_field_array(
                field_name, sc->reflect, vm.reflect_class, sc->interfaceCount
        );

        if(interfaces) {
            for(Int i = 0; i < sc->interfaceCount; i++) {
                copy_object(interfaces->o->node + i, sc->interfaces[i]->reflect);
            }
        }
    }

    // fields: _field_[]?;
    field_name = "fields";
    if(sc->fields && sc->instanceFields >= 1) {
        build_reflect_fields(sc->fields, sc->instanceFields,
                             field_name, sc->reflect);
    }

    // staticFields: _field_[]?;
    field_name = "static_fields";
    if(sc->fields && sc->staticFields >= 1) {
        build_reflect_fields(sc->fields + sc->instanceFields,
                             sc->staticFields, field_name, sc->reflect);
    }

    // functions: _function_[]?;
    field_name = "functions";
    if(sc->methods && sc->methodCount > 0) {
        auto functions = instantiate_class_field_array(
                field_name, sc->reflect, vm.reflect_function, sc->methodCount
        );

        if(functions) {
            for(Int i = 0; i < sc->methodCount; i++) {
                auto fun = functions->o->node + i;
                auto classFunction = sc->methods[i];
                copy_object(fun, build_reflect_function(classFunction));
            }
        }
    }
}

void init_reflect_objects() {
    ignore_static_resolve(true);
    for(Int i = 0; i < vm.mf.classes; i++) {
        auto sc = vm.classes + i;
        create_reflect_object(sc);
    }

    for(Int i = 0; i < vm.mf.classes; i++) {
        auto sc = vm.classes + i;
        build_reflect_object(sc);
    }

    ignore_static_resolve(false);
}