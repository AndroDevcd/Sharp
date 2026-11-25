//
// Created by bknun on 1/18/2018.
//

#ifndef SHARP_KEYPAIR_H
#define SHARP_KEYPAIR_H

template <class K, class T>
struct KeyPair
{
    KeyPair()
            :
            key(),
            value()
    {
    }

    KeyPair(K k, T v)
            :
            key(k),
            value(v)
    {
    }

    KeyPair(const KeyPair<K, T> &pair)
            :
            key(pair.key),
            value(pair.value)
    {
    }

    KeyPair<K, T>& operator=(const KeyPair<K, T> &kp) {
        key = kp.key;
        value = kp.value;

        return *this;
    }

    KeyPair<K, T>& with(K k, T t) {
        key = k;
        value = t;
        return *this;
    }

    void set(K k, T t) {
        key = k;
        value = t;
    }

    K key;
    T value;
};

#endif //SHARP_KEYPAIR_H
