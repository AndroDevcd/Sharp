cmake_minimum_required(VERSION 3.6)
project(Sharp)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_C_COMPILER "C:/MinGW/bin/gcc.exe")

set(BUILD_SHARED_LIBS OFF)
set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-static")

set(CMAKE_C_FLAGS_DEBUG  "-lstdc++ ${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_C_FLAGS_RELEASE  "-lstdc++ -s -O3 -Ofast ${CMAKE_CXX_FLAGS_MINSIZEREL}")

# compiler
add_definitions(-DMAKE_COMPILER)
set(SOURCE_FILES main.cpp lib/grammar/Runtime.cpp lib/grammar/Runtime.h lib/grammar/parser/tokenizer/tokenentity.cpp lib/grammar/parser/tokenizer/tokenentity.h lib/grammar/parser/tokenizer/tokentype.h lib/grammar/parser/tokenizer/tokenizer.cpp lib/grammar/parser/tokenizer/tokenizer.h lib/grammar/List.h lib/grammar/parser/ErrorManager.cpp lib/grammar/parser/ErrorManager.h lib/util/keypair.h lib/grammar/parser/Ast.cpp lib/grammar/parser/Ast.h lib/grammar/parser/Parser.cpp lib/grammar/parser/Parser.h lib/grammar/parser/ParserState.h lib/util/File.cpp lib/util/File.h lib/util/time.cpp lib/util/time.h lib/grammar/ClassObject.cpp lib/grammar/ClassObject.h lib/grammar/Field.cpp lib/grammar/Field.h lib/grammar/FieldType.h lib/grammar/AccessModifier.h lib/grammar/RuntimeNote.h lib/grammar/Param.cpp lib/grammar/Param.h lib/grammar/Method.h lib/grammar/Assembler.h lib/grammar/Method.cpp lib/grammar/Exception.h lib/grammar/OperatorOverload.h lib/runtime/init.cpp lib/runtime/oo/string.h lib/runtime/List.h lib/runtime/memory/GarbageCollector.cpp lib/runtime/memory/GarbageCollector.h lib/runtime/oo/Object.h lib/runtime/Mutex.cpp lib/runtime/Mutex.h lib/runtime/oo/Exception.cpp lib/runtime/oo/Exception.h lib/runtime/oo/Field.h lib/runtime/oo/ClassObject.cpp lib/runtime/oo/ClassObject.h)
#virtual machine
#set(SOURCE_FILES main.cpp)

add_executable(Sharp ${SOURCE_FILES})
