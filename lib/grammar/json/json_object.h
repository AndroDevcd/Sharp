//
// Created by bnunnally on 8/29/21.
//

#ifndef SHARP_JSON_OBJECT_H
#define SHARP_JSON_OBJECT_H

#include "json_member.h"
#include "../List.h"

class json_object {
public:
    json_object()
    :
        members()
    {}

    json_object(const json_object &jo)
    :
        json_object()
    {
        for(Int i = 0; i < jo.members.size(); i++) {
            members.__new() = new json_member();
            members.last()->copy(*jo.members.get(i));
        }
    }

    ~json_object() {
        free();
    }

    void free() {
        for(Int i = 0; i < members.size(); i++) {
            delete members.get(i);
        }

        members.free();
    }

    uInt size() {
        return members.size();
    }

    json_member* operator[](string name) {
        return getMember(name);
    }

    json_member* operator[](Int index) {
        return members.get(index);
    }

    void toString(stringstream &str, uInt &tabOffset) {
        str << "{" << endl;
        tabOffset++;

        for(uInt i = 0; i < size(); i++) {
            members.get(i)->toString(str, tabOffset);

            if((i + 1) < size()) {
                str << ",";
            }

            str << endl;
        }

        tabOffset--;
        addTabs(str, tabOffset);
        str << "}";
    }

    bool addMember(string name) {
        if(getMember(name)) return false;

        members.__new() = new json_member();
        members.last()->setName(name);
        return true;
    }

    void addMember(json_member *member) {
        members.add(member);
    }

private:
    json_member* getMember(string name) {
        for(Int i = 0; i < members.size(); i++) {
            if(members.get(i)->getName() == name) {
                return members.get(i);
            }
        }

        return NULL;
    }

    List<json_member*> members;
};

#endif //SHARP_JSON_OBJECT_H
