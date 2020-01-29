//
// Created by BraxtonN on 2/5/2018.
//

#ifndef SHARP_ASSEMBLER_H
#define SHARP_ASSEMBLER_H


#include <cstdint>
#include "../../../stdimports.h"
#include "../../util/KeyPair.h"
#include "../List.h"

class IrCode {
public:
    IrCode() {
        ir64.init();
        injectors.key.init();
        injectors.value.init();
    }

    void init() {
        ir64.init();
        injectors.key.init();
        injectors.value.init();
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

    void addinjector_unsafe(std::string key) {
        injectors.key.push_back(key);
        injectors.value.__new();
        injectors.value.last().init();
    }

    bool add(string injector, int64_t i64) {
        if(has_injector(injector)) {
            getInjector(injector).ir64.push_back(i64);
            return true;
        }
        return false;
    }

    IrCode& push_i64(int64_t i64){
        ir64.push_back(i64);
        return *this;
    }
    IrCode& push_i64(int64_t i64, int64_t xtra){
        ir64.push_back(i64);
        ir64.push_back(xtra);
        return *this;
    }

    List<int64_t> ir64;
    KeyPair<List<string>, List<IrCode>> injectors;

    IrCode& free() {
        ir64.free();
        injectors.key.free();

        for(unsigned int i = 0; i < injectors.value.size(); i++) {
            injectors.value.at(i).free();
        }
        injectors.value.free();
        return *this;
    }

    void injecti64(int64_t i, int64_t i64) {
        if(i < 0) return;

        ir64.insert(i, i64);
    }

    void inject(int64_t i, IrCode& assembler) {
        if(i < 0) return;

        int64_t start = i, iter = 0;
        for(;;) {
            if(iter >= assembler.ir64.size())
                break;

            ir64.insert(start++, assembler.ir64.get(iter++));
        }
    }

    void inject(string injector) {
        IrCode& assembler = getInjector(injector);

        int64_t start = ir64.size(), iter = 0;
        for(;;) {
            if(iter >= assembler.ir64.size())
                break;

            ir64.insert(start++, assembler.ir64.get(iter++));
        }

        assembler.free();
    }

    int64_t size() {
        return ir64.size();
    }

    IrCode& getInjector(string key) {
        for(unsigned int i = 0; i < injectors.key.size(); i++) {
            if(injectors.key.at(i) == key)
                return injectors.value.get(i);
        }

        addinjector(key);
        return injectors.value.last();
    }

private:

    void remove_injector(string key) {
        for(unsigned int i = 0; i < injectors.key.size(); i++) {
            if(injectors.key.at(i) == key) {
                injectors.value.remove(i);
                injectors.key.remove(i);
                return;
            }
        }
    }

    bool has_injector(string key) {
        for(unsigned int i = 0; i < injectors.key.size(); i++) {
            if(injectors.key.at(i) == key)
                return true;
        }
        return false;
    }
};


#endif //SHARP_ASSEMBLER_H
