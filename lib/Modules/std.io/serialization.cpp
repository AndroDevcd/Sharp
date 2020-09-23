//
// Created by BNunnally on 9/14/2020.
//
#include <iomanip>
#include "serialization.h"
#include "../../runtime/VirtualMachine.h"

HashMap<Int, Int> exportStreamInfo(0x1024, false);
HashMap<Int, SharpObject*> importStreamInfo(0x1024, false);
Int recursion = 0, refId;

recursive_mutex exportMutex;
stringstream dataStream;
void cleanup() {
    dataStream.str("");
    exportStreamInfo.free();
    importStreamInfo.free();
}

string export_obj(SharpObject* obj) {
    recursion++;
    if(recursion == 1) {
        refId=0;
    }

    Int refResult;
    if(obj && obj->size > 0) {
        dataStream << (char)EXPORT_SECRET;
        dataStream << (char)TYPE(obj->info);
        dataStream << obj->size << (char)DATA_END;;
        dataStream << refId++ << (char)DATA_END;; // reference id
        exportStreamInfo.put((Int)obj, refId - 1);

        if(TYPE(obj->info) == _stype_var) {
            dataStream << (char)obj->ntype;
            dataStream << (char)EXPORT_DATA;
            if(obj->ntype == _INT8) {
                dataStream << vm.stringValue(obj) << (char) DATA_END;
            } else {
                for (uInt i = 0; i < obj->size; i++) {
                    dataStream << std::setprecision(16) << obj->HEAD[i] << (char) DATA_END;
                }
            }
            dataStream << (char)EXPORT_END;
            recursion--;
            return recursion == 0 ? dataStream.str() : "";
        } else if(IS_CLASS(obj->info)) {
            ClassObject &klass = vm.classes[CLASS(obj->info)];
            dataStream << (char)EXPORT_CLASS;
            uInt fieldAddress =  GENERATION(obj->info) == gc_perm ? klass.instanceFields : 0;
            uInt fieldSize = GENERATION(obj->info) == gc_perm ? klass.staticFields : klass.instanceFields;

            dataStream << klass.fullName.str() << (char)DATA_END;
            dataStream << (char)EXPORT_DATA;
            dataStream << (GENERATION(obj->info) == gc_perm ? 1 : 0);
            dataStream << (obj->array ? 1 : 0);

            if(obj->array) {
                for(uInt i = 0; i < obj->size; i++) {
                    SharpObject *objField = obj->node[i].object;
                    if(objField != NULL) {
                        if (!exportStreamInfo.get((Int)objField, refResult)) {
                            export_obj(objField);
                        } else {
                            dataStream << (char)EXPORT_REFERENCE;
                            dataStream << refResult << (char) DATA_END;
                        }
                    }
                    else
                        dataStream << (char)EXPORT_EMPTY;

                }
            } else {
                if(fieldSize != obj->size) {
                    recursion=0;
                    exportStreamInfo.free();
                    throw Exception(vm.IncompatibleClassExcept, "class: " + klass.name.str() + " size does not match field count");
                }

                for(Int i = 0; i < fieldSize; i++) {
                    Field &field = klass.fields[fieldAddress++];

                    dataStream << (char)EXPORT_FIELD;
                    dataStream << field.type << (char)DATA_END;
                    dataStream << field.name.str() << (char)DATA_END;
                    if (obj->node[i].object == NULL) {
                        dataStream << (char)EXPORT_EMPTY;
                    } else {
                        if (!exportStreamInfo.get((Int)obj->node[i].object, refResult)) {
                            export_obj(obj->node[i].object);
                        } else {
                            dataStream << (char)EXPORT_REFERENCE;
                            dataStream << refResult << (char) DATA_END;
                        }
                    }
                }
            }

            dataStream << (char)EXPORT_END;

            recursion--;
            return recursion == 0 ? dataStream.str() : "";
        } else {
            dataStream << (char)EXPORT_DATA;
            for(uInt i = 0; i < obj->size; i++) {
                if(obj->node[i].object != NULL) {
                    if (!exportStreamInfo.get((Int)obj->node[i].object, refResult)) {
                        export_obj(obj->node[i].object);
                    } else {
                        dataStream << (char)EXPORT_REFERENCE;
                        dataStream << refResult << (char) DATA_END;
                    }
                }
                else
                    dataStream << (char)EXPORT_EMPTY;
            }
            dataStream << (char)EXPORT_END;

            recursion--;
            return recursion == 0 ? dataStream.str() : "";
        }
    } else {
        recursion=0;
        exportStreamInfo.free();
        throw Exception(vm.NullptrExcept, "");
    }

    recursion--;
    return "";
}

