//
// Created by bknun on 8/29/2022.
//

#include "asm_statement.h"
#include "../../../types/types.h"
#include "../../../../../core/opcode/opcode.h"
#include "../../expressions/expression.h"
#include "../../expressions/primary/literal_expression.h"
#include "../../../dependency/dependancy.h"


opcode_arg get_asm_offset(Ast *ast) {
    opcode_arg offset = 0;
    if(ast->hasToken(PLUS) || ast->hasToken(MINUS)) {
        if(ast->getToken(3).getId() == INTEGER_LITERAL) {
            string hex_string = remove_underscores(ast->getToken(3).getValue());

#if _ARCH_BITS == 64
            offset = strtoll(hex_string.c_str(), NULL, 16);
#else
            offset = strtol(hex_string.c_str(), NULL, 16);
#endif
        } else {
            string hex_string = remove_underscores(ast->getToken(3).getValue());

#if _ARCH_BITS == 64
            offset = strtod(hex_string.c_str(), NULL);
#else
            offset = strtod(hex_string.c_str(), NULL);
#endif
        }
    }

    return offset;
}

string to_lower(string s) {
    string newstr = "";
    for(char c : s) {
        newstr += tolower(c);
    }
    return newstr;
}

machine_argument* compile_asm_register(Ast *ast) {
    if(to_lower(ast->getToken(0).getValue()) == "adx") {
        return new machine_argument(type_register, ADX);
    } else if(to_lower(ast->getToken(0).getValue()) == "cx") {
        return new machine_argument(type_register, CX);
    } else if(to_lower(ast->getToken(0).getValue()) == "cmt") {
        return new machine_argument(type_register, CMT);
    } else if(to_lower(ast->getToken(0).getValue()) == "ebx") {
        return new machine_argument(type_register, EBX);
    } else if(to_lower(ast->getToken(0).getValue()) == "ecx") {
        return new machine_argument(type_register, ECX);
    } else if(to_lower(ast->getToken(0).getValue()) == "ecf") {
        return new machine_argument(type_register, ECF);
    } else if(to_lower(ast->getToken(0).getValue()) == "edf") {
        return new machine_argument(type_register, EDF);
    } else if(to_lower(ast->getToken(0).getValue()) == "ehf") {
        return new machine_argument(type_register, EHF);
    } else if(to_lower(ast->getToken(0).getValue()) == "bmr") {
        return new machine_argument(type_register, BMR);
    } else if(to_lower(ast->getToken(0).getValue()) == "egx") {
        return new machine_argument(type_register, EGX);
    } else {
        stringstream err;
        err << ": unexpected asm register: " << to_lower(ast->getToken(0).getValue());
        create_new_error(INTERNAL_ERROR, ast->line, ast->col, err.str());
        return NULL;
    }
}

machine_argument* compile_asm_member_address(sharp_class *with_class, Ast *ast) {
    string memberName = ast->getToken(0).getValue();
    opcode_arg offset = get_asm_offset(ast);
    sharp_field *field;

    if(with_class == NULL) {
        field = resolve_local_field(memberName, &current_context);
        if (field != NULL) {
            if(offset > 0) {
                create_new_error(GENERIC, ast->line, ast->col,
                                 "offsets are not allowed with field members.");
            }

            create_dependency(field);
            return new machine_argument(field);
        }

        with_class = get_primary_class(&current_context);
    }

    field = resolve_field(memberName, with_class, true);
    if(field != NULL) {
        if(offset > 0) {
            create_new_error(GENERIC, ast->line, ast->col,
                             "offsets are not allowed with field members.");
        }

        create_dependency(field);
        return new machine_argument(field);
    }

    List<sharp_function*> functions;
    locate_functions_with_name(memberName, with_class,
                               undefined_function, true, true, functions);


    if (!functions.empty()) {
        if (offset < functions.size()) {
            create_dependency(functions.get(offset));
            return new machine_argument(functions.get(offset));
        } else {
            stringstream err;
            err << "cannot locate function with offset of (" << offset << ")";
            create_new_error(GENERIC, ast->line, ast->col, err.str());
        }
    } else {
        create_new_error(GENERIC, ast->line, ast->col,
                         "attempt to get address of a member `" + memberName +"` when none were found.");
    }

    return NULL;
}

