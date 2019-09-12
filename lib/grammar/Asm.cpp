//
// Created by BraxtonN on 2/27/2018.
//

#include "Asm.h"
#include "Asm.h"
#include "parser/tokenizer/tokenizer.h"
#include "parser/ErrorManager.h"
#include "parser/tokenizer/tokenizer.h"
#include "Runtime.h"
#include "../runtime/Opcode.h"
#include "../runtime/register.h"

bool Asm::isend() {
    return npos >= tk->getEntities().size() || current().getTokenType() == _EOF;
}

bool Asm::instruction_is(string name) {
    if(current().getId() == IDENTIFIER && current().getToken() == name) {
        npos++;
        expect_instr = false;
        return true;
    }

    return false;
}

string Asm::expect_identifier() {
    if(current().getId() == IDENTIFIER) {
        string name = current().getToken();
        npos++;
        return name;
    } else {
        tk->getErrors()->createNewError(GENERIC, current(), "expected identifier");
        return "";
    }
}

bool Asm::label_exists(string label) {
    for(unsigned int i = 0; i < label_map->size(); i++) {
        if(label_map->get(i).key == label)
            return true;
    }

    return false;
}

int64_t Asm::current_address() {
    return assembler->__asm64.size();
}

int64_t Asm::get_label(string label) {
    for(unsigned int i = 0; i < label_map->size(); i++) {
        if(label_map->get(i).key == label)
            return label_map->get(i).value;
    }

    return 0;
}

void Asm::expect_register() {
    if(current() == "%")
        npos++;

    if(current() == "adx") {
        i2.high_bytes = i64adx;
    } else if(current() == "cx") {
        i2.high_bytes = i64cx;
    } else if(current() == "cmt") {
        i2.high_bytes = i64cmt;
    } else if(current() == "ebx") {
        i2.high_bytes = i64ebx;
    } else if(current() == "ecx") {
        i2.high_bytes = i64ecx;
    } else if(current() == "ecf") {
        i2.high_bytes = i64ecf;
    } else if(current() == "edf") {
        i2.high_bytes = i64edf;
    } else if(current() == "ehf") {
        i2.high_bytes = i64ehf;
    } else if(current() == "bmr") {
        i2.high_bytes = i64bmr;
    } else if(current() == "egx") {
        i2.high_bytes = i64egx;
    } else {
        // error
        tk->getErrors()->createNewError(GENERIC, current(), "symbol `" + current().getToken() + "` is not a register");
    }

    npos++;
    i2.low_bytes = -1;
}

int Asm::get_offset() {
    if(current() == "+") {
        npos++;
        expect_int();

        return i2.high_bytes;
    } else if(current() == "-") {
        npos++;
        expect_int();

        return -i2.high_bytes;
    } else {
        return 0;
    }
}

bool Asm::hex_int(string s) {
    return s.size() > 2 && s.at(0) == '0' && s.at(1) == 'x';
}

void Asm::expect_int() {
    bool hash = false;
    if(current() == "#" && current().getId() != CHAR_LITERAL) {
        hash = true;
        npos++;
    }

    if(current().getId() == INTEGER_LITERAL || current().getId() == HEX_LITERAL) {
        double x;
        string int_string = RuntimeEngine::invalidateUnderscores(current().getToken());

        if(all_integers(int_string) || hex_int(int_string)) {
            x = std::strtoll (int_string.c_str(), NULL, 0);
            if(x > DA_MAX || x < DA_MIN) {
                stringstream ss;
                ss << "integral number too large: " + int_string;
                tk->getErrors()->createNewError(GENERIC, current(), ss.str());
            }
            i2.high_bytes = (int64_t )x;
            i2.low_bytes = -1;
        }else {
            x = std::strtod (int_string.c_str(), NULL);
            if((int64_t )x > DA_MAX || (int64_t )x < DA_MIN) {
                stringstream ss;
                ss << "integral number too large: " + int_string;
                tk->getErrors()->createNewError(GENERIC, current(), ss.str());
            }

            i2.high_bytes = (int64_t )x;

            if(current().getToken().find('.') != string::npos || current().getToken().find('e') != string::npos
               || current().getToken().find('E') != string::npos)
                i2.low_bytes = abs(RuntimeEngine::get_low_bytes(x));
            else
                i2.low_bytes = -1;
        }
    } else if(current().getId() == CHAR_LITERAL) {
        if(hash)
            tk->getErrors()->createNewError(GENERIC, current(), "invalid format, symbol '#' requires integer literal following it");

        if(current().getToken().size() > 1) {
            switch(current().getToken().at(1)) {
                case 'n':
                    i2.high_bytes = '\n';
                    break;
                case 't':
                    i2.high_bytes = '\t';
                    break;
                case 'b':
                    i2.high_bytes = '\b';
                    break;
                case 'v':
                    i2.high_bytes = '\v';
                    break;
                case 'r':
                    i2.high_bytes = '\r';
                    break;
                case 'f':
                    i2.high_bytes = '\f';
                    break;
                case '\\':
                    i2.high_bytes = '\\';
                    break;
                default:
                    i2.high_bytes = current().getToken().at(1);
                    break;
            }
            i2.low_bytes = -1;
        } else {
            i2.high_bytes = current().getToken().at(0);
            i2.low_bytes = -1;
        }
    } else if(current().getToken() == "true" || current().getToken() == "false") {
        if(hash)
            tk->getErrors()->createNewError(GENERIC, current(), "invalid format, symbol '#' requires integer literal following it");

        if(current().getToken() == "true") {
            i2.high_bytes = 1;
            i2.low_bytes = -1;
        } else {
            i2.high_bytes = 0;
            i2.low_bytes = -1;
        }
    } else {
        // error
        i2.high_bytes = 0;
        i2.low_bytes = -1;
        tk->getErrors()->createNewError(GENERIC, current(), "expected integer literal");
    }

    npos++;
}

