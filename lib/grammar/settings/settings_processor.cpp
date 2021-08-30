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

            if((member = jo["output_file"]) != NULL) {
                require_type(member, jtype_string);
                set_output_file(member->getValue()->getStringValue());
            }

            if((member = jo["optimize_level"]) != NULL) {
                if(member->getValue()->getType() == jtype_int) {
                    switch (member->getValue()->getIntValue()) {
                        case 0:
                            set_optimization_level(optimization_level::no_optimization);
                            break;
                        case 1:
                            set_optimization_level(optimization_level::basic_optimization);
                            break;
                        case 2:
                            set_optimization_level(optimization_level::high_performance_optimization);
                            break;
                        default: {
                            stringstream ss;
                            ss << "invalid optimize level: " << member->getValue()->getIntValue()
                               << ", allowed values are [ 0, 1, 2 ]";
                            error(ss.str());
                            break;
                        }
                    }
                } else if(member->getValue()->getType() == jtype_string) {
                    string olevel = member->getValue()->getStringValue();
                    
                    if(olevel == "none") set_optimization_level(optimization_level::no_optimization);
                    else if(olevel == "O1") set_optimization_level(optimization_level::basic_optimization);
                    else if(olevel == "O2") set_optimization_level(optimization_level::high_performance_optimization);
                } else {
                    stringstream ss;
                    ss << "invalid json value type: " << json_value::typeToString(member->getValue()->getType())
                        << " expect values are [ 0, 1, 2 ] or [ none, O1, O2 ]";
                    error(ss.str());
                }
            }

            if((member = jo["release_build"]) != NULL || (member = jo["release"]) != NULL) {
                require_type(member, jtype_bool);
                if(member->getValue()->getBoolValue()) {
                    set_optimization_level(high_performance_optimization);
                    enable_app_debugging(false);
                }
            }

            if((member = jo["cache_files"]) != NULL) {
                require_type(member, jtype_bool);
                options.file_caching = member->getValue()->getBoolValue();
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
                    add_ignored_file(options.project_dir + div + v->getStringValue());
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

                    add_ignored_directory(options.project_dir + div + v->getStringValue());
                }
            }

            if((member = jo["extern_libs"]) != NULL) {
                require_type(member, jtype_array);
                json_array *ja = member->getValue()->getArrayValue();

                for(Int i = 0; i < ja->size(); i++) {
                    json_value *v = ja->get_values().get(i);

                    if(v->getType() != jtype_string) {
                        error("array values in external libraries setting must be of type string");
                    }

                    add_library_path(v->getStringValue());
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

        jv->free();
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