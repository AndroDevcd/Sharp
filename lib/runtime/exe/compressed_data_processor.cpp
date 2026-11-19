//
// Created by bknun on 9/19/2022.
//

#include "compressed_data_processor.h"
#include "exe_processor.h"
#include "../../core/exe_macros.h"
#include "../virtual_machine.h"
#include "../main.h"
#include "../types/sharp_field.h"
#include "../../util/zip/zlib.h"

void process_compressed_data(KeyPair<int, string> &result) {
    if(next_char() == data_compress) {
        stringstream buf, __outbuf__;
        for(uInt i = cursor + 1; i < exeData.size(); i++) {
            __outbuf__ << exeData[i];
        }

        auto datas = __outbuf__.str();
        Zlib zlib;
        Zlib::AUTO_CLEAN=(true);
        zlib.Decompress_Buffer2Buffer(__outbuf__.str(), buf);


        exeData = buf.str();
        cursor = -1;
    } else cursor--;
}
