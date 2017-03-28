//
// Created by BraxtonN on 3/27/2017.
//

#ifndef SHARP_LIST_H
#define SHARP_LIST_H

#include "../../stdimports.h"
#include "../runtime/oo/Exception.h"

template <class T>
class List {
public:
    void init(){
        _Data=NULL;
        len=0;
    }

    void push_back(T& data) {
        __expand();
        _Data[len++]=data;
    }

    /*
     * Programmer must be responsible
     * for freeing that data himself
     */
    void remove(unsigned long _X) {
        if(_X>=len || len==0){
            stringstream ss;
            ss << "index out of bounds list::remove() _X: " << _X
               << " length: " << len << endl;
            throw Exception(ss.str());
        }

        if(len==1){
            free();
        }
        else if(len==2) {
            if(_X==0) {
                _Data[0]=_Data[1];
            }
            __shrink();
        } else {
            if(_X!=(len-1)) {
                for(unsigned long i = _X; i < len-1; i++) {
                    _Data[i] = _Data[i+1];
                }
            }
            __shrink();
        }


    }

    /*
     * Programmer must be responsible
     * for freeing that data himself
     */
    void remove(T& _X) {
        for(unsigned long i = 0; i < len; i++) {
            if(_X == _Data[i]) {
                remove(i);
                return;
            }
        }
    }

    T at(unsigned long _X) {
        if(_X>=len){
            stringstream ss;
            ss << "index out of bounds list::get() _X: " << _X
               << " length: " << len << endl;
            throw Exception(ss.str());
        }
        return _Data[_X];
    }

    T& get(unsigned long _X) {
        if(_X>=len){
            stringstream ss;
            ss << "index out of bounds list::get() _X: " << _X
               << " length: " << len << endl;
            throw Exception(ss.str());
        }
        return _Data[_X];
    }

    void free() {
        if(_Data != NULL)
            std::free(_Data);
        _Data = NULL;
        len=0;
    }

    CXX11_INLINE
    unsigned long size() { return len; }


private:
    CXX11_INLINE
    void __expand() {
        try{
            len++;
            if(_Data==NULL){
                _Data=(T*)memalloc(sizeof(T)*len);
            } else {
                _Data=(T*)memrealloc(_Data, sizeof(T)*len);
            }
        } catch(Exception &e){
            len--;
            throw e;
        }
    }

    CXX11_INLINE
    void __shrink(){
        try {
            len--;
            _Data=(T*)memrealloc(_Data, sizeof(T)*len);
        } catch(Exception &e) {
            len++;
            throw e;
        }
    }

    unsigned  long len;
    T* _Data;
};


#endif //SHARP_LIST_H
