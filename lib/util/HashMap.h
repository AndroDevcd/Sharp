//
// Created by BNunnally on 4/26/2020.
//

#ifndef SHARP_HASHMAP_H
#define SHARP_HASHMAP_H

#include <mutex>
#include "../../stdimports.h"

#define TABLE_SIZE 25

#define GUARD(mut) \
    std::lock_guard<recursive_mutex> guard(mut);

template <typename K, typename V>
class HashNode {
public:
    HashNode(const K &key, const V &value) :
            key(key), value(value), next(NULL) {
    }

    K getKey() const {
        return key;
    }

    V getValue() const {
        return value;
    }

    void setValue(V value) {
        HashNode::value = value;
    }

    HashNode *getNext() const {
        return next;
    }

    void setNext(HashNode *next) {
        HashNode::next = next;
    }

private:
    // key-value pair
    K key;
    V value;
    // next bucket with the same key
    HashNode *next;
};

template <typename K>
struct KeyHash {
    uInt operator()(const K& key, uInt tableSize) const
    {
        return key % tableSize;
    }
};

template <typename K, typename V, typename F = KeyHash<K>>
class HashMap {
public:
    HashMap()
    :
       monitor()
    {
        init(TABLE_SIZE);
    }

    HashMap(uInt tableSize)
    :
       monitor()
    {
        // construct zero initialized hash table of size
        init(tableSize);
    }

    void init(uInt size) {
        table = new HashNode<K, V> *[size];
        this->tableSize = size;

        for(Int i = 0; i < size; i++) {
            table[i] = NULL;
        }
    }

    ~HashMap() {
        monitor.lock();
        if(table != NULL) {
            // destroy all buckets one by one
            for (uInt i = 0; i < tableSize; ++i) {
                HashNode<K, V> *entry = table[i];
                while (entry != NULL) {
                    HashNode<K, V> *prev = entry;
                    entry = entry->getNext();
                    delete prev;
                }
                table[i] = NULL;
            }
            // destroy the hash table
            delete[] table;
            table = NULL;
        }
        monitor.unlock();
    }

    bool get(const K &key, V &value) {
        GUARD(monitor)
        uInt hashValue = hashFunc(key, tableSize);
        HashNode<K, V> *entry = table[hashValue];

        while (entry != NULL) {
            if (entry->getKey() == key) {
                value = entry->getValue();
                return true;
            }
            entry = entry->getNext();
        }
        return false;
    }

    void put(const K &key, const V &value) {
        GUARD(monitor)
        uInt hashValue = hashFunc(key, tableSize);
        HashNode<K, V> *prev = NULL;
        HashNode<K, V> *entry = table[hashValue];

        while (entry != NULL && entry->getKey() != key) {
            prev = entry;
            entry = entry->getNext();
        }

        if (entry == NULL) {
            entry = new HashNode<K, V>(key, value);
            if (prev == NULL) {
                // insert as first bucket
                table[hashValue] = entry;
            } else {
                prev->setNext(entry);
            }
        } else {
            // just update the value
            entry->setValue(value);
        }
    }

    void remove(const K &key) {
        GUARD(monitor)
        uInt hashValue = hashFunc(key, tableSize);
        HashNode<K, V> *prev = NULL;
        HashNode<K, V> *entry = table[hashValue];

        while (entry != NULL && entry->getKey() != key) {
            prev = entry;
            entry = entry->getNext();
        }

        if (entry == NULL) {
            // key not found
            return;
        }
        else {
            if (prev == NULL) {
                // remove first bucket of the list
                table[hashValue] = entry->getNext();
            } else {
                prev->setNext(entry->getNext());
            }
            delete entry;
        }
    }

private:
    // hash table
    HashNode<K, V> **table;
    F hashFunc;
    uInt tableSize;
    recursive_mutex monitor;
};

#endif //SHARP_HASHMAP_H
