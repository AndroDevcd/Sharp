//
// Created by bknun on 7/15/2023.
//

#include "native_generator.h"
#include "../../../../../stdimports.h"
#include "generator.h"
#include "../../../../util/File.h"
#include "../../../../core/access_flag.h"
#include "../../../settings/settings_processor.h"
#include "code/code_info.h"

#ifdef WIN32_
#include  <io.h>
#include <direct.h>
#else
#include <sys/statvfs.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif


bool is_cpp_keyword(string &word) {
    return word == "alignas" ||
           word == "alignof" ||
           word == "and" ||
           word == "and" ||
           word == "and_eq" ||
           word == "asm" ||
           word == "atomic_cancel" ||
           word == "atomic_commit" ||
           word == "atomic_noexcept" ||
           word == "auto" ||
           word == "bitand" ||
           word == "bitor" ||
           word == "bool" ||
           word == "break" ||
           word == "case" ||
           word == "catch" ||
           word == "char" ||
           word == "char16_t" ||
           word == "char32_t" ||
           word == "class" ||
           word == "compl" ||
           word == "concept" ||
           word == "const" ||
           word == "constexpr" ||
           word == "const_cast" ||
           word == "continue" ||
           word == "co_await " ||
           word == "co_return" ||
           word == "co_yield " ||
           word == "decltype" ||
           word == "default" ||
           word == "delete" ||
           word == "do" ||
           word == "double" ||
           word == "dynamic_cast" ||
           word == "else" ||
           word == "enum" ||
           word == "explicit" ||
           word == "export" ||
           word == "extern" ||
           word == "false" ||
           word == "float" ||
           word == "for" ||
           word == "friend" ||
           word == "goto" ||
           word == "if" ||
           word == "import" ||
           word == "inline" ||
           word == "int" ||
           word == "long" ||
           word == "module" ||
           word == "mutable" ||
           word == "namespace" ||
           word == "new" ||
           word == "noexcept" ||
           word == "not" ||
           word == "not_eq" ||
           word == "nullptr" ||
           word == "operator" ||
           word == "or" ||
           word == "or_eq" ||
           word == "private" ||
           word == "protected" ||
           word == "public" ||
           word == "register" ||
           word == "reinterpret_cast" ||
           word == "requires" ||
           word == "return" ||
           word == "short" ||
           word == "signed" ||
           word == "sizeof" ||
           word == "static" ||
           word == "static_assert" ||
           word == "static_cast" ||
           word == "struct" ||
           word == "switch" ||
           word == "synchronized" ||
           word == "template" ||
           word == "this" ||
           word == "thread_local" ||
           word == "throw" ||
           word == "true" ||
           word == "try" ||
           word == "typedef" ||
           word == "typeid" ||
           word == "typename" ||
           word == "union" ||
           word == "unsigned" ||
           word == "using" ||
           word == "virtual" ||
           word == "void" ||
           word == "volatile" ||
           word == "wchar_t" ||
           word == "while" ||
           word == "xor" ||
           word == "xor_eq";
}

string module_to_cpp_name(string &name) {
    bool hashFound = false;
    stringstream ss;
    for(Int i = 0; i < name.size(); i++) {
        if(name[i] == '.' )
            ss << '_';
        else ss << name[i];
    }
    return ss.str();
}

string class_to_cpp_name(string &name) {
    bool hashFound = false;
    stringstream ss;
    for(Int i = 0; i < name.size(); i++) {
        if(hashFound) {
            if(name[i] == '.' || name[i] == '#')
                ss << '_';
            else if(name[i] == '<' || name[i] == '>')
                ss << '$';
            else if(name[i] == ',')
                ss << "_0_";
            else if(name[i] == ' ') {}
            else ss << name[i];
        }

        if(name[i] == '#') {
            hashFound = true;
        }
    }
    return ss.str();
}

string func_to_cpp_name(string &name) {
    bool hashFound = false;
    stringstream ss;
    for(Int i = 0; i < name.size(); i++) {
        if(name[i] == '.' || name[i] == '#')
            ss << '_';
        else if(name[i] == '<' || name[i] == '>')
            ss << '$';
        else if(name[i] == ',')
            ss << "_0_";
        else if(name[i] == '@')
            ss << "_";
        else if(name[i] == ' ') {}
        else ss << name[i];
    }
    return ss.str();
}

