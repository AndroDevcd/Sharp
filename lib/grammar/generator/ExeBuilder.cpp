//
// Created by BNunnally on 5/15/2020.
//

#include "../backend/Compiler.h"
#include "ExeBuilder.h"

bool sortMethods(Method *m1, Method *m2) {
    return m1->address > m2->address;
}

bool sortClasses(ClassObject *c1, ClassObject *c2) {
    return c1->address > c2->address;
}

void ExeBuilder::createDumpFile() {
    stringstream tmpData;

    buf << "## Object Dump file for " << c_options.out << ":\n\n";

    if(c_options.optimize) {
        // TODO: put optimization results here
    }

    for(Int i = 0; i < compiler->classes.size(); i++) {
        ClassObject *klass = compiler->classes.get(i);
        addClass(klass);
    }
    allClasses.linearSort(sortClasses);

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

        buf << compiler->constantMap.get(i);
    }

    buf << "\n\n methods: \n";
    allMethods.linearSort(sortMethods);

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
            case fn_delegate:
                continue;
        }

        buf << fun->fullName << "[" << fun->address << "] params("
            << fun->params.size() << ") " << getNote(fun->meta) << "\n";

        buf << codeToString(fun);

        buf << "\n\n";
    }

    File::write((c_options.out + ".odf").c_str(), buf.str().c_str());
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
    allClasses.add(klass);

    allMethods.appendAll(klass->getFunctions());
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

string ExeBuilder::getFunction(Int address) {
    for(Int i = address; i < allMethods.size(); i++) {
        if(allMethods.get(i)->address == address)
            return allMethods.get(i)->fullName;
    }

    return "?";
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
        // TODO: put code info here

        switch(GET_OP(opcodeData)) {
            case Opcode::ILL:
            {
                ss<<"ill ";

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
                ss << "movi #" << code.ir32.get(x+1)  << ", ";
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
                ss<< registerToString(GET_Da(opcodeData));

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
                ss<< registerToString(code.ir32.get(++x));

                break;
            }
            case Opcode::ISUB:
            {
                ss<<"isub ";
                ss<< registerToString(GET_Da(opcodeData));
                ss<< ", #";
                ss<< registerToString(code.ir32.get(++x));

                break;
            }
            case Opcode::IMUL:
            {
                ss<<"imul ";
                ss<< registerToString(GET_Da(opcodeData));
                ss<< ", #";
                ss<< registerToString(code.ir32.get(++x));

                break;
            }
            case Opcode::IDIV:
            {
                ss<<"idiv ";
                ss<< registerToString(GET_Da(opcodeData));
                ss<< ", #";
                ss<< registerToString(code.ir32.get(++x));

                break;
            }
            case Opcode::IMOD:
            {
                ss<<"imod ";
                ss<< registerToString(GET_Da(opcodeData));
                ss<< ", #";
                ss<< registerToString(code.ir32.get(++x));

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
                    ss << getFunction(GET_Da(opcodeData)) << ">";

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
                ss<<"new_class @" << GET_Da(opcodeData);
                if(GET_Da(opcodeData) >= 0) {
                    ss << " // ";
                    ss << allClasses.get(GET_Da(opcodeData))->fullName;
                }

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
                ss<< registerToString(GET_Ca(opcodeData));
                ss<< " ";
                if(GET_Cb(opcodeData) >= 0)
                    ss << " // "; ss << allClasses.get(GET_Cb(opcodeData))->fullName << "[]";

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
                ss << code.ir32.get(x+1) << ", fp@";
                ss<<GET_Da(opcodeData); x++;

                break;
            }
            case Opcode::ISUBL:
            {
                ss<<"isubl ";
                ss << code.ir32.get(x+1) << ", fp@";
                ss<<GET_Da(opcodeData); x++;

                break;
            }
            case Opcode::IMULL:
            {
                ss<<"imull ";
                ss << code.ir32.get(x+1) << ", fp@";
                ss<<GET_Da(opcodeData); x++;

                break;
            }
            case Opcode::IDIVL:
            {
                ss<<"idivl ";
                ss << code.ir32.get(x+1) << ", fp@";
                ss<<GET_Da(opcodeData); x++;

                break;
            }
            case Opcode::IMODL:
            {
                ss<<"imodl ";
                ss << code.ir32.get(x+1) << ", fp@";
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
                if(code.ir32.get(x+1) < 0) ss << "[";
                ss<<code.ir32.get(x+1);
                if(code.ir32.get(x+1) < 0) ss << "]";
                x++;

                break;
            }
            case Opcode::IPOPL:
            {
                ss<<"ipopl ";
                ss<< GET_Da(opcodeData);

                break;
            }
            case Opcode::SWITCH:
            {
                ss<<"switch ";
                ss<< code.ir32.get(x+1);
                //ss<< " // " << getSwitchTable(method, GET_Da(x64));

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
}
