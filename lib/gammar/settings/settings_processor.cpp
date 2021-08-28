//
// Created by bnunnally on 8/27/21.
//

#include "settings_processor.h"
#include "../../util/File.h"

bool startsWith(string &str, string prefix)
{
    if(prefix.size() > str.size())
        return false;

    long index = 0;
    for(char ch : prefix) {
        if(ch != str.at(index++))
            return false;
    }
    return true;
}

bool in_ignored_folder(string fullpath) {
    for(uInt i = 0; i < options.ignored_directories.size(); i++) {
        if(startsWith(fullpath, options.ignored_directories.get(i)))
            return true;
    }

    return false;
}

void is_ignored_file(string fullpath) {
    for(uInt i = 0; i < options.ignored_files.size(); i++) {
        if(fullpath == path)
            return true;
    }

    return false;
}

void remove_ignored_files() {
    for(i := 0; i < options.source_files.size(); i++) {
        if(in_ignored_folder(files[i]) || is_ignored_file(files[i])) {
            options.source_files.removeAt(i); i--;
        }
    }
}

void validate_and_add_source_files(string &libPath) {
    list<string> files;
    File::list(libPath, files);

    for(const string &file : files) {
        if(File::endswith(".sharp", file)){
            options.source_files.addif(file);
        }
    }
}

void process_library_files(string versionNumber) {
    bool is_supported = false;
    for(uInt i = 0; i < supportedLibVersions; i++) {
        if(versionNumber == supported_versions[i]) {
            is_supported = true;
            break;
        }
    }

    if(!is_supported) {
        string err_msg = "Library api version: $version_num is not supported!";
        err_msg += " Supported versions are: {";

        for(uInt i = 0; i < supportedLibVersions; i++) {
           err_msg += " " + supported_versions[i];

           if((pos + 1) < size) {
              err_msg += ",";
           } else {
              err_msg += " ";
           }

           pos++;
        }

        err_msg += "}";

        error(err_msg);
    }

    string path;

#ifdef _WIN32
    path = "C:\\Program Files\\Sharp\\include\\" + versionNumber;
#else
    path = "/usr/include/sharp/" + versionNumber;
#endif

    validate_and_add_source_files(path);

    for(uInt i = 0; i < options.libraries.size(); i++) {
        validate_and_add_source_files(options.libraries.get(i));
    }
}