void Asm::expect(string token) {
    if(current() == token) {
        npos++;
    } else {
        tk->getErrors()->createNewError(GENERIC, current(), "expected `" + token + "`");
    }
}

List<string> Asm::parse_modulename() {
    List<string> name;
    name.add(current().getToken());

    npos++;
    while(current().getTokenType() == DOT) {
        name.add(current().getToken());
        npos++;

        name.add(expect_identifier());
    }

    return name;
}

Method* Asm::getScopedMethod(ClassObject* klass, string method, int64_t _offset, long line, long col) {

    Method* func;

    if((func = klass->getFunction(method, _offset))) {
        return func;
    } else if((func = klass->getOverload(RuntimeEngine::stringToOp(method), _offset))) {
        return func;
    } else if(method == klass->getName()) {
        if(_offset < klass->constructorCount()) {
            return klass->getConstructor(_offset);
        }
    }

    return NULL;
}

void Asm::removeDots(List<string>& lst) {
    List<string> slst;
    for(unsigned int i = 0; i < lst.size(); i++) {
        if(lst.at(i) == ".") {
            continue;
        } else {
            slst.push_back(lst.get(i));
        }
    }

    lst.addAll(slst);
}

void Asm::removeDots(List<native_string>& lst) {
    readjust:
    for(unsigned int i = 0; i < lst.size(); i++) {
        if(lst.at(i) == ".") {
            lst.get(i).free();
            lst.remove(i);
            goto readjust;
        }
    }
}

void Asm::expect_function() {
    if(!(current().getId() == IDENTIFIER && !Parser::iskeyword(current().getToken()))) {
        tk->getErrors()->createNewError(GENERIC, current(), "expected identifier");
        return;
    }

    List<string> module = parse_modulename();
    List<string> function;

    if(current().getTokenType() == HASH) {
        npos++;

        function.add(expect_identifier());

        while(current().getTokenType() == DOT ) {
            npos++;
            function.add(expect_identifier());
        }
    }

    int64_t offset = 0;
    if(current() == "+") {
        npos++;
        expect_int();

        offset = i2.high_bytes;
    }

    string module_name = "";
    if(function.size() > 0) {
        removeDots(function);

        for(unsigned int i = 0; i < module.size(); i++) {
            module_name += module.at(i);
        }

        module.addAll(function);
    } else {
        removeDots(module);
    }

    Method* method;
    if(module_name == "" && module.size() == 1) {
        string mname = module.at(0);
        if(instance->scopeMap.last().klass != NULL) {
            method = getScopedMethod(instance->scopeMap.last().klass, mname, offset, current().getLine(), current().getColumn());

            if(method != NULL) {
                i2.high_bytes = method->address;
                lastMethod = method;
            } else {
                tk->getErrors()->createNewError(COULD_NOT_RESOLVE, current(), " `" + mname + "`");
                return;
            }
        } else {
            tk->getErrors()->createNewError(COULD_NOT_RESOLVE, current(), " `" + mname + "`");
            return;
        }
    } else {
        ClassObject* klass = instance->getClass(module_name, module.get(0), instance->classes);

        if(klass != NULL) {
            for(unsigned int i = 1; i < module.size() - 1; i++) {
                if((klass = klass->getChildClass(module.get(i))) == NULL) {
                    tk->getErrors()->createNewError(COULD_NOT_RESOLVE, current(), " `" + module.get(i) + "`");
                    return;
                }
            }

            string mname = module.at(module.size()-1);
            method = getScopedMethod(klass, mname, offset, current().getLine(), current().getColumn());

            if(method != NULL) {
                i2.high_bytes = method->address;
                lastMethod = method;
            } else {
                tk->getErrors()->createNewError(COULD_NOT_RESOLVE, current(), " `" + mname + "`");
                return;
            }
        } else {
            tk->getErrors()->createNewError(COULD_NOT_RESOLVE, current(), " `" + module.get(0) + "`");
            return;
        }
    }
}

