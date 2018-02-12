//
// Created by BraxtonN on 2/11/2018.
//

#ifndef SHARP_LIST_H
#define SHARP_LIST_H

#include "../../stdimports.h"
#include "oo/Exception.h"

template <class T>
class List {
public:
    List(){
        init();
    }

    void init(){
        _Data=NULL;
        len=0;
    }

    void init(unsigned long inital){
        if(inital > 0) {
            _Data=NULL;
            len=inital;
            __expand();
        } else
            init();
    }

    void push_back(T& data) {
        __expand();
        _Data[len-1]=data;
    }

    void add(T data) {
        __expand();
        _Data[len-1]=data;
    }

    void push_back() {
        __expand();
    }

    void insert(long long pos, T& data) {
        if(pos>len || pos < 0) {
            stringstream ss;
            ss << "index out of bounds list::insert() _X: " << pos
               << " length: " << len << endl;
            throw Exception(ss.str());
        }

        if(len == 0) {
            push_back(data);
        } else {
            T* result = (T*)__malloc(sizeof(T)*(len+1));
            long long newLen=len+1;
            for(long long i = 0; i < pos; i++)
                result[i] = _Data[i];
            result[pos] = data;
            for(long long i = pos + 1; i < newLen; i++)
                result[i] = _Data[i - 1];

            free();
            len=newLen;
            _Data=result;
        }

    }

    /*
     * Programmer must be responsible
     * for freeing that data himself
     */
    void remove(unsigned long long _X) {
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
            T lastElement = last();
            long long newLen=len-1,iter=1;
            for(long long i = 0; i < newLen; i++) {
                if(iter == _X)
                    iter++;

                _Data[i] = _Data[iter];
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
        if(_X>=len || _X < 0){
            stringstream ss;
            ss << "index out of bounds list::get() _X: " << _X
               << " length: " << len << endl;
            throw Exception(ss.str());
        }
        return _Data[_X];
    }

    T& last() {
        if(len == 0){
            stringstream ss;
            ss << "illegal state list::last() length: 0" << endl;
            throw Exception(ss.str());
        }
        return _Data[len-1];
    }

    void free() {
        if(_Data != NULL)
            std::free(_Data);
        _Data = NULL;
        len=0;
    }

    unsigned long size() { return len; }

    void pop_back() {
        __shrink();
    }

    void addif(T _V) {
        for(unsigned int i = 0; i < len; i++) {
            if(_V == _Data[i])
                return;
        }
        push_back(_V);
    }

    void removefirst(T _V) {
        unsigned int iter = -1;
        for(unsigned int i = 0; i < len; i++) {
            if(_V == _Data[i]){
                iter = i;
                break;
            }
        }

        if(iter != -1) {
            remove(iter);
        }
    }

    /*
     * Programmer must be responsible
     * for freeing that data himself
     */
    void replace(unsigned long _X, T repl) {
        if(_X>=len || _X < 0){
            stringstream ss;
            ss << "index out of bounds list::replaceat() _X: " << _X
               << " length: " << len << endl;
            throw Exception(ss.str());
        }
        _Data[_X] = repl;
    }

    void addAll(List<T>& list) {
        free();
        for(unsigned int i = 0; i < list.size(); i++) {
            push_back(list.get(i));
        }
    }

    void appendAll(List<T>& list) {
        for(unsigned int i = 0; i < list.size(); i++) {
            push_back(list.get(i));
        }
    }

    void addAll(list<T> &list) {
        free();
        for(T& t : list) {
            push_back(t);
        }
    }

    bool find(T data) {
        for(unsigned int i = 0; i < len; i++) {
            if(data == _Data[i]){
                return true;
            }
        }
        return false;
    }

    long long indexof(T data) {
        for(unsigned int i = 0; i < len; i++) {
            if(data == _Data[i]){
                return i;
            }
        }
        return -1;
    }

    bool empty() { return len==0; }

private:
    CXX11_INLINE
    void __expand() {
        try{
            len++;
            if(_Data==NULL){
                _Data=(T*)__malloc(sizeof(T)*len);
            } else {
                _Data=(T*)__realloc(_Data, sizeof(T)*len);
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
            _Data=(T*)__realloc(_Data, sizeof(T)*len);
        } catch(Exception &e) {
            len++;
            throw e;
        }
    }

    unsigned  long len;
    T* _Data;
};

#endif //SHARP_LIST_H
