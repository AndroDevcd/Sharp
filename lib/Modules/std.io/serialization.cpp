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
#include "../../runtime/memory/garbage_collector.h"
#include <climits>
#include <unordered_map>
#include <vector>

// Thread-local serialization state
namespace {
    // Use hash map for faster object lookups (O(1) instead of O(n))
    thread_local std::unordered_map<sharp_object*, Int> objectMap;
    thread_local std::vector<sharp_object*> objectVector;
    thread_local serialized_classes_t serializedClasses;

    // Pre-allocated buffers
    thread_local serialize_buffer_t header;
    thread_local serialize_buffer_t body;
    thread_local deserialize_buffer_t dBuffer;

    // Reusable byte arrays
    thread_local unsigned char doubleBytes[sizeof(long double)];
    thread_local unsigned char _64Bytes[sizeof(int64_t)];
    thread_local unsigned char _16Bytes[sizeof(int16_t)];

    // Global counter
    thread_local Int guidCounter = OBJECT_ID_START;

    // Constants for initial buffer sizes
    constexpr Int INITIAL_HEADER_SIZE = KB_TO_BYTES(4);
    constexpr Int INITIAL_BODY_SIZE = KB_TO_BYTES(16);
    constexpr Int INITIAL_OBJECT_CAPACITY = 256;
}

// Fast buffer allocation with exponential growth
CXX11_INLINE void ensure_buffer_capacity(serialize_buffer_t& buffer, Int requiredSpace) {
    if (buffer.pos + requiredSpace >= buffer.size) {
        Int newSize = buffer.size == 0 ?
                      (&buffer == &header ? INITIAL_HEADER_SIZE : INITIAL_BODY_SIZE) :
                      buffer.size * 2;

        if (buffer.buf == nullptr) {
            buffer.buf = malloc_mem<char>(sizeof(char) * newSize);
            buffer.size = newSize;
        } else {
            buffer.buf = realloc_mem<char>(buffer.buf,
                                           sizeof(char) * newSize,
                                           sizeof(char) * buffer.size);
            buffer.size = newSize;
        }
    }
}

// Reset state between serialization operations
void reset_serialization_state() {
    objectMap.clear();
    objectVector.clear();
    guidCounter = OBJECT_ID_START;

    // Don't clear buffers, just reset position
    header.pos = -1;
    body.pos = -1;
}

// Clean up resources when done
void clean_serialization_resources() {
    if (header.size >= (gc.memoryLimit * 0.3)) {
        std::free(header.buf);
        header.buf = nullptr;
        release_bytes(sizeof(char) * header.size);
        header.size = 0;
    }

    if (body.size >= (gc.memoryLimit * 0.3)) {
        std::free(body.buf);
        body.buf = nullptr;
        release_bytes(sizeof(char) * body.size);
        body.size = 0;
    }

    objectMap.clear();
    objectVector.clear();
}

// Inline push functions for better performance
CXX11_INLINE void push_byte(serialize_buffer_t& buffer, char data) {
    if ((buffer.pos + 1) >= buffer.size) {
        ensure_buffer_capacity(buffer, 1);
    }
    buffer.buf[++buffer.pos] = data;
}

CXX11_INLINE void push_int32(serialize_buffer_t& buffer, int32_t data) {
    if ((buffer.pos + 4) >= buffer.size) {
        ensure_buffer_capacity(buffer, 4);
    }
    buffer.buf[++buffer.pos] = GET_i32w(data);
    buffer.buf[++buffer.pos] = GET_i32x(data);
    buffer.buf[++buffer.pos] = GET_i32y(data);
    buffer.buf[++buffer.pos] = GET_i32z(data);
}

// Template function for pushing any primitive type
template<typename T>
CXX11_INLINE void push_primitive(serialize_buffer_t& buffer, T data) {
    if ((buffer.pos + sizeof(T)) >= buffer.size) {
        ensure_buffer_capacity(buffer, sizeof(T));
    }

    const unsigned char* bytes = reinterpret_cast<const unsigned char*>(&data);
    for (size_t i = 0; i < sizeof(T); i++) {
        buffer.buf[++buffer.pos] = bytes[i];
    }
}

