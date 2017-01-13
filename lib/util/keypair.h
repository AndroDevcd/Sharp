//
// Created by bknun on 1/6/2017.
//

#ifndef SHARP_KEYPAIR_H
#define SHARP_KEYPAIR_H

template <class K, class T>
struct keypair
{
    keypair(K k, T v)
    :
            key(k),
            value(v)
    {
    }

    keypair()
    {
    }
    void set(K k, T val)
    {
        key = k;
        value = val;
    }

    K key;
    T value;
};

#endif //SHARP_KEYPAIR_H
