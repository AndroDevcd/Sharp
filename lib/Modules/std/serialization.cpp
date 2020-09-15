//
// Created by BNunnally on 9/14/2020.
//
#include <iomanip>
#include "serialization.h"
#include "../../runtime/VirtualMachine.h"

bool find_obj(void* arg, KeyPair<Int, SharpObject*> element) {
    return (SharpObject*)arg == element.value;
}

thread_local _List<KeyPair<Int, SharpObject*>> streamInfo;
thread_local Int recursion = 0, refId;
string export_obj(SharpObject* obj) {
    recursion++;
    if(recursion == 1) {
        refId=0;
    }

    if(obj && obj->size > 0) {
        stringstream dataStream;
        dataStream << (char)EXPORT_SECRET;
        dataStream << (char)TYPE(obj->info);
        dataStream << obj->size << (char)0;
        dataStream << refId++ << (char)0; // reference id
        streamInfo.add(KeyPair<Int, SharpObject*>(refId - 1, obj));

        if(TYPE(obj->info) == _stype_var) {
            dataStream << (char)obj->ntype;
            dataStream << (char)EXPORT_DATA;
            for(uInt i = 0; i < obj->size; i++) {
                dataStream << std::setprecision(16) << obj->HEAD[i] << (char)0;
            }
            dataStream << (char)EXPORT_END;
            return dataStream.str();
        } else {
            ClassObject &klass = vm.classes[CLASS(obj->info)];
            uInt fieldAddress =  GENERATION(obj->info) == gc_perm ? klass.instanceFields : 0;
            uInt fieldSize = GENERATION(obj->info) == gc_perm ? klass.staticFields : klass.instanceFields;

            dataStream << klass.fullName.str() << (char)0;
            dataStream << (char)EXPORT_DATA;
            dataStream << (GENERATION(obj->info) == gc_perm ? (char)1 : (char)0);
            dataStream << (obj->array ? (char)1 : (char)0);

            if(fieldSize != obj->size) {
                recursion=0;
                streamInfo.free();
                throw Exception(vm.IncompatibleClassExcept, "class: " + klass.name.str() + " size does not match field count");
            }


            if(obj->array) {
                for(uInt i = 0; i < obj->size; i++) {
                    SharpObject *objField = obj->node[i].object;
                    dataStream << export_obj(objField);
                }
            } else {
                for(Int i = 0; i < fieldSize; i++) {
                    Field &field = klass.fields[fieldAddress++];

                    dataStream << (char)EXPORT_FIELD;
                    dataStream << field.type << (char)0;
                    dataStream << field.name.str() << (char)0;
                    if (obj->node[i].object == NULL) {
                        dataStream << (char)EXPORT_EMPTY;
                    } else {
                        Int index = streamInfo.indexof(find_obj, obj->node[i].object);
                        if (index == -1) {
                            dataStream << export_obj(obj->node[i].object);
                        } else {
                            dataStream << (char)EXPORT_REFERENCE;
                            dataStream << streamInfo.at(index).key << (char) 0;
                        }
                    }
                }
            }

            dataStream << (char)EXPORT_END;

            return dataStream.str();
        }
    } else {
        recursion=0;
        streamInfo.free();
        throw Exception(vm.NullptrExcept, "");
    }

    recursion--;
    return "";
}

thread_local Int pos, streamSize;
char nextChar(double* data) {
    if(pos < streamSize) {
        return (char)data[++pos];
    } else
        throw Exception(vm.IndexOutOfBoundsExcept, "not enough data to process object stream, Is the data possibly corrupt?");
}

char peekChar(double* data) {
    if((pos+1) < streamSize) {
        return (char)data[pos+1];
    } else
        return 0;
}

Int readInt(double *data) {
    stringstream ss;

    for(;;) {
        char ch = nextChar(data);
        if(ch == 0) {
            break;
        }

        ss << ch;
    }

    return strtoll(ss.str().c_str(), NULL, 0);
}

string readString(double *data) {
    stringstream ss;

    for(;;) {
        char ch = nextChar(data);
        if(ch == 0) {
            break;
        }

        ss << ch;
    }

    return ss.str();
}

Int readDouble(double *data) {
    stringstream ss;

    for(;;) {
        char ch = nextChar(data);
        if(ch == 0) {
            break;
        }

        ss << ch;
    }

    return strtod(ss.str().c_str(), NULL);
}

