//
// Created by BNunnally on 10/5/2021.
//

#ifndef SHARP_GET_COMPONENT_REQUEST_H
#define SHARP_GET_COMPONENT_REQUEST_H

#include "../../../../stdimports.h"

struct get_component_request {
    get_component_request()
    :
            typeDefinitionName(""),
            componentName(""),
            resolvedTypeDefinition(NULL)
    {
    }

    get_component_request(const get_component_request &componentRequest)
            :
            typeDefinitionName(""),
            componentName(""),
            resolvedTypeDefinition(NULL)
    {
        typeDefinitionName = componentRequest.typeDefinitionName;
        componentName = componentRequest.componentName;
        resolvedTypeDefinition = componentRequest.resolvedTypeDefinition;
    }

    get_component_request(string &typeDefinitionName, string &componentName)
            :
            typeDefinitionName(""),
            componentName(""),
            resolvedTypeDefinition(NULL)
    {
        this->typeDefinitionName = typeDefinitionName;
        this->componentName = componentName;
    }


    string typeDefinitionName;
    string componentName;
    type_definition *resolvedTypeDefinition;
};

#endif //SHARP_GET_COMPONENT_REQUEST_H
