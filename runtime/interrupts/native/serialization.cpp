//
// SERIALIZATION: an object, and everything it refers to, as bytes - and back.     (std.export)
//
//      bytes := serialize(order);                      // Sharp
//      copy := deserialize(bytes.to_string()) as order;
//
// THE BYTES
//
//      START  VERSION
//      the classes:  how many, then for each: its hash (4 bytes), how many fields an instance has
//                    (4 bytes), the length of its name (1 byte), its name
//      the first object - and inside it, where each reference is, the object it refers to
//      END
//
// An object is one byte that says what it is, then:
//
//      NOTHING       null
//      SEEN          an object that was written before: its number
//      NUMBERS       a number array (a string's characters, a decimal[] ...): the element type (1 byte),
//                    how many, then the elements exactly as they lie in memory
//      OBJECTS       an object[]: how many, then each element
//      INSTANCE      a class instance: which class (its place in the list above), then each field in
//                    order: a number field as its 8 bytes, a reference field as an object
//      INSTANCES     an array of a class: which class, how many, then each element
//
// "How many", "which class" and "its number" are whole numbers written short: 7 bits to a byte, the
// eighth saying that another byte follows (so anything below 128 is one byte).
//
// AN OBJECT'S NUMBER is not written: the first object written is 0, the next 1, and so on, and the
// reader counts the same way as it builds them. SEEN is how two fields that hold the SAME object come
// back holding the same object, and how objects that refer to each other in a circle end.
//
// FIELDS have no names or numbers in the bytes either: they are written in the order the class has
// them, instance fields only (static fields belong to the class, not to an object). What keeps the
// reader and the writer in step is the list of classes: a class is found again by its hash, and its
// name and its number of fields must match - a program whose class changed refuses the old bytes
// instead of reading them wrongly.
//
// WHY IT IS FAST
//
//  - Number arrays, and so every string, are copied as one block (memcpy), not a byte at a time.
//  - "Was this object written before?" is asked of a table made for exactly that (seen_before): one
//    multiplication and, nearly always, one look, into a table of 4 bytes an object.
//  - A class is looked up in its list by its place among the program's classes, not searched for.
//  - Nothing is written that the reader can work out: no object numbers, no sizes of 4 bytes where
//    1 will do, no mark in front of every number field.
//  - The bytes are built in one buffer and copied once into the array that is handed back.
//  - Reading stores each new object into its owner without the lock a store of unknown ownership
//    takes (put_into).
//  - No function calls itself: the objects still to be finished are kept in a list (writing,
//    reading). A chain of a million objects - a long linked list - needs a longer list, not a
//    deeper stack, so it cannot overflow the thread's stack.
//
// (benchmark/serialization measures all of it:  bash test.sh try benchmark/serialization/serialization.sharp)
//
#include "serialization.h"
#include "../../memory/memory_helpers.h"
#include "../../memory/garbage_collector.h"
#include "../../virtual_machine.h"
#include "../../error/vm_exception.h"
#include "../../reflect/reflect_helpers.h"
#include "../../types/sharp_field.h"
#include <climits>
#include <cstring>
#include <vector>

namespace {
    // what the first two bytes are. (The format before this one began with 0x3b.)
    constexpr unsigned char START = 0x3c;
    constexpr unsigned char VERSION = 2;
    constexpr unsigned char OLD_START = 0x3b;
    constexpr unsigned char END = 0x3d;

    // what an object is
    constexpr unsigned char NOTHING = 0x08;
    constexpr unsigned char SEEN = 0x0f;
    constexpr unsigned char NUMBERS = 0x3a;
    constexpr unsigned char OBJECTS = 0x09;
    constexpr unsigned char INSTANCE = 0x1a;
    constexpr unsigned char INSTANCES = 0x1b;

    // =============================================================================================
    // writing
    // =============================================================================================

