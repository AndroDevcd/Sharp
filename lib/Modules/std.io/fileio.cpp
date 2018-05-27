//
// Created by BraxtonN on 5/25/2018.
//

#include "../../runtime/oo/string.h"
#include "../../../stdimports.h"
#include "../../runtime/List.h"
#include  <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fstream>
#include <dirent.h>

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
        if(full_path[i] != '\000')
            fullPath += full_path[i];
        else
            break;
    }
    return fullPath;
}

int FILE_EXISTS    = 0x01;
int FILE_REGULAR   = 0x02;
int FILE_DIRECTORY = 0x04;
int FILE_HIDDEN    = 0x08;
struct stat result;
long long get_file_attrs(native_string& path) {
    if(stat(path.str().c_str(), &result)==0)
    {
        long long mode = result.st_mode, attrs=0;

        // regular file
        if(S_ISREG(mode))
            attrs |= FILE_REGULAR;

        // directory
        if(S_ISDIR(mode))
            attrs |= FILE_DIRECTORY;

        // exists
        attrs |= FILE_EXISTS;

        long attributes = GetFileAttributes(path.c_str());
        if (attributes & FILE_ATTRIBUTE_HIDDEN)
            attrs |= FILE_HIDDEN;

        return attrs;
    }
    return 0;
}

/**
 *
 * @param path
 * @param access_flg
 * @return
 */
int check_access(native_string& path, int access_flg) {
    return _access( path.str().c_str(), access_flg );
}

long long last_update(native_string& path) {
    if(stat(path.str().c_str(), &result)==0)
    {
        return result.st_mtime;
    }
    return 0;
}

long long file_size(native_string &path)
{
    int rc = stat(path.str().c_str(), &result);
    return rc == 0 ? result.st_size : -1;
}

void create_file(native_string &path)
{
    std::ofstream o(path.str().c_str());
    o.close();
}

long delete_file(native_string &path)
{
    return remove(path.str().c_str());
}

List<native_string> get_file_list(native_string &path) {
    DIR *dir;
    List<native_string> files;
    struct dirent *ent;
    if ((dir = opendir (path.str().c_str())) != NULL) {
        /* print all the files and directories within directory */
        while ((ent = readdir (dir)) != NULL) {
            native_string file;
            for(long i = 0; i < ent->d_namlen; i++) {
                file += ent->d_name[i];
            }
            files.push_back(file);
        }
        closedir (dir);
    } else {
        /* could not open directory */
        return files;
    }
}

long make_dir(native_string &path)
{
    return _mkdir(path.str().c_str());
}

long delete_dir(native_string &path)
{
    return _rmdir(path.str().c_str());
}

long rename_file(native_string &path, native_string &newName)
{
    return rename(path.str().c_str(), newName.str().c_str());
}
