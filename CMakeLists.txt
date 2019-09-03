cmake_minimum_required(VERSION 3.6)
project(Sharp)

set(CMAKE_CXX_STANDARD 11)

#set(POSIX "os")
if (POSIX)

    set(BUILD_SHARED_LIBS OFF)
    set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-static")

    set(CMAKE_CXX_FLAGS_DEBUG  "-Werror -pthread -lstdc++ ${CMAKE_CXX_FLAGS_DEBUG}")
    set(CMAKE_CXX_FLAGS_MINSIZEREL  "-Werror -lstdc++ -s -O3 -Ofast ${CMAKE_CXX_FLAGS_MINSIZEREL}")
    set(CMAKE_CXX_FLAGS_RELEASE  " -pthread -lstdc++ -O3 -Ofast ${CMAKE_CXX_FLAGS_RELEASE}")
else()

    set(BUILD_SHARED_LIBS OFF)
    set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-static")

    set(CMAKE_CXX_FLAGS_DEBUG  "-Werror -g -lstdc++ ${CMAKE_CXX_FLAGS_DEBUG}")
    set(CMAKE_CXX_FLAGS_MINSIZEREL  "-Werror -lstdc++ -s -O3 -Ofast ${CMAKE_CXX_FLAGS_MINSIZEREL}")
    set(CMAKE_CXX_FLAGS_RELEASE   " -Werror -lstdc++  -O3 -Ofast ${CMAKE_CXX_FLAGS_RELEASE}")
endif ()

#set(SHARPC "build")

# compiler
if (SHARPC)
    add_definitions(-DMAKE_COMPILER)
    set(SOURCE_FILES main.cpp lib/grammar/Runtime.cpp lib/grammar/Runtime.h lib/grammar/parser/tokenizer/tokenentity.cpp lib/grammar/parser/tokenizer/tokenentity.h lib/grammar/parser/tokenizer/tokentype.h lib/grammar/parser/tokenizer/tokenizer.cpp lib/grammar/parser/tokenizer/tokenizer.h lib/grammar/List.h lib/grammar/parser/ErrorManager.cpp lib/grammar/parser/ErrorManager.h lib/util/KeyPair.h lib/grammar/parser/Ast.cpp lib/grammar/parser/Ast.h lib/grammar/parser/Parser.cpp lib/grammar/parser/Parser.h lib/grammar/parser/ParserState.h lib/util/zip/zcompress.cpp lib/util/zip/zcompress.h lib/util/zip/zcore.cpp lib/util/zip/zcore.h lib/util/zip/zdecompress.cpp lib/util/zip/zdecompress.h lib/util/zip/zinclude.h lib/util/zip/zlib.h lib/util/zip/zlib.cpp lib/util/zip/zstream.cpp lib/util/zip/zstream.h lib/util/File.cpp lib/util/File.h lib/util/time.cpp lib/util/time.h lib/grammar/ClassObject.cpp lib/grammar/ClassObject.h lib/grammar/Field.cpp lib/grammar/Field.h lib/grammar/FieldType.h lib/grammar/AccessModifier.h lib/grammar/RuntimeNote.h lib/grammar/Param.cpp lib/grammar/Param.h lib/grammar/Method.h lib/grammar/Assembler.h lib/grammar/Method.cpp lib/grammar/Exception.h lib/grammar/OperatorOverload.h stdimports.h lib/runtime/Opcode.h lib/runtime/register.h lib/grammar/Asm.h lib/grammar/Asm.cpp lib/runtime/oo/string.h lib/runtime/Exe.h lib/grammar/Optimizer.cpp lib/grammar/Optimizer.h lib/grammar/Runtime2.cpp)

    add_executable(Sharpc ${SOURCE_FILES})
else()
    #virtual machine
    add_definitions(-DDEBUGGING)
    file(GLOB jit_SRC
            "lib/util/jit/asmjit/src/asmjit/*.h"
            "lib/util/jit/asmjit/src/asmjit/x86/*.h"
            "lib/util/jit/asmjit/src/asmjit/x86/*.cpp"
            "lib/util/jit/asmjit/src/asmjit/core/*.h"
            "lib/util/jit/asmjit/src/asmjit/core/*.cpp"
            "lib/runtime/jit.cpp.old"
            "lib/runtime/jit.h.old"
    )
    #            "lib/runtime/jit.cpp.old"
    #            "lib/runtime/jit.h.old"
    set(SOURCE_FILES main.cpp  lib/runtime/main.cpp lib/runtime/oo/string.h lib/runtime/List.h lib/runtime/memory/GarbageCollector.cpp lib/runtime/memory/GarbageCollector.h lib/runtime/oo/Object.h lib/runtime/oo/Exception.cpp lib/runtime/oo/Exception.h lib/runtime/oo/Field.h lib/runtime/oo/ClassObject.cpp lib/runtime/oo/ClassObject.h lib/runtime/register.h lib/runtime/oo/Method.h lib/util/fmt/include/core.h lib/util/fmt/include/format.h lib/util/fmt/include/format-inl.h lib/util/fmt/include/ostream.h lib/util/fmt/include/posix.h lib/util/fmt/include/printf.h lib/util/fmt/include/ranges.h lib/util/fmt/include/time.h lib/util/fmt/src/format.cc lib/util/fmt/src/posix.cc lib/runtime/Thread.cpp lib/runtime/Thread.h lib/runtime/oo/Object.cpp lib/runtime/Manifest.h lib/runtime/Manifest.cpp lib/runtime/Exe.h lib/runtime/Exe.cpp lib/runtime/Environment.cpp lib/util/zip/zcompress.cpp lib/util/zip/zcompress.h lib/util/zip/zcore.cpp lib/util/zip/zcore.h lib/util/zip/zdecompress.cpp lib/util/zip/zdecompress.h lib/util/zip/zinclude.h lib/util/zip/zlib.cpp lib/util/zip/zlib.h lib/util/zip/zstream.cpp lib/util/zip/zstream.h lib/runtime/Environment.h lib/runtime/VirtualMachine.cpp lib/runtime/VirtualMachine.h lib/runtime/main.h lib/util/File.cpp lib/util/File.h lib/util/time.cpp lib/util/time.h lib/util/KeyPair.h stdimports.h lib/runtime/Opcode.h lib/grammar/Optimizer.cpp lib/grammar/Optimizer.h lib/Modules/std.io/fileio.cpp lib/Modules/std.io/fileio.h lib/runtime/profiler.h lib/runtime/profiler.cpp lib/runtime/SignalHandler.cpp lib/runtime/termios.cpp lib/runtime/termios.h lib/Modules/std.kernel/cmath.cpp lib/Modules/std.kernel/cmath.h lib/Modules/std.kernel/clist.cpp lib/Modules/std.kernel/clist.h lib/Modules/std/Random.cpp lib/Modules/std/Random.h lib/Modules/std.os.gui/win/Gui.cpp lib/Modules/std.os.gui/win/Gui.h ${jit_SRC} lib/runtime/JitAssembler.cpp lib/runtime/JitAssembler.h lib/runtime/Jit.cpp lib/runtime/Jit.h lib/runtime/x64Assembler.cpp lib/runtime/x64Assembler.h)

    add_executable(Sharp ${SOURCE_FILES})
endif ()
