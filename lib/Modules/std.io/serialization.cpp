//
// Created by BNunnally on 9/14/2020.
//
#include <iomanip>
#include "serialization.h"
#include "../../util/linked_list.h"
#include "../../runtime/memory/memory_helpers.h"
#include "../../runtime/memory/garbage_collector.h"
#include "../../core/exe_macros.h"
#include "../../runtime/virtual_machine.h"
#include "../../util/KeyPair.h"
#include "../../runtime/error/vm_exception.h"
#include "../../runtime/reflect/reflect_helpers.h"
#include "../../runtime/types/sharp_field.h"
#include <climits>

thread_local linkedlist<KeyPair<Int, sharp_object*>> processedObjects;
thread_local serialize_buffer_t header;
thread_local serialize_buffer_t body;
thread_local deserialize_buffer_t dBuffer;
thread_local serialized_classes_t searialized;
thread_local unsigned char doubleBytes[sizeof(long double)];
thread_local Int guid = OBJECT_ID_START;


CXX11_INLINE void alloc_buffer(serialize_buffer_t &buffer) {
    Int allocSize;
    if(buffer.size < MB_TO_BYTES(1)) // increase allocation size for large objects
        allocSize = BUFFER_ALLOC_CHUNK_SIZE_STANDARD;
    else allocSize = BUFFER_ALLOC_CHUNK_SIZE_LARGE;

    if(buffer.buf == nullptr) {
        buffer.buf = malloc_mem<char>(sizeof(char) * allocSize);
        buffer.size = allocSize;
    } else {
        buffer.buf = realloc_mem<char>(buffer.buf,
             sizeof(char) * (buffer.size + allocSize),
             sizeof(char) * buffer.size);
        buffer.size += allocSize;
    }
}

void clear_buffer(serialize_buffer_t &buf) {
    buf.pos = -1;
    guid = OBJECT_ID_START;
    if(buf.size >= (gc.memoryLimit * 0.01)) { 
        std::free(buf.buf); buf.buf = nullptr;
        release_bytes(sizeof(char) * buf.size);
        buf.size = 0;
    }
}

void clear_buffer(deserialize_buffer_t &buf) {
    buf.buf = nullptr;
    buf.pos = -1;
    buf.size = 0;
    std::free(buf.references); buf.references = nullptr;
}

void init_deserialization_buffer(long double *bytes, Int len) {
    dBuffer.buf = bytes;
    dBuffer.pos = -1;
    dBuffer.size = len;
    dBuffer.references = nullptr;
}

void init_serialized_classes() {
    if(searialized.serialized == nullptr) {
        searialized.serialized = malloc_mem<sharp_class*>(vm.mf.classes * sizeof(sharp_class*));
        searialized.size = vm.mf.classes;
    }

    searialized.count = 0;
}

void init_deserialized_buffer() {
    dBuffer.buf = nullptr;
    dBuffer.pos = -1;
    dBuffer.references = nullptr;
    dBuffer.size = 0;
}

void add_serialized_class(sharp_class *sc) {
    for(Int i = 0; i < searialized.count; i++) {
        if(sc->hash == searialized.serialized[i]->hash) {
            return;
        }
    }

    if(searialized.count >= searialized.size) {
        throw vm_exception("internal error: failed to add serialized class to buffer.");
    }

    searialized.serialized[searialized.count++] = sc;
}

void push_class_section() {
    push_data(header, CLASS_SECTION_BEGIN)
    push_int32(header, searialized.count)
    for(Int i = 0; i < searialized.count; i++) {
        auto klass = searialized.serialized[i];
        push_int32(header, klass->hash) // hash
        push_int32(header, klass->instanceFields)

        if(klass->fullName.size() <= SCHAR_MAX) { // name
            push_data(header, klass->fullName.size())
        } else {
            throw vm_exception("serialized class name too long for `" + klass->fullName + "`");
        }

        for (char c: klass->fullName) {
            push_data(header, c)
        }  
    }
      
    push_data(header, CLASS_SECTION_END)
}

void deserialize_class_section() {
    expect_data(CLASS_SECTION_BEGIN)
    Int classes;
    read_int32(classes)
    for(Int i = 0; i < classes; i++) {
        Int hash, fieldCount, classNameSize;
        read_int32(hash)
        read_int32(fieldCount)
        overflow_check
        classNameSize = read_data;

        string className;
        if((dBuffer.pos + classNameSize) >= dBuffer.size) { 
            throw vm_exception("invalid format: unexpected end of deserialization buffer");
        }

        for(int i = 0; i < classNameSize; i++) {
            className += read_data;
        }
        
        auto klass = locate_class_hashed(hash);
        if(klass == nullptr) {
            throw vm_exception("class not found: unable to locate serialized class `" + className + "`");
        }

        if(klass->fullName != className) {
            throw vm_exception("hash mismatch: unexpected hash mismatch when trying to deserialize class `" + className + "`");
        }

        if(klass->instanceFields != fieldCount) {
            throw vm_exception("field count mismatch: mismatching field sizes found when trying to deserialize class `" + className + "`");
        }
    }

    expect_data(CLASS_SECTION_END)
}