Int get_duplicate_count(sharp_function *func, List<sharp_function*> &methods, Int index) {
    Int duplicates = 0;
    for(Int i = 0; i < index; i++) {
        if(func->name == methods.get(i)->name) {
            duplicates++;
        }
    }

    return duplicates;
}

string type_to_cpp_type(data_type type, bool isArray) {
    switch(type) {
        case type_int8:
            if(isArray) return "SharpObject";
            return "int8_t";
        case type_int16:
            if(isArray) return "SharpObject";
            return "int16_t";
        case type_int32:
            if(isArray) return "SharpObject";
            return "int32_t";
        case type_int64:
            if(isArray) return "SharpObject";
            return "int64_t";
        case type_uint8:
            if(isArray) return "SharpObject";
            return "uint8_t";
        case type_uint16:
            if(isArray) return "SharpObject";
            return "uint16_t";
        case type_uint32:
            if(isArray) return "SharpObject";
            return "uint32_t";
        case type_uint64:
            if(isArray) return "SharpObject";
            return "uint64_t";
        case type_function_ptr:
        case type_var:
            if(isArray) return "SharpObject";
            return "var";
        case type_nil:
            return "void";
        default:
            return "SharpObject";
    }
}

string operator_to_string(string op) {
    if(op == "+=")
        return "$plus_equal";
    else if(op == "-=")
        return "$minus_equal";
    else if(op == "*=")
        return "$mult_equal";
    else if(op == "/=")
        return "$div_equal";
    else if(op == "&=")
        return "$and_equal";
    else if(op == "|=")
        return "$or_equal";
    else if(op == "^=")
        return "$xor_equal";
    else if(op == "%=")
        return "$mod_equal";
    else if(op == "=")
        return "$equals";
    else if(op == "++")
        return "$plus_plus";
    else if(op == "--")
        return "$minus_minus";
    else if(op == "*")
        return "$mult";
    else if(op == "/")
        return "$div";
    else if(op == "%")
        return "$mod";
    else if(op == "-")
        return "$minus";
    else if(op == "+")
        return "$plus";
    else if(op == "==")
        return "$equals_equals";
    else if(op == ">>")
        return "$right_shift";
    else if(op == "<<")
        return "$left_shift";
    else if(op == "<")
        return "$less_than";
    else if(op == ">")
        return "$greater_than";
    else if(op == "<=")
        return "$less_or_equals";
    else if(op == ">=")
        return "$great_or_equals";
    else if(op == "!=")
        return "$not_equals";
    else if(op == "!")
        return "$not";
    else if(op == "[")
        return "$array_at";
    else if(op == "**")
        return "$pow";
    else if(op == "&" )
        return "$and";
    else if(op == "&&" )
        return "$and_and";
    else if(op == "|")
        return "$or";
    else if(op == "||")
        return "$or_or";
    else if(op == "^")
        return "$xor";
    else return op; // error should not happen
}

void append_class_function_header_mappings(sharp_class *sc, stringstream &ss) {
    List<sharp_function*> &classMethods = sc->functions;

    if(classMethods.empty())
        return;

    if(check_flag(sc->flags, flag_global)) {
        ss << "scope_begin(";
        ss << (is_cpp_keyword(sc->module->name) ? "_" : "") << module_to_cpp_name(sc->module->name) << ") " << endl;
    } else {
        ss << "scope_begin(" << (is_cpp_keyword(sc->module->name) ? "_" : "") << module_to_cpp_name(sc->module->name) << ", ";
        string cppName = class_to_cpp_name(sc->fullName);

        ss << (is_cpp_keyword(cppName) ? "_" : "") << cppName << ") " << endl;
    }

    for(Int k = 0; k < classMethods.size(); k++) {
        sharp_function *func = classMethods.get(k);

        if(func->used) {
            Int duplicateCount = get_duplicate_count(func, classMethods, k);
            ss << endl << "\t" << "//" << (check_flag(func->flags, flag_native) ? " native" : "") << " def " << function_to_str(func) << " { .. }";
            ss << endl << "\t" << type_to_cpp_type(func->returnType.type, func->returnType.isArray)
               << " ";

            if (startsWith(func->name, "operator")) {
                ss << "op_" << operator_to_string(func->name.substr(8, func->name.size()));
            } else {
                string cppName = func_to_cpp_name(func->name);
                ss << (is_cpp_keyword(cppName) ? "_" : "") << cppName;
            }

            if (duplicateCount)
                ss << (duplicateCount + 1);
            ss << "(";

            if (!check_flag(func->flags, flag_static)) {
                ss << "SharpObject $instance";

                if (!func->parameters.empty())
                    ss << ", ";
            }

            for (Int l = 0; l < func->parameters.size(); l++) {
                sharp_field *param = func->parameters.get(l);
                ss << type_to_cpp_type(param->type.type, param->type.isArray);
                ss << " " << (is_cpp_keyword(param->name) ? "_" : "") << param->name;

                if ((l + 1) < func->parameters.size()) {
                    ss << ", ";
                }
            }

            ss << ");";
        }
    }

    ss << endl << "scope_end()" << endl << endl;
}

