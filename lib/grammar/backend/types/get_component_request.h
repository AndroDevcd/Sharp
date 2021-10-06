//
// Created by BNunnally on 10/5/2021.
//

#ifndef SHARP_GET_COMPONENT_REQUEST_H
#define SHARP_GET_COMPONENT_REQUEST_H

#include "../../../../stdimports.h"

struct get_component_request {
    get_component_request()
    :
        subComponentName(""),
        componentName(""),
        resolvedTypeDefinition(NULL)
    {
    }

    get_component_request(const get_component_request &componentRequest)
            :
            subComponentName(""),
            componentName(""),
            resolvedTypeDefinition(NULL)
    {
        subComponentName = componentRequest.subComponentName;
        componentName = componentRequest.componentName;
        resolvedTypeDefinition = componentRequest.resolvedTypeDefinition;
    }

    get_component_request(string &subComponentName, string &componentName)
            :
            subComponentName(""),
            componentName(""),
            resolvedTypeDefinition(NULL)
    {
        this->subComponentName = subComponentName;
        this->componentName = componentName;
    }


    string subComponentName;
    string componentName;
    type_definition *resolvedTypeDefinition;
};

#endif //SHARP_GET_COMPONENT_REQUEST_H
