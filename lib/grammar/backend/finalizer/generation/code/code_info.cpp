//
// Created by bknun on 8/8/2022.
//

#include "code_info.h"
#include "../generator.h"

void line_info::copy(const line_info &li) {
    start_pc = li.start_pc;
    line = li.line;
}

void code_info::copy(const code_info &ci) {
    free();

    address = ci.address;
    uuid = ci.uuid;
    stackSize = ci.stackSize;
    code.addAll(ci.code);

    for(Int i = 0; i < ci.lineTable.size(); i++) {
        lineTable.add(new line_info(*ci.lineTable.get(i)));
    }
}

void code_info::free() {
    code.free();
}

string register_to_str(int64_t r) {
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

extern uInt delegateFunctionAddressCounter;
string code_to_string(code_info *info) {
    List<opcode_instr> &code = info->code;
    stringstream ss, endData;
    Int linesProcessed = 0, currLine = 0;

    for(unsigned int x = 0; x < code.size(); x++) {
        opcode_instr opcodeData=code.get(x);
        ss << x <<std::hex << " [0x" << x << std::dec << "]"<< ": ";

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
                ss << "movi #" << (int)code.get(x+1)  << ", ";
                ss<< register_to_str(GET_Da(opcodeData)) ;
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
                ss<< register_to_str(GET_Ca(opcodeData)) << ": ";
                ss<< GET_Cb(opcodeData);

                break;
            }
            case Opcode::CAST:
            {
                ss<<"cast ";
                ss<< GET_Da(opcodeData) << " // ";
                ss << compressedCompilationClasses.get(GET_Da(opcodeData))->fullName;

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
                ss<< register_to_str(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< register_to_str(GET_Cb(opcodeData));

                break;
            }
            case Opcode::MOV16:
            {
                ss<<"mov16 ";
                ss<< register_to_str(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< register_to_str(GET_Cb(opcodeData));

                break;
            }
            case Opcode::MOV32:
            {
                ss<<"mov32 ";
                ss<< register_to_str(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< register_to_str(GET_Cb(opcodeData));

                break;
            }
            case Opcode::MOV64:
            {
                ss<<"mov64 ";
                ss<< register_to_str(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< register_to_str(GET_Cb(opcodeData));

                break;
            } case Opcode::MOVU8:
            {
                ss<<"movu8 ";
                ss<< register_to_str(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< register_to_str(GET_Cb(opcodeData));

                break;
            }
            case Opcode::MOVU16:
            {
                ss<<"movu16 ";
                ss<< register_to_str(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< register_to_str(GET_Cb(opcodeData));

                break;
            }
            case Opcode::MOVU32:
            {
                ss<<"movu32 ";
                ss<< register_to_str(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< register_to_str(GET_Cb(opcodeData));

                break;
            }
            case Opcode::MOVU64:
            {
                ss<<"movu64 ";
                ss<< register_to_str(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< register_to_str(GET_Cb(opcodeData));

                break;
            }
            case Opcode::RSTORE:
            {
                ss<<"rstore ";
                ss<< register_to_str(GET_Da(opcodeData));

                break;
            }
            case Opcode::ADD:
            {
                ss<<"add ";
                ss<< register_to_str(GET_Ba(opcodeData));
                ss<< ", ";
                ss<< register_to_str(GET_Bb(opcodeData));
                ss<< " -> ";
                ss<< register_to_str(GET_Bc(opcodeData));

                break;
            }
            case Opcode::SUB:
            {
                ss<<"sub ";
                ss<< register_to_str(GET_Ba(opcodeData));
                ss<< ", ";
                ss<< register_to_str(GET_Bb(opcodeData));
                ss<< " -> ";
                ss<< register_to_str(GET_Bc(opcodeData));

                break;
            }
            case Opcode::MUL:
            {
                ss<<"mul ";
                ss<< register_to_str(GET_Ba(opcodeData));
                ss<< ", ";
                ss<< register_to_str(GET_Bb(opcodeData));
                ss<< " -> ";
                ss<< register_to_str(GET_Bc(opcodeData));

                break;
            }
            case Opcode::DIV:
            {
                ss<<"div ";
                ss<< register_to_str(GET_Ba(opcodeData));
                ss<< ", ";
                ss<< register_to_str(GET_Bb(opcodeData));
                ss<< " -> ";
                ss<< register_to_str(GET_Bc(opcodeData));

                break;
            }
            case Opcode::MOD:
            {
                ss<<"mod ";
                ss<< register_to_str(GET_Ba(opcodeData));
                ss<< ", ";
                ss<< register_to_str(GET_Bb(opcodeData));
                ss<< " -> ";
                ss<< register_to_str(GET_Bc(opcodeData));

                break;
            }
            case Opcode::IADD:
            {
                ss<<"iadd ";
                ss<< register_to_str(GET_Da(opcodeData));
                ss<< ", #";
                ss<< code.get(++x);

                break;
            }
            case Opcode::ISUB:
            {
                ss<<"isub ";
                ss<< register_to_str(GET_Da(opcodeData));
                ss<< ", #";
                ss<< code.get(++x);

                break;
            }
            case Opcode::IMUL:
            {
                ss<<"imul ";
                ss<< register_to_str(GET_Da(opcodeData));
                ss<< ", #";
                ss<< code.get(++x);

                break;
            }
            case Opcode::IDIV:
            {
                ss<<"idiv ";
                ss<< register_to_str(GET_Da(opcodeData));
                ss<< ", #";
                ss<< code.get(++x);

                break;
            }
            case Opcode::IMOD:
            {
                ss<<"imod ";
                ss<< register_to_str(GET_Da(opcodeData));
                ss<< ", #";
                ss<< code.get(++x);

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
                ss<< register_to_str(GET_Da(opcodeData));

                break;
            }
            case Opcode::DEC:
            {
                ss<<"dec ";
                ss<< register_to_str(GET_Da(opcodeData));

                break;
            }
            case Opcode::MOVR:
            {
                ss<<"movr ";
                ss<< register_to_str(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< register_to_str(GET_Cb(opcodeData));

                break;
            }
            case Opcode::IALOAD:
            {
                ss<<"iaload ";
                ss<< register_to_str(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< register_to_str(GET_Cb(opcodeData));

                break;
            }
            case Opcode::ILOAD:
            {
                ss<<"iload ";
                ss<< register_to_str(GET_Da(opcodeData));

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
            case Opcode::JE:
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
                ss<< register_to_str(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< register_to_str(GET_Cb(opcodeData));

                break;
            }
            case Opcode::GT:
            {
                ss<<"gt ";
                ss<< register_to_str(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< register_to_str(GET_Cb(opcodeData));

                break;
            }
            case Opcode::LTE:
            {
                ss<<"lte ";
                ss<< register_to_str(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< register_to_str(GET_Cb(opcodeData));

                break;
            }
            case Opcode::GTE:
            {
                ss<<"gte ";
                ss<< register_to_str(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< register_to_str(GET_Cb(opcodeData));

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
                ss<< register_to_str(GET_Da(opcodeData));

                break;
            }
            case Opcode::PUT:
            {
                ss<<"put ";
                ss<< register_to_str(GET_Da(opcodeData));

                break;
            }
            case Opcode::PUTC:
            {
                ss<<"_putc ";
                ss<< register_to_str(GET_Da(opcodeData));

                break;
            }
            case Opcode::CHECKLEN:
            {
                ss<<"chklen ";
                ss<< register_to_str(GET_Da(opcodeData));

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
                ss<< register_to_str(GET_Da(opcodeData));

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
                ss << "call @" << GET_Da(opcodeData);
                ss << " // " << compressedCompilationFunctions.get(GET_Da(opcodeData) + delegateFunctionAddressCounter)->fullName;

                break;
            }
            case Opcode::CALLD:
            {
                ss<<"calld ";
                ss<< register_to_str(GET_Da(opcodeData));

                break;
            }
            case Opcode::NEWCLASS:
            {
                ss<<"new_class @" << compressedCompilationClasses.get(code.get(x+1))->fullName;
                x++;
                break;
            }
            case Opcode::MOVN:
            {
                ss<<"movn #" << code.get(++x);

                break;
            }
            case Opcode::SLEEP:
            {
                ss<<"sleep ";
                ss<< register_to_str(GET_Da(opcodeData));

                break;
            }
            case Opcode::TEST:
            {
                ss<<"test ";
                ss<< register_to_str(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< register_to_str(GET_Cb(opcodeData));

                break;
            }
            case Opcode::TNE:
            {
                ss<<"tne ";
                ss<< register_to_str(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< register_to_str(GET_Cb(opcodeData));

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
                ss<<"movg @" << compressedCompilationClasses.get(GET_Da(opcodeData))->fullName;

                break;
            }
            case Opcode::MOVND:
            {
                ss<<"movnd ";
                ss<< register_to_str(GET_Da(opcodeData));

                break;
            }
            case Opcode::NEWOBJARRAY:
            {
                ss<<"newobj_arry ";
                ss<< register_to_str(GET_Da(opcodeData));

                break;
            }
            case Opcode::NOT: //c
            {
                ss<<"not ";
                ss<< register_to_str(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< register_to_str(GET_Cb(opcodeData));

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
                ss<< register_to_str(GET_Da(opcodeData));

                break;
            }
            case Opcode::SHL:
            {
                ss<<"shl ";
                ss<< register_to_str(GET_Ba(opcodeData));
                ss<< ", ";
                ss<< register_to_str(GET_Bb(opcodeData));
                ss<< " -> ";
                ss<< register_to_str(GET_Bc(opcodeData));

                break;
            }
            case Opcode::SHR:
            {
                ss<<"shr ";
                ss<< register_to_str(GET_Ba(opcodeData));
                ss<< ", ";
                ss<< register_to_str(GET_Bb(opcodeData));
                ss<< " -> ";
                ss<< register_to_str(GET_Bc(opcodeData));

                break;
            }
            case Opcode::SKPE:
            {
                ss<<"skpe ";
                ss<< register_to_str(GET_Ca(opcodeData));
                ss << " // pc = " << (x + GET_Cb(opcodeData));

                break;
            }
            case Opcode::SKNE:
            {
                ss<<"skne ";
                ss<< register_to_str(GET_Ca(opcodeData));
                ss << " // pc = " << (x + GET_Cb(opcodeData));

                break;
            }
            case Opcode::CMP:
            {
                ss<<"cmp ";
                ss<< register_to_str(GET_Da(opcodeData));
                ss<< ", ";
                ss<< code.get(++x);

                break;
            }
            case Opcode::AND:
            {
                ss<<"and ";
                ss<< register_to_str(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< register_to_str(GET_Cb(opcodeData));

                break;
            }
            case Opcode::UAND:
            {
                ss<<"uand ";
                ss<< register_to_str(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< register_to_str(GET_Cb(opcodeData));

                break;
            }
            case Opcode::OR:
            {
                ss<<"or ";
                ss<< register_to_str(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< register_to_str(GET_Cb(opcodeData));

                break;
            }
            case Opcode::XOR:
            {
                ss<<"xor ";
                ss<< register_to_str(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< register_to_str(GET_Cb(opcodeData));

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
                ss<< register_to_str(GET_Da(opcodeData));
                ss<< " ";
                x++;
                ss << "@ " << compressedCompilationClasses.get(code.get(x))->fullName;

                break;
            }
            case Opcode::NEWSTRING:
            {
                ss << "newstr @" << GET_Da(opcodeData) << " (" << stringMap.get(GET_Da(opcodeData)) << ")";

                break;
            }
            case Opcode::ADDL:
            {
                ss<<"addl ";
                ss << register_to_str(GET_Ca(opcodeData)) << ", fp@";
                ss<<GET_Cb(opcodeData);

                break;
            }
            case Opcode::SUBL:
            {
                ss<<"subl ";
                ss << register_to_str(GET_Ca(opcodeData)) << ", fp@";
                ss<<GET_Cb(opcodeData);

                break;
            }
            case Opcode::MULL:
            {
                ss<<"mull ";
                ss << register_to_str(GET_Ca(opcodeData)) << ", fp@";
                ss<<GET_Cb(opcodeData);

                break;
            }
            case Opcode::DIVL:
            {
                ss<<"divl ";
                ss << register_to_str(GET_Ca(opcodeData)) << ", fp@";
                ss<<GET_Cb(opcodeData);

                break;
            }
            case Opcode::MODL:
            {
                ss<<"modl #";
                ss << register_to_str(GET_Ca(opcodeData)) << ", fp@";
                ss<<GET_Cb(opcodeData);

                break;
            }
            case Opcode::IADDL:
            {
                ss<<"iaddl ";
                ss << (int)code.get(x+1) << ", fp@";
                ss<<GET_Da(opcodeData); x++;

                break;
            }
            case Opcode::ISUBL:
            {
                ss<<"isubl ";
                ss << (int)code.get(x+1) << ", fp@";
                ss<<GET_Da(opcodeData); x++;

                break;
            }
            case Opcode::IMULL:
            {
                ss<<"imull ";
                ss << (int)code.get(x+1) << ", fp@";
                ss<<GET_Da(opcodeData); x++;

                break;
            }
            case Opcode::IDIVL:
            {
                ss<<"idivl ";
                ss << (int)code.get(x+1) << ", fp@";
                ss<<GET_Da(opcodeData); x++;

                break;
            }
            case Opcode::IMODL:
            {
                ss<<"imodl ";
                ss << (int)code.get(x+1) << ", fp@";
                ss<<GET_Da(opcodeData); x++;

                break;
            }
            case Opcode::LOADL:
            {
                ss<<"loadl ";
                ss << register_to_str(GET_Ca(opcodeData)) << ", fp+";
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
                ss << register_to_str(GET_Ca(opcodeData)) << ", sp+";
                if(GET_Cb(opcodeData) < 0) ss << "[";
                ss<<GET_Cb(opcodeData);
                if(GET_Cb(opcodeData) < 0) ss << "]";

                break;
            }
            case Opcode::SMOVR_2:
            {
                ss<<"smovr_2 ";
                ss << register_to_str(GET_Ca(opcodeData)) << ", fp+";
                if(GET_Cb(opcodeData) < 0) ss << "[";
                ss<<GET_Cb(opcodeData);
                if(GET_Cb(opcodeData) < 0) ss << "]";

                break;
            }
            case Opcode::ANDL:
            {
                ss<<"andl ";
                ss<< register_to_str(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< GET_Cb(opcodeData);

                break;
            }
            case Opcode::ORL:
            {
                ss<<"orl ";
                ss<< register_to_str(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< GET_Cb(opcodeData);

                break;
            }
            case Opcode::XORL:
            {
                ss<<"xorl ";
                ss<< register_to_str(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< GET_Cb(opcodeData);

                break;
            }
            case Opcode::RMOV:
            {
                ss<<"rmov ";
                ss<< register_to_str(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< register_to_str(GET_Cb(opcodeData));

                break;
            }
            case Opcode::IMOV:
            {
                ss<<"imov ";
                ss<< register_to_str(GET_Da(opcodeData));

                break;
            }
            case Opcode::SMOV:
            {
                ss<<"smov ";
                ss << register_to_str(GET_Ca(opcodeData)) << ", sp+";
                if(GET_Cb(opcodeData) < 0) ss << "[";
                ss<<GET_Cb(opcodeData);
                if(GET_Cb(opcodeData) < 0) ss << "]";

                break;
            }
            case Opcode::RETURNVAL:
            {
                ss<<"return_val ";
                ss<< register_to_str(GET_Da(opcodeData));

                break;
            }
            case Opcode::ISTORE:
            {
                ss<<"istore ";
                ss<< code.get(++x);

                break;
            }
            case Opcode::ISTOREL:
            {
                ss<<"istorel ";
                ss << ((int32_t)code.get(++x)) << ", fp+";
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
                ss<< register_to_str(GET_Da(opcodeData));

                break;
            }
            case Opcode::ITEST:
            {
                ss<<"itest ";
                ss<< register_to_str(GET_Da(opcodeData));

                break;
            }
            case Opcode::INVOKE_DELEGATE:
            {
                ss<<"invoke_delegate ";
                ss<< GET_Da(opcodeData);
                ss<< " { static=";
                ss<< GET_Ca(code.get(x+1))
                  << ", args=" << GET_Cb(code.get(x+1)) <<  " }";
                x++;

                break;
            }
            case Opcode::ISADD:
            {
                ss<<"isadd ";
                ss << GET_Ca(opcodeData) << ", sp+";
                if((int)code.get(x+1) < 0) ss << "[";
                ss<<(int)code.get(x+1);
                if((int)code.get(x+1) < 0) ss << "]";
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
                ss<< register_to_str(GET_Ca(opcodeData));
                if(GET_Cb(opcodeData) >= 0)
                    ss<< " @ " << GET_Cb(opcodeData) << " (" << constantMap.get(GET_Cb(opcodeData)) << ")";

                break;
            }
            case Opcode::NEG:
            {
                ss<<"neg ";
                ss<< register_to_str(GET_Ca(opcodeData));
                ss<< ", ";
                ss<< register_to_str(GET_Cb(opcodeData));
                break;
            }
            case Opcode::EXP:
            {
                ss<<"exp ";
                ss<< register_to_str(GET_Ba(opcodeData));
                ss<< ", ";
                ss<< register_to_str(GET_Bb(opcodeData));
                ss<< " -> ";
                ss<< register_to_str(GET_Bc(opcodeData));

                break;
            }
            case Opcode::IS:
            {
                ss<<"is ";
                ss<< register_to_str(GET_Da(opcodeData));
                ss<< " -> ";
                x++;
                ss << "@ " << compressedCompilationClasses.get(code.get(x))->fullName;
                break;
            }
            case Opcode::SMOVR_4:
            {
                ss<<"smovr_4 fp+";
                ss<< GET_Da(opcodeData);
                ss<< " <- ";
                x++;
                ss << "fp+" << (int32_t)code.get(x);
                break;
            }
            case Opcode::LOAD_ABS:
            {
                ss<<"load_abs stack+" << (GET_Da(opcodeData)) << " -> ebx";
                break;
            }
            case Opcode::MOV_ABS:
            {
                ss<<"mov_abs ebx -> stack+" << (GET_Da(opcodeData));
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

void try_catch_data::free() {
    deleteList(catchTable);
    delete finallyData; finallyData = NULL;
}
