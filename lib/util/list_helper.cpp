//
// Created by bknun on 4/20/2023.
//

#include "list_helper.h"


template<class T>
T get_item_at(std::list<T> lst, Int pos) {
    std::list<T> l; // look, no pointers!
    auto l_front = l.begin();

    std::advance(l_front, pos);
    return *l_front;
}