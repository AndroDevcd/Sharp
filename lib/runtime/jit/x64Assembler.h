//
// Created by braxtonn on 8/23/2019.
//

#ifndef SHARP_X64ASSEMBLER_H
#define SHARP_X64ASSEMBLER_H

#include "_BaseAssembler.h"

#ifdef BUILD_JIT
class x64Assembler : public _BaseAssembler {
public:
    x64Assembler()
    :
            _BaseAssembler(),
            assembler(NULL),
            code(NULL),
            logger(NULL),
            constants(NULL),
            labels(NULL),
            compiledMethod(NULL),
            stackSize(0),
            ctxPtr(),
            tmpPtr(),
            labelsPtr(),
            tmpInt(),
            lcodeStart(),
            lsetupAddressTable(),
            lendOfFunction(),
            ldataSection(),
            lsignalCheck()
    {
        initialize();
    }

private: // virtual functions
    x86::Mem getMemPtr(Int addr) override ;
    x86::Mem getMemBytePtr(Int addr) override ;
    x86::Mem getMemPtr(x86::Gp reg, Int addr) override ;
    x86::Mem getMemPtr(x86::Gp reg) override ;
    Int getRegisterSize() override ;
    void initializeRegisters() override ;
    void createFunctionPrologue() override ;
    void createFunctionEpilogue() override ;
    void beginCompilation(Method *method) override ;
    void endCompilation() override ;
    void addUserCode() override ;
    void addConstantsSection() override;

    // Logging methods
    void logComment(std::string) override ;

    // stack manip functions
    void allocateStackSpace() override ;
    void setupStackAndRegisterValues() override ;

    // control flow functions
    void setupGotoLabels() override ;
    void createFunctionLandmarks() override ;
    void setupAddressTable() override ;
    void storeLabelValues() override ;
    void validateVirtualStack() override ;
    void addThreadSignalCheck() override ;
    void addStackCheck() override ;

    // Helper functions
    void incPc() override ;
    int createJitFunc() override ;
    void updatePc();
    static Int getPc(fiber *);
    void threadStatusCheck(Label &retLbl, Int irAddr, bool incPc);
    void threadStatusCheck(Label &retLbl, bool incPc);
    void checkMasterShutdown(int64_t pc);
    void movConstToXmm(x86::Xmm xmm, double _const);
    void movRegister(x86::Xmm &vec, Int addr, bool store);
    static void popExceptionObject(fiber*);
    static void enableExceptionSignal(Thread*);
    static void enableThreadKillSignal(Thread*);
    void stackCheck(Int, Label&);

    // private fields
    x86::Assembler *assembler;
    CodeHolder *code;
    FileLogger *logger;
    Constants* constants;
    Label *labels;
    Method *compiledMethod;
    x86::Mem ctxPtr;
    x86::Mem tmpPtr;
    x86::Mem labelsPtr;
    x86::Mem tmpInt;
    x86::Mem returnAddress;
    Int stackSize;

    // Function Landmarks
    Label lcodeStart;
    Label lsetupAddressTable;
    Label lendOfFunction;
    Label lfunctionEpilogue;
    Label ldataSection;
    Label lsignalCheck;
    Label lvirtualStackCheck;
    Label lstackCheck;
};
#endif

#endif //SHARP_X64ASSEMBLER_H
