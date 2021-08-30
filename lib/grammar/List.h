//
// Created by bknun on 1/18/2018.
//

#ifndef SHARP_LIST2_H
#define SHARP_LIST2_H

#include "../../stdimports.h"
#include <algorithm>

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

    void push_back(T& data) {
        __expand();
        _Data[len-1]=data;
    }

    void add(T data) {
        __expand();
        _Data[len-1]=data;
    }

    T& __new() {
        __expand();
        return _Data[len-1];
    }

    T& operator[](uInt pos) {
        return get(pos);
    }

    void insert(uint32_t pos, T& data) {
        if(pos>len || pos < 0) {
            stringstream ss;
            ss << "index out of bounds list::insert() _X: " << pos
               << " length: " << len << endl;
            throw std::runtime_error(ss.str());
        }

        if(len == 0) {
            push_back(data);
        } else {
            T* result = new T[len+1];
            uint32_t newLen=len+1;
            for(uint32_t i = 0; i < pos; i++)
                result[i] = _Data[i];
            result[pos] = data;
            for(uint32_t i = pos + 1; i < newLen; i++)
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
    void removeAt(uint32_t _X) {
        if(_X>=len || len==0){
            stringstream ss;
            ss << "index out of bounds list::removeAt() _X: " << _X
               << " length: " << len << endl;
            throw std::runtime_error(ss.str());
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
            T* result = new T[len-1];
            uint32_t newLen=len-1,iter=0;
            for(uint32_t i = 0; i < _X; i++)
                result[iter++] = _Data[i];
            for(uint32_t i = _X+1; i < len; i++)
                result[iter++] = _Data[i];

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
        for(uint32_t i = 0; i < len; i++) {
            if(_X == _Data[i]) {
                removeAt(i);
                return;
            }
        }
    }

    T at(uint32_t _X) {
        if(_X>=len){
            stringstream ss;
            ss << "index out of bounds list::get() _X: " << _X
               << " length: " << len << endl;
            throw std::runtime_error(ss.str());
        }
        return _Data[_X];
    }

    T & get(uint32_t _X) const {
        if(_X>=len || _X < 0){
            stringstream ss;
            ss << "index out of bounds list::get() _X: " << _X
               << " length: " << len << endl;
            throw std::runtime_error(ss.str());
        }
        return _Data[_X];
    }

    T& last() {
        if(len == 0){
            stringstream ss;
            ss << "illegal state list::last() length: 0" << endl;
            throw std::runtime_error(ss.str());
        }
        return _Data[len-1];
    }

    void free() {
        if(_Data != NULL)
            delete[] (_Data);
        _Data = NULL;
        len=0;
    }

    uint32_t size() const { return len; }

    bool singular() { return len == 1; }

    void pop_back() {
        __shrink();
    }

    bool addif(T _V) {
        for(uint32_t i = 0; i < len; i++) {
            if(_V == _Data[i])
                return false;
        }

        push_back(_V);
        return true;
    }


    int32_t addIfIndex(T _V) {
        for(uint32_t i = 0; i < len; i++) {
            if(_V == _Data[i])
                return i;
        }

        push_back(_V);
        return len - 1;
    }

    bool sameElements(List<T> &list) {
        for(uint32_t i = 0; i < len; i++) {
            if(list._Data[i] != _Data[i])
                return false;
        }

        return true;
    }

    void removefirst(T _V) {
        uint32_t iter = -1;
        for(uint32_t i = 0; i < len; i++) {
            if(_V == _Data[i]){
                iter = i;
                break;
            }
        }

        if(iter != -1) {
            removeAt(iter);
        }
    }

    /*
     * Programmer must be responsible
     * for freeing that data himself
     */
    void replace(uint32_t _X, T repl) {
        if(_X>=len || _X < 0){
            stringstream ss;
            ss << "index out of bounds list::replaceat() _X: " << _X
               << " length: " << len << endl;
            throw std::runtime_error(ss.str());
        }
        _Data[_X] = repl;
    }

    void addAll(List<T>& list) {
        free();
        for(uint32_t i = 0; i < list.size(); i++) {
            push_back(list.get(i));
        }
    }

    void addAllUnique(List<T>& list) {
        free();
        for(uint32_t i = 0; i < list.size(); i++) {
            addif(list.get(i));
        }
    }

    void appendAll(List<T>& list) {
        for(uint32_t i = 0; i < list.size(); i++) {
            push_back(list.get(i));
        }
    }

    void appendAllUnique(List<T>& list) {
        for(uint32_t i = 0; i < list.size(); i++) {
            addif(list.get(i));
        }
    }

    void addAll(list<T> &list) {
        free();
        for(T& t : list) {
            push_back(t);
        }
    }

    bool find(T data) {
        for(uint32_t i = 0; i < len; i++) {
            if(data == _Data[i]){
                return true;
            }
        }
        return false;
    }

    long long count(T data) {
        long long num=0;
        for(uint32_t i = 0; i < len; i++) {
            if(data == _Data[i]){
                num++;
            }
        }
        return num;
    }

    int32_t indexof(T data) {
        for(uint32_t i = 0; i < len; i++) {
            if(data == _Data[i]){
                return i;
            }
        }
        return -1;
    }

    int32_t indexof(bool (*isElement)(T* e, void *e2), void *e2) {
        for(uint32_t i = 0; i < len; i++) {
            if(isElement(&_Data[i], e2)){
                return i;
            }
        }
        return -1;
    }

    bool find(bool (*isElement)(T* e, void *e2), void *e2) {
        for(uint32_t i = 0; i < len; i++) {
            if(isElement(&_Data[i], e2)){
                return true;
            }
        }
        return false;
    }

    void linearSort(bool (*swap)(T e1, T e2)) {
        for(uint32_t i = 0; i < len; i++) {

            for (uint32_t j = 0; j < len; j++) {
                if((j + 1) < len) {
                    if (swap(_Data[j], _Data[j+1])) {
                        T tmp = _Data[j];
                        _Data[j] = _Data[j+1];
                        _Data[j+1] = tmp;
                    }
                }
            }
        }

    }

    bool empty() { return len==0; }

private:
    CXX11_INLINE
    void __expand() {
        try{
            T* newbuf = new T[len+1];
            for(uint32_t i = 0; i < len; i++) {
                newbuf[i] = _Data[i];
            }
            delete[] _Data;
            _Data = newbuf;
            len++;
        } catch(std::exception &e){
            len--;
            throw e;
        }
    }

    CXX11_INLINE
    void __shrink(){
        try {
            if(len==0) {
                free();
                return;
            }

            T* newbuf = new T[len-1];
			for (uint32_t i = 0; i < len-1; i++) {
				newbuf[i] = _Data[i];
			}
            delete[] _Data;
            _Data = newbuf;
            len--;
        } catch(std::exception &e) {
            len++;
            throw e;
        }
    }

    uint32_t len;
    T* _Data;
};

#endif //SHARP_LIST2_H