void Asm::expect_class() {
    if(!(current().getId() == IDENTIFIER && !Parser::iskeyword(current().getToken()))) {
        tk->getErrors()->createNewError(GENERIC, current(), "expected identifier");
        return;
    }

    List<string> module = parse_modulename();
    List<string> klassHeiarchy;

    if(current().getTokenType() == HASH) {
        npos++;

        klassHeiarchy.add(expect_identifier());

        while(current().getTokenType() == DOT ) {
            npos++;
            klassHeiarchy.add(expect_identifier());
        }
    }

    string module_name = "";
    if(klassHeiarchy.size() > 0) {
        removeDots(klassHeiarchy);

        for(unsigned int i = 0; i < module.size(); i++) {
            module_name += module.at(i);
        }

        module.addAll(klassHeiarchy);
    } else {
        removeDots(module);
    }

    ClassObject* klass;
    if(module_name == "" && module.size() == 1) {
        if((klass = instance->getClass(module_name, module.get(0), instance->classes)) != NULL){
            i2.high_bytes = klass->address;
        } else {

            string kname = module.at(0);
            if(instance->scopeMap.last().klass != NULL) {
                klass = instance->scopeMap.last().klass->getChildClass(kname);

                if(klass != NULL) {
                    i2.high_bytes = klass->address;
                } else {
                    tk->getErrors()->createNewError(COULD_NOT_RESOLVE, current(), " `" + kname + "`");
                    return;
                }
            } else {
                tk->getErrors()->createNewError(COULD_NOT_RESOLVE, current(), " `" + kname + "`");
                return;
            }
        }
    } else {
        ClassObject* klass = instance->getClass(module_name, module.at(0), instance->classes);

        if(klass != NULL) {
            for(unsigned int i = 1; i < module.size(); i++) {
                if((klass = klass->getChildClass(module.at(i))) == NULL) {
                    tk->getErrors()->createNewError(COULD_NOT_RESOLVE, current(), " `" + module.at(i) + "`");
                    return;
                }
            }

            if(klass != NULL) {
                i2.high_bytes = klass->address;
            } else {
                tk->getErrors()->createNewError(COULD_NOT_RESOLVE, current(), " `" + module.at(0) + "`");
                return;
            }
        } else {
            tk->getErrors()->createNewError(COULD_NOT_RESOLVE, current(), " `" + module.at(0) + "`");
            return;
        }
    }
}

