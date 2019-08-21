//
// Created by BraxtonN on 2/27/2018.
//

#ifndef SHARP_ASM_H
#define SHARP_ASM_H

#include "Assembler.h"
#include "parser/tokenizer/tokenentity.h"
#include "Method.h"
#include "../runtime/oo/string.h"

class RuntimeEngine;
class Errors;
class tokenizer;
class Ast;

struct int2_t {
    int64_t high_bytes;
    int64_t low_bytes;
};

class Asm {

public:
    Asm()
            :
            npos(0),
            instance(NULL),
            code(""),
            expect_instr(false),
            lastMethod(NULL)
    {
    }

    void parse(Assembler& assembler, RuntimeEngine* instance, string& code, Ast* pAst);

    static string registrerToString(int64_t);

    int64_t errors, uo_errors;
private:
    int64_t npos, i64;
    int2_t i2;
    Assembler* assembler;
    RuntimeEngine* instance;
    Method* lastMethod;
    string code;
    tokenizer* tk;
    bool expect_instr;
    List<KeyPair<std::string, int64_t>>* label_map;

    bool isend();

    token_entity current();

    bool instruction_is(string name);

    void expect_int();

    void expect(string token);

    string expect_identifier();

    bool label_exists(string label);

    int64_t current_address();

    int64_t get_label(string name);

    void expect_function();

    List<string> parse_modulename();

    Method *getScopedMethod(ClassObject *klass, string method, int64_t _offset, long line, long col);

    void removeDots(List<string>& lst);

    void removeDots(List<native_string>& lst);

    void expect_class();

    int get_offset();

    void check_CB();

    bool hex_int(string s);

    void expect_register();
};

#endif //SHARP_ASM_H
