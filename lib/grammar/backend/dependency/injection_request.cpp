//
// Created by bknun on 6/9/2022.
//
#include "injection_request.h"
#include "../types/get_component_request.h"


get_component_request *injection_request::to_component_request() {
    string typeName = "";
    return new get_component_request(typeName, this->componentName);
}
