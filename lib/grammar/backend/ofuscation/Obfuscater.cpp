//
// Created by BNunnally on 5/14/2020.
//

#include "../Compiler.h"
#include "Obfuscater.h"
#include "../../../util/File.h"

#define ALPHABET_COUNT 26
#define MAX_BUFFER_SIZE 32
char alphabet[]
    = {
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
        'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'
    };

char buf[MAX_BUFFER_SIZE];
char resultBuf[MAX_BUFFER_SIZE];
List<ModuleData*> Obfuscater::modules;
List<FileData*> Obfuscater::files;

bool Obfuscater::findPackage(ModuleData **p1, void *p2) {
    return (*p1)->name == ((ModuleData*)p2)->name;
}

bool Obfuscater::findFile(FileData **f1, void *f2) {
    return (*f1)->name == ((ModuleData*)f2)->name;
}

void Obfuscater::invalidId(Int id) {
    stringstream ss;
    ss << ": could not obfuscate invalid unique id of " << id;
    throw std::runtime_error(ss.str());
}

string Obfuscater::generateName(Int id) {
    if(id < 0)
        invalidId(id);

    stringstream name;
    if(id < ALPHABET_COUNT) {
        name << alphabet[id];
        return name.str();
    }

    Int stringSize = 0;
    while(id > 0) {
        if(stringSize < MAX_BUFFER_SIZE) {
            Int letterIndex = id % ALPHABET_COUNT;
            letterIndex = letterIndex == 0 ? ALPHABET_COUNT-1 : letterIndex - 1; // we need this so we start at a after id of 26, i.e (aa, ab, ac, etc.)

            buf[stringSize++] = alphabet[letterIndex];
            id /= ALPHABET_COUNT;
        } else { // should never happen
            clearBuf();
            buf[0] = '?';
            stringSize = 1;
            break;
        }
    }

    Int bufIndex = 0;
    for(Int i = stringSize - 1; i >= 0; i--) {
        resultBuf[bufIndex++] = buf[i];
    }

    return string(resultBuf, stringSize);
}

void Obfuscater::clearBuf() {
    for(Int i = 0; i < MAX_BUFFER_SIZE; i++) {
        buf[i] = 0;
    }
}

bool Obfuscater::checkReliability(Int startId, Int sampleSize) {

    if(sampleSize <= MAX_SAMPLE_SIZE) {
        std::string names[sampleSize];
        Int stringSize = 0;
        for(Int i = 0; i < sampleSize; i++) {
            names[stringSize++] = Obfuscater::generateName(startId++);
        }

        for(Int i = 0; i < sampleSize; i++) {
            std::string &name = names[i];
            Int count = 0;

            for(Int j = 0; j < sampleSize; j++) {
                if (names[j] == name) {
                    if(count > 1) {
                        return false;
                    } else count++;
                }
            }
        }
    } else return false;
    return true;
}

void Obfuscater::obfuscate(ClassObject *klass) {
    if(klass->obfuscate) {
        if(IS_CLASS_GENERIC(klass->getClassType())
            && klass->getGenericOwner() == NULL)
            return;

        klass->name = generateName(klass->guid);
        obfuscateFullName(klass);

        for(Int i = 0; i < klass->fieldCount(); i++) {
            obfuscate(klass->getField(i));
        }

        for(Int i = 0; i < klass->getFunctionCount(); i++) {
            obfuscate(klass->getFunction(i));
        }

        List<ClassObject*> &childClasses = klass->getChildClasses();
        for(Int i = 0; i < childClasses.size(); i++) {
            obfuscate(childClasses.get(i));
        }
    }
}

void Obfuscater::obfuscate(DataEntity *de) {
    if(de->obfuscate) {
        de->name = generateName(de->guid);
        obfuscateFullName(de);
    }
}

void Obfuscater::obfuscateFullName(DataEntity *de) {

    if(de->owner == NULL) {
        ModuleData *package = de->module;
        if(package->obfuscate)
            package->name = generateName(package->guid);

        de->fullName = package->name + "#" + generateName(de->guid);
    } else {
        de->fullName = de->owner->fullName + "." + generateName(de->guid);
    }
}

