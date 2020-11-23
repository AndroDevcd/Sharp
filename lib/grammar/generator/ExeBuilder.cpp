//
// Created by BNunnally on 5/15/2020.
//

#include <iomanip>
#include "../backend/Compiler.h"
#include "ExeBuilder.h"
#include "../../runtime/Exe.h"
#include "../../util/zip/zlib.h"
#include "../main.h"
#include "../optimizer/Optimizer.h"

#ifdef WIN32_
#include  <io.h>
#include <direct.h>
#else
#include <sys/statvfs.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

string classToCPPName(string &name) {
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
            else ss << name[i];
        }

        if(name[i] == '#') {
            hashFound = true;
        }
    }
    return ss.str();
}

string moduleToCPPName(string &name) {
    bool hashFound = false;
    stringstream ss;
    for(Int i = 0; i < name.size(); i++) {
        if(name[i] == '.' )
            ss << '_';
        else ss << name[i];
    }
    return ss.str();
}

bool sortMethods(Method *m1, Method *m2) {
    return m1->address > m2->address;
}

bool sortClasses(ClassObject *c1, ClassObject *c2) {
    return c1->address > c2->address;
}

bool sortFields(Field *f1, Field *f2) {
    return f1->address > f2->address;
}

bool locateMethod(Method **m1, void *m2) {
    return Compiler::simpleParameterMatch((*m1)->params, ((Method*)m2)->params) && (*m1)->utype->equals(((Method*)m2)->utype);
}

string putInt32(int32_t i32)
{
    string str;

    str+=(uint8_t)GET_i32w(i32);
    str+=(uint8_t)GET_i32x(i32);
    str+=(uint8_t)GET_i32y(i32);
    str+=(uint8_t)GET_i32z(i32);
    return str;
}

void ExeBuilder::deleteTempFiles() {
    for(Int i = 1; i < allClasses.size(); i++) {
        stringstream fileName;

        fileName << c_options.nativeCodeDir
                 #ifdef WIN32_
                 << "\\"
                 #endif
                 #ifdef POSIX_
                 << "/"
                 #endif
                 << "_$Tmp_Sharp_class_" << i << ".cpp";

        remove(fileName.str().c_str());
    }

    if(!compiler->nativeCodeFound) {
#ifdef WIN32_
        _rmdir(c_options.nativeCodeDir.c_str());
#endif

#ifdef POSIX_
        rmdir(c_options.nativeCodeDir.c_str());
#endif
    }
}

