//
// Created by BraxtonN on 5/25/2018.
//

#include "../../runtime/oo/string.h"
#include "../../../stdimports.h"
#include  <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef WIN32
#include <unistd.h>
#endif

#ifdef WIN32
#define stat _stat
#endif


native_string resolve_path(native_string& path) {
    native_string fullPath;

    char full_path[MAX_PATH];

    GetFullPathName(path.str().c_str(), MAX_PATH, full_path, NULL);

    for(int i = 0; i < MAX_PATH; i++) {
        if(full_path[i] != 0x0)
            fullPath += full_path[i];
        else
            break;
    }
    return fullPath;
}

long get_file_attrs(native_string& path) {
    return GetFileAttributes(path.str().c_str());
}

/**
 *
 * @param path
 * @param access_flg
 * @return
 */
int check_access(native_string& path, int access_flg) {
    return _access( path.c_str(), access_flg );
}

struct stat result;
long long last_update(native_string& path) {
    if(stat(path.c_str(), &result)==0)
    {
        return result.st_mtime;
    }
    return 0;
}