    // the bytes written so far. Kept from one call to the next (a program that serializes often
    // does not ask for memory each time), unless it grew very large (give_back).
    struct out_buffer {
        char *at = nullptr;
        Int size = 0;       // bytes there is room for
        Int used = 0;       // bytes written
    };
    thread_local out_buffer body;           // the objects
    thread_local out_buffer head;           // START, VERSION and the classes: known only when the objects are written

    void room_for(out_buffer &b, Int more) {
        if(b.used + more <= b.size) return;
        Int larger = b.size == 0 ? KB_TO_BYTES(16) : b.size * 2;
        while(larger < b.used + more) larger *= 2;
        b.at = b.at == nullptr ? malloc_mem<char>(larger) : realloc_mem<char>(b.at, larger, b.size);
        b.size = larger;
    }

    inline void put_byte(out_buffer &b, unsigned char value) {
        if(b.used == b.size) room_for(b, 1);
        b.at[b.used++] = (char) value;
    }

    // a whole number, short: 7 bits to a byte, the lowest first, the eighth bit set on all but the last
    inline void put_count(out_buffer &b, uint32_t value) {
        if(b.used + 5 > b.size) room_for(b, 5);
        while(value >= 0x80) {
            b.at[b.used++] = (char) (value | 0x80);
            value >>= 7;
        }
        b.at[b.used++] = (char) value;
    }

    inline void put_bytes(out_buffer &b, const void *from, Int count) {
        if(b.used + count > b.size) room_for(b, count);
        memcpy(b.at + b.used, from, (size_t) count);
        b.used += count;
    }

    // 4 bytes, the lowest first, on every machine
    inline void put_int32(out_buffer &b, uint32_t value) {
        unsigned char four[4] = { (unsigned char) value, (unsigned char) (value >> 8), (unsigned char) (value >> 16), (unsigned char) (value >> 24) };
        put_bytes(b, four, 4);
    }

    // a buffer that grew to a large part of what the program may use is handed back; a small one is kept
    void give_back(out_buffer &b) {
        if(b.at != nullptr && b.size >= (Int) (gc.memoryLimit * 0.3)) {
            std::free(b.at);
            release_bytes((size_t) b.size);
            b = out_buffer();
        }
    }

    // ---- THE OBJECTS WRITTEN SO FAR, and the number of each.
    //
    // `order` is the objects in the order they were written: an object's number is its place in it.
    //
    // `seen` finds an object's number from its address. It is a table of numbers (each one more
    // than the object's, so that 0 is "empty"): the place to look first is worked out from the
    // address (place_for), and when another object's number is there the next place is tried, and
    // the next. It is kept at most half full, so the first look is nearly always the right one.
    //
    // An entry is 4 bytes - not the address and the number, 16 - because the table is what a large
    // call spends its time in: a million objects are a table far larger than the processor's cache,
    // and every look into it waits for memory. A quarter of the size is far fewer waits.
    //
    // Every call starts with a small table and makes it larger as objects come (grow_seen), so a
    // small object after a large one does not pay for clearing a large table. The memory itself is
    // kept from call to call unless it grew very large (end_seen).
    thread_local std::vector<sharp_object*> order;
    thread_local uint32_t *seen = nullptr;
    thread_local uint32_t seen_room = 0;        // how many entries the memory at `seen` holds
    thread_local uint32_t seen_size = 0;        // how many of them this call uses: a power of two

    constexpr uint32_t SEEN_FIRST = 1024;           // the table a call starts with
    constexpr uint32_t SEEN_KEPT = 1u << 20;        // memory for more entries than this (4 MB) is handed back after the call

    inline uint32_t place_for(const sharp_object *o) {
        // (objects are 16 bytes apart at least: the low bits say nothing. The multiplication spreads
        //  the rest over the table.)
        return (uint32_t) ((((uint64_t) (uintptr_t) o >> 4) * 0x9E3779B97F4A7C15ull) >> 32) & (seen_size - 1);
    }

