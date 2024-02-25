//
// Created by BNunnally on 9/5/2020.
//

#ifndef SHARP_LINKED_LIST_H
#define SHARP_LINKED_LIST_H

#include "../../stdimports.h"

template<class T>
struct node
{
    node()
    :
        data(),
        next(nullptr)
    {}

    node(const node<T> &n)
    :
        data(n.data),
        next(n.next)
    {}

    T data;
    node *next;
};

template<class T>
class linkedlist
{
private:
    node<T> *head, *tail;

public:
    uInt size;

    linkedlist()
    {
        head=NULL;
        tail=NULL;
    }

    void delete_all(void (*onDelete)(node<T> *)) {
        if(head != NULL) {
            auto cur = head;
            while(cur != nullptr) {
                auto next = cur->next;
                if(onDelete != nullptr)
                    onDelete(cur);
                delete cur;
                cur = next;
            }
        }

        head = nullptr;
        tail = nullptr;
    }

    node<T>* node_at(uInt pos) {
        if(head != NULL) {
            auto *cur = head;
            node<T> *temp = NULL;
            for (int i = 0; i < pos; i++) {
                cur = cur->next;
                if (cur == NULL)
                    return NULL;
            }
            return cur;
        }

        return NULL;
    }

    node<T>* node_at(void *data, bool (*isNode)(void *, node<T> *)) {
        if(head != NULL) {
            auto *cur = head;
            for (;;) {
                if (cur == NULL)
                    return NULL;
                else if(isNode(data, cur))
                    return cur;
                cur = cur->next;
            }
        }

        return NULL;
    }

    void foreach(void *data, void (*item)(void*, node<T> *)) {
        if(head != NULL) {
            auto *cur = head;
            for (;;) {
                if (cur == NULL)
                    return;
                else {
                    item(data, cur);
                    cur = cur->next;
                }
            }
        }
    }

    void delete_at(void *data, bool (*isNode)(void *, node<T> *)) {
        if(head != NULL) {
            auto *cur = head;
            node<T> *previous = NULL;
            for (;;) {
                previous = cur;
                cur = cur->next;
                if (cur == NULL)
                    return;
                else if(isNode(data, cur)) {
                    if (previous)
                        previous->next = cur->next;
                    delete cur;
                    return;
                }
            }
        }
    }

    void createnode(T value)
    {
        size++;
        auto *temp=new node<T>;
        temp->data=value;
        temp->next=NULL;
        if(head==NULL)
        {
            head=temp;
            tail=temp;
            temp=NULL;
        }
        else
        {
            tail->next=temp;
            tail=temp;
        }
    }

    void insert_start(T value)
    {
        size++;
        auto *temp=new node<T>;
        temp->data=value;
        temp->next=head;
        head=temp;
    }

    void insert_position(uInt pos, T value)
    {
        size++;
        node<T> *pre=NULL;
        auto *cur=head;
        node<T> *temp=NULL;
        for(int i=1;i<pos;i++)
        {
            pre=cur;
            cur=cur->next;
        }
        temp->data=value;
        pre->next=temp;
        temp->next=cur;
    }

    void delete_first()
    {
        if(head != NULL) {
            size--;
            auto *temp = head;
            head = head->next;
            delete temp;
        }
    }

    void delete_last()
    {
        if(head != NULL) {
            size--;
            auto *current = head;
            node<T> *previous = NULL;
            while (current->next != NULL) {
                previous = current;
                current = current->next;
            }

            if (previous) {
                tail = previous;
                previous->next = NULL;
            } else {
                head = NULL;
                tail = NULL;
            }
            delete current;
        }
    }

    void delete_position(uInt pos)
    {
        if(pos == 0)
            delete_first();
        else if(pos == (size - 1))
            delete_last();
        else if(head != NULL) {
            size--;
            auto *current = head;
            node<T> *previous = NULL;
            for (int i = 1; i < pos; i++) {
                previous = current;
                current = current->next;
            }

            if (previous)
                previous->next = current->next;
            delete current;
        }
    }
};

#endif //SHARP_LINKED_LIST_H