Int pos, streamSize;
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
        if(ch == DATA_END) {
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
        if(ch == DATA_END) {
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
        if(ch == DATA_END) {
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
        SharpObject *objPtr = NULL;

        if(type == _stype_var) {
            int ntype = (int)data[++pos];
            expectChar(data, EXPORT_DATA);
            SharpObject *object = gc.newObject(size, ntype);
            importStreamInfo.put(refrenceId, object);

            INC_REF(object)
            if(ntype == _INT8) {
                string str = readString(data);
                for (Int i = 0; i < size; i++) {
                    object->HEAD[i]
                            = str[i];
                }
            } else {
                for (Int i = 0; i < size; i++) {
                    object->HEAD[i]
                            = readDouble(data);
                }
            }

            expectChar(data, EXPORT_END);
            DEC_REF(object)
            return object;
        } else if(peekChar(data) == EXPORT_CLASS) {
            pos++;
            string className = readString(data);
            ClassObject *klass = vm.resolveClass(className);
            if(klass != NULL) {
                expectChar(data, EXPORT_DATA);
                bool staticInit = (char)data[++pos] == '1';
                bool isArray = (char)data[++pos] == '1';
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
                    importStreamInfo.put(refrenceId, object);

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
                                        Int refPtr = readInt(data);
                                        if(importStreamInfo.get(refPtr, objPtr)) {
                                            *fieldObj = objPtr;
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
                    importStreamInfo.put(refrenceId, object);

                    INC_REF(object)
                    for(Int i = 0; i < size; i++) {
                        if(peekChar(data) == EXPORT_EMPTY)
                            pos++;
                        else if(peekChar(data) == EXPORT_REFERENCE) {
                            pos++;
                            Int refPtr = readInt(data);

                            if(importStreamInfo.get(refPtr, objPtr)) {
                                object->node[i] = objPtr;
                            } else {
                                stringstream ss;
                                ss << "could not locate object with reference id of: " << refrenceId << " in the object stream";
                                throw Exception(vm.IllStateExcept, ss.str());
                            }
                        } else {
                            object->node[i] = load_obj(obj);
                        }
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
        } else {
            expectChar(data, EXPORT_DATA);
            SharpObject *object = gc.newObjectArray(size);
            importStreamInfo.put(refrenceId, object);

            INC_REF(object)
            for(Int i = 0; i < size; i++) {
                if(peekChar(data) == EXPORT_EMPTY)
                    pos++;
                else if(peekChar(data) == EXPORT_REFERENCE) {
                    pos++;
                    Int refPtr = readInt(data);
                    if(importStreamInfo.get(refPtr, objPtr)) {
                        object->node[i] = objPtr;
                    } else {
                        stringstream ss;
                        ss << "could not locate object with reference id of: " << refrenceId << " in the object stream";
                        throw Exception(vm.IllStateExcept, ss.str());
                    }
                } else {
                    object->node[i] = load_obj(obj);
                }
            }
            expectChar(data, EXPORT_END);
            DEC_REF(object)
            return object;
        }
    } else {
        throw Exception(vm.ObjectImportError, "export secret not found");
    }
}

void import_obj(SharpObject* obj) {
    if(obj && TYPE(obj->info) == _stype_var && obj->size > 0) {
        pos=-1; streamSize=obj->size;
        (++thread_self->this_fiber->sp)->object
           = load_obj(obj);
    } else {
        throw Exception(vm.NullptrExcept, "");
    }
}