    // a table of `size` entries, empty, with every object written so far put into it again
    void size_seen(uint32_t size) {
        if(size > seen_room) {
            if(seen != nullptr) {
                std::free(seen);
                release_bytes((size_t) seen_room * sizeof(uint32_t));
            }
            seen = malloc_mem<uint32_t>((uInt) size * sizeof(uint32_t));
            seen_room = size;
        }
        seen_size = size;
        memset(seen, 0, (size_t) size * sizeof(uint32_t));
        const uint32_t count = (uint32_t) order.size();
        for(uint32_t i = 0; i < count; i++) {
            uint32_t at = place_for(order[i]);
            while(seen[at] != 0) at = (at + 1) & (seen_size - 1);
            seen[at] = i + 1;
        }
    }

    // Was `o` written before? Yes: its number. No: it gets the next number, and is remembered.
    inline bool seen_before(sharp_object *o, uint32_t &number) {
        if((order.size() + 1) * 2 > seen_size) size_seen(seen_size * 2);
        uint32_t at = place_for(o);
        while(true) {
            uint32_t there = seen[at];
            if(there == 0) {
                number = (uint32_t) order.size();
                order.push_back(o);
                seen[at] = number + 1;
                return false;
            }
            if(order[there - 1] == o) {
                number = there - 1;
                return true;
            }
            at = (at + 1) & (seen_size - 1);
        }
    }

    void begin_seen() {
        order.clear();
        size_seen(SEEN_FIRST);
    }

    void end_seen() {
        order.clear();
        if(seen_room > SEEN_KEPT) {
            std::free(seen);
            release_bytes((size_t) seen_room * sizeof(uint32_t));
            seen = nullptr;
            seen_room = 0;
            seen_size = 0;
        }
    }

    // ---- THE CLASSES of the objects written, in the order they were first met. An instance says
    // which class it is by its place in this list.
    thread_local std::vector<sharp_class*> listed;
    thread_local std::vector<uint32_t> listed_in;       // for every class of the program: the call that listed it last ...
    thread_local std::vector<uint32_t> listed_at;       // ... and its place in the list then
    thread_local uint32_t this_call = 0;                // which call to serialize() this is (never 0)

    void begin_listed() {
        listed.clear();
        this_call++;
        if(listed_in.size() != (size_t) vm.mf.classes || this_call == 0) {
            // (the first call - or, four thousand million calls later, the count starting again)
            listed_in.assign((size_t) vm.mf.classes, 0);
            listed_at.assign((size_t) vm.mf.classes, 0);
            this_call = 1;
        }
    }

    inline uint32_t place_of(sharp_class *klass) {
        size_t which = (size_t) (klass - vm.classes);
        if(listed_in[which] != this_call) {
            listed_in[which] = this_call;
            listed_at[which] = (uint32_t) listed.size();
            listed.push_back(klass);
        }
        return listed_at[which];
    }

    // an object whose fields or elements are being written: `next` is the one that comes next
    struct write_step {
        sharp_object *o;
        uint32_t next;
        const bool *numbers;        // which of its slots are number fields (null: none)
    };
    thread_local std::vector<write_step> writing;

    // Write what `o` is. When it has fields or elements they are NOT written here: it is put on
    // `writing`, and write_all() goes through them.
    void write_object(sharp_object *o) {
        if(o == nullptr) {
            put_byte(body, NOTHING);
            return;
        }
        uint32_t number;
        if(seen_before(o, number)) {
            put_byte(body, SEEN);
            put_count(body, number);
            return;
        }

        if(o->type == type_class) {
            if(o->staticObject) throw vm_exception("cannot serialize static objects");
            sharp_class *klass = vm.classes + CLASS(o->info);
            if(o->arrayFlag) {
                put_byte(body, INSTANCES);
                put_count(body, place_of(klass));
                put_count(body, o->size);
                if(o->size > 0) writing.push_back({ o, 0, nullptr });
            } else {
                // (the reader builds an instance from its class: both must mean the same number of fields)
                if((Int) o->size != klass->instanceFields) {
                    throw vm_exception("cannot serialize an instance of `" + klass->fullName + "`: it does not have the fields of its class");
                }
                put_byte(body, INSTANCE);
                put_count(body, place_of(klass));
                if(o->size > 0) writing.push_back({ o, 0, number_slots_of(o) });
            }
        }
        else if(o->type == type_object) {
            put_byte(body, OBJECTS);
            put_count(body, o->size);
            if(o->size > 0) writing.push_back({ o, 0, nullptr });
        }
        else if(o->type <= type_var && o->type != type_unused_8) {
            put_byte(body, NUMBERS);
            put_byte(body, (unsigned char) o->type);
            put_count(body, o->size);
            put_bytes(body, o->data, (Int) o->size * element_size(o->type));
        }
        else {
            throw vm_exception("invalid type found while serializing object");
        }
    }

