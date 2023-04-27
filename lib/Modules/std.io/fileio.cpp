//
// Created by BraxtonN on 5/25/2018.
//

#include "../../../stdimports.h"
#include "../../util/File.h"
#include "../../runtime/memory/garbage_collector.h"

#ifdef WIN32_
#include  <io.h>
#include <direct.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fstream>
#include <dirent.h>
#include <utime.h>
#include <iomanip>
#include <mutex>

#ifndef WIN32
#include <unistd.h>
#include <sys/statvfs.h>
#define GetCurrentDir getcwd
#else
#define GetCurrentDir _getcwd
#endif

#ifdef WIN32
#define stat _stat
#endif

recursive_mutex fileMutex;
thread_local struct stat result;
uInt get_file_attrs(string& path) {
    guard_mutex(fileMutex);

    if(stat(path.c_str(), &result)==0)
    {
        uInt mode = result.st_mode, attrs=0;

        switch (mode & S_IFMT) {
            case S_IFBLK:  attrs |= FILE_BLOCK_DEVICE;     break;
            case S_IFCHR:  attrs |= FILE_CHARACTER;        break;
            case S_IFDIR:  attrs |= FILE_DIRECTORY;        break;
            case S_IFIFO:  attrs |= FILE_FIFO_PIPE;        break;
            case S_IFREG:  attrs |= FILE_REGULAR;          break;
            default:       attrs |= _FILE_UNKNOWN;         break;
        }

        // exists
        attrs |= FILE_EXISTS;

#ifdef WIN32_
        long attributes = GetFileAttributes(path.c_str());
        if (attributes & FILE_ATTRIBUTE_HIDDEN)
            attrs |= FILE_HIDDEN;
#endif
        return attrs;
    }
    return 0;
}


const int ACCESS_READ    = 0x04;
const int ACCESS_WRITE   = 0x02;
const int ACCESS_EXECUTE = 0x01;
const int ACCESS_OK      = 0x00;
/**
 *
 * @param path
 * @param access_flg
 * @return
 */
int check_access(string& path, int access_flg) {
#ifdef WIN32_
    return _access( path.c_str(), access_flg );
#endif

#ifdef POSIX_
    switch(access_flg) {
        case ACCESS_READ:
            access_flg = R_OK;
            break;
        case ACCESS_WRITE:
            access_flg = W_OK;
            break;
        case ACCESS_EXECUTE:
            access_flg = X_OK;
            break;
        case ACCESS_OK:
            access_flg = F_OK;
            break;
        default:
            return -1;
    }
    
    return access( path.c_str(), access_flg );
#endif
}

Int last_update(string& path, int tm_request) {
    guard_mutex(fileMutex);

    if(stat(path.c_str(), &result)==0)
    {
        switch(tm_request) {
            case 0:
                return result.st_mtime;
            case 1:
                return result.st_ctime;
            case 2:
                return result.st_atime;

        }
    }
    return 0;
}

Int file_size(string &path)
{
    guard_mutex(fileMutex);

    int rc = stat(path.c_str(), &result);
    return rc == 0 ? result.st_size : -1;
}

void current_directory(string &path) {
    char buff[FILENAME_MAX]; //create string buffer to hold path
    char *res = GetCurrentDir( buff, FILENAME_MAX );
    path = (buff);
}

void create_file(string &path)
{
    std::ofstream o(path.c_str());
    o.close();
}

long delete_file(string &path)
{
    return remove(path.c_str());
}

void get_file_list(string &path, std::list<string> &files) {
    guard_mutex(fileMutex);

    DIR *dir;
    struct dirent *ent;
#ifdef WIN32_
    string div = "\\";
#endif
#ifdef POSIX_
    string div = "/";
#endif
    if ((dir = opendir (path.c_str())) != NULL) {
        /* print all the files and directories within directory */
        while ((ent = readdir (dir)) != NULL) {
            if (!ent->d_name || ent->d_name[0] == '.') continue;
            string file;
            if(path.size() > 0 && !(path[path.size()-1] == '\\' || path[path.size()-1] == '/'))
                file = path + div + string(ent->d_name);
            else
                file = path + string(ent->d_name);


            if(stat(file.c_str(), &result) == 0 && S_ISDIR(result.st_mode)) {
                string folder(file + div);
                get_file_list(folder, files);
                continue;
            }

            files.push_back(file);
        }
        closedir (dir);
    } else {
        /* could not open directory */
    }
}