void expectChar(double *data, char ch) {
    if(data[++pos] != ch) {
        stringstream ss;
        ss << "expected char: " << (Int)ch << " during object import";
        throw Exception(vm.IllStateExcept, ss.str());
    }
}

SharpObject* load_obj(SharpObject* obj) {
    double *data = obj->HEAD;
    if(data[++pos] == EXPORT_SECRET) {
        sharp_type type = (sharp_type)(Int)data[++pos];
        Int size = readInt(data);
        Int refrenceId = readInt(data);

        if(type == _stype_var) {
            int ntype = (int)data[++pos];
            expectChar(data, EXPORT_DATA);
            SharpObject *object = gc.newObject(size, ntype);
            streamInfo.add(KeyPair<Int, SharpObject*>(refrenceId, object));

            INC_REF(object)
            for(Int i = 0; i < size; i++) {
                object->HEAD[i]
                  = readDouble(data);
            }

            expectChar(data, EXPORT_END);
            DEC_REF(object)
            return object;
        } else {
            string className = readString(data);
            ClassObject *klass = vm.resolveClass(className);
            if(klass != NULL) {
                expectChar(data, EXPORT_DATA);
                bool staticInit = (char)data[++pos] == 1;
                bool isArray = (char)data[++pos] == 1;
                uInt realSize = staticInit ? klass->staticFields : klass->instanceFields;


                SharpObject *object;

                if(!isArray) {
                    if(size > realSize) {
                        stringstream ss;
                        ss << "class: " << className << " requires(" << size
                           << ") fields when the class was found to only have(" << realSize << ") fields";
                        throw Exception(vm.IllStateExcept, ss.str());
                    }

                    object = gc.newObject(klass, staticInit, false);
                    streamInfo.add(KeyPair<Int, SharpObject*>(refrenceId, object));

                    INC_REF(object)
                    for(Int i = 0; i < size; i++) {
                        expectChar(data, EXPORT_FIELD);
                        DataType fieldType = (DataType)readInt(data);
                        string fieldName = readString(data);

                        Field* field = klass->getfield(fieldName);
                        if(field != NULL) {
                            if(field->type == fieldType) {
                                Object *fieldObj = vm.resolveField(fieldName, object);
                                if(peekChar(data) == EXPORT_EMPTY) {
                                    pos++;
                                    *fieldObj = (SharpObject*)NULL;
                                } else {
                                    if(peekChar(data) == EXPORT_REFERENCE) {
                                        pos++;
                                        Int index = streamInfo.indexof(find_obj, obj);
                                        if(index >= 0) {
                                            *fieldObj = streamInfo.at(index).value;
                                        } else {
                                            stringstream ss;
                                            ss << "could not locate object with reference id of: " << refrenceId << " in the object stream";
                                            throw Exception(vm.IllStateExcept, ss.str());
                                        }
                                    } else {
                                        *fieldObj = load_obj(obj);
                                    }
                                }
                            } else {
                                stringstream ss;
                                ss << "field: " << fieldName << " does not match type: " << dataTypeToString(fieldType, field->isArray) << " found in the object stream";
                                throw Exception(vm.IllStateExcept, ss.str());
                            }
                        } else {
                            stringstream ss;
                            ss << "field: " << fieldName << " was not found in class: " << className << " in the object stream";
                            throw Exception(vm.IllStateExcept, ss.str());
                        }
                    }
                } else {
                    object = gc.newObjectArray(size, klass);
                    streamInfo.add(KeyPair<Int, SharpObject*>(refrenceId, object));

                    INC_REF(object)
                    for(Int i = 0; i < size; i++) {
                        object->node[i] = load_obj(obj);
                    }
                }

                expectChar(data, EXPORT_END);
                DEC_REF(object)
                return object;
            } else {
                stringstream ss;
                ss << "class: " << className << " could not be found in the object stream";
                throw Exception(vm.IllStateExcept, ss.str());
            }
        }
    } else {
        throw Exception(vm.ObjectImportError, "export secret not found");
    }

    return NULL;
}

void import_obj(SharpObject* obj) {
    if(obj && TYPE(obj->info) == _stype_var && obj->size > 0) {
        pos=-1; streamSize=obj->size;
        (++thread_self->this_fiber->sp)->object
           = load_obj(obj);
        streamInfo.free();
    } else {
        throw Exception(vm.NullptrExcept, "");
    }
}

