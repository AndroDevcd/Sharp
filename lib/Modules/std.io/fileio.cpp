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
 *
 * windows
 * 00	Existence only
 * 02	Write-only
 * 04	Read-only
 * 06	Read and write
 *
 * EACCES
 * Access denied: the file's permission setting does not allow specified access.
 *
 * ENOENT
 * File name or path not found.
 *
 * EINVAL
 * Invalid parameter.
 */
int check_access(native_string& path, int access_flg) {
    int i = EACCES;
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
