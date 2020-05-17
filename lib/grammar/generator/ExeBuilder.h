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
    }

    ~ExeBuilder() {
        allMethods.free();
        allClasses.free();
    }

    void build();

private:
    List<Method*> allMethods;
    List<ClassObject*> allClasses;
    Compiler *compiler;
    stringstream buf;

    void createDumpFile();
    void dumpClassInfo(ClassObject*);
    void addClass(ClassObject*);
    string registerToString(int64_t r);
    string codeToString(Method*);
    string getNote(Meta &meta);

    string getFunction(Int address);
};


#endif //SHARP_EXEBUILDER_H