// Initialize serialization data structures
void init_serialization() {
    // Pre-allocate buffers if needed
    if (header.buf == nullptr && header.size == 0) {
        header.buf = malloc_mem<char>(INITIAL_HEADER_SIZE);
        header.size = INITIAL_HEADER_SIZE;
    }

    if (body.buf == nullptr && body.size == 0) {
        body.buf = malloc_mem<char>(INITIAL_BODY_SIZE);
        body.size = INITIAL_BODY_SIZE;
    }

    // Pre-allocate object vector capacity
    objectVector.reserve(INITIAL_OBJECT_CAPACITY);

    // Initialize serialized classes
    if (serializedClasses.serialized == nullptr) {
        serializedClasses.serialized = malloc_mem<sharp_class*>(vm.mf.classes * sizeof(sharp_class*));
        serializedClasses.size = vm.mf.classes;
    }
    serializedClasses.count = 0;
}

// Add a class to the serialized classes list
void add_serialized_class(sharp_class* sc) {
    for (Int i = 0; i < serializedClasses.count; i++) {
        if (sc->hash == serializedClasses.serialized[i]->hash) {
            return;
        }
    }

    if (serializedClasses.count >= serializedClasses.size) {
        throw vm_exception("internal error: failed to add serialized class to buffer.");
    }

    serializedClasses.serialized[serializedClasses.count++] = sc;
}

// Push class section to the header buffer
void push_class_section() {
    push_byte(header, CLASS_SECTION_BEGIN);
    push_int32(header, serializedClasses.count);

    for (Int i = 0; i < serializedClasses.count; i++) {
        auto klass = serializedClasses.serialized[i];
        push_int32(header, klass->hash);
        push_int32(header, klass->instanceFields);

        if (klass->fullName.size() <= SCHAR_MAX) {
            push_byte(header, klass->fullName.size());
        } else {
            throw vm_exception("serialized class name too long for `" + klass->fullName + "`");
        }

        const char* name = klass->fullName.c_str();
        const size_t nameLen = klass->fullName.size();

        // Ensure we have enough buffer space for the whole name
        ensure_buffer_capacity(header, nameLen);

        // Copy the entire name at once
        for (size_t i = 0; i < nameLen; i++) {
            header.buf[++header.pos] = name[i];
        }
    }

    push_byte(header, CLASS_SECTION_END);
}

