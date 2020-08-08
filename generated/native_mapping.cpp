#include "native_mapping.h"

using namespace std;
using namespace snb_api;

#ifdef __cplusplus
extern "C" {
#endif

EXPORTED uint32_t snb_link_proc(const char* funcName) {
	string name = funcName;

	if(name == "app#__srt_global.foo") {  return 1557;  }
	else return -1;
}

void call_app$__srt_global_foo() {
	import(app); 

	foo();
}

EXPORTED void snb_main(long procAddr) {
	try {
		switch(procAddr) {
			case 1557: 
				call_app$__srt_global_foo();
				break;
		}
	} catch(Exception &e) {
		snb_api::internal::prepareException(e.exceptionClass);
	}
}

#ifdef __cplusplus
}
#endif