machine_argument* compile_asm_literal(Ast *ast, operation_schema *scheme) {
    machine_argument *arg = NULL;
    Int multiplier = ast->hasSubAst(ast_pre_inc_e) ? -1 : 1;

    if(ast->hasToken("@")) {
        opcode_arg offset = get_asm_offset(ast);
        string labelName = ast->getToken(1).getValue();
        sharp_label *label = resolve_label(labelName, &current_context);

        if(label ==NULL) {
            label = create_label(labelName, &current_context, ast, scheme);
        }

        arg = new machine_argument(label);
        arg->number = offset;
    } else if(ast->hasToken("$")) {
        Ast *classAst = ast->getSubAst(ast_asm_class_item);
        sharp_type type = resolve(classAst->getSubAst(ast_utype));

        if(type.type == type_class) {
            if(ast->hasSubAst(ast_asm_member_item)) {
                arg = compile_asm_member_address(get_class_type(type), ast->getSubAst(ast_asm_member_item));
            } else {
                arg = new machine_argument(get_class_type(type));
            }
        } else {
            stringstream err;
            err << "invalid asm type: " << type_to_str(type) << ", expected class.";
            create_new_error(GENERIC, ast->line, ast->col, err.str());
        }
    } else if(ast->hasToken("[") && ast->hasSubAst(ast_asm_member_item)) {
        arg = compile_asm_member_address(NULL, ast->getSubAst(ast_asm_member_item));
    } else if(ast->getType() == ast_literal || ast->getTokenCount() == 1) {
        expression tmp;
        compile_literal_expression(&tmp, ast);

        switch(tmp.type.type) {
            case type_integer: {
                arg = new machine_argument(type_number, multiplier * tmp.type.integer);
                break;
            }
            case type_char: {
                arg = new machine_argument(type_number, multiplier * tmp.type._char);
                break;
            }
            case type_bool: {
                arg = new machine_argument(type_number, multiplier * tmp.type._bool);
                break;
            }
            case type_string: {
                arg = new machine_argument(tmp.type._string);
                break;
            }
            default: {
                stringstream err;
                err << ": unexpected asm literal type: " << type_to_str(tmp.type);
                create_new_error(INTERNAL_ERROR, ast->line, ast->col, err.str());
                break;
            }
        }
    } else {
        create_new_error(GENERIC, ast->line, ast->col, "expected asm literal");
    }

    return arg;
}

