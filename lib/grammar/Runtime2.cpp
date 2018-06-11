//
// Created by BraxtonN on 6/11/2018.
//

#include <dirent.h>
#include <sys/stat.h>
#include "../../stdimports.h"
#ifdef POSIX_
#include <cmath>
#endif
#include "Runtime.h"
#include "parser/ErrorManager.h"
#include "List.h"
#include "parser/Parser.h"
#include "../util/File.h"
#include "../runtime/Opcode.h"
#include "../runtime/register.h"
#include "Asm.h"
#include "../runtime/Exe.h"
#include "Optimizer.h"
#include "../util/zip/zlib.h"
#include "../util/time.h"
#include "Method.h"

/**
 * This is the second filr to our compiler
 * It has become so big that is is no longer useful be the code analyzer dute to oversized file
 */

string RuntimeEngine::getSwitchTable(Method* func, long addr) {
    stringstream ss;
    ss << " table [";

    SwitchTable &st = func->switchTable.get(addr);
    ss << "default: " << st.defaultAddress << " ";
    for(long i = 0; i < st.values.size(); i++) {
        ss << "case " << st.values.get(i) << ": @";
        ss << st.addresses.get(i);

        if((i+1) < st.values.get(i)) {
            ss << ", ";
        } else
            ss << " ";
    }
    ss << "]";
    return ss.str();
}
