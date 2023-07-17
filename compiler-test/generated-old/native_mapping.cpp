//
// Created by bknun on 7/15/2023.
//

#include "native_mapping.h"

using namespace std;
using namespace snb_api;

#ifdef __cplusplus
extern "C" {
#endif

EXPORTED uint32_t snb_link_proc(const char* funcName) {
    string name = funcName;

    if(name == "main#__srt_global.foo_native") {  return 128;  }
    else return -1;
}

void snb_main$__srt_global_foo_native() {
    cout << "hello world!" << endl;
}

namespace std_global {
    void string(SharpObject instance, SharpObject message) {}
}

EXPORTED void snb_main(long procAddr) {
    try {
        switch (procAddr) {
            case 128:
                snb_main$__srt_global_foo_native();
                break;
        }
    } catch(VMException &e) {
        using namespace snb_api::internal;

        if(!is_exception_thrown()) {
            if(e.exceptionClass)
                prepare_exception(e.exceptionClass);
            else {
                auto exceptionVar = create_local_variable(); // create space for new exception
                if(exceptionVar.obj == nullptr) return;

                new_class("std#error");
                assign_object(exceptionVar.obj, pop_object());
                auto message = get_field(exceptionVar.obj, "message");
                assign_object(message, create_string_class("cpp error: thrown exception class not found", std_global::string));

                exceptionVar.refresh(); // refresh local vars after function call
                prepare_exception(exceptionVar.obj);
                pop_object();
            }
        }
    }
}

#ifdef __cplusplus
}
#endif