void ExeBuilder::createClassFunctions() {
//    const char *longString = R""""(
//            This is
//            a very ())"""
//            long
//            string
//            )"""";

    for(Int i = 0; i < allClasses.size(); i++) {
        stringstream fileName, fileData;
        ClassObject *klass = allClasses.get(i);

        fileName << c_options.nativeCodeDir
#ifdef WIN32_
        << "\\"
#endif
#ifdef POSIX_
        << "/"
#endif
        << "_$Tmp_Sharp_class_" << i << ".cpp";

        fileData << "#include <runtime/Thread.h>" << endl;
        fileData << "#include <runtime/fiber.h>" << endl;
        fileData << "#include <runtime/Opcode.h>" << endl << endl;

        for(Int j = 0; j < klass->getFunctionCount(); j++) {
            Method *function = klass->getFunction(j);

            fileData << endl << "void " << classToCPPName(function->fullName)
                << "(Thread *thread) {" << endl;

            fileData << "\tregister fiber *this_fiber = thread->this_fiber;" << endl;

            fileData << endl << "\tstatic void* label_table[] = {";
            for(Int k = 0; k < function->data.code.size(); k++) {
                if(k > 0 && (k % 5 == 0)) fileData << endl << "\t\t\t";
                fileData << " &&INS_" << k;

                if((k + 1) < function->data.code.size())
                    fileData << ",";
            }
            fileData << " };" << endl;

            fileData << endl <<  "\ttry {" << endl;
            CodeHolder &code = function->data.code;
            for(Int k = 0; k < function->data.code.size(); k++) {
                fileData << endl << "\t\t";
                fileData << "INS_" << k << ':' << endl << "\t\t ";
                switch(GET_OP(code.ir32.at(k))) {
                    case Opcode::NOP: {
                        fileData << "inj_op_nop";
                        break;
                    }
                    case Opcode::INT: {
                        fileData << "update_pc(" << k << ")";
                        fileData << endl << "\t\t ";
                        fileData << "inj_op_int(" << GET_Da(code.ir32.at(k)) << ")";
                        break;
                    }
                    case Opcode::MOVI: {
                        fileData << "inj_op_movi(" << GET_Da(code.ir32.at(k)) << ", "
                            << code.ir32.at(k + 1) << ")";
                        k++;
                        break;
                    }
                    case Opcode::RET: {
                        fileData << "inj_op_ret(" << GET_Da(code.ir32.at(k)) << ")";
                        break;
                    }
                    case Opcode::HLT: {
                        fileData << "inj_op_hlt";
                        break;
                    }
                    case Opcode::NEWARRAY: {
                        fileData << "update_pc(" << k << ")";
                        fileData << endl << "\t\t ";
                        fileData << "grow_stack";
                        fileData << endl << "\t\t ";
                        fileData << "STACK_CHECK";
                        fileData << endl << "\t\t ";
                        fileData << "inj_op_newrray(" << GET_Ca(code.ir32.at(k)) << ", " << GET_Cb(code.ir32.at(k)) << ")";
                        break;
                    }
                    case Opcode::CAST: {
                        fileData << "update_pc(" << k << ")";
                        fileData << endl << "\t\t ";
                        fileData << "CHECK_NULL(inj_op_cast(" << GET_Da(code.ir32.at(k)) << "))";
                        break;
                    }
                    case Opcode::VARCAST: {
                        fileData << "update_pc(" << k << ")";
                        fileData << endl << "\t\t ";
                        fileData << "CHECK_NULL2(inj_op_varcast(" << GET_Ca(code.ir32.at(k)) << ", " << GET_Cb(code.ir32.at(k)) << "))";
                        break;
                    }
                    case Opcode::MOV8: {
                        fileData << "inj_op_mov8(" << GET_Ca(code.ir32.at(k))
                            << ", " << GET_Cb(code.ir32.at(k)) << ")";
                        break;
                    }
                    case Opcode::MOV16: {
                        fileData << "inj_op_mov16(" << GET_Ca(code.ir32.at(k))
                                 << ", " << GET_Cb(code.ir32.at(k)) << ")";
                        break;
                    }
                    case Opcode::MOV32: {
                        fileData << "inj_op_mov32(" << GET_Ca(code.ir32.at(k))
                                 << ", " << GET_Cb(code.ir32.at(k)) << ")";
                        break;
                    }
                    case Opcode::MOV64: {
                        fileData << "inj_op_mov64(" << GET_Ca(code.ir32.at(k))
                                 << ", " << GET_Cb(code.ir32.at(k)) << ")";
                        break;
                    }
                    case Opcode::MOVU8: {
                        fileData << "inj_op_movu8(" << GET_Ca(code.ir32.at(k))
                                 << ", " << GET_Cb(code.ir32.at(k)) << ")";
                        break;
                    }
                    case Opcode::MOVU16: {
                        fileData << "inj_op_movu16(" << GET_Ca(code.ir32.at(k))
                                 << ", " << GET_Cb(code.ir32.at(k)) << ")";
                        break;
                    }
                    case Opcode::MOVU32: {
                        fileData << "inj_op_movu32(" << GET_Ca(code.ir32.at(k))
                                 << ", " << GET_Cb(code.ir32.at(k)) << ")";
                        break;
                    }
                    case Opcode::MOVU64: {
                        fileData << "inj_op_movu64(" << GET_Ca(code.ir32.at(k))
                                 << ", " << GET_Cb(code.ir32.at(k)) << ")";
                        break;
                    }
                    case Opcode::RSTORE: {
                        fileData << "update_pc(" << k << ")";
                        fileData << endl << "\t\t ";
                        fileData << "grow_stack";
                        fileData << endl << "\t\t ";
                        fileData << "STACK_CHECK";
                        fileData << endl << "\t\t ";
                        fileData << "inj_op_rstore(" << GET_Da(code.ir32.at(k)) << ")";
                        break;
                    }
                    case Opcode::ADD: {
                        fileData << "inj_op_add(" << GET_Bc(code.ir32.at(k))
                            << ", " << GET_Ba(code.ir32.at(k)) << ", " << GET_Bb(code.ir32.at(k)) << ")";
                        break;
                    }
                    case Opcode::SUB: {
                        fileData << "inj_op_sub(" << GET_Bc(code.ir32.at(k))
                                 << ", " << GET_Ba(code.ir32.at(k)) << ", " << GET_Bb(code.ir32.at(k)) << ")";
                        break;
                    }
                    case Opcode::DIV: {
                        fileData << "inj_op_div(" << GET_Bc(code.ir32.at(k))
                                 << ", " << GET_Ba(code.ir32.at(k)) << ", " << GET_Bb(code.ir32.at(k)) << ")";
                        break;
                    }
                    case Opcode::MOD: {
                        fileData << "inj_op_mod(" << GET_Bc(code.ir32.at(k))
                                 << ", " << GET_Ba(code.ir32.at(k)) << ", " << GET_Bb(code.ir32.at(k)) << ")";
                        break;
                    }
                    case Opcode::IADD: {
                        fileData << "inj_op_iadd(" << GET_Da(code.ir32.at(k))
                                 << ", " << code.ir32.at(k + 1) << ")";
                        k++;
                        break;
                    }
                    case Opcode::ISUB: {
                        fileData << "inj_op_isub(" << GET_Da(code.ir32.at(k))
                                 << ", " << code.ir32.at(k + 1) << ")";
                        k++;
                        break;
                    }
                    case Opcode::IMUL: {
                        fileData << "inj_op_imul(" << GET_Da(code.ir32.at(k))
                                 << ", " << code.ir32.at(k + 1) << ")";
                        k++;
                        break;
                    }
                    case Opcode::IDIV: {
                        fileData << "inj_op_idiv(" << GET_Da(code.ir32.at(k))
                                 << ", " << code.ir32.at(k + 1) << ")";
                        k++;
                        break;
                    }
                    case Opcode::IMOD: {
                        fileData << "inj_op_imod(" << GET_Da(code.ir32.at(k))
                                 << ", " << code.ir32.at(k + 1) << ")";
                        k++;
                        break;
                    }
                    case Opcode::POP: {
                        fileData << "inj_op_pop";
                        break;
                    }
                    case Opcode::INC: {
                        fileData << "inj_op_inc(" << GET_Da(code.ir32.at(k)) << ")";
                        break;
                    }
                    case Opcode::DEC: {
                        fileData << "inj_op_dec(" << GET_Da(code.ir32.at(k)) << ")";
                        break;
                    }
                    case Opcode::MOVR: {
                        fileData << "inj_op_movr(" << GET_Ca(code.ir32.at(k)) << ", "
                            << GET_Cb(code.ir32.at(k)) << ")";
                        break;
                    }
                    case Opcode::BRH: {
                        fileData << "update_pc(" << k << ")";
                        fileData << endl << "\t\t ";
                        fileData << "HAS_SIGNAL";
                        fileData << endl << "\t\t ";
                        fileData << "inj_op_brh(context_switch_check(false))";
                        break;
                    }
                    case Opcode::IFE: {
                        fileData << "update_pc(" << k << ")";
                        fileData << endl << "\t\t ";
                        fileData << "HAS_SIGNAL";
                        fileData << endl << "\t\t ";
                        fileData << "inj_op_ife(context_switch_check(false))";
                        break;
                    }
                    case Opcode::IFNE: {
                        fileData << "update_pc(" << k << ")";
                        fileData << endl << "\t\t ";
                        fileData << "HAS_SIGNAL";
                        fileData << endl << "\t\t ";
                        fileData << "inj_op_ifne(context_switch_check(false))";
                        break;
                    }
                    case Opcode::LT: {
                        fileData << "inj_op_lt(" << GET_Ca(code.ir32.at(k)) << ", "
                                 << GET_Cb(code.ir32.at(k)) << ")";
                        break;
                    }
                    case Opcode::GT: {
                        fileData << "inj_op_gt(" << GET_Ca(code.ir32.at(k)) << ", "
                                 << GET_Cb(code.ir32.at(k)) << ")";
                        break;
                    }
                    case Opcode::LTE: {
                        fileData << "inj_op_lte(" << GET_Ca(code.ir32.at(k)) << ", "
                                 << GET_Cb(code.ir32.at(k)) << ")";
                        break;
                    }
                    case Opcode::GTE: {
                        fileData << "inj_op_gte(" << GET_Ca(code.ir32.at(k)) << ", "
                                 << GET_Cb(code.ir32.at(k)) << ")";
                        break;
                    }
                    case Opcode::MOVL: {
                        fileData << "inj_op_movl(" << GET_Da(code.ir32.at(k)) << ")";
                        break;
                    }
                    case Opcode::POPL: {
                        fileData << "inj_op_popl(" << GET_Da(code.ir32.at(k)) << ")";
                        break;
                    }
                    case Opcode::IPOPL: {
                        fileData << "inj_op_ipopl(" << GET_Da(code.ir32.at(k)) << ")";
                        break;
                    }
                    case Opcode::MOVSL: {
                        fileData << "inj_op_movsl(" << GET_Da(code.ir32.at(k)) << ")";
                        break;
                    }
                    case Opcode::SIZEOF: {
                        fileData << "inj_op_sizeof(" << GET_Da(code.ir32.at(k)) << ")";
                        break;
                    }
                    case Opcode::PUT: {
                        fileData << "inj_op_put(" << GET_Da(code.ir32.at(k)) << ")";
                        break;
                    }
                    case Opcode::PUTC: {
                        fileData << "inj_op_putc(" << GET_Da(code.ir32.at(k)) << ")";
                        break;
                    }
                    case Opcode::GET: {
                        fileData << "inj_op_get(" << GET_Da(code.ir32.at(k)) << ")";
                        break;
                    }
                    case Opcode::CHECKLEN: {
                        fileData << "update_pc(" << k << ")";
                        fileData << endl << "\t\t ";
                        fileData << "HAS_SIGNAL";
                        fileData << endl << "\t\t ";
                        fileData << "inj_op_checklen(" << GET_Da(code.ir32.at(k)) << ")";
                        break;
                    }
                    case Opcode::JMP: {
                        fileData << "inj_op_jmp(" << GET_Da(code.ir32.at(k)) << ", context_switch_check(false))";
                        break;
                    }
                    case Opcode::LOADPC: {
                        fileData << "inj_op_loadpc(" << GET_Da(code.ir32.at(k)) << ", " << k << ")";
                        break;
                    }
                    case Opcode::PUSHOBJ: {
                        fileData << "update_pc(" << k << ")";
                        fileData << endl << "\t\t ";
                        fileData << "grow_stack";
                        fileData << endl << "\t\t ";
                        fileData << "STACK_CHECK";
                        fileData << endl << "\t\t ";
                        fileData << "inj_op_pushobj";
                        break;
                    }
                    case Opcode::DEL: {
                        fileData << "inj_op_del";
                        break;
                    }
                    case Opcode::CALL: {
                        fileData << "update_pc(" << k << ")";
                        fileData << endl << "\t\t ";
                        fileData << "inj_op_call(" <<  GET_Da(code.ir32.at(k)) << ")";
                        fileData << endl << "\t\t ";
                        fileData << "HAS_SIGNAL";
                        break;
                    }
                    case Opcode::CALLD: {
                        fileData << "update_pc(" << k << ")";
                        fileData << endl << "\t\t ";
                        fileData << "inj_op_calld(" <<  GET_Da(code.ir32.at(k)) << ")";
                        fileData << endl << "\t\t ";
                        fileData << "HAS_SIGNAL";
                        break;
                    }
                }
            }

            fileData << endl << "\t}";

    const char *exceptionCatchSection = R""""(
    catch (Exception &e) {
        sendSignal(thread->signal, tsig_except, 1);
    }

    exception_catch:
    if(state == THREAD_KILLED) {
        sendSignal(thread->signal, tsig_except, 1);
        return;
    }

    if(!VirtualMachine::catchException()) {
        returnMethod(thread);
        return;
    }

    goto *label_table[this_fiber->pc];)"""";

            fileData << exceptionCatchSection << endl << "}" << endl;
        }

        if(File::write(fileName.str().c_str(), fileData.str())) {
            cout << progname << ": error: failed to write out to executable " << c_options.out << endl;
            exit(1);
        }
    }
}

