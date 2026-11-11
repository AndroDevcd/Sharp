//
// Pass "none": changes nothing.
// It exists so the optimizer pipeline can be exercised on every function before
// any real pass is trusted.
//

#include "passes.h"

bool pass_none(function_ir &ir) {
    (void)ir;
    return false;
}
