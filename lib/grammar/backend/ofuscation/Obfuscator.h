//
// Created by BNunnally on 5/14/2020.
//

#ifndef SHARP_OBFUSCATOR_H
#define SHARP_OBFUSCATOR_H

#include "../../../../stdimports.h"
#include "../oo/ClassObject.h"

struct PackageData {
    PackageData(string name, Int guid)
    :
        name(name),
        guid(guid)
    {
    }

    string name;
    Int guid;
    bool ignore;
};

class Compiler;
class Obfuscator {
public:
    Obfuscator(Compiler *compiler)
    :
        compiler(compiler)
    {
    }

    void obfuscate();

    static PackageData* getPackage(string name);
    static void addPackage(string name, Int id);

    static List<PackageData*> packages;
private:
    Compiler *compiler;

    static string generateName(Int id);
    static bool findPackage(PackageData **p1, void *p2);
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
#endif //SHARP_OBFUSCATOR_H
