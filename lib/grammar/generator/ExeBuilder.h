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
        allDelegates.init();
    }

    ~ExeBuilder() {
        allMethods.free();
        allClasses.free();
        allDelegates.free();
    }

    void build();

private:
    List<Method*> allMethods;
    List<Method*> allDelegates;
    List<ClassObject*> allClasses;
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

    void buildDataSection();

    void putMethodData(Method *fun);

    void putMethodCode(Method *fun);

    void buildMetaSection();

    int32_t getFpOffset(Method *fun);

    int32_t getSpOffset(Method *fun);

    void putMethodParams(Method *fun);
};


#endif //SHARP_EXEBUILDER_H
