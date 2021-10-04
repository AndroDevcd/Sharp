//
// Created by BNunnally on 10/3/2021.
//

#ifndef SHARP_INJECTION_REQUEST_H
#define SHARP_INJECTION_REQUEST_H

#include "../../../../stdimports.h"

struct injection_request {
    injection_request()
    :
        componentName("")
    {}

    injection_request(const injection_request &request)
    :
        componentName("")
    {
        this->componentName = request.componentName;
    }

    injection_request(string &componentName)
    :
        componentName("")
    {
        this->componentName = componentName;
    }

    string componentName;
};

#endif //SHARP_INJECTION_REQUEST_H
