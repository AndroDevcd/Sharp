cmake_minimum_required(VERSION 3.6)
project(Sharp)

set(CMAKE_C_STANDARD 11)

#set(POSIX "os")
if (POSIX)

    set(BUILD_SHARED_LIBS OFF)
#-fsanitize=address -fno-omit-frame-pointer
    set(CMAKE_CXX_FLAGS_DEBUG  " -fsanitize=address -fPIE -pie -fno-omit-frame-pointer -mno-red-zone -std=c++11 -O0 -Werror -pthread -lstdc++ -g ${CMAKE_CXX_FLAGS_DEBUG}")
    set(CMAKE_CXX_FLAGS_MINSIZEREL  "-Werror -lstdc++ -mno-red-zone -s -O3 -Ofast ${CMAKE_CXX_FLAGS_MINSIZEREL}")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO   "-fsanitize=address  -mno-red-zone -pthread -lstdc++ -O3 -Ofast ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    set(CMAKE_CXX_FLAGS_RELEASE   "-mno-red-zone  -pthread -lstdc++ -O3 -Ofast ${CMAKE_CXX_FLAGS_RELEASE}")
else()

    set(BUILD_SHARED_LIBS OFF)
    set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-static")

    set(CMAKE_CXX_FLAGS_DEBUG  "-std=c++11 -Werror -g -lstdc++   ${CMAKE_C_FLAGS_DEBUG}")
    set(CMAKE_CXX_FLAGS_MINSIZEREL  "-std=c++11 -mno-red-zone -Werror -lstdc++ -s -O3 -Ofast ${CMAKE_C_FLAGS_MINSIZEREL}")
    set(CMAKE_CXX_FLAGS_RELEASE   "-std=c++11  -mno-red-zone -Werror -lstdc++  -O3 -g ${CMAKE_C_FLAGS_RELEASE}")
endif ()

set(SHARPC "build")
#set(LEGACY "build")

# compiler
if (SHARPC)
    add_definitions(-DMAKE_COMPILER)

    file(GLOB compiler_SRC
            "lib/grammar/*.h"
            "lib/grammar/*.cpp"
            "lib/grammar/json/*.h"
            "lib/grammar/json/*.cpp"
            "lib/grammar/settings/*.h"
            "lib/grammar/settings/*.cpp"
            "lib/grammar/frontend/tokenizer/*.h"
            "lib/grammar/frontend/tokenizer/*.cpp"
            "lib/grammar/frontend/parser/*.h"
            "lib/grammar/frontend/parser/*.cpp"
            "lib/grammar/frontend/*.h"
            "lib/grammar/frontend/*.cpp"
            "lib/grammar/taskdelegator/*.h"
            "lib/grammar/taskdelegator/*.cpp"
            "lib/grammar/backend/dependency/*.h"
            "lib/grammar/backend/dependency/*.cpp"
            "lib/grammar/backend/preprocessor/*.h"
            "lib/grammar/backend/preprocessor/*.cpp"
            "lib/grammar/backend/postprocessor/*.h"
            "lib/grammar/backend/postprocessor/*.cpp"
            "lib/grammar/backend/astparser/*.h"
            "lib/grammar/backend/astparser/*.cpp"
            "lib/grammar/backend/context/*.h"
            "lib/grammar/backend/context/*.cpp"
            "lib/grammar/backend/compiler/*.h"
            "lib/grammar/backend/compiler/*.cpp"
            "lib/grammar/backend/operation/*.h"
            "lib/grammar/backend/operation/*.cpp"
            "lib/grammar/backend/*.h"
            "lib/grammar/backend/*.cpp"
            "lib/grammar/backend/types/*.h"
            "lib/grammar/backend/types/*.cpp"
            "lib/util/*.cpp"
    )


    set(SOURCE_FILES main.cpp stdimports.h ${compiler_SRC} lib/grammar/compiler_info.cpp lib/grammar/backend/meta_data.h lib/grammar/backend/meta_data.cpp lib/grammar/backend/types/sharp_type.h)

    add_executable(Sharpc ${SOURCE_FILES})
elseif(LEGACY)
    add_definitions(-DMAKE_COMPILER)

    file(GLOB compiler_SRC
            "lib/old_grammar/*.h"
            "lib/old_grammar/*.cpp"
            "lib/old_grammar/optimizer/*.h"
            "lib/old_grammar/optimizer/*.cpp"
            "lib/old_grammar/generator/*.h"
            "lib/old_grammar/generator/*.cpp"
            "lib/old_grammar/frontend/tokenizer/*.h"
            "lib/old_grammar/frontend/tokenizer/*.cpp"
            "lib/old_grammar/frontend/parser/*.h"
            "lib/old_grammar/frontend/parser/*.cpp"
            "lib/old_grammar/frontend/*.h"
            "lib/old_grammar/frontend/*.cpp"
            "lib/old_grammar/backend/*.h"
            "lib/old_grammar/backend/*.cpp"
            "lib/old_grammar/backend/oo/*.h"
            "lib/old_grammar/backend/oo/*.cpp"
            "lib/old_grammar/backend/ofuscation/*.h"
            "lib/old_grammar/backend/ofuscation/*.cpp"
            "lib/old_grammar/backend/data/*.h"
            "lib/old_grammar/backend/data/*.cpp"
            "lib/old_grammar/backend/code/*.h"
            "lib/util/*.cpp"
            "lib/util/*.h"
            "lib/util/zip/*.cpp"
            "lib/util/zip/*.h"
            "lib/runtime/Opcode.h"
            "lib/runtime/Opcode.cpp"
            )


    set(SOURCE_FILES main.cpp stdimports.h ${compiler_SRC})

    add_executable(LegacySharpc ${SOURCE_FILES})
