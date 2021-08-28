//
// Created by BNunnally on 5/15/2020.
//

#ifndef SHARP_EXEBUILDER_H
#define SHARP_EXEBUILDER_H

class Compiler;
class ClassObject;
class Method;
class ExeBuilder {
public:
    ExeBuilder(Compiler *compiler)
            :
            compiler(compiler)
    {
        allMethods.init();
        allClasses.init();
        functionPointers.init();
    }

    ~ExeBuilder() {
        allMethods.free();
        allClasses.free();
        functionPointers.free();
    }

    void build();

private:
    List<Method*> allMethods;
    List<ClassObject*> allClasses;
    List<Method*> functionPointers;
    Compiler *compiler;
    stringstream buf;
    stringstream dataSec;

    void createDumpFile();
    void buildExe();
    void dumpClassInfo(ClassObject*);
    void addClass(ClassObject*);
    string registerToString(int64_t r);
    string codeToString(Method*);
    string getNote(Meta &meta);

    void buildHeader();

    void buildManifest();

    void buildSymbolSection();

    void buildFieldData(ClassObject *pObject);
    void buildMethodData(ClassObject *pObject);
    void buildInterfaceData(ClassObject *pObject);

    void buildStringSection();
    void buildConstantSection();
    void addFunctionPointers();
    void buildMetaDataSection();
    void createNativeHeaderFile();
    int32_t getSecondarySpOffset(Method *fun);
    void appendClassHeaderFunctions(ClassObject* klass, stringstream &ss);
    void appendProcAddrFunctions(ClassObject* klass, stringstream &ss, bool &firstFunc);
    void createProcAddrFunc(stringstream &ss);
    void appendCallFunctions(ClassObject* klass, stringstream& ss);
    void createCallFunc(stringstream &ss);
    void appendMainFunctions(ClassObject* klass, stringstream& ss);
    void createMainFunc(stringstream &ss);
    void createNativeSourceFile();
    void appendSharpMappingSourceFile(ClassObject* klass, stringstream& ss);
    void createSharpMappingSourceFile();
    void createNativeSourceCode();
    void buildFieldData(Field *field);
    void putSymbol(Utype *utype, stringstream &buf);

    void buildDataSection();
    void addSymbol(Utype *utype);
    void getSymbol(Utype *utype);

    void putMethodData(Method *fun);

    void putMethodCode(Method *fun);

    void buildMetaSection();

    int32_t getFpOffset(Method *fun);

    int32_t getSpOffset(Method *fun);

    void putMethodParams(Method *fun);
};


#endif //SHARP_EXEBUILDER_H
