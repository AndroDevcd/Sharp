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

thread_local linkedlist<KeyPair<Int, sharp_object*>> processedObjects;
thread_local char *serializeBuffer = nullptr;
thread_local long double *deserializeBuffer = nullptr;
thread_local Int bufferSize = 0;
thread_local Int bufferPos = -1;
thread_local Int guid = OBJECT_ID_START;

CXX11_INLINE void alloc_buffer() {
    Int allocSize;
    if(bufferSize < MB_TO_BYTES(1)) // increase allocation size for large objects
        allocSize = BUFFER_ALLOC_CHUNK_SIZE_STANDARD;
    else allocSize = BUFFER_ALLOC_CHUNK_SIZE_LARGE;

    if(serializeBuffer == nullptr) {
        serializeBuffer = malloc_mem<char>(sizeof(char) * allocSize);
        bufferSize = allocSize;
    } else {
        serializeBuffer = realloc_mem<char>(serializeBuffer,
             sizeof(char) * (bufferSize + allocSize),
             sizeof(char) * bufferSize);
        bufferSize += allocSize;
    }
}

void serialize_object(sharp_object *o) {
    if(o != nullptr) {
        auto processedObj = processedObjects.node_at(o, [](void *obj, node<KeyPair<Int, sharp_object *>> *n) {
            return n->data.value == obj;
        });

        if (processedObj == nullptr) {
            Int id = ++guid;
            push_data(BEGIN_OBJECT)
            push_int32(id)
            push_int32(o->size)
            push_int32(o->type)

            processedObjects.insert_start(KeyPair<Int, sharp_object *>(id, o));
            if (o->type == type_class) {
                push_data(CLASS_NAME_BEGIN)

                auto classType = vm.classes + CLASS(o->info);
                for (char c: classType->name) {
                    push_data(c)
                }

                push_data(CLASS_NAME_END)
            }

            push_data(DATA_BEGIN)
            if (o->type <= type_var) {
                for (Int i = 0; i < o->size; i++) {
                    string value = to_string(o->HEAD[i]);
                    push_data(ITEM_START)
                    for (char c: value) {
                        push_data(c)
                    }
                    push_data(ITEM_END)
                }
            } else {
                for (Int i = 0; i < o->size; i++) {
                    push_data(ITEM_START)
                    serialize_object(o->node[i].o);
                    push_data(ITEM_END)
                }
            }
            push_data(DATA_END)
        } else {
            push_data(BEGIN_OBJECT)
            push_data(REFERENCE_OBJECT)
            push_int32(processedObj->data.key)
        }
    } else {
        push_data(BEGIN_OBJECT)
        push_data(NULL_OBJECT)
    }
}

void serialize(object *from, object *to) {
    try {
        processedObjects.delete_all(nullptr); // just in case
        bufferPos = -1;
        bufferSize = 0;
        guid = OBJECT_ID_START;

        push_data(SERIALIZE_START)
        serialize_object(from->o);
        push_data(SERIALIZE_END)

        auto serializedData = create_object(bufferPos + 1, type_int8);
        copy_object(to, serializedData);

        for(Int i = 0; i < (bufferPos + 1); i++) {
            serializedData->HEAD[i] = serializeBuffer[i];
        }
    } catch(runtime_error &err) {}

    processedObjects.delete_all(nullptr);
    std::free(serializeBuffer); serializeBuffer = nullptr;
    release_bytes(sizeof(char) * bufferSize);
}

void deserialize_object(object *to) {
    expect_data(BEGIN_OBJECT)

    Int typeOrId, size, type;
    read_int32(typeOrId)
    read_int32(size)
    read_int32(type)

    if(typeOrId == NULL_OBJECT) {
        copy_object(to, (sharp_object*) nullptr);
    } else if(typeOrId == REFERENCE_OBJECT) {
        auto processedObj = processedObjects.node_at((void*)typeOrId, [](void *id, node<KeyPair<Int, sharp_object *>> *n) {
            return (Int)id == n->data.key;
        });

        if(processedObj != nullptr) {
            copy_object(to, processedObj->data.value);
        } else {
            throw vm_exception("invalid object reference found in deserialization buffer");
        }
    } else {
        sharp_class *serializedClass = nullptr;
        sharp_object *deserializedObject;

        if (type == type_class) {
            expect_data(CLASS_NAME_BEGIN)
            string className;

            for(;;) {
                if((bufferPos + 1) >= bufferSize) {
                    throw vm_exception("invalid format: unexpected end of deserialization buffer");
                }

                auto data = read_data;

                if(data == CLASS_NAME_END)
                    break;
                else {
                    className += data;
                }
            }

            serializedClass = locate_class_simple(className.c_str());
            if(serializedClass == nullptr) {
                throw vm_exception("failed to load class `" + className + "` from deserialization buffer");
            }

            if(size == 1) {
                deserializedObject = create_object(serializedClass);
            } else {
                deserializedObject = create_object(serializedClass, size);
            }
        } else if(type <= type_var) {
            deserializedObject = create_object(size, (data_type)type);
        } else {
            deserializedObject = create_object(size);
        }

        processedObjects.insert_start(KeyPair<Int, sharp_object *>(typeOrId, deserializedObject));
        copy_object(to, deserializedObject);
        expect_data(DATA_BEGIN)

        for(Int i = 0; i < size; i++) {
            expect_data(ITEM_START)
            if(type <= type_var) {
                string number;
                for(;;) {
                    if((bufferPos + 1) >= bufferSize) {
                        throw vm_exception("invalid format: unexpected end of deserialization buffer");
                    }

                    auto data = read_data;

                    if(data == ITEM_END)
                        break;
                    else {
                        number += data;
                    }
                }

                deserializedObject->HEAD[i] = strtod(number.c_str(), nullptr);
            } else {
                deserialize_object(deserializedObject->node + i);
                expect_data(ITEM_END)
            }
        }

        expect_data(DATA_END)
    }
}

void deserialize(object *from, object *to) {
    try {
        if(from->o && from->o->type <= type_var) {
            bufferSize = from->o->size;
            bufferPos = -1;
            deserializeBuffer = from->o->HEAD;
            guid = OBJECT_ID_START;

            expect_data(SERIALIZE_START)
            deserialize_object(to);
            expect_data(SERIALIZE_END)

            deserializeBuffer = nullptr;
            processedObjects.delete_all(nullptr);
        } else {
            throw vm_exception("attempting to deserialize null or non-string object");
        }
    } catch(runtime_error &err) {
        processedObjects.delete_all(nullptr);
        copy_object(to, (sharp_object*) nullptr);
    }
}