// TODO: add functionality to search by local variable name to get address
void Asm::parse(Assembler &assembler, RuntimeEngine *instance, string& code, Ast* pAst) {
    if(code == "") return;

    this->assembler = &assembler;
    this->instance = instance;
    this->code = code;
    tk = new tokenizer(code, "stdin");
    label_map = &instance->scopeMap.last().label_map;
    RuntimeNote note = RuntimeNote(instance->activeParser->sourcefile, instance->activeParser->getErrors()->getLine(pAst->line),
                                   pAst->line, pAst->col);
    KeyPair<std::string, int64_t> label;

    // TODO: add smov & smovr instructions
    if(tk->getErrors()->hasErrors())
    {
        cout << note.getNote("Assembler messages:");
        tk->getErrors()->printErrors();

        errors+= tk->getErrors()->getErrorCount();
        uo_errors+= tk->getErrors()->getUnfilteredErrorCount();
    } else {
        /* parse assembly */
        int2_t itmp;

        while(!isend())
        {
            if(instruction_is("nop")) {
                assembler.push_i64(SET_Ei(i64, op_NOP));
            } else if(instruction_is("int")) {
                expect_int();
                assembler.push_i64(SET_Di(i64, op_INT, i2.high_bytes));
            } else if(instruction_is("movi")) {
                if(current() == "$") {
                    int64_t _offset=0, _register=0, address=-1;
                    string name;
                    expect("$");

                    if(current() == "$") {
                        npos++;
                        address = assembler.__asm64.size() == 0 ? 0 : assembler.__asm64.size()-1;
                    } else {
                        name = expect_identifier();
                    }


                    if(current() == "+") {
                        npos++;
                        expect_int();
                        _offset = i2.high_bytes;
                    }

                    expect(",");
                    expect_register();
                    _register=i2.high_bytes;

                    if(address!=-1) {
                        assembler.push_i64(SET_Di(i64, op_MOVI, (address+_offset)), _register);
                    } else {
                        instance->scopeMap.last().addStore(name, _register, _offset,
                                                            assembler, pAst->line, pAst->col);
                    }
                } else if(current() == "<"){
                    expect("<");
                    expect_function();
                    expect(">");

                    itmp = i2;
                    expect(",");
                    expect_register();
                    assembler.push_i64(SET_Di(i64, op_MOVI, itmp.high_bytes), i2.high_bytes);

                } else {
                    if(current() == "#")
                        npos++;
                    expect_int();
                    itmp = i2;
                    expect(",");
                    expect_register();
                    assembler.push_i64(SET_Di(i64, op_MOVI, itmp.high_bytes), i2.high_bytes);
                }
            } else if(instruction_is("ret")) {
                assembler.push_i64(SET_Ei(i64, op_RET));
            } else if(instruction_is("hlt")) {
                assembler.push_i64(SET_Ei(i64, op_HLT));
            } else if(instruction_is("newarray")) {
                expect_register();
                assembler.push_i64(SET_Di(i64, op_NEWARRAY, i2.high_bytes));
            } else if(instruction_is("cast")) {
                if(current() == "<") {
                    expect("<");
                    expect_class();
                    itmp = i2;
                    expect(">");
                    expect(",");
                    expect_register();
                    assembler.push_i64(SET_Di(i64, op_MOVI, itmp.high_bytes), i2.high_bytes);
                } else
                    expect_register();

                assembler.push_i64(SET_Di(i64, op_CAST, i2.high_bytes));
            } else if(instruction_is("var_cast")) {
                i2.high_bytes = 0;
                if(current() == "[") {
                    expect("[");
                    expect("]");
                    i2.high_bytes = 1;
                }
                assembler.push_i64(SET_Di(i64, op_VARCAST, i2.high_bytes));
            } else if(instruction_is("mov8")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_MOV8, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("mov16")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_MOV16, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("mov32")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_MOV32, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("mov64")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_MOV64, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("movu8")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_MOVU8, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("movu16")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_MOVU16, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("movu32")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_MOVU32, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("movu64")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_MOVU64, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("rstore")) {
                expect_register();

                assembler.push_i64(SET_Di(i64, op_RSTORE, i2.high_bytes));
            } else if(instruction_is("add")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_ADD, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));

                expect(",");
                expect_register();
                assembler.push_i64(i2.high_bytes);
            } else if(instruction_is("sub")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_SUB, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));

                expect(",");
                expect_register();
                assembler.push_i64(i2.high_bytes);
            } else if(instruction_is("mul")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_MUL, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));

                expect(",");
                expect_register();
                assembler.push_i64(i2.high_bytes);
            } else if(instruction_is("div")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_DIV, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));

                expect(",");
                expect_register();
                assembler.push_i64(i2.high_bytes);
            } else if(instruction_is("mod")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_MOD, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));

                expect(",");
                expect_register();
                assembler.push_i64(i2.high_bytes);
            } else if(instruction_is("iadd")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_int();

                check_CB();

                assembler.push_i64(SET_Ci(i64, op_IADD, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("isub")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_int();

                check_CB();

                assembler.push_i64(SET_Ci(i64, op_ISUB, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("imul")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_int();

                check_CB();

                assembler.push_i64(SET_Ci(i64, op_IMUL, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("idiv")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_int();

                check_CB();

                assembler.push_i64(SET_Ci(i64, op_IDIV, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("imod")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_int();

                check_CB();

                assembler.push_i64(SET_Ci(i64, op_IMOD, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("pop")) {
                assembler.push_i64(SET_Ei(i64, op_POP));
            } else if(instruction_is("inc")) {
                expect_register();

                assembler.push_i64(SET_Di(i64, op_INC, i2.high_bytes));
            } else if(instruction_is("dec")) {
                expect_register();

                assembler.push_i64(SET_Di(i64, op_DEC, i2.high_bytes));
            } else if(instruction_is("movr")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_MOVR, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("iaload")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_IALOAD, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            }  else if(instruction_is("brh")) {
                assembler.push_i64(SET_Ei(i64, op_BRH));
            } else if(instruction_is("ife")) {
                assembler.push_i64(SET_Ei(i64, op_IFE));
            } else if(instruction_is("ifne")) {
                assembler.push_i64(SET_Ei(i64, op_IFNE));
            } else if(instruction_is("lt")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_LT, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("gt")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_GT, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("le")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_LTE, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("ge")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_GTE, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("movl")) {
                if(current() == "<") {
                    npos++;
                    string local = expect_identifier();

                    if(instance->scopeMap.last().getLocalField(local) == NULL || (i2.high_bytes = instance->scopeMap.last().getLocalField(local)->value.address) == -1)  {
                        tk->getErrors()->createNewError(COULD_NOT_RESOLVE, current(), " `" + local + "`");
                    }
                    expect(">");
                } else {
                    expect_int();
                }

                assembler.push_i64(SET_Di(i64, op_MOVL, i2.high_bytes));
            } else if(instruction_is("movsl")) {
                expect_int();
                assembler.push_i64(SET_Di(i64, op_MOVSL, i2.high_bytes));
            } else if(instruction_is("movbi")) {
                expect_int();

                if(i2.low_bytes != -1) {
                    assembler.push_i64(SET_Di(i64, op_MOVBI, i2.high_bytes), i2.low_bytes);
                } else {
                    itmp = i2;
                    expect(",");
                    expect_int();

                    assembler.push_i64(SET_Di(i64, op_MOVBI, itmp.high_bytes), i2.high_bytes);
                }

            } else if(instruction_is("sizeof")) {
                expect_register();

                assembler.push_i64(SET_Di(i64, op_SIZEOF, i2.high_bytes));
            } else if(instruction_is("put")) {
                expect_register();

                assembler.push_i64(SET_Di(i64, op_PUT, i2.high_bytes));
            } else if(instruction_is("putc")) {
                expect_register();

                assembler.push_i64(SET_Di(i64, op_PUTC, i2.high_bytes));
            } else if(instruction_is("get")) {
                expect_register();

                assembler.push_i64(SET_Di(i64, op_GET, i2.high_bytes));
            } else if(instruction_is("checklen")) {
                expect_register();

                assembler.push_i64(SET_Di(i64, op_CHECKLEN, i2.high_bytes));
            } else if(instruction_is("goto")) {
                expect("$");
                string name = expect_identifier();
                int64_t _offset=0;

                if(current() == "+") {
                    npos++;

                    int64_t adx = get_label(name);
                    expect_int();

                    _offset = i2.high_bytes;
                    i2.high_bytes=adx;
                } else {
                    i2.high_bytes=get_label(name);
                }

                if(label_exists(name)) {
                    assembler.push_i64(SET_Di(i64, op_GOTO, (i2.high_bytes+_offset)));
                } else {
                    instance->scopeMap.last().addBranch(name, _offset, assembler, pAst->line, pAst->col);
                }
            } else if(instruction_is("loadpc")) {
                expect_register();

                assembler.push_i64(SET_Di(i64, op_LOADPC, i2.high_bytes));
            } else if(instruction_is("pushobj")) {
                assembler.push_i64(SET_Ei(i64, op_PUSHOBJ));
            } else if(instruction_is("del")) {
                assembler.push_i64(SET_Ei(i64, op_DEL));
            } else if(instruction_is("call")) {
                expect("<");
                expect_function();
                expect(">");
                assembler.push_i64(SET_Di(i64, op_CALL, i2.high_bytes));
            } else if(instruction_is("calld")) {
                expect_register();
                assembler.push_i64(SET_Di(i64, op_CALL, i2.high_bytes));
            } else if(instruction_is("new_class")) {
                expect("<");
                expect_class();
                expect(">");
                assembler.push_i64(SET_Di(i64, op_NEWCLASS, i2.high_bytes));
            } else if(instruction_is("movn")) {
                expect_int();

                assembler.push_i64(SET_Di(i64, op_MOVN, i2.high_bytes));
            }  else if(instruction_is("sleep")) {
                expect_register();

                assembler.push_i64(SET_Di(i64, op_SLEEP, i2.high_bytes));
            } else if(instruction_is("test")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_TEST, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("tne")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_TNE, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("_lock")) {
                assembler.push_i64(SET_Ei(i64, op_LOCK));
            } else if(instruction_is("_ulock")) {
                assembler.push_i64(SET_Ei(i64, op_ULOCK));
            } else if(instruction_is("skp")) {
                expect_int();

                assembler.push_i64(SET_Di(i64, op_SKIP, i2.high_bytes));
            } else if(instruction_is("skpe")) {
                expect_int();

                assembler.push_i64(SET_Di(i64, op_SKPE, i2.high_bytes));
            } else if(instruction_is("skpne")) {
                expect_int();

                assembler.push_i64(SET_Di(i64, op_SKNE, i2.high_bytes));
            } else if(instruction_is("exp")) {
                expect_register();

                assembler.push_i64(SET_Di(i64, op_EXP, i2.high_bytes));
            } else if(instruction_is("movnd")) {
                expect_register();

                assembler.push_i64(SET_Di(i64, op_MOVND, i2.high_bytes));
            }  else if(instruction_is("newobjarry")) {
                expect_register();

                assembler.push_i64(SET_Di(i64, op_NEWOBJARRAY, i2.high_bytes));
            } else if(instruction_is("not")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_NOT, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(current() == ".") {
                npos++;

                string name = expect_identifier();
                if(!label_exists(name)) {
                    label.set(name, current_address());
                    label_map->add(label);
                } else {
                    npos--;
                    tk->getErrors()->createNewError(GENERIC, current(), "redefinition of label `" + name + "`");
                    npos++;
                }

                expect_instr = true;
                expect(":");
            } else if(instruction_is("loadval")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_int();

                assembler.push_i64(SET_Ci(i64, op_LOADVAL, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("shl")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_SHL, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));

                expect(",");
                expect_register();
                assembler.push_i64(i2.high_bytes);
            } else if(instruction_is("shr")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_SHR, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));

                expect(",");
                expect_register();
                assembler.push_i64(i2.high_bytes);
            } else if(instruction_is("and")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_AND, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("uand")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_UAND, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("or")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_OR, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("xor")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_XOR, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("throw")) {
                assembler.push_i64(SET_Ei(i64, op_THROW));
            } else if(instruction_is("check_null")) {
                assembler.push_i64(SET_Ei(i64, op_CHECKNULL));
            } else if(instruction_is("return_obj")) {
                assembler.push_i64(SET_Ei(i64, op_RETURNOBJ));
            } else if(instruction_is("new_classarry")) {
                expect_register();
                itmp = i2;
                expect(",");

                expect("<");
                expect_class();
                expect(">");

                assembler.push_i64(SET_Ci(i64, op_NEWCLASSARRAY, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("new_string")) {
                expect_int();

                assembler.push_i64(SET_Di(i64, op_NEWSTRING, i2.high_bytes));
            } else if(instruction_is("addl")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_int();

                assembler.push_i64(SET_Ci(i64, op_ADDL, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("subl")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_int();

                assembler.push_i64(SET_Ci(i64, op_SUBL, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("mull")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_int();

                assembler.push_i64(SET_Ci(i64, op_MULL, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("divl")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_int();

                assembler.push_i64(SET_Ci(i64, op_DIVL, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("modl")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_int();

                assembler.push_i64(SET_Ci(i64, op_MODL, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("iaddl")) {
                expect_int();
                itmp = i2;
                expect(",");
                expect_int();

                assembler.push_i64(SET_Ci(i64, op_IADDL, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("isubl")) {
                expect_int();
                itmp = i2;
                expect(",");
                expect_int();

                assembler.push_i64(SET_Ci(i64, op_ISUBL, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("imull")) {
                expect_int();
                itmp = i2;
                expect(",");
                expect_int();

                assembler.push_i64(SET_Ci(i64, op_IMULL, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("idivl")) {
                expect_int();
                itmp = i2;
                expect(",");
                expect_int();

                assembler.push_i64(SET_Ci(i64, op_IDIVL, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("imodl")) {
                expect_int();
                itmp = i2;
                expect(",");
                expect_int();

                assembler.push_i64(SET_Ci(i64, op_IMODL, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("loadl")) {
                expect_register();
                itmp = i2;
                expect(",");
                if(current() == "<") {
                    npos++;
                    string local = expect_identifier();

                    if(instance->scopeMap.last().getLocalField(local) == NULL ||  (i2.high_bytes = instance->scopeMap.last().getLocalField(local)->value.address) == -1)  {
                        tk->getErrors()->createNewError(COULD_NOT_RESOLVE, current(), " `" + local + "`");
                    }
                    expect(">");
                } else {
                    expect_int();
                }

                assembler.push_i64(SET_Ci(i64, op_LOADL, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("iaload_2")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_IALOAD_2, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("popobj")) {
                assembler.push_i64(SET_Ei(i64, op_POPOBJ));
            } else if(instruction_is("smovr_2")) {
                expect_register();
                itmp = i2;
                expect(",");
                if(current() == "<") {
                    npos++;
                    string local = expect_identifier();

                    if(instance->scopeMap.last().getLocalField(local) == NULL || (i2.high_bytes = instance->scopeMap.last().getLocalField(local)->value.address) == -1)  {
                        tk->getErrors()->createNewError(COULD_NOT_RESOLVE, current(), " `" + local + "`");
                    }
                    expect(">");
                } else {
                    expect_int();
                }

                assembler.push_i64(SET_Ci(i64, op_SMOVR_2, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("andl")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_int();

                assembler.push_i64(SET_Ci(i64, op_ANDL, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("orl")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_int();

                assembler.push_i64(SET_Ci(i64, op_ORL, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("xorl")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_int();

                assembler.push_i64(SET_Ci(i64, op_XORL, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("rmov")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_register();

                assembler.push_i64(SET_Ci(i64, op_RMOV, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("smov")) {
                expect_register();
                itmp = i2;
                expect(",");
                expect_int();

                assembler.push_i64(SET_Ci(i64, op_SMOV, abs(itmp.high_bytes), (itmp.high_bytes<0), i2.high_bytes));
            } else if(instruction_is("return_val")) {
                expect_register();

                assembler.push_i64(SET_Di(i64, op_RETURNVAL, i2.high_bytes));
            } else if(instruction_is("istore")) {
                expect_int();

                assembler.push_i64(SET_Di(i64, op_ISTORE, i2.high_bytes));
            } else if(instruction_is("istorel")) {
                expect_int();
                itmp = i2;
                expect(",");
                expect_int();

                assembler.push_i64(SET_Di(i64, op_ISTOREL, itmp.high_bytes), i2.high_bytes);
            } else if(instruction_is("pushnil")) {
                assembler.push_i64(SET_Ei(i64, op_PUSHNIL));
            } else if(instruction_is("ipushl")) {
                if(current() == "<") {
                    npos++;
                    string local = expect_identifier();

                    if(instance->scopeMap.last().getLocalField(local) == NULL || (i2.high_bytes = instance->scopeMap.last().getLocalField(local)->value.address) == -1)  {
                        tk->getErrors()->createNewError(COULD_NOT_RESOLVE, current(), " `" + local + "`");
                    }
                    expect(">");
                } else {
                    expect_int();
                }

                assembler.push_i64(SET_Di(i64, op_IPUSHL, i2.high_bytes));
            } else if(instruction_is("pushl")) {
                if(current() == "<") {
                    npos++;
                    string local = expect_identifier();

                    if(instance->scopeMap.last().getLocalField(local) == NULL || (i2.high_bytes = instance->scopeMap.last().getLocalField(local)->value.address) == -1)  {
                        tk->getErrors()->createNewError(COULD_NOT_RESOLVE, current(), " `" + local + "`");
                    }
                    expect(">");
                } else {
                    expect_int();
                }

                assembler.push_i64(SET_Di(i64, op_PUSHL, i2.high_bytes));
            } else if(instruction_is("popl")) {
                if(current() == "<") {
                    npos++;
                    string local = expect_identifier();

                    if(instance->scopeMap.last().getLocalField(local) == NULL || (i2.high_bytes = instance->scopeMap.last().getLocalField(local)->value.address) == -1)  {
                        tk->getErrors()->createNewError(COULD_NOT_RESOLVE, current(), " `" + local + "`");
                    }
                    expect(">");
                } else {
                    expect_int();
                }

                assembler.push_i64(SET_Di(i64, op_POPL, i2.high_bytes));
            } else if(instruction_is("ipopl")) {
                if(current() == "<") {
                    npos++;
                    string local = expect_identifier();

                    if(instance->scopeMap.last().getLocalField(local) == NULL || (i2.high_bytes = instance->scopeMap.last().getLocalField(local)->value.address) == -1)  {
                        tk->getErrors()->createNewError(COULD_NOT_RESOLVE, current(), " `" + local + "`");
                    }
                    expect(">");
                } else {
                    expect_int();
                }

                assembler.push_i64(SET_Di(i64, op_IPOPL, i2.high_bytes));
            } else if(instruction_is("itest")) {
                expect_int();

                assembler.push_i64(SET_Di(i64, op_ITEST, i2.high_bytes));
            }  else if(instruction_is("inv_del")) {

                expect("<");
                expect_function();
                itmp = i2;
                expect(">");

                expect_int();

                assembler.push_i64(SET_Ci(i64, op_INVOKE_DELEGATE, itmp.high_bytes, (itmp.high_bytes<0), i2.high_bytes));
            }   else if(instruction_is("inv_del_static")) {

                expect("<");
                expect_function();
                itmp = i2;
                expect(">");
                expect(",");
                expect_int();

                assembler.push_i64(SET_Ci(i64, op_INVOKE_DELEGATE_STATIC, itmp.high_bytes, (itmp.high_bytes<0), i2.high_bytes), (lastMethod ? lastMethod->owner->address : 0));
            }   else if(instruction_is("isadd")) {

                expect_int();
                itmp = i2;
                expect(",");
                expect_int();

                assembler.push_i64(SET_Ci(i64, op_ISADD, itmp.high_bytes, (itmp.high_bytes<0), i2.high_bytes));
            }  else if(instruction_is("cmp")) {

                expect_register();
                itmp = i2;
                expect(",");
                expect_int();

                assembler.push_i64(SET_Ci(i64, op_CMP, itmp.high_bytes, (itmp.high_bytes<0), i2.high_bytes));
            }   else if(instruction_is("je")) {

                expect_int();

                assembler.push_i64(SET_Di(i64, op_JE, i2.high_bytes));
            }   else if(instruction_is("jne")) {
                expect_int();

                assembler.push_i64(SET_Di(i64, op_JNE, i2.high_bytes));
            }    else if(instruction_is("tls_movl")) {
                if(current() == "<") {
                    npos++;
                    string local = expect_identifier();

                    if(instance->scopeMap.last().getLocalField(local) == NULL || (i2.high_bytes = instance->scopeMap.last().getLocalField(local)->value.address) == -1)  {
                        tk->getErrors()->createNewError(COULD_NOT_RESOLVE, current(), " `" + local + "`");
                    }
                    expect(">");
                } else {
                    expect_int();
                }

                assembler.push_i64(SET_Di(i64, op_TLS_MOVL, i2.high_bytes));
            } else if(instruction_is("dup")) {
                assembler.push_i64(SET_Ei(i64, op_DUP));
            } else if(instruction_is("popobj_2")) {
                assembler.push_i64(SET_Ei(i64, op_POPOBJ_2));
            } else if(instruction_is("swap")) {
                assembler.push_i64(SET_Ei(i64, op_SWAP));
            } else {
                if(current() == "<") {
                    npos++;
                    string local = expect_identifier();

                    if(instance->scopeMap.last().getLocalField(local) == NULL || (i2.high_bytes = instance->scopeMap.last().getLocalField(local)->value.address) == -1)  {
                        tk->getErrors()->createNewError(COULD_NOT_RESOLVE, current(), " `" + local + "`");
                    }
                    expect(">");
                } else
                    npos++;

                tk->getErrors()->createNewError(GENERIC, current(), "expected instruction");
            }
        }

        if(expect_instr) {
            tk->getErrors()->createNewError(GENERIC, current(), "expected instruction");
        }

        if(tk->getErrors()->hasErrors())
        {

            cout << note.getNote("Assembler messages:");
            tk->getErrors()->printErrors();

            instance->errorCount+= tk->getErrors()->getErrorCount();
            instance->unfilteredErrorCount+= tk->getErrors()->getUnfilteredErrorCount();
        }
    }

    tk->free();
    this->code = "";
}

void Asm::check_CB() {
    if(i2.high_bytes > CA_MAX || i2.high_bytes < CA_MIN) {
        stringstream ss;
        ss << "integral number too large: " << i2.high_bytes;
        tk->getErrors()->createNewError(GENERIC, current(), ss.str());
    }
}

token_entity Asm::current() {
    return tk->getEntities().get(npos);
}

string Asm::registrerToString(int64_t r) {
    switch(r) {
        case i64adx:
            return "adx";
        case i64cx:
            return "cx";
        case i64cmt:
            return "cmt";
        case i64ebx:
            return "ebx";
        case i64ecx:
            return "ecx";
        case i64ecf:
            return "ecf";
        case i64edf:
            return "edf";
        case i64ehf:
            return "ehf";
        case i64bmr:
            return "bmr";
        case i64egx:
            return "egx";
        default: {
            stringstream ss;
            ss << "? (" << r << ")";
            return ss.str();
        }
    }
}