void generate_function_mapping_header() {
    stringstream ss;
    ss << "#ifndef SHARP_FUNCTION_MAPPING_H" << endl;
    ss << "#define SHARP_FUNCTION_MAPPING_H" << endl << endl;
    ss << "#include \"snb_api.h\"" << endl << endl;
    ss << "using namespace snb_api;" << endl << endl;

    for(Int j = 0; j < compressedCompilationClasses.size(); j++) {
        append_class_function_header_mappings(compressedCompilationClasses.get(j), ss);
    }

    ss << "#endif //SHARP_FUNCTION_MAPPING_H" << endl;

#ifdef _WIN32
    string mappingFile = options.native_code_dir + "\\function_mapping.h";
    _mkdir(options.native_code_dir.c_str());
#else
    string mappingFile = options.native_code_dir + "/function_mapping.h";
    mkdir(options.native_code_dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif

    File::write(mappingFile.c_str(), ss.str());
}

bool has_native_methods() {
    for(Int i = 0; i < compressedCompilationFunctions.size(); i++) {
        if(check_flag(compressedCompilationFunctions[i]->flags, flag_native)) {
            return true;
        }
    }

    return false;
}

void create_proc_addr_function(sharp_class* sc, stringstream &ss, bool &firstFunc) {
    List<sharp_function *> &classMethods = sc->functions;

    for(Int k = 0; k < classMethods.size(); k++) {
        sharp_function *func = classMethods.get(k);

        if(func->used && check_flag(func->flags, flag_native)) {
            if(firstFunc) {
                ss << "\tif(";
                firstFunc = false;
            }
            else
                ss << "\telse if(";

            ss << "name == \"" << func->fullName << "\") {  return " << func->ci->address << ";  }" << endl;
        }
    }
}

void create_proc_addr_function(stringstream &ss) {
    ss << "EXPORTED uint32_t snb_link_proc(const char* funcName) {" << endl
       << "\tstring name = funcName;" << endl << endl;

    bool firstFunc = true;

    for(Int j = 0; j < compressedCompilationClasses.size(); j++) {
        create_proc_addr_function(compressedCompilationClasses.get(j), ss, firstFunc);
    }

    ss << "\telse return -1;" << endl;
    ss << "}" << endl << endl;
}


void append_call_functions(sharp_class* klass, stringstream& ss) {
    List<sharp_function *> &classMethods = klass->functions;

    for(Int k = 0; k < classMethods.size(); k++) {
        sharp_function *func = classMethods.get(k);

        if(!check_flag(func->flags, flag_native)) continue;
        Int duplicateCount = get_duplicate_count(func, classMethods, k);

        ss << "void call_" << module_to_cpp_name(func->owner->module->name)
            << "$" << class_to_cpp_name(func->owner->fullName) << "_" << func->name;

        if(duplicateCount)
            ss << (duplicateCount + 1);
        ss << "() {";

        if(check_flag(func->owner->flags, flag_global)) {
            ss << endl << "\timport(" << module_to_cpp_name(func->owner->module->name) << "); " << endl;
        } else {
            ss << endl << "\timport(" << module_to_cpp_name(func->owner->module->name) << ", ";
            ss << class_to_cpp_name(func->owner->fullName) << "); " << endl;
        }

        ss << endl << "\tinternal::call(" << func->ci->address << ");";
        Int localAddr = 0;
        if(!check_flag(func->flags, flag_static)) {
            localAddr++;
            ss << endl << "\tauto $instance = internal::get_local_object_at(0);";
        }

        for(Int l = 0; l < func->parameters.size(); l++) {
            sharp_field *param = func->parameters.get(l);

            ss << endl << "\t" << type_to_cpp_type(param->type.type, param->type.isArray) << " " << param->name;
            if(type_to_cpp_type(param->type.type, param->type.isArray) == "SharpObject")
                ss << " = internal::get_local_object_at(" << localAddr << ");";
            else ss << " = *internal::get_local_number_at(" << localAddr << ");";


            localAddr++;
        }

        ss << endl << "\t";

        if(func->returnType.type != type_nil) {
            ss << "auto $result = ";
        }

        ss << (is_cpp_keyword(func->name) ? "_" : "") << func->name;
        ss << "(";

        for(Int l = 0; l < func->parameters.size(); l++) {
            sharp_field *param = func->parameters.get(l);
            ss << (is_cpp_keyword(param->name) ? "_" : "") << param->name;

            if((l + 1) < func->parameters.size()) {
                ss << ", ";
            }
        }

        ss << ");";
        if(func->returnType.type != type_nil) {
            ss << endl << "\t" << "internal::push_object($result);";
        }

        ss << endl << "}" << endl << endl;
    }
}

void create_call_func(stringstream &ss) {
    for(Int j = 0; j < compressedCompilationClasses.size(); j++) {
        append_call_functions(compressedCompilationClasses.get(j), ss);
    }
}

void append_main_functions(sharp_class* klass, stringstream& ss) {
    List<sharp_function *> &classMethods = klass->functions;

    for(Int k = 0; k < classMethods.size(); k++) {
        sharp_function *func = classMethods.get(k);

        if(!check_flag(func->flags, flag_native)) continue;
        Int duplicateCount = get_duplicate_count(func, classMethods, k);

        ss << "\t\t\t";
        ss << "case " << func->ci->address << ": " << endl;
        ss << "\t\t\t\tcall_" << module_to_cpp_name(func->owner->module->name) << "$"<< class_to_cpp_name(func->owner->fullName) << "_" << func->name;

        if(duplicateCount)
            ss << (duplicateCount + 1);
        ss << "();" << endl;
        ss << "\t\t\t\tbreak;" << endl;
    }
}

void create_main_func(stringstream &ss) {
    ss << "EXPORTED void snb_main(long procAddr) {" << endl;
    ss << "\tif(!internal::validate_app_id(" << application_id << ")) " << endl;
    ss << "\t\treturn;" << endl << endl;
    ss << "\ttry {" << endl;
    ss << "\t\tswitch(procAddr) {" << endl;

    for(Int j = 0; j < compressedCompilationClasses.size(); j++) {
        append_main_functions(compressedCompilationClasses.get(j), ss);
    }

    ss << "\t\t}" << endl;
    ss << "\t} catch(VMException &e) {" << endl;
    ss << "\t\tusing namespace snb_api::internal;" << endl;
    ss << "\t\tif(!is_exception_thrown()) {" << endl;
    ss << "\t\t\tif(e.exceptionClass)" << endl;
    ss << "\t\t\t\tprepare_exception(e.exceptionClass);" << endl;
    ss << "\t\t}" << endl;
    ss << "\t}" << endl;
    ss << "}" << endl;
}

void generate_native_mapping() {
    stringstream ss;
    ss << "#include \"function_mapping.h\"" << endl
       << endl
       << "using namespace std;" << endl
       << "using namespace snb_api;" << endl << endl;

    ss << "#ifdef __cplusplus" << endl
       << "extern \"C\" {" << endl
       << "#endif" << endl << endl;

    create_proc_addr_function(ss);
    create_call_func(ss);
    create_main_func(ss);

    ss << endl
       << "#ifdef __cplusplus" << endl
       << "}" << endl
       << "#endif" << endl;

#ifdef _WIN32
    string mappingFile = options.native_code_dir + "\\native_mapping.cpp";
    _mkdir(options.native_code_dir.c_str());
#else
    string mappingFile = options.native_code_dir + "/native_mapping.cpp";
    mkdir(options.native_code_dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif

    File::write(mappingFile.c_str(), ss.str());
}

void append_code_mapping_source_file(sharp_class* klass, stringstream& ss) {
    List<sharp_function *> &classMethods = klass->functions;

    if(classMethods.empty())
        return;

    if(check_flag(klass->flags, flag_global)) {
        ss << "scope_begin(";
        ss << (is_cpp_keyword(klass->module->name) ? "_" : "") << module_to_cpp_name(klass->module->name) << ") " << endl;
    } else {
        ss << "scope_begin(" << (is_cpp_keyword(klass->module->name) ? "_" : "") << module_to_cpp_name(klass->module->name) << ", ";
        string cppName = class_to_cpp_name(klass->fullName);

        ss << (is_cpp_keyword(cppName) ? "_" : "") << cppName << ") " << endl;
    }

    for(Int k = 0; k < classMethods.size(); k++) {
        sharp_function *func = classMethods.get(k);

        if(check_flag(func->flags, flag_native) || !func->used) continue;
        Int duplicateCount = get_duplicate_count(func, classMethods, k);
        ss << endl << "\t" << type_to_cpp_type(func->returnType.type, func->returnType.isArray)
           << " ";

        if(startsWith(func->name, "operator")) {
            ss << "op_" << operator_to_string(func->name.substr(8, func->name.size()));
        } else {
            ss << (is_cpp_keyword(func->name) ? "_" : "") << func_to_cpp_name(func->name);
        }

        if(duplicateCount)
            ss << (duplicateCount + 1);
        ss << "(";

        if(!check_flag(func->flags, flag_static)) {
            ss << "SharpObject $instance";

            if(!func->parameters.empty())
                ss << ", ";
        }

        for(Int l = 0; l < func->parameters.size(); l++) {
            sharp_field *param = func->parameters.get(l);
            ss << type_to_cpp_type(param->type.type, param->type.isArray);
            ss << " " << (is_cpp_keyword(param->name) ? "_" : "") << param->name;

            if((l + 1) < func->parameters.size()) {
                ss << ", ";
            }
        }

        ss << ") {";

        if(!check_flag(func->flags, flag_static)) {
            ss << endl << "\t\tinternal::push_object($instance);";
        }

        for(Int l = 0; l < func->parameters.size(); l++) {
            sharp_field *param = func->parameters.get(l);

            ss << endl << "\t\t";

            if(type_to_cpp_type(param->type.type, param->type.isArray) == "SharpObject") {
                ss << "internal::push_object(" << (is_cpp_keyword(param->name) ? "_" : "") << param->name << ");";
            } else {
                ss << "internal::push_number(" << (is_cpp_keyword(param->name) ? "_" : "") << param->name << ");";
            }
        }

        ss << endl << "\t\tcall(" << func->ci->address << ");" << endl << endl;
        ss << "\t\tcheck_for_err();" << endl;

        if(func->returnType.type != type_nil) {
            ss << "\t\tauto $result = ";

            if(type_to_cpp_type(func->returnType.type, func->returnType.isArray) == "SharpObject") {
                ss << "pop_object();" << endl;
            } else {
                ss << "pop_number();" << endl;
            }
            ss << "\t\treturn $result;" << endl;
        }
        ss << "\t}" << endl;
    }

    ss << endl << "scope_end()" << endl << endl;
}

void generation_function_mapping() {
    stringstream ss;
    ss << "#include \"function_mapping.h\"" << endl
       << endl
       << "using namespace snb_api::internal;" << endl << endl;

    for(Int j = 0; j < compressedCompilationClasses.size(); j++) {
        append_code_mapping_source_file(compressedCompilationClasses.get(j), ss);
    }

#ifdef _WIN32
    string mappingFile = options.native_code_dir + "\\function_mapping.cpp";
    _mkdir(options.native_code_dir.c_str());
#else
    string mappingFile = options.native_code_dir + "/function_mapping.cpp";
    mkdir(options.native_code_dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif

    File::write(mappingFile.c_str(), ss.str());
}

void generate_native_code() {
    if(has_native_methods())
    {
        generate_function_mapping_header();
        generate_native_mapping();
        generation_function_mapping();
    }
}