void Obfuscater::generateMappingFile() {
    stringstream ss;
    ss << "/*** Dumpfile for " << c_options.out << " ***/\n\n";

    ss << "[Packages]\n";
    for(Int i = 0; i < modules.size(); i++) {
        ModuleData *package = modules.get(i);
        ss << package->name << "=";
        if(package->obfuscate)
            ss << generateName(package->guid) << endl;
        else ss << "no_change\n";
    }

    ss << "\n\n[Files]\n";
    for(Int i = 0; i < files.size(); i++) {
        FileData *file = files.get(i);
        ss << file->name << "=";
        if(file->obfuscate)
            ss << generateName(file->guid) << endl;
        else ss << "no_change\n";
    }

    ss << "\n\n[Classes]\n";
    for(Int i = 0; i < compiler->classes.size(); i++) {
        ClassObject *klass = compiler->classes.get(i);
        printClassMapping(ss, klass);
    }

    ss << "\n\n[Fields]\n";
    for(Int i = 0; i < compiler->classes.size(); i++) {
        ClassObject *klass = compiler->classes.get(i);
        printClassFieldMapping(ss, klass);
    }

    ss << "\n\n[Methods]\n";
    for(Int i = 0; i < compiler->classes.size(); i++) {
        ClassObject *klass = compiler->classes.get(i);
        printClassMethodMapping(ss, klass);
    }

    File::write(c_options.mapFile.c_str(), ss.str());
}

void Obfuscater::printClassMapping(stringstream &ss, ClassObject *klass) {
    ss << klass->fullName << " = ";

    if(klass->obfuscate)
        ss << generateName(klass->guid) << endl;
    else ss << "no_change" << endl;

    List<ClassObject*> &childClasses = klass->getChildClasses();
    for(Int i = 0; i < childClasses.size(); i++) {
        printClassMapping(ss, childClasses.get(i));
    }
}

void Obfuscater::printClassFieldMapping(stringstream &ss, ClassObject *klass) {
    for(Int i = 0; i < klass->fieldCount(); i++) {
        Field *field = klass->getField(i);
        ss << field->fullName << " = ";

        if(field->owner->obfuscate && field->obfuscate)
            ss << generateName(field->guid) << endl;
        else ss << "no_change" << endl;
    }

    List<ClassObject*> &childClasses = klass->getChildClasses();
    for(Int i = 0; i < childClasses.size(); i++) {
        printClassFieldMapping(ss, childClasses.get(i));
    }
}

void Obfuscater::printClassMethodMapping(stringstream &ss, ClassObject *klass) {
    for(Int i = 0; i < klass->getFunctionCount(); i++) {
        Method* fun = klass->getFunction(i);
        ss << fun->fullName << " = ";

        if(fun->owner->obfuscate && fun->obfuscate)
            ss << generateName(klass->getFunction(i)->guid) << endl;
        else ss << "no_change" << endl;
    }

    List<ClassObject*> &childClasses = klass->getChildClasses();
    for(Int i = 0; i < childClasses.size(); i++) {
        printClassMethodMapping(ss, childClasses.get(i));
    }
}

void Obfuscater::obfuscate() {
    if(c_options.obfuscate) {
        generateMappingFile();

        for(Int i = 0; i < compiler->classes.size(); i++) {
            ClassObject *klass = compiler->classes.get(i);
            obfuscate(klass);
        }

        for(Int i = 0; i < modules.size(); i++) {
            ModuleData *package = modules.get(i);

            if(package->obfuscate) {
                package->name = generateName(package->guid);
            }
        }

        for(Int i = 0; i < files.size(); i++) {
            FileData *file = files.get(i);

            if(file->obfuscate) {
                file->name = generateName(file->guid);
            }
        }
    }
}

ModuleData *Obfuscater::getModule(string name) {
    ModuleData package(name, 0);
    Int index = modules.indexof(findPackage, &package);

    return index == -1 ? NULL : modules.get(index);
}

void Obfuscater::addModule(string name, uInt id) {
    if(getModule(name) == NULL) {
        modules.add(new ModuleData(name, id));
    }
}

FileData *Obfuscater::getFile(string name) {
    FileData file(name, 0);
    Int index = files.indexof(findFile, &file);

    return index == -1 ? NULL : files.get(index);
}

void Obfuscater::addFile(string name, uInt id) {
    if(getFile(name) == NULL) {
        files.add(new FileData(name, id));
    }
}
