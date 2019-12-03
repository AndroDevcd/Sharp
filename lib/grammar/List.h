//
// Created by bknun on 1/18/2018.
//

#ifndef SHARP_LIST2_H
#define SHARP_LIST2_H

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

    void insert(unsigned long long pos, T& data) {
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
            unsigned long long newLen=len+1;
            for(unsigned long long i = 0; i < pos; i++)
                result[i] = _Data[i];
            result[pos] = data;
            for(unsigned long long i = pos + 1; i < newLen; i++)
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
            unsigned long long newLen=len-1,iter=0;
            for(unsigned long long i = 0; i < _X; i++)
                result[iter++] = _Data[i];
            for(unsigned long long i = _X+1; i < len; i++)
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
        for(unsigned long long i = 0; i < len; i++) {
            if(_X == _Data[i]) {
                remove(i);
                return;
            }
        }
    }

    T at(unsigned long long _X) {
        if(_X>=len){
            stringstream ss;
            ss << "index out of bounds list::get() _X: " << _X
               << " length: " << len << endl;
            throw std::runtime_error(ss.str());
        }
        return _Data[_X];
    }

    T& get(unsigned long long _X) {
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

    unsigned long long size() { return len; }

    bool singular() { return len == 1; }

    void pop_back() {
        __shrink();
    }

    bool addif(T _V) {
        for(unsigned long long i = 0; i < len; i++) {
            if(_V == _Data[i])
                return false;
        }

        push_back(_V);
        return true;
    }

    bool sameElements(List<T> &list) {
        for(unsigned long long i = 0; i < len; i++) {
            if(list._Data[i] != _Data[i])
                return false;
        }

        return true;
    }

    void removefirst(T _V) {
        unsigned long long iter = -1;
        for(unsigned long long i = 0; i < len; i++) {
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
    void replace(unsigned long long _X, T repl) {
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
        for(unsigned long long i = 0; i < list.size(); i++) {
            push_back(list.get(i));
        }
    }

    void addAllUnique(List<T>& list) {
        free();
        for(unsigned long long i = 0; i < list.size(); i++) {
            addif(list.get(i));
        }
    }

    void appendAll(List<T>& list) {
        for(unsigned long long i = 0; i < list.size(); i++) {
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
        for(unsigned long long i = 0; i < len; i++) {
            if(data == _Data[i]){
                return true;
            }
        }
        return false;
    }

    long long count(T data) {
        long long num=0;
        for(unsigned long long i = 0; i < len; i++) {
            if(data == _Data[i]){
                num++;
            }
        }
        return num;
    }

    unsigned long long indexof(T data) {
        for(unsigned long long i = 0; i < len; i++) {
            if(data == _Data[i]){
                return i;
            }
        }
        return -1;
    }

    void linearSort(bool (*swap)(T e1, T e2)) {
        for(unsigned int i = 0; i < len; i++) {

            for (unsigned int j = 0; j < len; j++) {
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
            for(unsigned long long i = 0; i < len; i++) {
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
			for (unsigned long long i = 0; i < len-1; i++) {
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

    unsigned  long long len;
    T* _Data;
};

#endif //SHARP_LIST2_H
