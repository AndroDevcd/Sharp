//
// Created by BraxtonN on 1/22/2018.
//

#include "File.h"
#include<stdio.h>
#include <fstream>

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

int FILE_EXISTS       = 0x01;
int FILE_REGULAR      = 0x02;
int FILE_DIRECTORY    = 0x04;
int FILE_BLOCK_DEVICE = 0x08;
int FILE_CHARACTER    = 0x10;
int FILE_FIFO_PIPE    = 0x20;
int _FILE_UNKNOWN     = 0x40;
int FILE_HIDDEN       = 0x80;
struct stat result;
bool File::exists(const char *file)
{
    string f = file;
    return (File::getFileAttrs(f) & FILE_EXISTS);
}

bool is_whitespace(char c)
{
    return ((c ==' ') || (c =='\n') ||
            (c =='\r') || (c =='\t') ||
            (c =='\b') || (c =='\v') ||
            (c =='\f')) ;
}

bool File::empty(const char *file)
{
    buffer data;
    read_alltext(file, data);
    for(long i = 0; i < data.size(); i++)
    {
        if(!is_whitespace(data.at(i)))
            return false;
    }

    return true;
}

int File::write(const char *f, buffer& data)
{
    try {
        FILE* fp=NULL;

        remove( f );
        fp = fopen(f,"wb+");
        if(fp == nullptr)
            return 1;  // could not open file

        if(data.size() > 0) {
            unsigned int p=0;

            do {
                fputc( data.at(p++), fp );
            }while(p<data.size());
        }

        fclose(fp);
        return 0;
    }
    catch(std::bad_alloc& ba){
        return -1;
    }
}

int64_t file_size(FILE *fp)
{
    int64_t len, cur;
    cur = ftell( fp );            /* remember where we are */
    fseek( fp, 0L, SEEK_END );    /* move to the end */
    len = ftell( fp );            /* get position there */
    fseek( fp, cur, SEEK_SET );   /* return back to original position */

    return ( len );
}

void File::read_alltext(const char *f, buffer& _out)
{
    if(!exists(f))
        return;

    try {

        FILE* fp=NULL;
        int64_t len;

        fp = fopen(f,"rb");
        if(fp == nullptr)
            return;  // could not open file


        len = file_size(fp);
        if(len == -1) 1;
        char c;

        do {
            c = getc(fp);
            if(len > 0) {
                _out << c;
            } else {
                break;
            }
        }while(len--);
        fclose(fp);
    }
    catch(std::bad_alloc& ba){
        return;
    }
    catch(std::exception& e){
        return;
    }
}

void File::currentDirectory(string &dir) {
    char buff[FILENAME_MAX]; //create string buffer to hold path
    char *res = GetCurrentDir( buff, FILENAME_MAX );
    dir = (buff);
}

void File::list(string &path, std::list<string> &files) {

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
                list(folder, files);
                continue;
            }

            files.push_back(file);
        }
        closedir (dir);
    } else {
        /* could not open directory */
    }
}

string File::name(string& path) {
    char div;
#ifdef WIN32_
    div = '\\';
#else
    div = '/';
#endif

    if(path == "" || path.size()==0) {
        return "";
    }

    for(uInt i = path.size()-1; i > 0; i--) {
        if(path[i] == div) {
            return path.substr(i + 1, path.size());
        }
    }

    return path;
}

long File::makeDir(string &path)
{
#ifdef WIN32_
    return _mkdir(path.c_str());
#endif

#ifdef POSIX_
    return mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
}


void File::resolvePath(string& path, string &fullPath) {
#ifdef WIN32_
    char full_path[MAX_PATH];
    GetFullPathName(path.c_str(), MAX_PATH, full_path, NULL);

    for(int i = 0; i < MAX_PATH; i++) {
        if(full_path[i] != '\000')
            fullPath += full_path[i];
        else
            break;
    }

#endif

#ifdef POSIX_
    char full_path[PATH_MAX];
    if(realpath(path.c_str(), full_path) != 0) {
        for(int i = 0; i < PATH_MAX; i++) {
            if(full_path[i] != '\000')
                fullPath += full_path[i];
            else
                break;
        }
    }

#endif
}

bool File::endswith(string ext, string file) {
    size_t ext_t = ext.length();
    stringstream extension;
    for(size_t i = file.length() - ext_t; i < file.length(); i++){
        extension << file.at(i);
    }

    return extension.str() == ext;
}

void File::buffer::_push_back(char _C) {
    if(_Data == NULL)
        begin();

    if(actualSize >= rawSize) {
        rawSize+=STREAM_CHUNK;
        void* tptr=realloc(_Data, rawSize * sizeof(char));
        if(tptr == NULL) {
            throw std::bad_alloc();
        }
        _Data=(char* )tptr;
    }

    _Data[actualSize++]=_C;
}

char File::buffer::at(stream_t _X) {
    if(_X >= actualSize || _X < 0) {
        stringstream _s;
        _s << "buffer::at() _X: " << _X << " >= size: " << actualSize;
        throw std::out_of_range(_s.str());
    }
    return _Data[_X];
}

string File::buffer::to_str() {
    stringstream ss;
    for(uInt i=0; i < size(); i++) {
        ss<<_Data[i];
    }
    return ss.str();
}

uInt File::getFileAttrs(string& path) {
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

int File::write(const char *f, string data)
{
    try {
        FILE* fp=NULL;

        remove( f );
        fp = fopen(f,"wb+");
        if(fp == nullptr)
            return 1;  // could not open file

        unsigned int p=0;
        do {
            fputc( data.at(p++), fp );
        }while(p<data.size());

        fclose(fp);
        return 0;
    }
    catch(std::bad_alloc& ba){
        return -1;
    }
}
