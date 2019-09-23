//
// Created by BraxtonN on 2/11/2018.
//

#ifndef SHARP_LIST_H
#define SHARP_LIST_H

#include "../../stdimports.h"
#include "oo/Exception.h"

template <class T>
class _List {
public:
    _List(){
        init();
    }

    void init(){
        _Data=NULL;
        len=0;
        max=0;
    }

    void init(unsigned long inital){
        if(inital > 0) {
            _Data=NULL;
            len=inital;
            max=0;
            __expand();
        } else
            init();
    }

    void setMax(unsigned long max) {
        this->max = max;
    }

    void push_back(T& data) {
        __expand();
        _Data[len-1]=data;
    }

    void add(T data) {
        __expand();
        _Data[len-1]=data;
    }

    void __new() {
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
        if(_X>=len || len<=0){
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
            T* result = (T*)__malloc(sizeof(T)*(len-1));
            long long newLen=len-1;
            for(long long i = 0; i < _X; i++)
                result[i] = _Data[i];
            for(long long i = _X; i < newLen; i++)
                result[i] = _Data[i + 1];

            free();
            len=newLen;
            _Data=result;
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

    void addReplace(T _X, T& _V) {
        for(unsigned long i = 0; i < len; i++) {
            if(_X == _Data[i]) {
                replace(i, _V);
                return;
            }
        }

        add(_V);
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

    unsigned long  long size() { return len; }

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

    int removefirst(T _V) {
        long long iter = -1;
        for(unsigned int i = 0; i < len; i++) {
            if(_V == _Data[i]){
                iter = i;
                break;
            }
        }

        if(iter != -1) {
            remove(iter);
            return 0;
        }
        return 1;
    }

    /*
     * Programmer must be responsible
     * for freeing that data himself
     */
    void replace(unsigned long _X, T repl) {
        if(_X>=len || _X < 0){
            stringstream ss;
            ss << "index out of bounds list::replace() _X: " << _X
               << " length: " << len << endl;
            throw Exception(ss.str());
        }
        _Data[_X] = repl;
    }

    void addAll(_List<T>& list) {
        free();
        for(unsigned int i = 0; i < list.size(); i++) {
            push_back(list.get(i));
        }
    }

    void appendAll(_List<T>& list) {
        for(unsigned int i = 0; i < list.size(); i++) {
            push_back(list.get(i));
        }
    }

    void addAll(std::list<T> &list) {
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

    long long indexof(bool (*pfind)(void*, T element), void* data) {
        for(unsigned int i = 0; i < len; i++) {
            if(pfind(data, _Data[i])){
                return i;
            }
        }
        return -1;
    }

    long long indexof(T data) {
        for(unsigned int i = 0; i < len; i++) {
            if(data == _Data[i]){
                return i;
            }
        }
        return -1;
    }

    void del(T data) {
        for(unsigned long long c = 0; c < len; c++) {
            if(data == _Data[c]){

                if(len==1){
                    free();
                }
                else if(len==2) {
                    if(c==0) {
                        _Data[0]=_Data[1];
                    }
                    __shrink();
                } else {
                    T* result = (T*)__malloc(sizeof(T)*(len-1));
                    long long newLen=len-1;
                    for(long long i = 0; i < c; i++)
                        result[i] = _Data[i];
                    for(long long i = c; i < newLen; i++)
                        result[i] = _Data[i + 1];

                    std::free(_Data);
                    len=newLen;
                    _Data=result;
                }

                return;
            }
        }
    }

    bool empty() { return len==0; }

    T* _Data, *ptr;
    unsigned  long long len;
private:
    CXX11_INLINE
    void __expand() {
        try{
            len++;
            if(max != 0 && len >= max) {
                stringstream ss;
                ss << "index out of bounds list::expand() _X: " << len
                   << " max: " << max << endl;
                throw Exception(ss.str());
            }

            if(_Data==NULL){
                ptr=(T*)__malloc(sizeof(T)*len);
            } else {
                ptr=(T*)__realloc(_Data, sizeof(T)*len, sizeof(T)*(len-1));
            }

            _Data=ptr;
        } catch(Exception &e){
            len--;
            throw e;
        }
    }

    CXX11_INLINE
    void __shrink(){
        try {
            len--;
            if(len==0)
                free();
            else
                _Data=(T*)__realloc(_Data, sizeof(T)*len, sizeof(T)*(len-1));
        } catch(Exception &e) {
            len++;
            throw e;
        }
    }

    unsigned  long long max;
};

#endif //SHARP_LIST_H