    void write_all(sharp_object *first) {
        writing.clear();
        write_object(first);
        while(!writing.empty()) {
            write_step &step = writing.back();
            if(step.next == step.o->size) {
                writing.pop_back();
                continue;
            }
            uint32_t at = step.next++;
            if(step.numbers != nullptr && step.numbers[at]) {
                put_bytes(body, &number_slot(step.o, at).i, 8);
            } else {
                write_object(step.o->node[at].o);       // (may add to `writing`: `step` is not used after it)
            }
        }
    }

    void write_head() {
        head.used = 0;
        put_byte(head, START);
        put_byte(head, VERSION);
        put_count(head, (uint32_t) listed.size());
        for(sharp_class *klass : listed) {
            if(klass->fullName.size() > 255) {
                throw vm_exception("serialized class name too long for `" + klass->fullName + "`");
            }
            put_int32(head, klass->hash);
            put_int32(head, (uint32_t) klass->instanceFields);
            put_byte(head, (unsigned char) klass->fullName.size());
            put_bytes(head, klass->fullName.data(), (Int) klass->fullName.size());
        }
    }

    // =============================================================================================
    // reading
    // =============================================================================================

    struct in_buffer {
        const unsigned char *at = nullptr;
        Int size = 0;
        Int pos = 0;        // the next byte to read
    };
    thread_local in_buffer in;

    thread_local std::vector<sharp_object*> made;       // the objects built so far: an object's number is its place here

    inline void need(Int count) {
        if(count < 0 || in.pos + count > in.size) {
            throw vm_exception("invalid format: unexpected end of deserialization buffer");
        }
    }

    inline unsigned char get_byte() {
        need(1);
        return in.at[in.pos++];
    }

    inline uint32_t get_count() {
        uint32_t value = 0;
        for(int shift = 0; shift < 35; shift += 7) {
            unsigned char piece = get_byte();
            value |= (uint32_t) (piece & 0x7f) << shift;
            if((piece & 0x80) == 0) return value;
        }
        throw vm_exception("invalid format: a number in the deserialization buffer is too long");
    }

    inline uint32_t get_int32() {
        need(4);
        const unsigned char *four = in.at + in.pos;
        in.pos += 4;
        return (uint32_t) four[0] | ((uint32_t) four[1] << 8) | ((uint32_t) four[2] << 16) | ((uint32_t) four[3] << 24);
    }

    void read_classes() {
        listed.clear();
        uint32_t count = get_count();
        for(uint32_t i = 0; i < count; i++) {
            uint32_t hash = get_int32();
            Int fields = (Int) get_int32();
            Int name_size = get_byte();
            need(name_size);
            string name((const char*) in.at + in.pos, (size_t) name_size);
            in.pos += name_size;

            sharp_class *klass = locate_class_hashed(hash);
            if(klass == nullptr) {
                throw vm_exception("class not found: unable to locate serialized class `" + name + "`");
            }
            if(klass->fullName != name) {
                throw vm_exception("hash mismatch: unexpected hash mismatch when trying to deserialize class `" + name + "`");
            }
            if(klass->instanceFields != fields) {
                throw vm_exception("field count mismatch: mismatching field sizes found when trying to deserialize class `" + name + "`");
            }
            listed.push_back(klass);
        }
    }

