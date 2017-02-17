//
// Created by BraxtonN on 2/16/2017.
//

#ifndef SHARP_BYTECODESTREAM_H
#define SHARP_BYTECODESTREAM_H

#include "../../stdimports.h"

template <class T>
extern inline T& element_at(list<T>& l, size_t x);

class BytecodeStream {
public:
    BytecodeStream()
    :
            iter(0)
    {
        bytecode = new list<double>();
    }

    double& operator++() {
        return element_at(*bytecode, iter++);
    }

    double& operator--() {
        return element_at(*bytecode, iter--);
    }

    uint64_t size() {
        return bytecode->size();
    }

    list<double>* getStream() {
        return bytecode;
    }

    void add(double byte) {
        bytecode->push_back(byte);
    }

    void add(BytecodeStream& stream) {
        for(double byte : *stream.getStream()) {
            bytecode->push_back(byte);
        }
    }


    void inject(BytecodeStream& stream, uint64_t offset) {
        std::list<double> tlist;
        double bytec;

        if(offset >= bytecode->size())
            offset = bytecode->size() - 1;

        uint64_t  it = 0;
        for(double& byte : *bytecode) {
            if(it == offset) break;

            tlist.push_back(byte);
            it++;
        }

        for(double byte : *stream.getStream()) {
            tlist.push_back(byte);
        }

        for(double i = it; i < bytecode->size(); i++) {
            bytec = element_at(*bytecode, i);
            tlist.push_back(bytec);
        }
        *bytecode = tlist;
    }

    void free() {
        bytecode->clear();
        std::free (bytecode);
    }

    void flush() {
        bytecode->clear();
        iter = 0;
    }

    ~BytecodeStream() {
        free();
    }

private:
    uint64_t iter;
    std::list<double>* bytecode;
};

#endif //SHARP_BYTECODESTREAM_H