else()
    #virtual machine
    add_definitions(-DDEBUGGING)

    #            "lib/runtime/jit.cpp.old"
    #            "lib/runtime/jit.h.old"
    set(SOURCE_FILES main.cpp lib/runtime/scheduler/scheduler.cpp lib/runtime/main.cpp lib/runtime/architecture.h lib/runtime/List.h lib/runtime/memory/GarbageCollector.cpp lib/runtime/memory/GarbageCollector.h lib/runtime/symbols/Object.h lib/runtime/symbols/Exception.cpp lib/runtime/symbols/Exception.h lib/runtime/symbols/Field.h lib/runtime/symbols/ClassObject.cpp lib/runtime/symbols/ClassObject.h lib/runtime/register.h lib/runtime/fiber.h lib/runtime/fiber.cpp lib/runtime/symbols/Method.h lib/util/fmt/include/core.h lib/util/fmt/include/format.h lib/util/fmt/include/format-inl.h lib/util/fmt/include/ostream.h lib/util/fmt/include/posix.h lib/util/fmt/include/printf.h lib/util/fmt/include/ranges.h lib/util/fmt/include/time.h lib/util/fmt/src/format.cc lib/util/fmt/src/posix.cc lib/runtime/Thread.cpp lib/runtime/Thread.h lib/runtime/symbols/Object.cpp lib/runtime/Manifest.h lib/runtime/Manifest.cpp lib/runtime/Exe.h lib/runtime/Exe.cpp lib/util/zip/zcompress.cpp lib/util/zip/zcompress.h lib/util/zip/zcore.cpp lib/util/zip/zcore.h lib/util/zip/zdecompress.cpp lib/util/zip/zdecompress.h lib/util/zip/zinclude.h lib/util/zip/zlib.cpp lib/util/zip/zlib.h lib/util/zip/zstream.cpp lib/util/zip/zstream.h lib/runtime/VirtualMachine.cpp lib/runtime/VirtualMachine.h lib/runtime/main.h lib/util/File.cpp lib/util/File.h lib/util/time.cpp lib/util/time.h lib/util/KeyPair.h stdimports.h lib/runtime/Opcode.h lib/Modules/std.io/fileio.cpp lib/Modules/std.io/fileio.h lib/runtime/profiler.h lib/runtime/profiler.cpp lib/runtime/SignalHandler.cpp lib/runtime/termios.cpp lib/runtime/termios.h lib/Modules/math/cmath.cpp lib/Modules/math/cmath.h lib/Modules/std/Random.cpp lib/Modules/std/Random.h lib/Modules/std.os.gui/win/Gui.cpp lib/Modules/std.os.gui/win/Gui.h lib/runtime/Opcode.cpp lib/util/HashMap.h lib/runtime/symbols/symbol.h lib/runtime/symbols/symbol.cpp lib/runtime/symbols/param.h lib/Modules/std.io/memory.cpp lib/Modules/std.io/memory.h lib/runtime/snb/snb.cpp lib/runtime/snb/snb.h lib/runtime/Library.h lib/runtime/snb/util.h lib/runtime/scheduler/scheduler.h lib/runtime/linked_list.h lib/Modules/std.io/serialization.h lib/Modules/std.io/serialization.cpp lib/runtime/architecture.h lib/runtime/OpcodeInjection.h
             lib/runtime/scheduler/thread_controller.cpp lib/runtime/scheduler/thread_controller.h lib/runtime/scheduler/task_controller.cpp lib/runtime/scheduler/task_controller.h lib/runtime/scheduler/thread_exception_controller.cpp lib/runtime/scheduler/thread_exception_controller.h lib/runtime/scheduler/idle_scheduler.cpp lib/runtime/scheduler/idle_scheduler.h lib/grammar/backend/access_flag.h lib/grammar/backend/access_flag.cpp)
    if (POSIX)
        set(LRT_LIB -lrt -ldl)
    endif ()

    add_executable(Sharp ${SOURCE_FILES})

    if (POSIX)
        target_link_libraries(Sharp ${LRT_LIB})
    endif ()
endif ()