    // an object whose fields or elements are being read
    struct read_step {
        sharp_object *o;
        uint32_t next;
        uint32_t count;
        sharp_class *klass;         // the class of an instance, or of the elements of an array of a class (null: an object[])
        const bool *numbers;        // an instance: which of its slots are number fields
        bool instance;
    };
    thread_local std::vector<read_step> reading;

    // Put `value` into the slot `to` of the object `owner` (null: `to` is not a slot of an object -
    // it is where the caller wants the first object).
    //
    // copy_object() is the store for code that does not know whose slot it has: it takes a lock and
    // keeps the value alive through the next collection, every time. Here the owner IS known, and it
    // was made a moment ago by this very call: storing into it is what the write barrier is for,
    // and for an owner that is young - as a new object is - that is nothing at all. A million
    // objects read back are a million locks not taken.
    inline void put_into(object *to, sharp_object *owner, sharp_object *value) {
        if(owner == nullptr) {
            copy_object(to, value);
        } else {
            store_object(to, value);
            write_barrier(owner, value, to);
        }
    }

    // Read what comes next and put it into `to`, a slot of `owner` (put_into). `kind` is the type
    // the place it goes into has (-1: any), `of` the class its elements must be (an array of a
    // class). An object with fields or elements is built EMPTY and put on `reading`: read_all()
    // fills it.
    void read_object(object *to, sharp_object *owner, int kind, sharp_class *of) {
        unsigned char what = get_byte();

        if(what == NOTHING) {
            put_into(to, owner, nullptr);
        }
        else if(what == SEEN) {
            uint32_t number = get_count();
            if(number >= made.size()) {
                throw vm_exception("invalid object reference found in deserialization buffer");
            }
            sharp_object *o = made[number];
            if(kind != -1 && o->type != kind) {
                throw vm_exception("invalid type found when processing deserialization buffer`");
            }
            if(of != nullptr && !are_classes_related(vm.classes + CLASS(o->info), of)) {
                throw vm_exception("type mismatch when deserializing buffer, expected class `" + of->fullName + "`");
            }
            put_into(to, owner, o);
        }
        else if(what == NUMBERS) {
            int type = get_byte();
            uint32_t size = get_count();
            if(type > type_var || type == type_unused_8) {
                throw vm_exception("invalid numeric type found while deserializing object");
            }
            if(kind != -1 && type != kind) {
                throw vm_exception("invalid type found when processing deserialization buffer`");
            }
            if(of != nullptr) {
                throw vm_exception("type mismatch when deserializing buffer, expected class `" + of->fullName + "` but number was found.");
            }
            Int bytes = (Int) size * element_size(type);
            need(bytes);                                        // (before anything is made: bytes that lie cannot ask for memory)
            sharp_object *o = create_object(size, (data_type) type);
            made.push_back(o);
            put_into(to, owner, o);
            memcpy(o->data, in.at + in.pos, (size_t) bytes);
            in.pos += bytes;
        }
        else if(what == OBJECTS) {
            uint32_t size = get_count();
            if(kind != -1 && kind <= type_var) {
                throw vm_exception("invalid type found when processing deserialization buffer`");
            }
            need(size);                                         // (every element is a byte at least)
            sharp_object *o = create_object((Int) size);
            made.push_back(o);
            put_into(to, owner, o);
            if(size > 0) reading.push_back({ o, 0, size, nullptr, nullptr, false });
        }
        else if(what == INSTANCE || what == INSTANCES) {
            uint32_t place = get_count();
            if(place >= listed.size()) {
                throw vm_exception("invalid class found in deserialization buffer");
            }
            sharp_class *klass = listed[place];
            if(kind != -1 && kind != type_object && kind != type_class) {
                throw vm_exception("invalid type found when processing deserialization buffer`");
            }
            if(of != nullptr && !are_classes_related(klass, of)) {
                throw vm_exception("type mismatch when deserializing buffer, expected class `" + of->fullName + "` but class `" + klass->fullName + "` was found.");
            }

            if(what == INSTANCES) {
                uint32_t size = get_count();
                need(size);
                sharp_object *o = create_object(klass, (Int) size);
                made.push_back(o);
                put_into(to, owner, o);
                if(size > 0) reading.push_back({ o, 0, size, klass, nullptr, false });
            } else {
                sharp_object *o = create_object(klass);
                made.push_back(o);
                put_into(to, owner, o);
                if(klass->instanceFields > 0) {
                    reading.push_back({ o, 0, (uint32_t) klass->instanceFields, klass, number_slots_of(o), true });
                }
            }
        }
        else {
            throw vm_exception("unexpected data found in deserialization buffer");
        }
    }

