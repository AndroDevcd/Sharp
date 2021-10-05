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
            subComponentName(componentRequest.subComponentName),
            componentName(componentRequest.componentName),
            resolvedTypeDefinition(componentRequest.resolvedTypeDefinition)
    {
    }

    get_component_request(string &subComponentName, string &componentName)
            :
            subComponentName(subComponentName),
            componentName(componentName),
            resolvedTypeDefinition(NULL)
    {
    }


    string subComponentName;
    string componentName;
    component_type *resolvedTypeDefinition;
};

#endif //SHARP_GET_COMPONENT_REQUEST_H