long delete_dir(string &path)
{
#ifdef WIN32_
    return _rmdir(path.c_str());
#endif

#ifdef POSIX_
    return rmdir(path.c_str());
#endif
}

long rename_file(string &path, string &newName)
{
    return rename(path.c_str(), newName.c_str());
}

time_t update_time(string &path, time_t time)
{
    guard_mutex(fileMutex);

    struct utimbuf new_times;

    if(stat(path.c_str(), &result)==0) {
        new_times.actime = result.st_atime; /* keep atime unchanged */
        new_times.modtime = time;    /* set mtime to current time */
        utime(path.c_str(), &new_times);
        return time;
    }
    return -1;
}

#ifdef _WIN32

static const mode_t MS_MODE_MASK = 0x0000ffff;           ///< low word

int __chmod(string &path, mode_t set_mode, bool enable, bool userOnly)
{
    guard_mutex(fileMutex);

    if(stat(path.c_str(), &result)==0) {
        Int mode = result.st_mode;

        if (set_mode & ACCESS_READ) {

            if(enable)
                mode |= _S_IREAD;
            else
                mode ^= _S_IREAD;
        }

        if (set_mode & ACCESS_WRITE) {

            if(enable)
                mode |= _S_IWRITE;
            else
                mode ^= _S_IWRITE;
        }

        int res = _chmod(path.c_str(), mode);

        if (res != 0) {
            res = errno;
        }

        return (res);
    }

    return -1;
}
#else
int __chmod(string &path, mode_t set_mode, bool enable, bool userOnly)
{
    if(stat(path.c_str(), &result)==0) {
        Int mode = result.st_mode;

        if (set_mode & ACCESS_READ) {

            if(enable)
                mode |= S_IRUSR;
            else
                mode &= ~S_IRUSR;
        }

        if (set_mode & ACCESS_WRITE) {

            if(enable)
                mode |= S_IWUSR;
            else {
                mode &= ~S_IWUSR;
                mode &= ~S_IWGRP;
                mode &= ~S_IWOTH;
            }
        }
        
        if (set_mode & ACCESS_EXECUTE) {

            if(enable)
                mode |= S_IXUSR;
            else
                mode &= ~S_IXUSR;
        }

        int result = chmod(path.c_str(), mode);

        if (result != 0) {
            result = errno;
        }

        return (result);
    }

    return -1;
}
#endif

const int SPACE_TOTAL  = 0;
const int SPACE_FREE   = 1;
const int SPACE_USABLE = 2;

#ifdef POSIX_

long GetAvailableSpace(const char* path, int request)
{
  struct statvfs st;
  
  if (statvfs(path, &st) != 0) {
    // error happens, just quits here
    return -1;
  }
  
  switch (request) {
        case SPACE_TOTAL:
            return st.f_bsize * st.f_blocks;
        case SPACE_FREE:
            return st.f_bsize * st.f_bavail;
        case SPACE_USABLE:
            return st.f_bsize * st.f_bavail;
        default:
            return 0;
   }

  return st.f_bsize * st.f_bavail;
}

#endif

Int disk_space(long request) {

#ifdef WIN32_

    Int lpFreeBytesAvailable=0,
            lpTotalNumberOfBytes=0,
            lpTotalNumberOfFreeBytes=0;
            
    GetDiskFreeSpaceEx(NULL, (PULARGE_INTEGER)&lpFreeBytesAvailable,
                       (PULARGE_INTEGER)&lpTotalNumberOfBytes, (PULARGE_INTEGER)&lpTotalNumberOfFreeBytes);
    switch (request) {
        case SPACE_TOTAL:
            return lpTotalNumberOfBytes;
        case SPACE_FREE:
            return lpTotalNumberOfFreeBytes;
        case SPACE_USABLE:
            return lpFreeBytesAvailable;
        default:
            return 0;
    }
#endif

#ifdef POSIX_
    return GetAvailableSpace("/", request);
#endif
}

void read_file(string &path, string &outStr) {
    guard_mutex(fileMutex);
    File::buffer buf;
    File::read_alltext(path.c_str(), buf);

    outStr = buf.to_str();
    buf.end();
}