// Recursive function to serialize an object
void serialize_object(sharp_object* o) {
    if (o == nullptr) {
        push_byte(body, NULL_OBJECT);
        return;
    }

    // Check if we've already processed this object
    auto it = objectMap.find(o);
    if (it != objectMap.end()) {
        // Reference to previously processed object
        push_byte(body, REFERENCE_OBJECT);
        push_int32(body, it->second);
        return;
    }

    // Add to processed objects
    Int id = guidCounter++;
    objectMap[o] = id;
    objectVector.push_back(o);

    // Serialize based on object type
    if (o->type == type_class) {
        if (o->arrayFlag == 1) {
            push_byte(body, CLASS_ARRAY_OBJECT);
        } else {
            push_byte(body, CLASS_OBJECT);
        }

        push_int32(body, id);

        if (o->arrayFlag == 1) {
            push_int32(body, o->size);
        }

        auto klass = vm.classes + CLASS(o->info);
        push_int32(body, klass->hash);
        add_serialized_class(klass);

        // Pre-check buffer capacity for all elements
        ensure_buffer_capacity(body, o->size * 5);  // Minimum bytes per object reference

        for (Int i = 0; i < o->size; i++) {
            serialize_object(o->node[i].o);
        }
    }
    else if (o->type == type_object) {
        push_byte(body, STANDARD_OBJECT);
        push_int32(body, id);
        push_int32(body, o->size);

        // Pre-check buffer capacity
        ensure_buffer_capacity(body, o->size * 5);

        for (Int i = 0; i < o->size; i++) {
            serialize_object(o->node[i].o);
        }
    }
    else if (o->type <= type_var) {
        push_byte(body, NUMERIC_OBJECT);
        push_int32(body, id);
        push_int32(body, o->size);
        push_byte(body, o->type);

        // Pre-allocate enough space for all primitive data
        Int elementSize = 0;
        switch (o->type) {
            case type_var: elementSize = sizeof(long double); break;
            case type_function_ptr:
            case type_int32:
            case type_uint32: elementSize = sizeof(int32_t); break;
            case type_int64:
            case type_uint64: elementSize = sizeof(int64_t); break;
            case type_int16:
            case type_uint16: elementSize = sizeof(int16_t); break;
            case type_int8:
            case type_uint8: elementSize = 1; break;
            default:
                throw vm_exception("invalid numeric type found while serializing object");
        }

        ensure_buffer_capacity(body, o->size * elementSize);

        // Batch serialize primitive arrays
        switch (o->type) {
            case type_var: {
                for (Int i = 0; i < o->size; i++) {
                    push_primitive<long double>(body, o->HEAD[i]);
                }
                return;
            }
            case type_function_ptr: {
                for (Int i = 0; i < o->size; i++) {
                    push_int32(body, (int32_t)o->HEAD[i]);
                }
                return;
            }
            case type_int64:
            case type_uint64: {
                for (Int i = 0; i < o->size; i++) {
                    push_primitive<int64_t>(body, (int64_t)o->HEAD[i]);
                }
                return;
            }
            case type_int32:
            case type_uint32: {
                for (Int i = 0; i < o->size; i++) {
                    push_int32(body, (int32_t)o->HEAD[i]);
                }
                return;
            }
            case type_int16:
            case type_uint16: {
                for (Int i = 0; i < o->size; i++) {
                    push_primitive<int16_t>(body, (int16_t)o->HEAD[i]);
                }
                return;
            }
            case type_int8:
            case type_uint8: {
                for (Int i = 0; i < o->size; i++) {
                    push_byte(body, (int8_t)o->HEAD[i]);
                }
                return;
            }
            default:
                throw vm_exception("invalid numeric type found while serializing object");
        }
    }
    else {
        throw vm_exception("invalid type found while serializing object");
    }
}

// Main serialization function
void serialize(object* from, object* to) {
    try {
        if (GENERATION(from->o->info) == gc_perm) {
            throw vm_exception("cannot serialize static objects");
        }

        // Initialize data structures
        init_serialization();
        reset_serialization_state();

        // Start serialization
        push_byte(header, SERIALIZE_START);
        serialize_object(from->o);

        // Write metadata
        push_int32(header, objectVector.size());
        push_class_section();
        push_byte(body, SERIALIZE_END);

        // Create the combined buffer
        const Int totalSize = (header.pos + 1) + (body.pos + 1);
        auto serializedData = create_object(totalSize, type_int8);
        copy_object(to, serializedData);

        // Copy header and body into the final buffer
        Int destPos = 0;
        for (Int i = 0; i <= header.pos; i++) {
            serializedData->HEAD[destPos++] = header.buf[i];
        }

        for (Int i = 0; i <= body.pos; i++) {
            serializedData->HEAD[destPos++] = body.buf[i];
        }

        // Clean up
        clean_serialization_resources();
    }
    catch (runtime_error& err) {
        copy_object(to, (sharp_object*)nullptr);
        clean_serialization_resources();
        throw vm_exception(err.what());
    }
}

// Locate a serialized class by hash
sharp_class* locate_serialized_class(Int hash) {
    for (Int i = 0; i < serializedClasses.count; i++) {
        if (hash == serializedClasses.serialized[i]->hash) {
            return serializedClasses.serialized[i];
        }
    }

    throw vm_exception("internal error: could not find class.");
    return nullptr;
}

// Initialize deserialization buffer
void init_deserialization_buffer(long double* bytes, Int len) {
    dBuffer.buf = bytes;
    dBuffer.pos = -1;
    dBuffer.size = len;
    dBuffer.references = nullptr;
}

// Macros for reading data
#define expect_data(data) \
    if ((dBuffer.pos + 1) >= dBuffer.size) { \
        throw vm_exception("invalid format: unexpected end of deserialization buffer");\
    } else if (formatted_buffer(++dBuffer.pos) != data) { \
        throw vm_exception("unexpected data found in deserialization buffer");\
    }