    void read_all(object *first) {
        reading.clear();
        made.clear();
        read_object(first, nullptr, -1, nullptr);
        while(!reading.empty()) {
            read_step step = reading.back();        // (a copy: read_object may add to `reading`)
            if(step.next == step.count) {
                reading.pop_back();
                continue;
            }
            reading.back().next++;
            uint32_t at = step.next;
            if(step.instance) {
                if(step.numbers != nullptr && step.numbers[at]) {
                    need(8);
                    memcpy(&number_slot(step.o, at).i, in.at + in.pos, 8);
                    in.pos += 8;
                } else {
                    read_object(step.o->node + at, step.o, step.klass->fields[at].type->type, nullptr);
                }
            } else {
                read_object(step.o->node + at, step.o, -1, step.klass);
            }
        }
    }
}

void serialize(object *from, object *to) {
    try {
        if(from->o != nullptr && GENERATION(from->o->info) == gc_perm) {
            throw vm_exception("cannot serialize static objects");
        }

        body.used = 0;
        begin_seen();
        begin_listed();

        write_all(from->o);
        put_byte(body, END);
        write_head();

        // the head and the objects, one after the other, in the array that is handed back
        sharp_object *bytes = create_object(head.used + body.used, type_int8);
        copy_object(to, bytes);
        memcpy(bytes->data, head.at, (size_t) head.used);
        memcpy((char*) bytes->data + head.used, body.at, (size_t) body.used);

        give_back(body);
        give_back(head);
        end_seen();
    }
    catch(runtime_error &err) {
        copy_object(to, (sharp_object*) nullptr);
        writing.clear();
        give_back(body);
        give_back(head);
        end_seen();
        throw vm_exception(err.what());
    }
}

void deserialize(object *from, object *to) {
    try {
        if(from->o == nullptr || from->o->type > type_var) {
            throw vm_exception("attempting to deserialize null or non-string object");
        }
        // the data to read is an array of bytes (what serialize() produces)
        if(from->o->type != type_int8 && from->o->type != type_uint8) {
            throw vm_exception("deserialize: the data must be an _int8[]");
        }
        in.at = (const unsigned char*) from->o->data;
        in.size = from->o->size;
        in.pos = 0;

        unsigned char start = get_byte();
        if(start == OLD_START) {
            throw vm_exception("this data was serialized by an older Sharp: its format is not read any more (serialize it again)");
        }
        if(start != START) {
            throw vm_exception("unexpected data found in deserialization buffer");
        }
        if(get_byte() != VERSION) {
            throw vm_exception("this data was serialized by another version of Sharp: its format is not known here");
        }
        read_classes();
        read_all(to);
        if(get_byte() != END) {
            throw vm_exception("unexpected data found in deserialization buffer");
        }

        made.clear();
        in = in_buffer();
    }
    catch(runtime_error &err) {
        made.clear();
        reading.clear();
        in = in_buffer();
        copy_object(to, (sharp_object*) nullptr);
        throw vm_exception(err.what());
    }
}