void ExeBuilder::buildExe() {
#ifdef WIN32_
    _mkdir(c_options.nativeCodeDir.c_str());
#endif
#ifdef POSIX_
    mkdir(c_options.nativeCodeDir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif

    // C:\Program Files\Sharp\include
    createClassFunctions();
    deleteTempFiles();
//    buildHeader();
//    buildSymbolSection();
//    buildStringSection();
//    buildConstantSection();
//
//    buildDataSection();
//    buildMetaDataSection();
//
//    string data = dataSec.str();
//    if(data.size() >= data_compress_threshold) {
//        dataSec.str("");
//
//        buf << (char)data_compress;
//        stringstream __outbuf__;
//        Zlib zlib;
//
//        Zlib::AUTO_CLEAN=(true);
//        zlib.Compress_Buffer2Buffer(data, __outbuf__, ZLIB_LAST_SEGMENT);
//        data.clear();
//
//        buf << __outbuf__.str(); __outbuf__.str("");
//    } else {
//        buf << dataSec.str();
//        dataSec.str("");
//    }
//
//    if(File::write(c_options.out.c_str(), buf.str())) {
//        cout << progname << ": error: failed to write out to executable " << c_options.out << endl;
//    }
}

void parseGenericName(Int &i, string &name, stringstream &ss) {
    ss << '$';
    i++;
    for(; i < name.size(); i++) {
        if(name[i] == '>')
        {
            ss << '$';
            return;
        }

        if(name[i] == ',')
            ss << "_0_";
        else if(name[i] == '<') {
            parseGenericName(i, name, ss);
        }
        else ss << name[i];
    }
}

Int getDuplicateCount(Method *func, List<Method*> methods, Int index) {
    Int duplicates = 0;
    for(Int i = 0; i < index; i++) {
        if(func->name == methods.get(i)->name) {
            duplicates++;
        }
    }

    return duplicates;
}

string typeToCPPType(DataType type, bool isArray) {
    switch(type) {
        case _INT8:
            if(isArray) return "_int8_array";
            return "_int8";
        case _INT16:
            if(isArray) return "_int16_array";
            return "_int16";
        case _INT32:
            if(isArray) return "_int32_array";
            return "_int32";
        case _INT64:
            if(isArray) return "_int64_array";
            return "_int64";
        case _UINT8:
            if(isArray) return "_uint8_array";
            return "_uint8";
        case _UINT16:
            if(isArray) return "_uint16_array";
            return "_uint16";
        case _UINT32:
            if(isArray) return "_uint32_array";
            return "_uint32";
        case _UINT64:
            if(isArray) return "_uint64_array";
            return "_uint64";
        case FNPTR:
        case VAR:
            if(isArray) return "var_array";
            return "var";
        case NIL:
            return "void";
        default:
            return "object";
    }
}

bool isCppKeyword(string &word) {
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

string operatorToString(string op) {
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
    else if(op == "|")
        return "$or";
    else if(op == "^")
        return "$xor";
    else return op; // error should not happen
}

void ExeBuilder::appendClassHeaderFunctions(ClassObject* klass, stringstream &ss) {
    List<Method*> &classMethods = klass->getFunctions();

    if(classMethods.empty())
        return;

    if(klass->isGlobalClass()) {
        ss << "scope_begin(";
        ss << (isCppKeyword(klass->module->name) ? "_" : "") << moduleToCPPName(klass->module->name) << ") " << endl;
    } else {
        ss << "scope_begin(" << (isCppKeyword(klass->module->name) ? "_" : "") << moduleToCPPName(klass->module->name) << ", ";
        string cppName = classToCPPName(klass->fullName);

        ss << (isCppKeyword(cppName) ? "_" : "") << cppName << ") " << endl;
    }

    for(Int k = 0; k < classMethods.size(); k++) {
        Method *func = classMethods.get(k);
        Int duplicateCount = getDuplicateCount(func, classMethods, k);
        ss << endl << "\t" << typeToCPPType(func->utype->getResolvedType()->type, func->utype->isArray())
           << " ";

        if(startsWith(func->name, "operator")) {
            ss << "op_" << operatorToString(func->name.substr(8, func->name.size()));
        } else {
            ss << (isCppKeyword(func->name) ? "_" : "") << func->name;
        }

        if(duplicateCount)
            ss << (duplicateCount + 1);
        ss << "(";

        if(!func->flags.find(STATIC)) {
            ss << "object $instance";

            if(!func->params.empty())
                ss << ", ";
        }

        for(Int l = 0; l < func->params.size(); l++) {
            Field *param = func->params.get(l);
            ss << typeToCPPType(param->type, param->utype->isArray());
            if(param->type <= VAR)
                ss << "&";
            ss << " " << (isCppKeyword(param->name) ? "_" : "") << param->name;

            if((l + 1) < func->params.size()) {
                ss << ", ";
            }
        }

        ss << ");";
    }

    ss << endl << "scope_end()" << endl << endl;
}

void ExeBuilder::createNativeHeaderFile() {
    stringstream ss;
    ss << "#ifndef SHARP_NATIVE_MAPPING_H" << endl;
    ss << "#define SHARP_NATIVE_MAPPING_H" << endl << endl;
    ss << "#include \"snb_api.h\"" << endl << endl;
    ss << "using namespace snb_api;" << endl << endl;


    for(Int j = 0; j < allClasses.size(); j++) {
        appendClassHeaderFunctions(allClasses.get(j), ss);
    }

    ss << "#endif //SHARP_NATIVE_MAPPING_H" << endl;

#ifdef _WIN32
    string mappingFile = c_options.nativeCodeDir + "\\native_mapping.h";
    _mkdir(c_options.nativeCodeDir.c_str());
#else
    string mappingFile = c_options.nativeCodeDir + "/native_mapping.h";
    mkdir(c_options.nativeCodeDir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif

    File::write(mappingFile.c_str(), ss.str());
}

void ExeBuilder::appendProcAddrFunctions(ClassObject* klass, stringstream &ss, bool &firstFunc) {
    List<Method *> &classMethods = klass->getFunctions();

    for(Int k = 0; k < classMethods.size(); k++) {
        Method *func = classMethods.get(k);

        if(func->isNative()) {
            if(firstFunc) {
                ss << "\tif(";
                firstFunc = false;
            }
            else
                ss << "\telse if(";

            ss << "name == \"" << func->fullName << "\") {  return " << func->address << ";  }" << endl;
        }
    }
}

void ExeBuilder::createProcAddrFunc(stringstream &ss) {
    ss << "EXPORTED uint32_t snb_link_proc(const char* funcName) {" << endl
       << "\tstring name = funcName;" << endl << endl;

    bool firstFunc = true;

    for(Int j = 0; j < allClasses.size(); j++) {
        appendProcAddrFunctions(allClasses.get(j), ss, firstFunc);
    }

    ss << "\telse return -1;" << endl;
    ss << "}" << endl << endl;
}

void ExeBuilder::appendCallFunctions(ClassObject* klass, stringstream& ss) {
    List<Method *> &classMethods = klass->getFunctions();

    for(Int k = 0; k < classMethods.size(); k++) {
        Method *func = classMethods.get(k);

        if(!func->isNative()) continue;
        Int duplicateCount = getDuplicateCount(func, classMethods, k);

        ss << "void call_" << moduleToCPPName(func->module->name) << "$" << classToCPPName(func->owner->fullName) << "_" << func->name;

        if(duplicateCount)
            ss << (duplicateCount + 1);
        ss << "() {";

        if(func->owner->isGlobalClass()) {
            ss << endl << "\timport(" << moduleToCPPName(func->module->name) << "); " << endl;
        } else {
            ss << endl << "\timport(" << moduleToCPPName(func->module->name) << ", ";
            ss << classToCPPName(func->owner->fullName) << "); " << endl;
        }

        Int localAddr = 0;
        if(!func->flags.find(STATIC)) {
            localAddr++;
            ss << endl << "\tobject $instance = internal::getfpLocalAt(0);";
        }

        for(Int l = 0; l < func->params.size(); l++) {
            Field *param = func->params.get(l);

            if(param->utype->getResolvedType()->type <= VAR && param->isArray) {
                ss << endl << "\tobject " << "$tmpField" << localAddr << " = internal::getfpLocalAt(" << localAddr << ");";
            }

            ss << endl << "\t" << typeToCPPType(param->utype->getResolvedType()->type, param->utype->isArray())
               << " " << (isCppKeyword(param->name) ? "_" : "") << param->name;

            if(param->utype->getResolvedType()->type <= VAR) {
                if(param->isArray) {
                    stringstream fieldName;
                    fieldName << "$tmpField" << localAddr;
                    ss << "(internal::getVarPtr(" << fieldName.str() << "), internal::getSize(" << fieldName.str() << "), "
                        << fieldName.str() << ");";
                } else {
                    ss << "(internal::getfpNumAt(" << localAddr << "));";
                }
            } else {
                ss << " = internal::getfpLocalAt(" << localAddr << ");";
            }

            localAddr++;
        }

        ss << endl << "\t";

        if(func->utype->getResolvedType()->type != NIL) {
            if (func->utype->getResolvedType()->type <= VAR) {
                if (func->utype->isArray()) {
                    ss << "object $result = internal::getfpLocalAt(0);" << endl << "\t";
                    ss << typeToCPPType(func->utype->getResolvedType()->type, true) << " $returnVal_ = ";
                } else {
                    ss << "var $result(internal::getfpNumAt(0));" << endl << "\t";
                    ss << "$result = ";
                }
            } else {
                ss << "object $result = internal::getfpLocalAt(0);" << endl << "\t";
                ss << typeToCPPType(func->utype->getResolvedType()->type, func->utype->isArray()) << " $returnVal_ = ";
            }
        }

        ss << (isCppKeyword(func->name) ? "_" : "") << func->name;
        ss << "(";

        for(Int l = 0; l < func->params.size(); l++) {
            Field *param = func->params.get(l);
            ss << (isCppKeyword(param->name) ? "_" : "") << param->name;

            if((l + 1) < func->params.size()) {
                ss << ", ";
            }
        }

        if(func->utype->getResolvedType()->type != NIL) {
            if (func->utype->getResolvedType()->type <= VAR) {
                if (func->utype->isArray()) {
                    ss << ");" << endl << "\tset($result, $returnVal_);";
                } else {
                    ss << ");";
                }
            } else {
                ss << ");" << endl << "\tset($result, $returnVal_);";
            }
        } else
            ss << ");";


        ss << endl << "}" << endl << endl;
    }
}

void ExeBuilder::createCallFunc(stringstream &ss) {
    for(Int j = 0; j < allClasses.size(); j++) {
        appendCallFunctions(allClasses.get(j), ss);
    }
}

void ExeBuilder::appendMainFunctions(ClassObject* klass, stringstream& ss) {
    List<Method *> &classMethods = klass->getFunctions();

    for(Int k = 0; k < classMethods.size(); k++) {
        Method *func = classMethods.get(k);

        if(!func->isNative()) continue;
        Int duplicateCount = getDuplicateCount(func, classMethods, k);

        ss << "\t\t\t";
        ss << "case " << func->address << ": " << endl;
        ss << "\t\t\t\tcall_" << moduleToCPPName(func->module->name) << "$"<< classToCPPName(func->owner->fullName) << "_" << func->name;

        if(duplicateCount)
            ss << (duplicateCount + 1);
        ss << "();" << endl;
        ss << "\t\t\t\tbreak;" << endl;
    }
}

void ExeBuilder::createMainFunc(stringstream &ss) {
    ss << "EXPORTED void snb_main(long procAddr) {" << endl;
    ss << "\ttry {" << endl;
    ss << "\t\tswitch(procAddr) {" << endl;

    for(Int j = 0; j < allClasses.size(); j++) {
        appendMainFunctions(allClasses.get(j), ss);
    }

    ss << "\t\t}" << endl;
    ss << "\t} catch(Exception &e) {" << endl;
    ss << "\t\tsnb_api::internal::prepareException(e.exceptionClass);" << endl;
    ss << "\t}" << endl;
    ss << "}" << endl;
}

void ExeBuilder::createNativeSourceFile() {
    stringstream ss;
    ss << "#include \"native_mapping.h\"" << endl
          << endl
          << "using namespace std;" << endl
          << "using namespace snb_api;" << endl << endl;

    ss << "#ifdef __cplusplus" << endl
          << "extern \"C\" {" << endl
          << "#endif" << endl << endl;

    createProcAddrFunc(ss);
    createCallFunc(ss);
    createMainFunc(ss);

    ss << endl
          << "#ifdef __cplusplus" << endl
          << "}" << endl
          << "#endif" << endl;

#ifdef _WIN32
    string mappingFile = c_options.nativeCodeDir + "\\native_mapping.cpp";
    _mkdir(c_options.nativeCodeDir.c_str());
#else
    string mappingFile = c_options.nativeCodeDir + "/native_mapping.cpp";
    mkdir(c_options.nativeCodeDir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif

    File::write(mappingFile.c_str(), ss.str());
}

void ExeBuilder::appendSharpMappingSourceFile(ClassObject* klass, stringstream& ss) {
    List<Method*> &classMethods = klass->getFunctions();

    if(classMethods.empty())
        return;

    if(klass->isGlobalClass()) {
        ss << "scope_begin(";
        ss << (isCppKeyword(klass->module->name) ? "_" : "") << moduleToCPPName(klass->module->name) << ") " << endl;
    } else {
        ss << "scope_begin(" << (isCppKeyword(klass->module->name) ? "_" : "") << moduleToCPPName(klass->module->name) << ", ";
        string cppName = classToCPPName(klass->fullName);

        ss << (isCppKeyword(cppName) ? "_" : "") << cppName << ") " << endl;
    }

    for(Int k = 0; k < classMethods.size(); k++) {
        Method *func = classMethods.get(k);

        if(func->isNative()) continue;
        Int duplicateCount = getDuplicateCount(func, classMethods, k);
        ss << endl << "\t" << typeToCPPType(func->utype->getResolvedType()->type, func->utype->isArray())
           << " ";

        if(startsWith(func->name, "operator")) {
            ss << "op_" << operatorToString(func->name.substr(8, func->name.size()));
        } else {
            ss << (isCppKeyword(func->name) ? "_" : "") << func->name;
        }

        if(duplicateCount)
            ss << (duplicateCount + 1);
        ss << "(";

        if(!func->flags.find(STATIC)) {
            ss << "object $instance";

            if(!func->params.empty())
                ss << ", ";
        }

        for(Int l = 0; l < func->params.size(); l++) {
            Field *param = func->params.get(l);
            ss << typeToCPPType(param->type, param->utype->isArray());
            if(param->type <= VAR)
                ss << "&";
            ss << " " << (isCppKeyword(param->name) ? "_" : "") << param->name;

            if((l + 1) < func->params.size()) {
                ss << ", ";
            }
        }

        ss << ") {";

        if(!func->flags.find(STATIC)) {
            ss << endl << "\t\tpushObj($instance);";
        }

        for(Int l = 0; l < func->params.size(); l++) {
            Field *param = func->params.get(l);

            ss << endl << "\t\t";

            if(param->utype->getResolvedType()->type <= VAR) {
                if(param->isArray) {
                    ss << "pushObj(" << (isCppKeyword(param->name) ? "_" : "") << param->name << ".handle);";
                } else {
                    ss << "pushNum(" << (isCppKeyword(param->name) ? "_" : "") << param->name << ".value());";
                }
            } else {
                ss << "pushObj(" << (isCppKeyword(param->name) ? "_" : "") << param->name << ");";
            }
        }

        ss << endl << "\t\tcall(" << func->address << ");" << endl << endl;
        ss << "\t\tif(internal::exceptionCheck()) {" << endl;
        ss << "\t\t\tthrow Exception(internal::getExceptionObject(), \"\");" << endl;
        ss << "\t\t}" << endl;

        if(func->utype->getResolvedType()->type != NIL) {
            ss << endl;

            if(func->utype->getResolvedType()->type <= VAR && func->utype->isArray()) {
                ss << "\t\tobject $tmpObj = getSpObjAt(0);" << endl;
            }

            if(func->utype->getResolvedType()->type <= VAR) {
                ss << "\t\t" << typeToCPPType(func->utype->getResolvedType()->type, func->utype->isArray()) << " $result";

                if(func->utype->isArray()) {
                    ss << "(getVarPtr($tmpObj), getSize($tmpObj), $tmpObj);";
                } else {
                    ss << "(getSpNumAt(0));" << endl;
                    ss << "\t\tdecSp(1);" << endl;
                }
            } else {
                ss << "\t\tobject $result = getSpObjAt(0);" << endl;
                ss << "\t\tdecSp(1);" << endl;
            }

            ss << "\t\treturn $result;" << endl;
        }
        ss << "\t}" << endl;
    }

    ss << endl << "scope_end()" << endl << endl;
}

void ExeBuilder::createSharpMappingSourceFile() {
    stringstream ss;
    ss << "#include \"native_mapping.h\"" << endl
       << endl
       << "using namespace snb_api::internal;" << endl << endl;

    for(Int j = 0; j < allClasses.size(); j++) {
        appendSharpMappingSourceFile(allClasses.get(j), ss);
    }

#ifdef _WIN32
    string mappingFile = c_options.nativeCodeDir + "\\sharp_mapping.cpp";
    _mkdir(c_options.nativeCodeDir.c_str());
#else
    string mappingFile = c_options.nativeCodeDir + "/sharp_mapping.cpp";
    mkdir(c_options.nativeCodeDir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif

    File::write(mappingFile.c_str(), ss.str());
}

void ExeBuilder::createNativeSourceCode() {
    createNativeHeaderFile();
    createNativeSourceFile();
    createSharpMappingSourceFile();
}

void ExeBuilder::createDumpFile() {
    stringstream tmpData;

    buf << "## Object Dump file for " << c_options.out << ":\n\n";

    for(Int i = 0; i < compiler->classes.size(); i++) {
        ClassObject *klass = compiler->classes.get(i);
        addClass(klass);
    }

    allClasses.linearSort(sortClasses);
    allMethods.linearSort(sortMethods);

    if(c_options.optimize) {
        Optimizer optimizer(&allMethods);
        optimizer.optimize();

        buf << "pre codebase size: " << optimizer.preCodebaseSize << endl;
        buf << "post codebase size: " << optimizer.postCodebaseSize << endl;
        buf << "optimized opcodes: " << (optimizer.preCodebaseSize - optimizer.postCodebaseSize) << endl;
        buf << "net % gain " << (((double)(optimizer.preCodebaseSize - optimizer.postCodebaseSize) / (double)optimizer.preCodebaseSize) * 100) << endl;
        buf << endl << endl;
    }

    for(Int i = 0; i < allClasses.size(); i++) {
        ClassObject *klass = allClasses.get(i);
        dumpClassInfo(klass);
    }

    buf << "\n\n strings: \n";
    for(Int i = 0; i < compiler->stringMap.size(); i++) {
        buf << "\n[" << i << "]: ";

        if(compiler->stringMap.at(i).size() >= 100)
            buf << compiler->stringMap.get(i).substr(0, 100);
        else buf << compiler->stringMap.get(i);
    }

    buf << "\n\n constants: \n";
    for(Int i = 0; i < compiler->constantMap.size(); i++) {
        buf << "\n[" << i << "]: ";

        buf << std::scientific
            << std::setprecision(std::numeric_limits<double>::digits10 + 1);
        buf << compiler->constantMap.get(i);
        buf << std::dec;
    }

    buf << "\n\n methods: \n";
    for(Int i = 0; i < allMethods.size(); i++) {
        Method *fun = allMethods.get(i);

        if(fun->fnType == fn_constructor && fun->data.code.ir32.empty()) {
            fun->data.code.addIr(OpBuilder::ret(NO_ERR));
        }

        switch(fun->fnType) {
            case fn_normal:
                buf << "function: ";
                break;
            case fn_constructor:
                buf << "constructor: ";
                break;
            case fn_ptr:
                buf << "pointer: ";
                break;
            case fn_lambda:
                buf << "lambda: ";
                break;
            case fn_op_overload:
                buf << "overload function: ";
                break;
        }

        buf << fun->fullName << "[" << fun->address << "] params("
            << fun->params.size() << ") " << getNote(fun->meta) << "\n";

        buf << codeToString(fun);

        buf << "\n\n";
    }

    if(File::write((c_options.out + ".odf").c_str(), buf.str().c_str())) {
        cout << progname << ": error: failed to write out to dump file: " << c_options.out + ".odf" << endl;
    }
}

string ExeBuilder::getNote(Meta& meta) {
    stringstream note;
    note << "in file: ";
    note << meta.file->name << ":" << meta.ln << ":" << meta.col << ": note:  " << endl;

    note << '\t' << '\t' << meta.line << endl << '\t' << '\t';

    for(int i = 0; i < meta.col-1; i++)
        note << " ";
    note << "^" << endl;

    return note.str();
}

void ExeBuilder::dumpClassInfo(ClassObject *klass) {
    switch(klass->getClassType()) {
        case class_normal:
            buf << "class: ";
            break;
        case class_interface:
            buf << "interface: ";
            break;
        case class_enum:
            buf << "enum: ";
            break;
        case class_generic:
            buf << "generic: ";
            break;
    }

    buf << klass->fullName << "[" << klass->address << "] fields("
        << klass->totalFieldCount() << ") methods(" << klass->totalFunctionCount() << ")\n";
}

void ExeBuilder::addClass(ClassObject *klass) {
    if(IS_CLASS_GENERIC(klass->getClassType()) && klass->getGenericOwner() == NULL)
        return;
    allClasses.add(klass);

    allMethods.appendAll(klass->getFunctions());
    if(klass->isGlobalClass())
        return;

    List<ClassObject*> &childClasses = klass->getChildClasses();
    for(Int i = 0; i < childClasses.size(); i++) {
        addClass(childClasses.get(i));
    }
}


/*
 * DEBUG: This is a debug function made to get the name of a register
 */
string ExeBuilder::registerToString(int64_t r) {
    switch(r) {
        case ADX:
            return "adx";
        case CX:
            return "cx";
        case CMT:
            return "cmt";
        case EBX:
            return "ebx";
        case ECX:
            return "ecx";
        case ECF:
            return "ecf";
        case EDF:
            return "edf";
        case EHF:
            return "ehf";
        case BMR:
            return "bmr";
        case EGX:
            return "egx";
        default: {
            stringstream ss;
            ss << "? (" << r << ")";
            return ss.str();
        }
    }
}

/*
 * DEBUG: This is a debug function made to allow early testing in compiler code generation
 */
string ExeBuilder::codeToString(Method* fun) {
    CodeHolder &code = fun->data.code;
    stringstream ss, endData;
    Int linesProcessed = 0, currLine = 0;

    for(unsigned int x = 0; x < code.size(); x++) {
        opcode_instr opcodeData=code.ir32.get(x);
        ss << x <<std::hex << " [0x" << x << std::dec << "]"<< ": ";

        if(linesProcessed < fun->data.lineTable.size() && x >= fun->data.lineTable.get(linesProcessed).start_pc) {
            currLine = fun->data.lineTable.get(linesProcessed++).line;
            ss << "line: " <<  currLine << ' ';
        } else {
            if(currLine <=9) ss << "        ";
            else if(currLine <=99) ss << "         ";
            else if(currLine <=999) ss << "          ";
            else if(currLine <=9999) ss << "           ";
            else ss << "            ";
        }

        endData << "   ";
        for(Int i = 0; i < fun->data.protectedCodeTable.size(); i++) {
            if(x == fun->data.protectedCodeTable.get(i).start_pc) {
                endData << ": protected asm start";
            } else if(x == fun->data.protectedCodeTable.get(i).end_pc) {
                endData << ": protected asm end";
            }
        }

        for(Int i = 0; i < fun->data.tryCatchTable.size(); i++) {
            TryCatchData &tryCatchData = fun->data.tryCatchTable.get(i);
            if(x == tryCatchData.try_start_pc) {
                endData << ": try start";
            } else if(x == tryCatchData.try_end_pc) {
                endData << ": try end";
            }

            if(x == tryCatchData.block_start_pc) {
                endData << ": try block start";
            } else if(x == tryCatchData.block_end_pc) {
                endData << ": try block end";
            }

            for(Int j = 0; j < tryCatchData.catchTable.size(); j++) {
                CatchData &catchData = tryCatchData.catchTable.get(j);

                if(x == catchData.handler_pc){
                    endData << ": catch(" << allClasses.get(catchData.classAddress)->fullName << ")";
                }
            }

            if(tryCatchData.finallyData != NULL) {
                if(x == tryCatchData.finallyData->start_pc) {
                    endData << ": finally start";
                } else if(x == tryCatchData.finallyData->end_pc) {
                    endData << ": finally end";
                }
            }
        }

        switch(GET_OP(opcodeData)) {
            case Opcode::ILL:
            {
                ss<<"{!: ill} ";

                break;
            }
            case Opcode::NOP:
            {
                ss<<"nop";

                break;
            }
            case Opcode::INT:
            {
                ss<<"int 0x" << std::hex << GET_Da(opcodeData) << std::dec;

                break;
            }
            case Opcode::MOVI:
            {
                ss << "movi #" << (int)code.ir32.get(x+1)  << ", ";
                ss<< registerToString(GET_Da(opcodeData)) ;
                x++;
                break;
            }
            case Opcode::RET:
            {
                ss<<"ret" << (GET_Da(opcodeData) == ERR_STATE ? " ERR" : "");

                break;
            }
            case Opcode::HLT:
            {
                ss<<"hlt";

                break;
            }
            case Opcode::NEWARRAY:
            {
                ss<<"newarry ";
                ss<< registerToString(GET_Ca(opcodeData)) << ": ";
                ss<< GET_Cb(opcodeData);

                break;
            }
            case Opcode::CAST:
            {
                ss<<"cast ";
                ss<< registerToString(GET_Da(opcodeData));

                break;
            }
            case Opcode::VARCAST:
            {
                ss<<"vcast ";
                ss<< GET_Ca(opcodeData);
                ss << " -> " << (GET_Cb(opcodeData) == 1 ? "[]" : "");

                break;
            }
            case Opcode::MOV8:
            {
                ss<<"mov8 ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));

                break;
            }
            case Opcode::MOV16:
            {
                ss<<"mov16 ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));

                break;
            }
            case Opcode::MOV32:
            {
                ss<<"mov32 ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));

                break;
            }
            case Opcode::MOV64:
            {
                ss<<"mov64 ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));

                break;
            } case Opcode::MOVU8:
            {
                ss<<"movu8 ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));

                break;
            }
            case Opcode::MOVU16:
            {
                ss<<"movu16 ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));

                break;
            }
            case Opcode::MOVU32:
            {
                ss<<"movu32 ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));

                break;
            }
            case Opcode::MOVU64:
            {
                ss<<"movu64 ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));

                break;
            }
            case Opcode::RSTORE:
            {
                ss<<"rstore ";
                ss<< registerToString(GET_Da(opcodeData));

                break;
            }
            case Opcode::ADD:
            {
                ss<<"add ";
                ss<< registerToString(GET_Ba(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Bb(opcodeData));
                ss<< " -> ";
                ss<< registerToString(GET_Bc(opcodeData));

                break;
            }
            case Opcode::SUB:
            {
                ss<<"sub ";
                ss<< registerToString(GET_Ba(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Bb(opcodeData));
                ss<< " -> ";
                ss<< registerToString(GET_Bc(opcodeData));

                break;
            }
            case Opcode::MUL:
            {
                ss<<"mul ";
                ss<< registerToString(GET_Ba(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Bb(opcodeData));
                ss<< " -> ";
                ss<< registerToString(GET_Bc(opcodeData));

                break;
            }
            case Opcode::DIV:
            {
                ss<<"div ";
                ss<< registerToString(GET_Ba(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Bb(opcodeData));
                ss<< " -> ";
                ss<< registerToString(GET_Bc(opcodeData));

                break;
            }
            case Opcode::MOD:
            {
                ss<<"mod ";
                ss<< registerToString(GET_Ba(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Bb(opcodeData));
                ss<< " -> ";
                ss<< registerToString(GET_Bc(opcodeData));

                break;
            }
            case Opcode::IADD:
            {
                ss<<"iadd ";
                ss<< registerToString(GET_Da(opcodeData));
                ss<< ", #";
                ss<< code.ir32.get(++x);

                break;
            }
            case Opcode::ISUB:
            {
                ss<<"isub ";
                ss<< registerToString(GET_Da(opcodeData));
                ss<< ", #";
                ss<< code.ir32.get(++x);

                break;
            }
            case Opcode::IMUL:
            {
                ss<<"imul ";
                ss<< registerToString(GET_Da(opcodeData));
                ss<< ", #";
                ss<< code.ir32.get(++x);

                break;
            }
            case Opcode::IDIV:
            {
                ss<<"idiv ";
                ss<< registerToString(GET_Da(opcodeData));
                ss<< ", #";
                ss<< code.ir32.get(++x);

                break;
            }
            case Opcode::IMOD:
            {
                ss<<"imod ";
                ss<< registerToString(GET_Da(opcodeData));
                ss<< ", #";
                ss<< code.ir32.get(++x);

                break;
            }
            case Opcode::POP:
            {
                ss<<"pop";

                break;
            }
            case Opcode::INC:
            {
                ss<<"inc ";
                ss<< registerToString(GET_Da(opcodeData));

                break;
            }
            case Opcode::DEC:
            {
                ss<<"dec ";
                ss<< registerToString(GET_Da(opcodeData));

                break;
            }
            case Opcode::MOVR:
            {
                ss<<"movr ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));

                break;
            }
            case Opcode::IALOAD:
            {
                ss<<"iaload ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));

                break;
            }
            case Opcode::BRH:
            {
                ss<<"brh";

                break;
            }
            case Opcode::IFE:
            {
                ss<<"ife";

                break;
            }
            case Opcode::IFNE:
            {
                ss<<"ifne";

                break;
            }
            case Opcode::JE: // TODO: continue here
            {
                ss<<"je " << GET_Da(opcodeData);

                break;
            }
            case Opcode::JNE:
            {
                ss<<"jne " << GET_Da(opcodeData);

                break;
            }
            case Opcode::LT:
            {
                ss<<"lt ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));

                break;
            }
            case Opcode::GT:
            {
                ss<<"gt ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));

                break;
            }
            case Opcode::LTE:
            {
                ss<<"lte ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));

                break;
            }
            case Opcode::GTE:
            {
                ss<<"gte ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));

                break;
            }
            case Opcode::MOVL:
            {
                ss<<"movl " << GET_Da(opcodeData);

                break;
            }
            case Opcode::POPL:
            {
                ss<<"popl " << GET_Da(opcodeData);

                break;
            }
            case Opcode::MOVSL:
            {
                ss<<"movsl #";
                ss<< GET_Da(opcodeData);

                break;
            }
            case Opcode::SIZEOF:
            {
                ss<<"sizeof ";
                ss<< registerToString(GET_Da(opcodeData));

                break;
            }
            case Opcode::PUT:
            {
                ss<<"put ";
                ss<< registerToString(GET_Da(opcodeData));

                break;
            }
            case Opcode::PUTC:
            {
                ss<<"_putc ";
                ss<< registerToString(GET_Da(opcodeData));

                break;
            }
            case Opcode::CHECKLEN:
            {
                ss<<"chklen ";
                ss<< registerToString(GET_Da(opcodeData));

                break;
            }
            case Opcode::JMP:
            {
                ss<<"jmp @" << GET_Da(opcodeData);

                break;
            }
            case Opcode::LOADPC:
            {
                ss<<"loadpc ";
                ss<< registerToString(GET_Da(opcodeData));

                break;
            }
            case Opcode::PUSHOBJ:
            {
                ss<<"pushobj";

                break;
            }
            case Opcode::DEL:
            {
                ss<<"del";

                break;
            }
            case Opcode::CALL:
            {
                ss << "call @" << GET_Da(opcodeData) << " // <";
                if(GET_Da(opcodeData) >= 0)
                    ss << allMethods.get(GET_Da(opcodeData))->fullName << ">";

                break;
            }
            case Opcode::CALLD:
            {
                ss<<"calld ";
                ss<< registerToString(GET_Da(opcodeData));

                break;
            }
            case Opcode::NEWCLASS:
            {
                ss<<"new_class @" << code.ir32.get(x+1);
                if(code.ir32.get(x+1) >= 0 && code.ir32.get(x+1) < allClasses.size()) {
                    ss << " // ";
                    ss << allClasses.get(code.ir32.get(x+1))->fullName;
                } else ss << "out of bounds: " << code.ir32.get(x+1);
                x++;
                break;
            }
            case Opcode::MOVN:
            {
                ss<<"movn #" << code.ir32.get(++x);

                break;
            }
            case Opcode::SLEEP:
            {
                ss<<"sleep ";
                ss<< registerToString(GET_Da(opcodeData));

                break;
            }
            case Opcode::TEST:
            {
                ss<<"test ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));

                break;
            }
            case Opcode::TNE:
            {
                ss<<"tne ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));

                break;
            }
            case Opcode::LOCK:
            {
                ss<<"_lck ";

                break;
            }
            case Opcode::ULOCK:
            {
                ss<<"_ulck";

                break;
            }
            case Opcode::MOVG:
            {
                ss<<"movg @"<< GET_Da(opcodeData);
                if(GET_Da(opcodeData) >= 0)
                    ss << " // @"; ss << allClasses.get(GET_Da(opcodeData))->fullName;

                break;
            }
            case Opcode::MOVND:
            {
                ss<<"movnd ";
                ss<< registerToString(GET_Da(opcodeData));

                break;
            }
            case Opcode::NEWOBJARRAY:
            {
                ss<<"newobj_arry ";
                ss<< registerToString(GET_Da(opcodeData));

                break;
            }
            case Opcode::NOT: //c
            {
                ss<<"not ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));

                break;
            }
            case Opcode::SKIP:// d
            {
                ss<<"skip @";
                ss<< GET_Da(opcodeData);
                ss << " // pc = " << (x + GET_Da(opcodeData));

                break;
            }
            case Opcode::LOADVAL:
            {
                ss<<"loadval ";
                ss<< registerToString(GET_Da(opcodeData));

                break;
            }
            case Opcode::SHL:
            {
                ss<<"shl ";
                ss<< registerToString(GET_Ba(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Bb(opcodeData));
                ss<< " -> ";
                ss<< registerToString(GET_Bc(opcodeData));

                break;
            }
            case Opcode::SHR:
            {
                ss<<"shr ";
                ss<< registerToString(GET_Ba(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Bb(opcodeData));
                ss<< " -> ";
                ss<< registerToString(GET_Bc(opcodeData));

                break;
            }
            case Opcode::SKPE:
            {
                ss<<"skpe ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss << " // pc = " << (x + GET_Cb(opcodeData));

                break;
            }
            case Opcode::SKNE:
            {
                ss<<"skne ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss << " // pc = " << (x + GET_Cb(opcodeData));

                break;
            }
            case Opcode::CMP:
            {
                ss<<"cmp ";
                ss<< registerToString(GET_Da(opcodeData));
                ss<< ", ";
                ss<< code.ir32.get(++x);

                break;
            }
            case Opcode::AND:
            {
                ss<<"and ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));

                break;
            }
            case Opcode::UAND:
            {
                ss<<"uand ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));

                break;
            }
            case Opcode::OR:
            {
                ss<<"or ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));

                break;
            }
            case Opcode::XOR:
            {
                ss<<"xor ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));

                break;
            }
            case Opcode::THROW:
            {
                ss<<"throw ";

                break;
            }
            case Opcode::CHECKNULL:
            {
                ss<<"checknull";

                break;
            }
            case Opcode::RETURNOBJ:
            {
                ss<<"returnobj";

                break;
            }
            case Opcode::NEWCLASSARRAY:
            {
                ss<<"new_classarray ";
                ss<< registerToString(GET_Da(opcodeData));
                ss<< " ";
                x++;
                if(code.ir32.get(x) >= 0 && code.ir32.get(x) < allClasses.size()) {
                    ss << " // ";
                    ss << allClasses.get(code.ir32.get(x))->fullName << "[]";
                }
                else ss << "out of bounds: " << code.ir32.get(x);

                break;
            }
            case Opcode::NEWSTRING:
            {
                ss << "newstr @" << GET_Da(opcodeData) << " // ";
                if(GET_Da(opcodeData) >= 0)
                    ss << compiler->stringMap.get(GET_Da(opcodeData));

                break;
            }
            case Opcode::ADDL:
            {
                ss<<"addl ";
                ss << registerToString(GET_Ca(opcodeData)) << ", fp@";
                ss<<GET_Cb(opcodeData);

                break;
            }
            case Opcode::SUBL:
            {
                ss<<"subl ";
                ss << registerToString(GET_Ca(opcodeData)) << ", fp@";
                ss<<GET_Cb(opcodeData);

                break;
            }
            case Opcode::MULL:
            {
                ss<<"mull ";
                ss << registerToString(GET_Ca(opcodeData)) << ", fp@";
                ss<<GET_Cb(opcodeData);

                break;
            }
            case Opcode::DIVL:
            {
                ss<<"divl ";
                ss << registerToString(GET_Ca(opcodeData)) << ", fp@";
                ss<<GET_Cb(opcodeData);

                break;
            }
            case Opcode::MODL:
            {
                ss<<"modl #";
                ss << registerToString(GET_Ca(opcodeData)) << ", fp@";
                ss<<GET_Cb(opcodeData);

                break;
            }
            case Opcode::IADDL:
            {
                ss<<"iaddl ";
                ss << (int)code.ir32.get(x+1) << ", fp@";
                ss<<GET_Da(opcodeData); x++;

                break;
            }
            case Opcode::ISUBL:
            {
                ss<<"isubl ";
                ss << (int)code.ir32.get(x+1) << ", fp@";
                ss<<GET_Da(opcodeData); x++;

                break;
            }
            case Opcode::IMULL:
            {
                ss<<"imull ";
                ss << (int)code.ir32.get(x+1) << ", fp@";
                ss<<GET_Da(opcodeData); x++;

                break;
            }
            case Opcode::IDIVL:
            {
                ss<<"idivl ";
                ss << (int)code.ir32.get(x+1) << ", fp@";
                ss<<GET_Da(opcodeData); x++;

                break;
            }
            case Opcode::IMODL:
            {
                ss<<"imodl ";
                ss << (int)code.ir32.get(x+1) << ", fp@";
                ss<<GET_Da(opcodeData); x++;

                break;
            }
            case Opcode::LOADL:
            {
                ss<<"loadl ";
                ss << registerToString(GET_Ca(opcodeData)) << ", fp+";
                ss<<GET_Cb(opcodeData);

                break;
            }
            case Opcode::POPOBJ:
            {
                ss<<"popobj";

                break;
            }
            case Opcode::SMOVR:
            {
                ss<<"smovr ";
                ss << registerToString(GET_Ca(opcodeData)) << ", sp+";
                if(GET_Cb(opcodeData) < 0) ss << "[";
                ss<<GET_Cb(opcodeData);
                if(GET_Cb(opcodeData) < 0) ss << "]";

                break;
            }
            case Opcode::SMOVR_2:
            {
                ss<<"smovr_2 ";
                ss << registerToString(GET_Ca(opcodeData)) << ", fp+";
                if(GET_Cb(opcodeData) < 0) ss << "[";
                ss<<GET_Cb(opcodeData);
                if(GET_Cb(opcodeData) < 0) ss << "]";

                break;
            }
            case Opcode::ANDL:
            {
                ss<<"andl ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< GET_Cb(opcodeData);

                break;
            }
            case Opcode::ORL:
            {
                ss<<"orl ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< GET_Cb(opcodeData);

                break;
            }
            case Opcode::XORL:
            {
                ss<<"xorl ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< GET_Cb(opcodeData);

                break;
            }
            case Opcode::RMOV:
            {
                ss<<"rmov ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));

                break;
            }
            case Opcode::SMOV:
            {
                ss<<"smov ";
                ss << registerToString(GET_Ca(opcodeData)) << ", sp+";
                if(GET_Cb(opcodeData) < 0) ss << "[";
                ss<<GET_Cb(opcodeData);
                if(GET_Cb(opcodeData) < 0) ss << "]";

                break;
            }
            case Opcode::RETURNVAL:
            {
                ss<<"return_val ";
                ss<< registerToString(GET_Da(opcodeData));

                break;
            }
            case Opcode::ISTORE:
            {
                ss<<"istore ";
                ss<< code.ir32.get(++x);

                break;
            }
            case Opcode::ISTOREL:
            {
                ss<<"istorel ";
                ss << ((int32_t)code.ir32.get(++x)) << ", fp+";
                ss<<GET_Da(opcodeData);

                break;
            }
            case Opcode::IPUSHL:
            {
                ss<<"ipushl #";
                ss<< GET_Da(opcodeData);

                break;
            }
            case Opcode::PUSHL:
            {
                ss<<"pushl ";
                ss<< GET_Da(opcodeData);

                break;
            }
            case Opcode::PUSHNULL:
            {
                ss<<"pushnull ";

                break;
            }
            case Opcode::GET:
            {
                ss<<"get ";
                ss<< registerToString(GET_Da(opcodeData));

                break;
            }
            case Opcode::ITEST:
            {
                ss<<"itest ";
                ss<< registerToString(GET_Da(opcodeData));

                break;
            }
            case Opcode::INVOKE_DELEGATE:
            {
                ss<<"invoke_delegate ";
                ss<< GET_Da(opcodeData);
                ss<< " { static=";
                ss<< GET_Ca(code.ir32.get(x+1))
                  << ", args=" << GET_Cb(code.ir32.get(x+1)) <<  " }";
                x++;

                break;
            }
            case Opcode::ISADD:
            {
                ss<<"isadd ";
                ss << GET_Ca(opcodeData) << ", sp+";
                if((int)code.ir32.get(x+1) < 0) ss << "[";
                ss<<(int)code.ir32.get(x+1);
                if((int)code.ir32.get(x+1) < 0) ss << "]";
                x++;

                break;
            }
            case Opcode::IPOPL:
            {
                ss<<"ipopl ";
                ss<< GET_Da(opcodeData);

                break;
            }
            case Opcode::TLS_MOVL:
            {
                ss<<"tls_movl ";
                ss<< GET_Da(opcodeData);

                break;
            }
            case Opcode::DUP:
            {
                ss<<"dup ";

                break;
            }
            case Opcode::POPOBJ_2:
            {
                ss<<"popobj2 ";

                break;
            }
            case Opcode::SWAP:
            {
                ss<<"swap ";

                break;
            }
            case Opcode::SMOVR_3:
            {
                ss<<"smovr_3";
                ss << " fp+";
                if(GET_Da(opcodeData) < 0) ss << "[";
                ss<<GET_Da(opcodeData);
                if(GET_Da(opcodeData) < 0) ss << "]";

                break;
            }
            case Opcode::LDC:
            {
                ss<<"ldc ";
                ss<< registerToString(GET_Ca(opcodeData));
                if(GET_Cb(opcodeData) >= 0)
                    ss<< " // " << compiler->constantMap.get(GET_Cb(opcodeData));

                break;
            }
            case Opcode::NEG:
            {
                ss<<"neg ";
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Cb(opcodeData));
                break;
            }
            case Opcode::EXP:
            {
                ss<<"exp ";
                ss<< registerToString(GET_Ba(opcodeData));
                ss<< ", ";
                ss<< registerToString(GET_Bb(opcodeData));
                ss<< " -> ";
                ss<< registerToString(GET_Bc(opcodeData));

                break;
            }
            case Opcode::IS:
            {
                ss<<"is ";
                ss<< registerToString(GET_Da(opcodeData));
                ss<< " -> ";
                x++;
                if((int32_t)code.ir32.get(x) >= 0)
                  ss<< allClasses.get(code.ir32.get(x))->fullName;
                else ss << (int32_t)code.ir32.get(x);
                break;
            }
            default:
                ss << "? (" << GET_OP(opcodeData) << ")";

                break;
        }


        ss << endData.str(); endData.str("");
        ss << "\n";
    }

    return ss.str();
}

void ExeBuilder::build() {
    createDumpFile();
    if(compiler->nativeCodeFound)
        createNativeSourceCode();
    buildExe();
}

string copychars(char c, int t) {
    native_string s;
    int it = 0;

    while (it++ < t)
        s += c;

    return s.str();
}

void ExeBuilder::buildHeader() {
    buf << (char)file_sig << "SEF"; buf << copychars(nil, zoffset);
    buf << (char)digi_sig1 << (char)digi_sig2 << (char)digi_sig3;

    buildManifest();
}

void ExeBuilder::buildManifest() {
    buf << (char)manif;
    buf << ((char)0x02); buf << c_options.out << ((char)nil);
    buf << ((char)0x4); buf << c_options.vers << ((char)nil);
    buf << ((char)0x5); buf << (c_options.debug ? 1 : 0);
    buf << ((char)0x6); buf << putInt32(compiler->mainMethod->address);
    buf << ((char)0x7); buf << putInt32(allMethods.size());
    buf << ((char)0x8); buf << putInt32(compiler->classSize);
    buf << ((char)0x9); buf << file_vers << ((char)nil);
    buf << ((char)0x0c); buf << putInt32(compiler->stringMap.size());
    buf << ((char)0x0e); buf << c_options.target << ((char)nil);
    buf << ((char)0x0f); buf << putInt32(compiler->parsers.size());
    buf << ((char)0x1b); buf << putInt32(compiler->threadLocals);
    buf << ((char)0x1c); buf << putInt32(compiler->constantMap.size());
    buf << '\n' << (char)eoh;
}

void ExeBuilder::buildSymbolSection() {
    buf << (char)ssymbol;

    addFunctionPointers();
    for(Int i = 0; i < allClasses.size(); i++) {
        ClassObject *klass = allClasses.get(i);

        buf << (char)data_class;
        buf << putInt32(klass->address);
        buf << (klass->owner == NULL ? putInt32(-1) : putInt32(klass->owner->address));
        buf << (klass->getSuperClass() == NULL ? putInt32(-1) : putInt32(klass->getSuperClass()->address));
        buf << putInt32(klass->guid);
        buf << klass->name << ((char)nil);
        buf << klass->fullName << ((char)nil);
        buf << putInt32(klass->getStaticFieldCount());
        buf << putInt32(klass->totalInstanceFieldCount());
        buf << putInt32(klass->totalFunctionCount());
        buf << putInt32(klass->totalInterfaceCount());

        buildFieldData(klass);
        buildMethodData(klass);
        buildInterfaceData(klass);
    }

    buf << '\n' << (char)eos;
}

void ExeBuilder::buildInterfaceData(ClassObject *klass) {
    if(klass->getSuperClass() != NULL)
        buildInterfaceData(klass->getSuperClass());

    List<ClassObject*> &interfaces = klass->getInterfaces();
    for(Int i = 0; i < interfaces.size(); i++) {
        ClassObject *_interface = interfaces.get(i);
        buf << (char)data_interface;
        buf << putInt32(_interface->address) << ((char)nil);
    }
}
void ExeBuilder::buildMethodData(ClassObject *klass) {
    if(klass->getSuperClass() != NULL)
        buildMethodData(klass->getSuperClass());

    for(Int i = 0; i < klass->getFunctionCount(); i++) {
        Method *function = klass->getFunction(i);
        buf << (char)data_method;
        buf << putInt32(function->address) << ((char)nil);
    }
}

void ExeBuilder::buildFieldData(Field *field) {
    buf << (char)data_field;
    buf << field->name << ((char)nil);
    buf << field->fullName << ((char)nil);
    buf << putInt32(field->address);
    Int accessTypes = 0;
    for(Int j = 0; j < field->flags.size(); j++) {
        accessTypes |= field->flags.get(j);
    }

    buf << putInt32(field->type);
    buf << putInt32(field->guid);
    buf << putInt32(accessTypes);
    buf << (field->isArray ? 1 : 0);
    buf << (field->locality == stl_thread ? 1 : 0);
    putSymbol(field->utype, buf);
    buf << putInt32(field->owner->address);
    buf << ((char)nil) << ((char)nil);
}

void ExeBuilder::buildFieldData(ClassObject *klass) {
    List<Field*> instanceFields;

    ClassObject *tmp = klass;
    while(tmp != NULL) {
        for(Int i = 0; i < tmp->fieldCount(); i++) {
            if(!tmp->getField(i)->flags.find(STATIC)) {
                instanceFields.add(tmp->getField(i));
            }
        }

        tmp = tmp->getSuperClass();
    }


    instanceFields.linearSort(sortFields);
    for (Int i = 0; i < instanceFields.size(); i++) {
        Field *field = instanceFields.get(i);
        buildFieldData(field);
    }

    for (Int i = 0; i < klass->fieldCount(); i++) {
        Field *field = klass->getField(i);
        if(field->flags.find(STATIC)) {
            buildFieldData(field);
        }
    }
}

void ExeBuilder::buildStringSection() {
    buf << (char)sstring;

    for(Int i = 0; i < compiler->stringMap.size(); i++) {
        buf << (char)data_string;
        buf << putInt32(compiler->stringMap.get(i).size());
        buf << compiler->stringMap.get(i);
    }

    buf << '\n' << (char)eos;
}

void ExeBuilder::buildConstantSection() {
    buf << (char)sconst;

    for(Int i = 0; i < compiler->constantMap.size(); i++) {
        buf << (char)data_const;
        buf << std::scientific
            << std::setprecision(std::numeric_limits<double>::digits10 + 1);
        buf << compiler->constantMap.get(i);
        buf << std::dec << ((char)nil);
    }

    buf << '\n' << (char)eos;
}

void ExeBuilder::addFunctionPointers() {

    for(Int i = 0; i < allClasses.size(); i++) {
        ClassObject *klass = allClasses.get(i);

        for(Int j = 0; j < klass->fieldCount(); j++) {
            if(klass->getField(j)->type == FNPTR) {
                if(functionPointers.indexof(locateMethod, klass->getField(j)->utype->getMethod()) == -1) {
                    functionPointers.add(klass->getField(j)->utype->getMethod());
                }
            }
        }

        for(Int j = 0; j < klass->getFunctionCount(); j++) {
            Method *fun = klass->getFunction(j);
            if(fun->utype->getResolvedType()->type == FNPTR) {
                if(functionPointers.indexof(locateMethod, fun->utype->getMethod()) == -1) {
                    functionPointers.add(fun->utype->getMethod());
                }
            }

            for(Int x = 0; x < fun->params.size(); x++) {
                Field *param = fun->params.get(x);
                if(param->type == FNPTR) {
                    if(functionPointers.indexof(locateMethod, param->utype->getMethod()) == -1) {
                        functionPointers.add(param->utype->getMethod());
                    }
                }
            }
        }
    }

    buf << putInt32(functionPointers.size());
    for(Int i = 0; i < functionPointers.size(); i++) {
        Method *fun = functionPointers.get(i);
        buf << (char)data_symbol;

        buf << putInt32(fun->params.size());
        for(Int j = 0; j < fun->params.size(); j++) {
            Field *param = fun->params.get(j);
            putSymbol(param->utype, buf);
            buf << (param->isArray ? 1 : 0);
        }

        putSymbol(fun->utype, buf);
        buf << (fun->utype->isArray() ? 1 : 0);
    }
}

void ExeBuilder::putSymbol(Utype *utype, stringstream &buf) {
    buf << putInt32(utype->getResolvedType()->type);
    if(utype->getResolvedType()->type == CLASS) {
        buf << putInt32(utype->getClass()->address);
    } else if(utype->getResolvedType()->type == FNPTR) {
        Method *fun = utype->getMethod();
        buf << putInt32(functionPointers.indexof(locateMethod, fun));
    }
}

void ExeBuilder::buildDataSection() {
    dataSec << (char)sdata;

    for(Int i = 0; i < allMethods.size(); i++) {
        putMethodData(allMethods.get(i));
    }

    for(Int i = 0; i < allMethods.size(); i++) {
        putMethodCode(allMethods.get(i));
    }


    dataSec << '\n' << (char)eos;
}

void ExeBuilder::putMethodData(Method *fun) {
    dataSec << (char)data_method;
    dataSec << putInt32(fun->address);
    dataSec << putInt32(fun->guid);
    dataSec << fun->name << ((char)nil);
    dataSec << fun->fullName << ((char)nil);
    dataSec << putInt32(Obfuscater::files.indexof(fun->meta.file));
    dataSec << putInt32(fun->owner->address);
    dataSec << putInt32(fun->fnType);
    dataSec << putInt32(fun->data.localVariablesSize);
    dataSec << putInt32(fun->data.code.size());

    Int accessTypes = 0;
    for(Int j = 0; j < fun->flags.size(); j++) {
        accessTypes |= fun->flags.get(j);
    }
    dataSec << putInt32(accessTypes);
    dataSec << putInt32(fun->delegateAddr);
    dataSec << putInt32(getFpOffset(fun));
    dataSec << putInt32(getSpOffset(fun));
    dataSec << putInt32(getSecondarySpOffset(fun));
    putSymbol(fun->utype, dataSec);
    dataSec << (fun->utype->isArray() ? 1 : 0);
    dataSec << putInt32(fun->params.size());
    for(Int i = 0; i < fun->params.size(); i++) {
        Field *param = fun->params.get(i);
        putSymbol(fun->utype, dataSec);
        dataSec << (param->isArray ? 1 : 0);
    }


    dataSec << putInt32(fun->data.lineTable.size());
    for(Int i = 0; i < fun->data.lineTable.size(); i++) {
        dataSec << putInt32(fun->data.lineTable.get(i).start_pc);
        dataSec << putInt32(fun->data.lineTable.get(i).line);
    }

    dataSec << putInt32(fun->data.tryCatchTable.size());
    for(Int i = 0; i < fun->data.tryCatchTable.size(); i++) {
        TryCatchData &tryCatchData = fun->data.tryCatchTable.get(i);
        dataSec << putInt32(tryCatchData.try_start_pc);
        dataSec << putInt32(tryCatchData.try_end_pc);
        dataSec << putInt32(tryCatchData.block_start_pc);
        dataSec << putInt32(tryCatchData.block_end_pc);

        dataSec << putInt32(tryCatchData.catchTable.size());
        for(Int j = 0; j < tryCatchData.catchTable.size(); j++) {
            CatchData &catchData = tryCatchData.catchTable.get(j);
            dataSec << putInt32(catchData.handler_pc);
            dataSec << putInt32(catchData.localFieldAddress);
            dataSec << putInt32(catchData.classAddress);
        }

        if(tryCatchData.finallyData != NULL) {
            dataSec << ((char)1);
            dataSec << putInt32(tryCatchData.finallyData->start_pc);
            dataSec << putInt32(tryCatchData.finallyData->end_pc);
            dataSec << putInt32(tryCatchData.finallyData->exception_object_field_address);
        } else
            dataSec << ((char)nil);
    }
}

void ExeBuilder::putMethodCode(Method *fun) {
    dataSec << (char)data_byte;

    for(Int i = 0; i < fun->data.code.size(); i++) {
        dataSec << putInt32(fun->data.code.ir32.get(i));
    }
}

void ExeBuilder::buildMetaDataSection() {
    dataSec << (char)smeta;

    for(Int i = 0; i < Obfuscater::files.size(); i++) {
        dataSec << (char)data_file;
        dataSec << Obfuscater::files.get(i)->name << ((char)nil);

        if(c_options.debug) {
            dataSec << putInt32(compiler->parsers.get(i)->getData().size());
            dataSec << compiler->parsers.get(i)->getData() << ((char)nil);
        }
    }

    dataSec << '\n' << (char)eos;
}

int32_t ExeBuilder::getFpOffset(Method *fun) {
    bool staticFunc = fun->flags.find(STATIC);
    Int paramSize = fun->params.size();
    if(fun->utype->isRelated(compiler->nilUtype)) {
        return paramSize + (!staticFunc ? 0 : -1);
    } else {
        if(staticFunc) {
            if(paramSize == 0) {
                return -1;
            } else {
                return fun->params.size() - 1;
            }
        } else {
            if(paramSize == 0) {
                return 0;
            } else {
                return fun->params.size();
            }
        }
    }
}

int32_t ExeBuilder::getSecondarySpOffset(Method *fun) {
    bool staticFunc = fun->flags.find(STATIC);
    Int paramSize = fun->params.size();
    Int exclusiveStackSize = fun->data.localVariablesSize;
    if(!staticFunc) exclusiveStackSize--;
    Int additionalStackPaceRequires = exclusiveStackSize - paramSize;
    return additionalStackPaceRequires > 0 ? additionalStackPaceRequires : 0;
}

int32_t ExeBuilder::getSpOffset(Method *fun) {
    bool staticFunc = fun->flags.find(STATIC);
    Int paramSize = fun->params.size();
    if(fun->utype->isRelated(compiler->nilUtype)) {
        return paramSize + (!staticFunc ? 1 : 0);
    } else {
        if(staticFunc) {
            if(paramSize == 0) {
                return -1;
            } else {
                return fun->params.size() - 1;
            }
        } else {
            if(paramSize == 0) {
                return 0;
            } else {
                return fun->params.size();
            }
        }
        return fun->params.size();
    }
}