#define overflow_check \
    if ((dBuffer.pos + 1) >= dBuffer.size) { \
        throw vm_exception("invalid format: unexpected end of deserialization buffer");\
    }

#define read_int32(out) \
    if ((dBuffer.pos + 4) >= dBuffer.size) { \
        throw vm_exception("invalid format: unexpected end of deserialization buffer");\
    } \
    (out) = SET_i32(formatted_buffer(dBuffer.pos+1), formatted_buffer(dBuffer.pos+2), \
                formatted_buffer(dBuffer.pos+3), formatted_buffer(dBuffer.pos+4)); \
    dBuffer.pos += 4;

#define read_data \
    formatted_buffer(++dBuffer.pos)

#define formatted_buffer(pos) \
    ((uint8_t) dBuffer.buf[(pos)])

#define read_bytes(type, bytes, out) \
    if ((dBuffer.pos + sizeof(type)) >= dBuffer.size) { \
        throw vm_exception("invalid format: unexpected end of deserialization buffer");\
    } \
    for (int jj = 0; jj < sizeof(type); jj++) { \
        bytes[jj] = read_data; \
    } \
    out = *reinterpret_cast<type*>(bytes);

// Deserialize class section
void deserialize_class_section() {
    expect_data(CLASS_SECTION_BEGIN)
    Int classes;
    read_int32(classes)

    for (Int i = 0; i < classes; i++) {
        Int hash, fieldCount, classNameSize;
        read_int32(hash)
        read_int32(fieldCount)
        overflow_check
        classNameSize = read_data;

        string className;
        if ((dBuffer.pos + classNameSize) >= dBuffer.size) {
            throw vm_exception("invalid format: unexpected end of deserialization buffer");
        }

        for (int i = 0; i < classNameSize; i++) {
            className += read_data;
        }

        auto klass = locate_class_hashed(hash);
        if (klass == nullptr) {
            throw vm_exception("class not found: unable to locate serialized class `" + className + "`");
        }

        if (klass->fullName != className) {
            throw vm_exception("hash mismatch: unexpected hash mismatch when trying to deserialize class `" + className + "`");
        }

        if (klass->instanceFields != fieldCount) {
            throw vm_exception("field count mismatch: mismatching field sizes found when trying to deserialize class `" + className + "`");
        }

        add_serialized_class(klass);
    }

    expect_data(CLASS_SECTION_END)
}

