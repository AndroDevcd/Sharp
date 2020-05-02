//
// Created by BraxtonN on 2/5/2018.
//

#ifndef SHARP_ASSEMBLER_H
#define SHARP_ASSEMBLER_H


#include <cstdint>
#include "../../../../stdimports.h"
#include "../../../util/KeyPair.h"
#include "../../List.h"

class CodeHolder {
public:
    CodeHolder() {
        ir32.init();
        injectors.key.init();
        injectors.value.init();
        instanceCaptured = false;
    }

    void init() {
        CodeHolder();
    }

    bool addinjector(string key) {
        if(!has_injector(key)) {
            injectors.key.push_back(key);
            injectors.value.__new();
            injectors.value.last().init();
            return true;
        }
        return false;
    }

    bool add(string injector, int32_t i32) {
        if(has_injector(injector)) {
            getInjector(injector).ir32.add(i32);
            return true;
        }
        return false;
    }

    CodeHolder& addIr(uint32_t i32){
        ir32.push_back(i32);
        return *this;
    }

    // conditinal state variables for processing fields
    bool instanceCaptured;

    List<uint32_t> ir32;
    KeyPair<List<string>, List<CodeHolder>> injectors;

    CodeHolder& free() {
        ir32.free();
        injectors.key.free();

        for(unsigned int i = 0; i < injectors.value.size(); i++) {
            injectors.value.at(i).free();
        }
        injectors.value.free();
        instanceCaptured = false;
        return *this;
    }

    CodeHolder& freeInjectors() {
        for(unsigned int i = 0; i < injectors.value.size(); i++) {
            injectors.value.at(i).free();
        }
        injectors.value.free();
        injectors.key.free();
        return *this;
    }

    /**
     * This function consumes data returned by the Opcode::Builder
     * to be added to the code holder
     * @param buffer
     * @return
     */
    CodeHolder& addIr(uint32_t *buffer) {
        const int BUFFER_SIZE = 3;
        for(int i = 0; i < BUFFER_SIZE; i++) {
            if(buffer[i] != -1)
                addIr(buffer[i]);
            else
                return *this;
        }

        return *this;
    }

    CodeHolder& removeIrEnd(uint32_t count) {
        while(ir32.size() > 0) {
            if(count-- != 0) {
                ir32.pop_back();
            } else break;
        }

        return *this;
    }

    void inject(int32_t i, CodeHolder& assembler) {
        if(i < 0) return;

        int32_t start = i, iter = 0;
        for(;;) {
            if(iter >= assembler.ir32.size())
                break;

            ir32.insert(start++, assembler.ir32.get(iter++));
        }
    }

    void inject(CodeHolder& assembler) {
        int32_t start = ir32.size(), iter = 0;
        for(;;) {
            if(iter >= assembler.ir32.size())
                break;

            ir32.insert(start++, assembler.ir32.get(iter++));
        }
    }

    void inject(string injector) {
        CodeHolder& assembler = getInjector(injector);

        int32_t start = ir32.size(), iter = 0;
        for(;;) {
            if(iter >= assembler.ir32.size())
                break;

            ir32.insert(start++, assembler.ir32.get(iter++));
        }

        assembler.free();
    }

    int32_t size() {
        return ir32.size();
    }

    CodeHolder& getInjector(string key) {
        for(unsigned int i = 0; i < injectors.key.size(); i++) {
            if(injectors.key.at(i) == key)
                return injectors.value.get(i);
        }

        addinjector(key);
        return injectors.value.last();
    }

    void copyInjectors(CodeHolder &code) {
        freeInjectors();

        for(unsigned int i = 0; i < code.injectors.key.size(); i++) {
            getInjector(code.injectors.key.get(i)).inject(code.injectors.value.get(i));
        }
    }

    void removeInjector(string key) {
        for(unsigned int i = 0; i < injectors.key.size(); i++) {
            if(injectors.key.at(i) == key) {
                injectors.value.removeAt(i);
                injectors.key.removeAt(i);
                return;
            }
        }
    }
private:

    bool has_injector(string key) {
        for(unsigned int i = 0; i < injectors.key.size(); i++) {
            if(injectors.key.at(i) == key)
                return true;
        }
        return false;
    }
};


#endif //SHARP_ASSEMBLER_H
