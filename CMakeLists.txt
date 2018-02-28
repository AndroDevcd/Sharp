cmake_minimum_required(VERSION 3.6)
project(Sharp)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_C_COMPILER "C:/MinGW/bin/gcc.exe")

set(BUILD_SHARED_LIBS OFF)
set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-static")

set(CMAKE_C_FLAGS_DEBUG  "-lstdc++ ${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_C_FLAGS_RELEASE  "-lstdc++ -s -O3 -Ofast ${CMAKE_CXX_FLAGS_MINSIZEREL}")
set(SHARPC "build")

# Linux compiler console: g++ *.cpp ../../main.cpp ../util/*.cpp parser/*.cpp parser/*/*.cpp -lstdc++ -s -O3 -Ofast -o bootstrap -std=c++11

# compiler
if (SHARPC)
    add_definitions(-DMAKE_COMPILER)
    set(SOURCE_FILES main.cpp lib/grammar/Runtime.cpp lib/grammar/Runtime.h lib/grammar/parser/tokenizer/tokenentity.cpp lib/grammar/parser/tokenizer/tokenentity.h lib/grammar/parser/tokenizer/tokentype.h lib/grammar/parser/tokenizer/tokenizer.cpp lib/grammar/parser/tokenizer/tokenizer.h lib/grammar/List.h lib/grammar/parser/ErrorManager.cpp lib/grammar/parser/ErrorManager.h lib/util/KeyPair.h lib/grammar/parser/Ast.cpp lib/grammar/parser/Ast.h lib/grammar/parser/Parser.cpp lib/grammar/parser/Parser.h lib/grammar/parser/ParserState.h lib/util/File.cpp lib/util/File.h lib/util/time.cpp lib/util/time.h lib/grammar/ClassObject.cpp lib/grammar/ClassObject.h lib/grammar/Field.cpp lib/grammar/Field.h lib/grammar/FieldType.h lib/grammar/AccessModifier.h lib/grammar/RuntimeNote.h lib/grammar/Param.cpp lib/grammar/Param.h lib/grammar/Method.h lib/grammar/Assembler.h lib/grammar/Method.cpp lib/grammar/Exception.h lib/grammar/OperatorOverload.h stdimports.h lib/runtime/Opcode.h lib/runtime/register.h lib/grammar/Asm.h lib/grammar/Asm.cpp lib/runtime/oo/string.h lib/runtime/Exe.h)
else()
    #virtual machine
    add_definitions(-DDEBUGGING)
    set(SOURCE_FILES main.cpp lib/runtime/init.cpp lib/runtime/oo/string.h lib/runtime/List.h lib/runtime/memory/GarbageCollector.cpp lib/runtime/memory/GarbageCollector.h lib/runtime/oo/Object.h lib/runtime/Mutex.cpp lib/runtime/Mutex.h lib/runtime/oo/Exception.cpp lib/runtime/oo/Exception.h lib/runtime/oo/Field.h lib/runtime/oo/ClassObject.cpp lib/runtime/oo/ClassObject.h lib/runtime/register.h lib/runtime/oo/Method.h lib/runtime/Thread.cpp lib/runtime/Thread.h lib/runtime/oo/Object.cpp lib/runtime/Manifest.h lib/runtime/Manifest.cpp lib/runtime/Exe.h lib/runtime/Exe.cpp lib/runtime/Environment.cpp lib/runtime/Environment.h lib/runtime/VirtualMachine.cpp lib/runtime/VirtualMachine.h lib/runtime/init.h lib/util/File.cpp lib/util/File.h lib/util/time.cpp lib/util/time.h lib/util/KeyPair.h stdimports.h lib/runtime/Opcode.h lib/grammar/Asm.cpp lib/grammar/Asm.h)
endif ()

add_executable(Sharp ${SOURCE_FILES})
