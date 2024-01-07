//
// Created by bknun on 11/12/2023.
//

#include "obfuscator.h"

#define ALPHABET_COUNT 26
#define MAX_BUFFER_SIZE 32
char alphabet[]
    = {
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
        'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'
    };

char buf[MAX_BUFFER_SIZE];
char resultBuf[MAX_BUFFER_SIZE];
linkedlist<KeyPair<sharp_module*, string>> packages;
linkedlist<KeyPair<sharp_file*, string>> files;
linkedlist<KeyPair<sharp_class*, string>> classMap;
linkedlist<KeyPair<sharp_field*, string>> fields;
linkedlist<KeyPair<sharp_function*, string>> methods;


void invalid_id(Int id) {
    stringstream ss;
    ss << ": could not obfuscate invalid unique id of " << id;
    throw std::runtime_error(ss.str());
}

void clear_buf() {
    for(Int i = 0; i < MAX_BUFFER_SIZE; i++) {
        buf[i] = 0;
    }
}

string generate_name(Int id) {
    if(id < 0)
        invalid_id(id);

    stringstream name;
    if(id < ALPHABET_COUNT) {
        name << alphabet[id];
        return name.str();
    }

    Int stringSize = 0;
    while(id > 0) {
        if(stringSize < MAX_BUFFER_SIZE) {
            Int letterIndex = id % ALPHABET_COUNT;
            letterIndex = letterIndex == 0 ? ALPHABET_COUNT-1 : letterIndex - 1; // we need this so we start at a after id of 26, i.e (aa, ab, ac, etc.)

            buf[stringSize++] = alphabet[letterIndex];
            id /= ALPHABET_COUNT;
        } else { // should never happen
            clear_buf();
            buf[0] = '?';
            stringSize = 1;
            break;
        }
    }

    Int bufIndex = 0;
    for(Int i = stringSize - 1; i >= 0; i--) {
        resultBuf[bufIndex++] = buf[i];
    }

    return string(resultBuf, stringSize);
}

template<class T>
bool item_exists(linkedlist<KeyPair<T, string>> &list, T item) {
    return list.node_at(item, [](void *data, node<KeyPair<T, string>> *node) -> bool { 
        return (T)data == node->data.key; 
    }) != NULL;
}

sharp_module* obfuscate(sharp_module *subject) {
    if(!item_exists<sharp_module*>(packages, subject)) {
        packages.createnode(KeyPair<sharp_module*, string>(subject, subject->name));
        subject->name = generate_name(subject->uid);
    }

    return subject;
}

sharp_file* obfuscate(sharp_file *subject) {
    if(!item_exists<sharp_file*>(files, subject)) {
        files.createnode(KeyPair<sharp_file*, string>(subject, subject->name));
        subject->name = generate_name(subject->uid);
    }

    return subject;
}

bool should_obfuscate(obfuscation_modifier subject, sharp_class *parent) {
    if(parent != NULL) {
        return parent->obfuscateModifier != modifier_keep_inclusive
            && (subject == modifier_obfuscate 
                    || subject == modifier_obfuscate_inclusive
                    || parent->obfuscateModifier == modifier_obfuscate_inclusive);
    } else {
        return subject == modifier_obfuscate 
                    || subject == modifier_obfuscate_inclusive;
    }
}    

sharp_class* obfuscate(sharp_class *subject) {
    if(!item_exists<sharp_class*>(classMap, subject)) {
        if(subject->obfuscateModifier == modifier_obfuscate 
            || subject->obfuscateModifier == modifier_obfuscate_inclusive) {
            classMap.createnode(KeyPair<sharp_class*, string>(subject, subject->name));
            subject->name = generate_name(subject->uid);
            
            if(subject->owner == NULL || check_flag(subject->owner->flags, flag_global)) {
                subject->fullName = obfuscate(subject->module)->name + "#"
                        + subject->name;
            } else {
                subject->fullName = obfuscate(subject->owner)->fullName + "."
                            + subject->name;
            }
        }

        for(Int i = 0; i < subject->fields.size(); i++) {
            sharp_field* field = subject->fields[i];
            if(should_obfuscate(field->obfuscateModifier, subject) && field->used) {
                obfuscate(field);
            }
        }

        for(Int i = 0; i < subject->functions.size(); i++) {
            sharp_function* function = subject->functions[i];
            if(should_obfuscate(function->obfuscateModifier, subject) && function->used) {
                obfuscate(function);
            }
        }

        for(Int i = 0; i < subject->children.size(); i++) {
            sharp_class* child = subject->children[i];
            if(should_obfuscate(child->obfuscateModifier, subject) && child->used) {
                obfuscate(child);
            }
        }
    }

    return subject;
}

sharp_field* obfuscate(sharp_field *subject) {
    if(!item_exists<sharp_field*>(fields, subject)) {
        fields.createnode(KeyPair<sharp_field*, string>(subject, subject->fullName));
        subject->name = generate_name(subject->uid);
        
        if(subject->owner != NULL)
            subject->fullName = obfuscate(subject->owner)->fullName + "." + subject->name;
        else subject->fullName = subject->name;
    }

    return subject;
}

sharp_function* obfuscate(sharp_function *subject) {
    if(!item_exists<sharp_function*>(methods, subject)) {
        methods.createnode(KeyPair<sharp_function*, string>(subject, subject->fullName));
        subject->name = generate_name(subject->uid);
        subject->fullName = obfuscate(subject->owner)->fullName + "." + subject->name;
    }

    return subject;
}
