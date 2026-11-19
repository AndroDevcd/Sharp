//
// Created by bknun on 9/18/2022.
//

#include "meta_data.h"
#include "../util/list_helper.h"

CXX11_INLINE void init_struct(file_data *fd) {
    new (&fd->name) string();
    new (&fd->lines) linkedlist<string>();
}

string get_line_info(file_data *file, Int line) {
    if(line >= 0 && line < file->lines.size && file->lines.node_at(line))
        return file->lines.node_at(line)->data;
    else return "";
}

string get_info(file_data *file, Int line) {
    line -= 2;
    bool linesPrinted = false;
    stringstream ss;
    string tmp;

    for(Int i = 0; i < 5; i++) {
        tmp = get_line_info(file, line);
        if (!tmp.empty()) {
            linesPrinted = true;
            if(i == 2)
                ss << endl << "\t>> " << line << ":    ";
            else
                ss << endl << "\t   " << line << ":    ";
            ss << tmp;
        }
        line++;
    }
    if(linesPrinted)
        return ss.str();
    else
        return "";
}
