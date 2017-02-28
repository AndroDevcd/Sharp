cmake_minimum_required(VERSION 3.6)
project(Sharp)

set(CMAKE_CXX_STANDARD 11)


set(BUILD_SHARED_LIBS OFF)
set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-static")

#set(CMAKE_CXX_FLAGS_DEBUG  "-s ${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-s -Os -O3 -Ofast ${CMAKE_CXX_FLAGS_MINSIZEREL}")

# compiler
#set(SOURCE_FILES main.cpp lib/grammar/parser/tokenizer/tokenentity.h lib/grammar/parser/tokenizer/tokenizer.h lib/grammar/parser/tokenizer/tokentype.h stdimports.h lib/grammar/parser/tokenizer/tokenentity.cpp lib/grammar/parser/tokenizer/tokenizer.cpp lib/grammar/parser/parseerrors.h lib/util/keypair.h lib/grammar/parser/parseerrors.cpp lib/grammar/parser/parser.h lib/grammar/parser/ast.h lib/grammar/parser/ast.cpp lib/grammar/parser/parser.cpp lib/util/file.h lib/util/file.cpp lib/grammar/parser/parserstate.h lib/grammar/runtime.h lib/grammar/runtime.cpp lib/grammar/ClassObject.cpp lib/grammar/ClassObject.h lib/grammar/Method.cpp lib/grammar/Method.h lib/grammar/Field.h lib/grammar/NativeField.h lib/grammar/AccessModifier.h lib/grammar/Param.h lib/grammar/Param.cpp lib/grammar/Field.cpp lib/grammar/OperatorOverload.h lib/grammar/RuntimeNote.h lib/grammar/Environment.cpp lib/grammar/Environment.h lib/grammar/BytecodeStream.h lib/grammar/Opcode.h)

#virtual machine
set(SOURCE_FILES main.cpp lib/runtime/startup.h lib/runtime/startup.cpp lib/util/keypair.h lib/util/file.h lib/util/file.cpp lib/runtime/internal/Environment.cpp lib/runtime/internal/Environment.h lib/runtime/oo/Method.cpp lib/runtime/oo/Method.h lib/runtime/oo/Object.cpp lib/runtime/oo/Object.h lib/runtime/oo/ClassObject.cpp lib/runtime/oo/ClassObject.h lib/runtime/alloc/GC.cpp lib/runtime/alloc/GC.h lib/runtime/Manifest.h lib/runtime/Meta.cpp lib/runtime/Meta.h lib/runtime/interp/vm.h lib/runtime/interp/vm.cpp lib/runtime/oo/Array.cpp lib/runtime/oo/Array.h lib/runtime/internal/Exe.h lib/runtime/internal/Exe.cpp lib/runtime/alloc/mark.h lib/runtime/oo/Field.cpp lib/runtime/oo/Field.h lib/runtime/internal/Thread.cpp lib/runtime/internal/Thread.h lib/runtime/oo/string.cpp lib/runtime/oo/string.h lib/runtime/interp/Opcode.h lib/runtime/interp/FastStack.cpp lib/runtime/interp/FastStack.h lib/runtime/internal/Monitor.cpp lib/runtime/internal/Monitor.h lib/runtime/oo/Exception.h lib/runtime/oo/Exception.cpp lib/runtime/oo/Reference.cpp lib/runtime/oo/Reference.h lib/runtime/interp/CallStack.cpp lib/runtime/interp/CallStack.h)


add_executable(Sharp ${SOURCE_FILES})