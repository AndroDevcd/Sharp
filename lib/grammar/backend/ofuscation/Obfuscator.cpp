//
// Created by BNunnally on 5/14/2020.
//

#include "../../../runtime/oo/string.h"
#include "../Compiler.h"
#include "Obfuscator.h"

#define ALPHABET_COUNT 26
#define MAX_BUFFER_SIZE 1028
char alphabet[]
    = {
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
        'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'
    };

char buf[MAX_BUFFER_SIZE];
char resultBuf[MAX_BUFFER_SIZE];
List<PackageData*> Obfuscator::packages;

bool Obfuscator::findPackage(PackageData **p1, void *p2) {
    return (*p1)->name == ((PackageData*)p2)->name;
}

string Obfuscator::generateName(Int id) {
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
        } else {
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

void Obfuscator::clearBuf() {
    for(Int i = 0; i < MAX_BUFFER_SIZE; i++) {
        buf[i] = 0;
    }
}

bool Obfuscator::checkReliability(Int startId, Int sampleSize) {

    if(sampleSize <= MAX_SAMPLE_SIZE) {
        string *names = new string[sampleSize];
        Int stringSize = 0;
        for(Int i = 0; i < sampleSize; i++) {
            names[stringSize++] = Obfuscator::generateName(startId++);
        }

        for(Int i = 0; i < sampleSize; i++) {
            string &name = names[i];
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

void Obfuscator::obfuscate(ClassObject *klass) {
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

void Obfuscator::obfuscate(DataEntity *de) {
    if(de->obfuscate) {
        de->name = generateName(de->guid);
        obfuscateFullName(de);
    }
}

void Obfuscator::obfuscateFullName(DataEntity *de) {

    if(de->owner == NULL) {
        PackageData *package = getPackage(de->module);
        de->fullName = generateName(package->guid) + "#" + generateName(de->guid);
    } else {
        de->fullName = de->owner->fullName + "." + generateName(de->guid);
    }
}

void Obfuscator::generateMappingFile() {
    stringstream ss;
    ss << "/*** Dumpfile for " << c_options.out << " ***/\n\n";

    ss << "[Packages]\n";
    for(Int i = 0; i < packages.size(); i++) {
        PackageData *package = packages.get(i);
        ss << package->name << "=" << generateName(package->guid) << endl;
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

void Obfuscator::printClassMapping(stringstream &ss, ClassObject *klass) {
    ss << klass->fullName << " = " << generateName(klass->guid) << endl;

    List<ClassObject*> &childClasses = klass->getChildClasses();
    for(Int i = 0; i < childClasses.size(); i++) {
        printClassMapping(ss, childClasses.get(i));
    }
}

void Obfuscator::printClassFieldMapping(stringstream &ss, ClassObject *klass) {
    for(Int i = 0; i < klass->fieldCount(); i++) {
        ss << klass->getField(i)->fullName << " = " << generateName(klass->getField(i)->guid) << endl;
    }

    List<ClassObject*> &childClasses = klass->getChildClasses();
    for(Int i = 0; i < childClasses.size(); i++) {
        printClassFieldMapping(ss, childClasses.get(i));
    }
}

void Obfuscator::printClassMethodMapping(stringstream &ss, ClassObject *klass) {
    for(Int i = 0; i < klass->getFunctionCount(); i++) {
        ss << klass->getFunction(i)->fullName << " = " << generateName(klass->getFunction(i)->guid) << endl;
    }

    List<ClassObject*> &childClasses = klass->getChildClasses();
    for(Int i = 0; i < childClasses.size(); i++) {
        printClassMethodMapping(ss, childClasses.get(i));
    }
}

void Obfuscator::obfuscate() {
    if(c_options.obfuscate) {
        generateMappingFile();

        for(Int i = 0; i < compiler->classes.size(); i++) {
            ClassObject *klass = compiler->classes.get(i);
            obfuscate(klass);
        }

        for(Int i = 0; i < packages.size(); i++) {
            PackageData *package = packages.get(i);

            if(!package->ignore) {
                package->ignore = true;
                package->name = generateName(package->guid);
            }
        }
    }
}

PackageData *Obfuscator::getPackage(string name) {
    PackageData package(name, 0);
    Int index = packages.indexof(findPackage, &package);

    return index == -1 ? NULL : packages.get(index);
}

void Obfuscator::addPackage(string name, Int id) {
    if(getPackage(name) == NULL) {
        packages.add(new PackageData(name, id));
    }
}
