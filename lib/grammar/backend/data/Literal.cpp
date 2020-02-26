
#include "Literal.h"

void Literal::free() {
    release();
    stringData = "";
}