// Deserialize an object
void deserialize_object(object* to, int expectedType, sharp_class* sc) {
    overflow_check
    Int objType = read_data;

    if (objType == NULL_OBJECT) {
        copy_object(to, (sharp_object*)nullptr);
    }
    else if (objType == REFERENCE_OBJECT) {
        Int id;
        read_int32(id)
        auto processedObj = dBuffer.references[id];

        if (processedObj != nullptr) {
            if (expectedType != -1 && processedObj->type != expectedType) {
                throw vm_exception("invalid type found when processing deserialization buffer`");
            }
            if (sc && !are_classes_related(vm.classes + CLASS(processedObj->info), sc)) {
                throw vm_exception("type mismatch when deserializing buffer, expected class `" + sc->fullName + "`");
            }

            copy_object(to, processedObj);
        } else {
            throw vm_exception("invalid object reference found in deserialization buffer");
        }
    }
    else if (objType == NUMERIC_OBJECT) {
        Int size, type, id;
        sharp_object* deserializedObject;
        read_int32(id)
        read_int32(size)
        overflow_check
        type = read_data;

        if (expectedType != -1 && type != expectedType) {
            throw vm_exception("invalid type found when processing deserialization buffer`");
        }
        if (sc) {
            throw vm_exception("type mismatch when deserializing buffer, expected class `" + sc->fullName + "` but number was found.");
        }

        deserializedObject = create_object(size, (data_type)type);
        dBuffer.references[id] = deserializedObject;
        copy_object(to, deserializedObject);

        switch (type) {
            case type_var: {
                for (Int i = 0; i < size; i++) {
                    read_bytes(long double, doubleBytes, deserializedObject->HEAD[i])
                }
                break;
            }
            case type_function_ptr: {
                for (Int i = 0; i < size; i++) {
                    read_int32(deserializedObject->HEAD[i])
                }
                break;
            }
            case type_int64:
            case type_uint64: {
                for (Int i = 0; i < size; i++) {
                    read_bytes(int64_t, _64Bytes, deserializedObject->HEAD[i])
                }
                break;
            }
            case type_int32:
            case type_uint32: {
                for (Int i = 0; i < size; i++) {
                    read_int32(deserializedObject->HEAD[i])
                }
                break;
            }
            case type_int16:
            case type_uint16: {
                for (Int i = 0; i < size; i++) {
                    read_bytes(int16_t, _16Bytes, deserializedObject->HEAD[i])
                }
                break;
            }
            case type_int8:
            case type_uint8: {
                for (Int i = 0; i < size; i++) {
                    deserializedObject->HEAD[i] = read_data;
                }
                break;
            }
            default:
                throw vm_exception("invalid numeric type found while serializing object");
                break;
        }
    }
    else if (objType == STANDARD_OBJECT) {
        Int size, id;
        sharp_object* deserializedObject;
        read_int32(id)
        read_int32(size)

        if (expectedType != -1 && expectedType <= type_var) {
            throw vm_exception("invalid type found when processing deserialization buffer`");
        }

        deserializedObject = create_object(size);
        dBuffer.references[id] = deserializedObject;
        copy_object(to, deserializedObject);

        for (Int i = 0; i < size; i++) {
            deserialize_object(deserializedObject->node + i, -1, nullptr);
        }
    }
    else if (objType == CLASS_OBJECT || objType == CLASS_ARRAY_OBJECT) {
        Int size, id;
        uint32_t hash;
        sharp_object* deserializedObject;
        read_int32(id)
        if (objType == CLASS_ARRAY_OBJECT) {
            read_int32(size)
        }
        read_int32(hash)

        auto klass = locate_serialized_class(hash);
        if (expectedType != -1 && expectedType != type_object && expectedType != type_class) {
            throw vm_exception("invalid type found when processing deserialization buffer`");
        }
        if (sc && !are_classes_related(klass, sc)) {
            throw vm_exception("type mismatch when deserializing buffer, expected class `" + sc->fullName + "` but class `" + klass->fullName + "` was found.");
        }

        if (objType == CLASS_OBJECT) {
            deserializedObject = create_object(klass);
        } else {
            deserializedObject = create_object(klass, size);
        }

        dBuffer.references[id] = deserializedObject;
        copy_object(to, deserializedObject);

        if (objType == CLASS_ARRAY_OBJECT) {
            for (Int i = 0; i < size; i++) {
                deserialize_object(deserializedObject->node + i, -1, klass);
            }
        } else {
            for (Int i = 0; i < klass->instanceFields; i++) {
                deserialize_object(deserializedObject->node + i, klass->fields[i].type->type, nullptr);
            }
        }
    } else {
        throw vm_exception("unexpected data found in deserialization buffer");
    }
}

// Main deserialization function
void deserialize(object* from, object* to) {
    Int objects = 0;
    try {
        if (from->o && from->o->type <= type_var) {
            init_deserialization_buffer(from->o->HEAD, from->o->size);
            init_serialization();

            expect_data(SERIALIZE_START)
            read_int32(objects)
            dBuffer.references = (sharp_object**)calloc_mem<sharp_object*>(objects, sizeof(sharp_object*));
            deserialize_class_section();

            deserialize_object(to, -1, nullptr);
            expect_data(SERIALIZE_END)

            // Clean up
            std::free(dBuffer.references);
            dBuffer.references = nullptr;
            release_bytes(objects * sizeof(sharp_object*));
            dBuffer.buf = nullptr;
            dBuffer.pos = -1;
            dBuffer.size = 0;
        } else {
            throw vm_exception("attempting to deserialize null or non-string object");
        }
    } catch (runtime_error& err) {
        if (dBuffer.references) {
            std::free(dBuffer.references);
            dBuffer.references = nullptr;
            release_bytes(objects * sizeof(sharp_object*));
        }
        dBuffer.buf = nullptr;
        dBuffer.pos = -1;
        dBuffer.size = 0;

        copy_object(to, (sharp_object*)nullptr);
        throw vm_exception(err.what());
    }
}
