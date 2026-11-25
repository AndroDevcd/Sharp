//
// Created by bknun on 9/19/2022.
//

#include "compressed_data_processor.h"
#include "exe_processor.h"
#include "exe_macros.h"
#include "../virtual_machine.h"
#include "../main.h"
#include "../types/sharp_field.h"

/*
 * The old C++ compiler compressed the rest of a large program from here on and
 * marked it with the `data_compress` byte. The compiler has not written that
 * byte for a long time and the VM no longer unpacks it: a file that still has
 * it is refused, with what to do about it.
 */
void process_compressed_data(KeyPair<int, string> &result) {
    if(next_char() == data_compress) {
        throw runtime_error("file `" + executable + "` was made by an old compiler that compressed it; "
                            "compile the program again with this version's sharpc.\n");
    } else cursor--;
}
