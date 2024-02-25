//
// Created by bnunnally on 8/27/21.
//

#include "settings_processor.h"
#include "../../util/File.h"
#include "../json/json.h"

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

bool is_ignored_file(string fullpath) {
    for(uInt i = 0; i < options.ignored_files.size(); i++) {
        if(fullpath == options.ignored_files.get(i))
            return true;
    }

    return false;
}

void require_type(json_member *member, json_type type) {
    if(member->getType() != type) {
        stringstream ss;
        ss << "settings option: \"" << member->getName() 
            << "\" requires type of " << json_value::typeToString(type);
        error(ss.str());
    }
}

void process_settings() {
    string div;

#ifdef __WIN32
    div = "\\";
#else
    div = "/";
#endif

    json_value *jv = parse_json(
            options.project_dir + div + SETTINGS_FILE
            );

    if(jv != NULL) {
        if(jv->getType() == jtype_object) {
            json_object &jo = *jv->getJsonObject();
            json_member *member = NULL;
            
            if((member = jo["compile_only"]) != NULL) {
                require_type(member, jtype_bool);
                enable_compile_only(member->getValue()->getBoolValue());
            }

            if((member = jo["name"]) != NULL) {
                require_type(member, jtype_string);
                options.project_name = member->getValue()->getStringValue();
            }

            if((member = jo["show_all_errors"]) != NULL) {
                require_type(member, jtype_bool);
                enable_show_all_errors(member->getValue()->getBoolValue());
            }

            if((member = jo["output"]) != NULL) {
                require_type(member, jtype_string);
                set_output_file(member->getValue()->getStringValue());
            }

            if((member = jo["release_build"]) != NULL || (member = jo["release"]) != NULL) {
                require_type(member, jtype_bool);
                if(member->getValue()->getBoolValue()) {
                    enable_code_obfuscation(true);
                    enable_app_debugging(false);
                }
            }

            if((member = jo["debug"]) != NULL) {
                require_type(member, jtype_bool);
                enable_app_debugging(member->getValue()->getBoolValue());
            }

            if((member = jo["magic_mode"]) != NULL) {
                require_type(member, jtype_bool);
                enable_magic_mode(member->getValue()->getBoolValue());
            }

            if((member = jo["compiler_debug_mode"]) != NULL) {
                require_type(member, jtype_bool);
                enable_debug_mode(member->getValue()->getBoolValue());
            }

            if((member = jo["target"]) != NULL) {
                require_type(member, jtype_string);
                set_target_platform(member->getValue()->getStringValue());
            }

            if((member = jo["warnings"]) != NULL) {
                require_type(member, jtype_bool);
                enable_warnings(member->getValue()->getBoolValue());
            }

            if((member = jo["enable_warnings"]) != NULL
                || (member = jo["disable_warnings"]) != NULL) {
                require_type(member, jtype_array);
                bool enable = (member = jo["enable_warnings"]) != NULL;
                json_array *ja = member->getValue()->getArrayValue();

                for(Int i = 0; i < ja->size(); i++) {
                    json_value *v = ja->get_values().get(i);

                    if(v->getType() != jtype_string) {
                        error("array values in warnings setting must be of type string");
                    }

                    enable_warning_type(v->getStringValue(), enable);
                }
            }

            if((member = jo["green_mode"]) != NULL) {
                require_type(member, jtype_bool);
                enable_green_mode(member->getValue()->getBoolValue());
            }

            if((member = jo["ignore_files"]) != NULL) {
                require_type(member, jtype_array);
                json_array *ja = member->getValue()->getArrayValue();

                for(Int i = 0; i < ja->size(); i++) {
                    json_value *v = ja->get_values().get(i);

                    if(v->getType() != jtype_string) {
                        error("array values in file ignore setting must be of type string");
                    }

                    // try relative path first then assume its absolute
                    string actualPath = options.project_dir + div + v->getStringValue();
                    if(File::exists(actualPath.c_str()))
                        add_ignored_file(actualPath);
                    else add_ignored_file(v->getStringValue());
                }
            }

            if((member = jo["ignore_folders"]) != NULL) {
                require_type(member, jtype_array);
                json_array *ja = member->getValue()->getArrayValue();

                for(Int i = 0; i < ja->size(); i++) {
                    json_value *v = ja->get_values().get(i);

                    if(v->getType() != jtype_string) {
                        error("array values in folder ignore setting must be of type string");
                    }

                    // try relative path first then assume its absolute
                    string actualPath = options.project_dir + div + v->getStringValue();
                    if(File::exists(actualPath.c_str()))
                        add_ignored_directory(actualPath);
                    else add_ignored_directory(v->getStringValue());
                }
            }

            if((member = jo["extern_lib"]) != NULL) {
                require_type(member, jtype_array);
                json_array *ja = member->getValue()->getArrayValue();

                for(Int i = 0; i < ja->size(); i++) {
                    json_value *v = ja->get_values().get(i);

                    if(v->getType() != jtype_string) {
                        error("array values in external libraries setting must be of type string");
                    }

                    // try relative path first then assume its absolute
                    string actualPath = options.project_dir + div + v->getStringValue();
                    if(File::exists(actualPath.c_str()))
                        add_library_path(actualPath);
                    else add_library_path(v->getStringValue());
                }
            }

            if((member = jo["warnings_as_errors"]) != NULL) {
                require_type(member, jtype_bool);
                if(member->getValue()->getBoolValue()) {
                    enable_warnings_as_errors();
                }
            }

            if((member = jo["error_limit"]) != NULL) {
                if(member->getType() == jtype_int) {
                    stringstream ss;
                    ss << member->getValue()->getIntValue();
                    set_error_limit(ss.str());
                } else if(member->getType() == jtype_string) {
                    set_error_limit(member->getValue()->getStringValue());
                } else
                    require_type(member, jtype_int);
            }

            if((member = jo["target_api"]) != NULL) {
                require_type(member, jtype_string);
                process_library_files(member->getValue()->getStringValue());
            } else
                process_library_files(PROG_VERS);
        } else {
            error("format is incorrect, initial type must be of type object");
        }
    }

    delete jv;
}

void remove_ignored_files() {
    for(Int i = 0; i < options.source_files.size(); i++) {
        if(in_ignored_folder(options.source_files.get(i)) || is_ignored_file(options.source_files.get(i))) {
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

           if((i + 1) < supportedLibVersions) {
              err_msg += ",";
           } else {
              err_msg += " ";
           }
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