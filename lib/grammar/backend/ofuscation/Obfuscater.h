//
// Created by BNunnally on 5/14/2020.
//

#ifndef SHARP_OBFUSCATER_H
#define SHARP_OBFUSCATER_H

#include "../../../../stdimports.h"
#include "../oo/ClassObject.h"

struct ModuleData {
    ModuleData(string name, uInt guid)
    :
            name(name),
            guid(guid),
            obfuscate(c_options.obfuscate)
    {
    }

    string name;
    uInt guid;
    bool obfuscate;
};

struct FileData {
    FileData(string name, uInt guid)
    :
            name(name),
            guid(guid),
            obfuscate(c_options.obfuscate)
    {
    }

    string name;
    uInt guid;
    bool obfuscate;
};

class Compiler;
class Obfuscater {
public:
    Obfuscater(Compiler *compiler)
    :
        compiler(compiler)
    {
    }

    ~Obfuscater() {
        for(Int i = 0; i < modules.size(); i++) {
            delete modules.get(i);
        }
        modules.free();
    }

    void obfuscate();

    static ModuleData* getModule(string name);
    static void addModule(string name, uInt id);
    static FileData* getFile(string name);
    static void addFile(string name, uInt id);

    static List<ModuleData*> modules;
    static List<FileData*> files;
private:
    Compiler *compiler;

    static string generateName(Int id);
    static bool findPackage(ModuleData **p1, void *p2);
    static bool findFile(FileData **f1, void *f2);
    static bool checkReliability(Int startId, Int sampleSize);
    static void obfuscate(ClassObject *klass);
    static void obfuscate(DataEntity *de);
    static void obfuscateFullName(DataEntity *de);
    static void clearBuf();

    void generateMappingFile();
    void printClassMapping(stringstream &ss, ClassObject *klass);
    void printClassFieldMapping(stringstream &ss, ClassObject *klass);
    void printClassMethodMapping(stringstream &ss, ClassObject *klass);
};

#define MAX_SAMPLE_SIZE 100000
#endif //SHARP_OBFUSCATER_H
