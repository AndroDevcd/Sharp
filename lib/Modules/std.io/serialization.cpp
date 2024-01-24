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
thread_local Int serializeBufferSize = 0;
thread_local Int deserializerBufferSize = 0;
thread_local Int bufferPos = -1;
thread_local Int guid = OBJECT_ID_START;

CXX11_INLINE void alloc_buffer() {
    Int allocSize;
    if(serializeBufferSize < MB_TO_BYTES(1)) // increase allocation size for large objects
        allocSize = BUFFER_ALLOC_CHUNK_SIZE_STANDARD;
    else allocSize = BUFFER_ALLOC_CHUNK_SIZE_LARGE;

    if(serializeBuffer == nullptr) {
        serializeBuffer = malloc_mem<char>(sizeof(char) * allocSize);
        serializeBufferSize = allocSize;
    } else {
        serializeBuffer = realloc_mem<char>(serializeBuffer,
             sizeof(char) * (serializeBufferSize + allocSize),
             sizeof(char) * serializeBufferSize);
        serializeBufferSize += allocSize;
    }
}

void preserve_or_free_serialize_buffer() {
    if(serializeBufferSize >= BUFFER_ALLOC_CACHE_LIMIT) {
        std::free(serializeBuffer); serializeBuffer = nullptr;
        release_bytes(sizeof(char) * serializeBufferSize);
        serializeBufferSize = 0;
    }

    bufferPos = -1;
}

void serialize_object(sharp_object *o) {
    if(o != nullptr) {
        auto processedObj = processedObjects.node_at(o, [](void *obj, node<KeyPair<Int, sharp_object *>> *n) {
            return n->data.value == obj;
        });

        if (processedObj == nullptr) {
            Int id = ++guid;
            push_data(serializeBufferSize, BEGIN_OBJECT)
            push_int32(serializeBufferSize, id)
            push_int32(serializeBufferSize, o->size)
            push_int32(serializeBufferSize, o->type)

            processedObjects.insert_start(KeyPair<Int, sharp_object *>(id, o));
            if (o->type == type_class) {
                push_data(serializeBufferSize, CLASS_NAME_BEGIN)

                auto classType = vm.classes + CLASS(o->info);
                for (char c: classType->name) {
                    push_data(serializeBufferSize, c)
                }

                push_data(serializeBufferSize, CLASS_NAME_END)
            }

            push_data(serializeBufferSize, DATA_BEGIN)
            if (o->type <= type_var) {
                for (Int i = 0; i < o->size; i++) {
                    string value = to_string(o->HEAD[i]);
                    push_data(serializeBufferSize, ITEM_START)
                    for (char c: value) {
                        push_data(serializeBufferSize, c)
                    }
                    push_data(serializeBufferSize, ITEM_END)
                }
            } else {
                for (Int i = 0; i < o->size; i++) {
                    push_data(serializeBufferSize, ITEM_START)
                    serialize_object(o->node[i].o);
                    push_data(serializeBufferSize, ITEM_END)
                }
            }
            push_data(serializeBufferSize, DATA_END)
        } else {
            push_data(serializeBufferSize, BEGIN_OBJECT)
            push_data(serializeBufferSize, REFERENCE_OBJECT)
            push_int32(serializeBufferSize, processedObj->data.key)
        }
    } else {
        push_data(serializeBufferSize, BEGIN_OBJECT)
        push_data(serializeBufferSize, NULL_OBJECT)
    }
}

void serialize(object *from, object *to) {
    try {
        processedObjects.delete_all(nullptr); // just in case
        guid = OBJECT_ID_START;

        push_data(serializeBufferSize, SERIALIZE_START)
        serialize_object(from->o);
        push_data(serializeBufferSize, SERIALIZE_END)

        auto serializedData = create_object(bufferPos + 1, type_int8);
        copy_object(to, serializedData);

        for(Int i = 0; i < (bufferPos + 1); i++) {
            serializedData->HEAD[i] = serializeBuffer[i];
        }
    } catch(runtime_error &err) {}

    processedObjects.delete_all(nullptr);
    preserve_or_free_serialize_buffer();
}

void deserialize_object(object *to) {
    expect_data(deserializerBufferSize, BEGIN_OBJECT)

    Int typeOrId, size, type;
    read_int32(deserializerBufferSize, typeOrId)
    read_int32(deserializerBufferSize, size)
    read_int32(deserializerBufferSize, type)

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
            expect_data(deserializerBufferSize, CLASS_NAME_BEGIN)
            string className;

            for(;;) {
                if((bufferPos + 1) >= deserializerBufferSize) {
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
        expect_data(deserializerBufferSize, DATA_BEGIN)

        for(Int i = 0; i < size; i++) {
            expect_data(deserializerBufferSize, ITEM_START)
            if(type <= type_var) {
                string number;
                for(;;) {
                    if((bufferPos + 1) >= deserializerBufferSize) {
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
                expect_data(deserializerBufferSize, ITEM_END)
            }
        }

        expect_data(deserializerBufferSize, DATA_END)
    }
}

void deserialize(object *from, object *to) {
    try {
        if(from->o && from->o->type <= type_var) {
            deserializerBufferSize = from->o->size;
            bufferPos = -1;
            deserializeBuffer = from->o->HEAD;
            guid = OBJECT_ID_START;

            expect_data(deserializerBufferSize, SERIALIZE_START)
            deserialize_object(to);
            expect_data(deserializerBufferSize, SERIALIZE_END)

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

