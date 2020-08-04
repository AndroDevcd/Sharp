#include "native_mapping.h"

using namespace std;
using namespace snb_api;

#ifdef __cplusplus
extern "C" {
#endif

EXPORTED uint32_t snb_link_proc(const char* funcName) {
    string name = funcName;

    if(name == "main#__srt_global.printMessage") {  return 1557;  }
    else return -1;
}

void call_main$__srt_global_printMessage() {
    import(main);

    printMessage();
}

EXPORTED void snb_main(long procAddr) {
    try {
        switch (procAddr) {
            case 1557:
                call_main$__srt_global_printMessage();
                break;
        }
    } catch(Exception &e) {
        snb_api::internal::prepareException(e.exceptionClass);
    }
}

#ifdef __cplusplus
}
#endif
