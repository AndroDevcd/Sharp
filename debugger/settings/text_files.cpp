//
// See text_files.h.
//

#include "text_files.h"
#include <cstdio>

bool read_text_file(const string &file, string &text) {
    FILE *in = fopen(file.c_str(), "rb");
    if(in == nullptr) return false;

    text.clear();
    char bytes[4096];
    size_t count;
    while((count = fread(bytes, 1, sizeof(bytes), in)) > 0) text.append(bytes, count);
    fclose(in);
    return true;
}

bool write_text_file(const string &file, const string &text) {
    FILE *out = fopen(file.c_str(), "wb");
    if(out == nullptr) return false;

    bool all = fwrite(text.data(), 1, text.size(), out) == text.size();
    return fclose(out) == 0 && all;
}

std::vector<string> lines_of(const string &text) {
    std::vector<string> lines;
    size_t from = 0;
    while(from < text.size()) {
        size_t end = text.find('\n', from);
        if(end == string::npos) end = text.size();
        string line = text.substr(from, end - from);
        if(!line.empty() && line[line.size() - 1] == '\r') line.erase(line.size() - 1);
        lines.push_back(line);
        from = end + 1;
    }
    return lines;
}