void compile_asm_instruction(operation_schema *scheme, Ast *branch, string &opcode) {
    if(opcode == "nop") {
        create_machine_instruction_operation(scheme,
                   new machine_instruction(Opcode::NOP));
    } else if(opcode == "int") {
        create_machine_instruction_operation(scheme,
                    new machine_instruction(
                            Opcode::INT,
                            compile_asm_literal(branch->getSubAst(ast_literal), scheme)
                    )
        );
    }  else if(opcode == "movi") {
        create_machine_instruction_operation(scheme,
                   new machine_instruction(
                           Opcode::MOVI,
                           compile_asm_literal(branch->getSubAst(ast_assembly_literal), scheme),
                           compile_asm_register(branch->getSubAst(ast_assembly_register))
                   )
        );
    } else if(opcode == "ret") {
        create_machine_instruction_operation(scheme,
                   new machine_instruction(
                           Opcode::RET,
                           compile_asm_literal(branch->getSubAst(ast_assembly_literal), scheme)
                   )
        );
    } else if(opcode == "hlt") {
        create_machine_instruction_operation(scheme,
                   new machine_instruction(
                           Opcode::HLT
                   )
        );
    } else if(opcode == "newVarArray") {
        create_machine_instruction_operation(scheme,
                  new machine_instruction(
                          Opcode::NEWARRAY,
                          compile_asm_register(branch->getSubAst(ast_assembly_register)),
                          compile_asm_literal(branch->getSubAst(ast_literal), scheme)
                  )
        );
    } else if(opcode == "cast") {
        create_machine_instruction_operation(scheme,
                 new machine_instruction(
                         Opcode::CAST,
                         compile_asm_register(branch->getSubAst(ast_assembly_register))
                 )
        );
    } else if(opcode == "mov8") {
        create_machine_instruction_operation(scheme,
                 new machine_instruction(
                         Opcode::MOV8,
                         compile_asm_register(branch->getSubAst(ast_assembly_register)),
                         compile_asm_register(branch->getSubAst(ast_assembly_register))
                )
        );
    } else if(opcode == "mov16") {
        create_machine_instruction_operation(scheme,
              new machine_instruction(
                      Opcode::MOV16,
                      compile_asm_register(branch->getSubAst(ast_assembly_register)),
                      compile_asm_register(branch->getSubAst(ast_assembly_register))
              )
        );
    } else if(opcode == "mov32") {
        create_machine_instruction_operation(scheme,
              new machine_instruction(
                      Opcode::MOV32,
                      compile_asm_register(branch->getSubAst(ast_assembly_register)),
                      compile_asm_register(branch->getSubAst(ast_assembly_register))
              )
        );
    } else if(opcode == "mov64") {
        create_machine_instruction_operation(scheme,
              new machine_instruction(
                      Opcode::MOV64,
                      compile_asm_register(branch->getSubAst(ast_assembly_register)),
                      compile_asm_register(branch->getSubAst(ast_assembly_register))
              )
        );
    } else if(opcode == "movu8") {
        create_machine_instruction_operation(scheme,
             new machine_instruction(
                     Opcode::MOVU8,
                     compile_asm_register(branch->getSubAst(ast_assembly_register)),
                     compile_asm_register(branch->getSubAst(ast_assembly_register))
             )
        );
    } else if(opcode == "movu16") {
        create_machine_instruction_operation(scheme,
             new machine_instruction(
                     Opcode::MOVU16,
                     compile_asm_register(branch->getSubAst(ast_assembly_register)),
                     compile_asm_register(branch->getSubAst(ast_assembly_register))
             )
        );
    } else if(opcode == "movu32") {
        create_machine_instruction_operation(scheme,
             new machine_instruction(
                     Opcode::MOVU32,
                     compile_asm_register(branch->getSubAst(ast_assembly_register)),
                     compile_asm_register(branch->getSubAst(ast_assembly_register))
             )
        );
    } else if(opcode == "movu64") {
        create_machine_instruction_operation(scheme,
             new machine_instruction(
                     Opcode::MOVU64,
                     compile_asm_register(branch->getSubAst(ast_assembly_register)),
                     compile_asm_register(branch->getSubAst(ast_assembly_register))
             )
        );
    } else if(opcode == "rstore") {
        create_machine_instruction_operation(scheme,
             new machine_instruction(
                     Opcode::RSTORE,
                     compile_asm_register(branch->getSubAst(ast_assembly_register))
             )
        );
    } else if(opcode == "add") {
        create_machine_instruction_operation(scheme,
            new machine_instruction(
                    Opcode::ADD,
                    compile_asm_register(branch->getSubAst(0)),
                    compile_asm_register(branch->getSubAst(1)),
                    compile_asm_register(branch->getSubAst(2))
            )
        );
    } else if(opcode == "sub") {
        create_machine_instruction_operation(scheme,
            new machine_instruction(
                    Opcode::SUB,
                    compile_asm_register(branch->getSubAst(0)),
                    compile_asm_register(branch->getSubAst(1)),
                    compile_asm_register(branch->getSubAst(2))
            )
        );
    } else if(opcode == "mul") {
        create_machine_instruction_operation(scheme,
            new machine_instruction(
                    Opcode::MUL,
                    compile_asm_register(branch->getSubAst(0)),
                    compile_asm_register(branch->getSubAst(1)),
                    compile_asm_register(branch->getSubAst(2))
            )
        );
    } else if(opcode == "div") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::DIV,
                   compile_asm_register(branch->getSubAst(0)),
                   compile_asm_register(branch->getSubAst(1)),
                   compile_asm_register(branch->getSubAst(2))
           )
        );
    } else if(opcode == "mod") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::MOD,
                   compile_asm_register(branch->getSubAst(0)),
                   compile_asm_register(branch->getSubAst(1)),
                   compile_asm_register(branch->getSubAst(2))
           )
        );
    } else if(opcode == "iadd") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::IADD,
                   compile_asm_register(branch->getSubAst(0)),
                   compile_asm_literal(branch->getSubAst(1), scheme)
           )
        );
    } else if(opcode == "isub") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::ISUB,
                   compile_asm_register(branch->getSubAst(0)),
                   compile_asm_literal(branch->getSubAst(1), scheme)
           )
        );
    } else if(opcode == "imul") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::IMUL,
                   compile_asm_register(branch->getSubAst(0)),
                   compile_asm_literal(branch->getSubAst(1), scheme)
           )
        );
    } else if(opcode == "idiv") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::IDIV,
                   compile_asm_register(branch->getSubAst(0)),
                   compile_asm_literal(branch->getSubAst(1), scheme)
           )
        );
    } else if(opcode == "imod") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::IMOD,
                   compile_asm_register(branch->getSubAst(0)),
                   compile_asm_literal(branch->getSubAst(1), scheme)
           )
        );
    } else if(opcode == "pop") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::POP
           )
        );
    } else if(opcode == "inc") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::INC,
                   compile_asm_register(branch->getSubAst(0))
           )
        );
    } else if(opcode == "dec") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::DEC,
                   compile_asm_register(branch->getSubAst(0))
           )
        );
    } else if(opcode == "movr") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::MOVR,
                   compile_asm_register(branch->getSubAst(0)),
                   compile_asm_register(branch->getSubAst(1))
           )
        );
    } else if(opcode == "iaload") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::IALOAD,
                   compile_asm_register(branch->getSubAst(0)),
                   compile_asm_register(branch->getSubAst(1))
           )
        );
    } else if(opcode == "brh") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::BRH
           )
        );
    } else if(opcode == "ife") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::IFE
           )
        );
    } else if(opcode == "ifne") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::IFNE
           )
        );
    } else if(opcode == "lt") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::LT,
                   compile_asm_register(branch->getSubAst(0)),
                   compile_asm_register(branch->getSubAst(1))
           )
        );
    } else if(opcode == "gt") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::GT,
                   compile_asm_register(branch->getSubAst(0)),
                   compile_asm_register(branch->getSubAst(1))
           )
        );
    } else if(opcode == "le") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::LTE,
                   compile_asm_register(branch->getSubAst(0)),
                   compile_asm_register(branch->getSubAst(1))
           )
        );
    } else if(opcode == "ge") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::GTE,
                   compile_asm_register(branch->getSubAst(0)),
                   compile_asm_register(branch->getSubAst(1))
           )
        );
    } else if(opcode == "movl") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::MOVL,
                   compile_asm_literal(branch->getSubAst(0), scheme)
           )
        );
    } else if(opcode == "movsl") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::MOVSL,
                   compile_asm_literal(branch->getSubAst(0), scheme)
           )
        );
    } else if(opcode == "sizeof") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::SIZEOF,
                   compile_asm_register(branch->getSubAst(0))
           )
        );
    } else if(opcode == "put") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::PUT,
                   compile_asm_register(branch->getSubAst(0))
           )
        );
    } else if(opcode == "putc") {
        create_machine_instruction_operation(scheme,
            new machine_instruction(
                    Opcode::PUTC,
                    compile_asm_register(branch->getSubAst(0))
            )
        );
    } else if(opcode == "checklen") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::CHECKLEN,
                   compile_asm_register(branch->getSubAst(0))
           )
        );
    } else if(opcode == "jmp") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::JMP,
                   compile_asm_literal(branch->getSubAst(0), scheme)
           )
        );
    } else if(opcode == "loadpc") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::LOADPC,
                   compile_asm_register(branch->getSubAst(0))
           )
        );
    } else if(opcode == "pushObj") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::PUSHOBJ
           )
        );
    } else if(opcode == "del") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::DEL
           )
        );
    } else if(opcode == "call") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::CALL,
                   compile_asm_literal(branch->getSubAst(0), scheme)
           )
        );
    } else if(opcode == "newClass") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::NEWCLASS,
                   compile_asm_literal(branch->getSubAst(0), scheme)
           )
        );
    } else if(opcode == "movn") {
        create_machine_instruction_operation(scheme,
            new machine_instruction(
                    Opcode::MOVN,
                    compile_asm_literal(branch->getSubAst(0), scheme)
            )
        );
    } else if(opcode == "sleep") {
        create_machine_instruction_operation(scheme,
            new machine_instruction(
                    Opcode::SLEEP,
                    compile_asm_register(branch->getSubAst(0))
            )
        );
    } else if(opcode == "te") {
        create_machine_instruction_operation(scheme,
            new machine_instruction(
                    Opcode::TEST,
                    compile_asm_register(branch->getSubAst(0)),
                    compile_asm_register(branch->getSubAst(1))
            )
        );
    } else if(opcode == "tne") {
        create_machine_instruction_operation(scheme,
            new machine_instruction(
                    Opcode::TNE,
                    compile_asm_register(branch->getSubAst(0)),
                    compile_asm_register(branch->getSubAst(1))
            )
        );
    } else if(opcode == "lock") {
        create_machine_instruction_operation(scheme,
            new machine_instruction(
                    Opcode::LOCK
            )
        );
    } else if(opcode == "ulock") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::ULOCK
           )
        );
    } else if(opcode == "movg") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::MOVG,
                   compile_asm_literal(branch->getSubAst(0), scheme)
           )
        );
    } else if(opcode == "movnd") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::MOVND,
                   compile_asm_register(branch->getSubAst(0))
           )
        );
    } else if(opcode == "newObjArray") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::NEWOBJARRAY,
                   compile_asm_register(branch->getSubAst(0))
           )
        );
    } else if(opcode == "not") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::NOT,
                   compile_asm_register(branch->getSubAst(0)),
                   compile_asm_register(branch->getSubAst(1))
           )
        );
    } else if(opcode == "skip") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::SKIP,
                   compile_asm_literal(branch->getSubAst(0), scheme)
           )
        );
    } else if(opcode == "loadVal") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::LOADVAL,
                   compile_asm_register(branch->getSubAst(0))
           )
        );
    } else if(opcode == "shl") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::SHL,
                   compile_asm_register(branch->getSubAst(0)),
                   compile_asm_register(branch->getSubAst(1)),
                   compile_asm_register(branch->getSubAst(2))
           )
        );
    } else if(opcode == "shr") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::SHR,
                   compile_asm_register(branch->getSubAst(0)),
                   compile_asm_register(branch->getSubAst(1)),
                   compile_asm_register(branch->getSubAst(2))
           )
        );
    } else if(opcode == "skipife") {
        create_machine_instruction_operation(scheme,
            new machine_instruction(
                    Opcode::SKPE,
                    compile_asm_register(branch->getSubAst(0)),
                    compile_asm_literal(branch->getSubAst(1), scheme)
            )
        );
    } else if(opcode == "skipifne") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::SKNE,
                   compile_asm_register(branch->getSubAst(0)),
                   compile_asm_literal(branch->getSubAst(1), scheme)
           )
        );
    } else if(opcode == "and") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::AND,
                   compile_asm_register(branch->getSubAst(0)),
                   compile_asm_register(branch->getSubAst(1))
           )
        );
    } else if(opcode == "uand") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::UAND,
                   compile_asm_register(branch->getSubAst(0)),
                   compile_asm_register(branch->getSubAst(1))
           )
        );
    } else if(opcode == "or") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::OR,
                   compile_asm_register(branch->getSubAst(0)),
                   compile_asm_register(branch->getSubAst(1))
           )
        );
    } else if(opcode == "xor") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::XOR,
                   compile_asm_register(branch->getSubAst(0)),
                   compile_asm_register(branch->getSubAst(1))
           )
        );
    } else if(opcode == "throw") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::THROW
           )
        );
    } else if(opcode == "checknull") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::CHECKNULL,
                   compile_asm_register(branch->getSubAst(0))
           )
        );
    } else if(opcode == "returnObj") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::RETURNOBJ
           )
        );
    } else if(opcode == "newClassArray") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::NEWCLASSARRAY,
                   compile_asm_register(branch->getSubAst(0)),
                   compile_asm_literal(branch->getSubAst(1), scheme)
           )
        );
    } else if(opcode == "newString") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::NEWSTRING,
                   compile_asm_literal(branch->getSubAst(0), scheme)
           )
        );
    } else if(opcode == "addl") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::ADDL,
                   compile_asm_register(branch->getSubAst(0)),
                   compile_asm_literal(branch->getSubAst(1), scheme)
           )
        );
    } else if(opcode == "subl") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::SUBL,
                   compile_asm_register(branch->getSubAst(0)),
                   compile_asm_literal(branch->getSubAst(1), scheme)
           )
        );
    } else if(opcode == "mull") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::MULL,
                   compile_asm_register(branch->getSubAst(0)),
                   compile_asm_literal(branch->getSubAst(1), scheme)
           )
        );
    } else if(opcode == "divl") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::DIVL,
                   compile_asm_register(branch->getSubAst(0)),
                   compile_asm_literal(branch->getSubAst(1), scheme)
           )
        );
    } else if(opcode == "modl") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::MODL,
                   compile_asm_register(branch->getSubAst(0)),
                   compile_asm_literal(branch->getSubAst(1), scheme)
           )
        );
    } else if(opcode == "iaddl") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::IADDL,
                   compile_asm_literal(branch->getSubAst(0), scheme),
                   compile_asm_literal(branch->getSubAst(1), scheme)
           )
        );
    } else if(opcode == "isubl") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::ISUBL,
                   compile_asm_literal(branch->getSubAst(0), scheme),
                   compile_asm_literal(branch->getSubAst(1), scheme)
           )
        );
    } else if(opcode == "imull") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::IMULL,
                   compile_asm_literal(branch->getSubAst(0), scheme),
                   compile_asm_literal(branch->getSubAst(1), scheme)
           )
        );
    } else if(opcode == "idivl") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::IDIVL,
                   compile_asm_literal(branch->getSubAst(0), scheme),
                   compile_asm_literal(branch->getSubAst(1), scheme)
           )
        );
    } else if(opcode == "imodl") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::IMODL,
                   compile_asm_literal(branch->getSubAst(0), scheme),
                   compile_asm_literal(branch->getSubAst(1), scheme)
           )
        );
    } else if(opcode == "loadl") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::LOADL,
                   compile_asm_register(branch->getSubAst(0)),
                   compile_asm_literal(branch->getSubAst(1), scheme)
           )
        );
    } else if(opcode == "popObj") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::POPOBJ
           )
        );
    } else if(opcode == "smovr") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::SMOVR,
                   compile_asm_register(branch->getSubAst(0)),
                   compile_asm_literal(branch->getSubAst(1), scheme)
           )
        );
    } else if(opcode == "andl") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::ANDL,
                   compile_asm_register(branch->getSubAst(0)),
                   compile_asm_literal(branch->getSubAst(1), scheme)
           )
        );
    } else if(opcode == "orl") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::ORL,
                   compile_asm_register(branch->getSubAst(0)),
                   compile_asm_literal(branch->getSubAst(1), scheme)
           )
        );
    } else if(opcode == "xorl") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::XORL,
                   compile_asm_register(branch->getSubAst(0)),
                   compile_asm_literal(branch->getSubAst(1), scheme)
           )
        );
    } else if(opcode == "rmov") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::RMOV,
                   compile_asm_register(branch->getSubAst(0)),
                   compile_asm_register(branch->getSubAst(1))
           )
        );
    } else if(opcode == "smov") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::SMOV,
                   compile_asm_register(branch->getSubAst(0)),
                   compile_asm_literal(branch->getSubAst(1), scheme)
           )
        );
    } else if(opcode == "returnVal") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::RETURNVAL,
                   compile_asm_register(branch->getSubAst(0))
           )
        );
    } else if(opcode == "istore") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::ISTORE,
                   compile_asm_literal(branch->getSubAst(0), scheme)
           )
        );
    } else if(opcode == "smovr2") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::SMOVR_2,
                   compile_asm_register(branch->getSubAst(1)),
                   compile_asm_literal(branch->getSubAst(0), scheme)
           )
        );
    } else if(opcode == "smovr3") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::SMOVR_3,
                   compile_asm_literal(branch->getSubAst(0), scheme)
           )
        );
    } else if(opcode == "istorel") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::ISTOREL,
                   compile_asm_literal(branch->getSubAst(0), scheme),
                   compile_asm_literal(branch->getSubAst(1), scheme)
           )
        );
    } else if(opcode == "popl") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::POPL,
                   compile_asm_literal(branch->getSubAst(0), scheme)
           )
        );
    } else if(opcode == "pushNull") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::PUSHNULL
           )
        );
    } else if(opcode == "ipushl") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::IPUSHL,
                   compile_asm_literal(branch->getSubAst(0), scheme)
           )
        );
    } else if(opcode == "pushl") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::PUSHL,
                   compile_asm_literal(branch->getSubAst(0), scheme)
           )
        );
    } else if(opcode == "itest") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::ITEST,
                   compile_asm_register(branch->getSubAst(0))
           )
        );
    } else if(opcode == "invokeDelegate") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::INVOKE_DELEGATE,
                   compile_asm_literal(branch->getSubAst(0), scheme),
                   compile_asm_literal(branch->getSubAst(1), scheme),
                   compile_asm_literal(branch->getSubAst(2), scheme)
           )
        );
    } else if(opcode == "get") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::GET,
                   compile_asm_register(branch->getSubAst(0))
           )
        );
    } else if(opcode == "isadd") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::ISADD,
                   compile_asm_literal(branch->getSubAst(0), scheme),
                   compile_asm_literal(branch->getSubAst(1), scheme)
           )
        );
    } else if(opcode == "je") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::JE,
                   compile_asm_literal(branch->getSubAst(0), scheme)
           )
        );
    } else if(opcode == "jne") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::JNE,
                   compile_asm_literal(branch->getSubAst(0), scheme)
           )
        );
    } else if(opcode == "ipopl") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::IPOPL,
                   compile_asm_literal(branch->getSubAst(0), scheme)
           )
        );
    } else if(opcode == "cmp") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::CMP,
                   compile_asm_register(branch->getSubAst(0)),
                   compile_asm_literal(branch->getSubAst(1), scheme)
           )
        );
    } else if(opcode == "calld") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::CALLD,
                   compile_asm_register(branch->getSubAst(0))
           )
        );
    } else if(opcode == "varCast") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::VARCAST,
                   compile_asm_literal(branch->getSubAst(0), scheme),
                   compile_asm_literal(branch->getSubAst(1), scheme)
           )
        );
    } else if(opcode == "tlsMovl") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::TLS_MOVL,
                   compile_asm_literal(branch->getSubAst(0), scheme)
           )
        );
    } else if(opcode == "dup") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::DUP
           )
        );
    } else if(opcode == "popObj2") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::POPOBJ_2
           )
        );
    } else if(opcode == "swap") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::SWAP
           )
        );
    } else if(opcode == "ldc") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::LDC,
                   compile_asm_register(branch->getSubAst(0)),
                   compile_asm_literal(branch->getSubAst(1), scheme)
           )
        );
    } else if(opcode == "neg") {
        create_machine_instruction_operation(scheme,
           new machine_instruction(
                   Opcode::NEG,
                   compile_asm_register(branch->getSubAst(0)),
                   compile_asm_register(branch->getSubAst(1))
           )
        );
    } else if(opcode == ".") {
        string name = branch->getToken(1).getValue();

        sharp_label *label = resolve_label(name, &current_context);

        if(label == NULL) {
            label = create_label(name, &current_context, branch, scheme);
        }

        create_set_label_operation(scheme, label);
    } else {
        create_new_error(GENERIC, branch->line, branch->col,
                         "attempt to execute unknown opcode `" + opcode + "`.");
    }
}

void compile_asm_statement(Ast *ast, operation_schema *scheme) {
    Ast *asmBlock = ast->getSubAst(ast_assembly_block);
    operation_schema *subScheme = new operation_schema();
    subScheme->schemeType = scheme_asm;

    for(Int i = 0; i < asmBlock->getSubAstCount(); i++) {
        Ast *branch = asmBlock->getSubAst(i);
        string opcode = branch->getToken(0).getValue();
        compile_asm_instruction(subScheme, branch, opcode);
    }

    add_scheme_operation(scheme, subScheme);
    delete subScheme;
}