void serialize_object(sharp_object *o) {
    if(o != nullptr) {
        auto processedObj = processedObjects.node_at(o, [](void *obj, node<KeyPair<Int, sharp_object *>> *n) {
            return n->data.value == obj;
        });

        if (processedObj == nullptr) {
            Int id = guid++;
            processedObjects.createnode(KeyPair<Int, sharp_object *>(id, o));

            if(o->type == type_class) {
                push_data(body, CLASS_OBJECT)
                push_int32(body, id)
                push_int32(body, o->size)
                auto klass = vm.classes + CLASS(o->info);
                push_int32(body, klass->hash)
                add_serialized_class(klass);


                for (Int i = 0; i < o->size; i++) {
                    serialize_object(o->node[i].o);
                }
            } else if(o->type == type_object) {
                push_data(body, STANDARD_OBJECT)
                push_int32(body, id)
                push_int32(body, o->size)

                for (Int i = 0; i < o->size; i++) {
                    serialize_object(o->node[i].o);
                }
            } else if(o->type <= type_var) {
                push_data(body, NUMERIC_OBJECT)
                push_int32(body, id)
                push_int32(body, o->size)
                push_data(body, o->type)

                for (Int i = 0; i < o->size; i++) {
                    push_double(body, o->HEAD[i])
                }
            } else {
                throw vm_exception("invalid type found while serializing object");
            }
        } else {
            push_data(body, REFERENCE_OBJECT)
            push_int32(body, processedObj->data.key)
        }
    } else {
        push_data(body, NULL_OBJECT)
    }
}

void serialize(object *from, object *to) {
    try {
        processedObjects.delete_all(nullptr); // just in case
        init_serialized_classes();
        guid = OBJECT_ID_START;

        push_data(header, SERIALIZE_START)
        serialize_object(from->o);
        push_int32(header, processedObjects.size)
        push_class_section();
        push_data(body, SERIALIZE_END)

        // todo: join header + body into 1


        auto serializedData = create_object((header.pos + body.pos) + 2, type_int8);
        copy_object(to, serializedData);

        for(Int i = 0; i < (header.pos + 1); i++) {
            serializedData->HEAD[i] = header.buf[i];
        }

        Int realPos = header.pos + 1;
        for(Int i = 0; i < (body.pos + 1); i++) {
            serializedData->HEAD[realPos++] = body.buf[i];
        }

        clear_buffer(header);
        clear_buffer(body);
    } catch(runtime_error &err) {
        copy_object(to, (sharp_object*) nullptr);
        throw vm_exception(err.what());
    }

    processedObjects.delete_all(nullptr);
}

void deserialize_object(object *to, sharp_field *field) {
    overflow_check
    Int objType = read_data;

    if(objType == NULL_OBJECT) {
        copy_object(to, (sharp_object*) nullptr);
    } else if(objType == REFERENCE_OBJECT) {
        Int id;
        read_int32(id)
        auto processedObj = dBuffer.references[id];

        if(processedObj != nullptr) {
            copy_object(to, processedObj);
        } else {
            throw vm_exception("invalid object reference found in deserialization buffer");
        }
    } else if(objType == NUMERIC_OBJECT) {
        Int size, type, id;
        sharp_object *deserializedObject;
        read_int32(id)
        read_int32(size)
        overflow_check
        type = read_data;

        if(field != nullptr && type != field->type->type) {
            throw vm_exception("type mismatch on field `" + field->name + 
                "` during deserialization of class `" + field->owner->fullName + "`");
        }

        deserializedObject = create_object(size, (data_type)type);
        dBuffer.references[id] = deserializedObject;
        copy_object(to, deserializedObject);

        for(Int i = 0; i < size; i++) {
            read_double(deserializedObject->HEAD[i])
        }
    }
    else if(objType == STANDARD_OBJECT) {
        Int size, id;
        sharp_object *deserializedObject;
        read_int32(id)
        read_int32(size)

        if(field != nullptr && type_object != field->type->type) {
            throw vm_exception("type mismatch on field `" + field->name + 
                "` during deserialization of class `" + field->owner->fullName + "`");
        }

        deserializedObject = create_object(size);
        dBuffer.references[id] = deserializedObject;
        copy_object(to, deserializedObject);

        for(Int i = 0; i < size; i++) {
            deserialize_object(deserializedObject->node + i, nullptr);
        }
    }
    else if(objType == CLASS_OBJECT) {
        Int size, hash, id;
        sharp_object *deserializedObject;
        read_int32(id)
        read_int32(size)
        read_int32(hash)
        auto klass = locate_class_hashed(hash);

        if(size == 1) {
            deserializedObject = create_object(klass);
        } else {
            deserializedObject = create_object(klass, size);
        }

        dBuffer.references[id] = deserializedObject;
        copy_object(to, deserializedObject);

        if(size > 1) {
            for(Int i = 0; i < size; i++) {
                deserialize_object(deserializedObject->node + i, nullptr);
            }
        } else {
            for(Int i = 0; i < klass->instanceFields; i++) {
                deserialize_object(deserializedObject->node + i, klass->fields + i);
            }
        }
    } else {
        throw vm_exception("unexpected data found in deserialization buffer");
    }
}

void deserialize(object *from, object *to) {
    try {
        if(from->o && from->o->type <= type_var) {
            init_deserialization_buffer(from->o->HEAD, from->o->size);
            init_serialized_classes();
            guid = OBJECT_ID_START;

            expect_data(SERIALIZE_START)
            Int objects;
            read_int32(objects)
            dBuffer.references = (sharp_object**)calloc_mem<sharp_object*>(objects, sizeof(sharp_object*));
            deserialize_class_section();

            deserialize_object(to, nullptr);
            expect_data(SERIALIZE_END)

            clear_buffer(dBuffer);
        } else {
            throw vm_exception("attempting to deserialize null or non-string object");
        }
    } catch(runtime_error &err) {
        clear_buffer(dBuffer);
        copy_object(to, (sharp_object*) nullptr);
        throw vm_exception(err.what());
    }
}

