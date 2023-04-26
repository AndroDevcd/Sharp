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

    set(CMAKE_CXX_FLAGS_DEBUG  "-std=c++11 -Werror  -g -lstdc++   ${CMAKE_C_FLAGS_DEBUG}")
    set(CMAKE_CXX_FLAGS_MINSIZEREL  "-std=c++11 -mno-red-zone -Werror -lstdc++ -s -O3 -Ofast ${CMAKE_C_FLAGS_MINSIZEREL}")
    set(CMAKE_CXX_FLAGS_RELEASE   "-std=c++11  -mno-red-zone -Werror -lstdc++  -O3 -g ${CMAKE_C_FLAGS_RELEASE}")
endif ()

#set(SHARPC "build")
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
            "lib/grammar/backend/compiler/expressions/*.h"
            "lib/grammar/backend/compiler/expressions/*.cpp"
            "lib/grammar/backend/compiler/expressions/binary/*.h"
            "lib/grammar/backend/compiler/expressions/binary/*.cpp"
            "lib/grammar/backend/compiler/expressions/primary/*.h"
            "lib/grammar/backend/compiler/expressions/primary/*.cpp"
            "lib/grammar/backend/compiler/expressions/unary/*.h"
            "lib/grammar/backend/compiler/expressions/unary/*.cpp"
            "lib/grammar/backend/operation/*.h"
            "lib/grammar/backend/operation/*.cpp"
            "lib/grammar/backend/*.h"
            "lib/grammar/backend/*.cpp"
            "lib/grammar/backend/types/*.h"
            "lib/grammar/backend/types/*.cpp"
            "lib/grammar/backend/compiler/di/*.h"
            "lib/grammar/backend/compiler/di/*.cpp"
            "lib/grammar/backend/compiler/functions/*.h"
            "lib/grammar/backend/compiler/functions/*.cpp"
            "lib/grammar/backend/compiler/functions/statements/*.h"
            "lib/grammar/backend/compiler/functions/statements/*.cpp"
            "lib/grammar/backend/finalizer/*.h"
            "lib/grammar/backend/finalizer/*.cpp"
            "lib/grammar/backend/finalizer/optimization/*.h"
            "lib/grammar/backend/finalizer/optimization/*.cpp"
            "lib/grammar/backend/finalizer/generation/*.h"
            "lib/grammar/backend/finalizer/generation/*.cpp"
            "lib/grammar/backend/finalizer/generation/code/*.h"
            "lib/grammar/backend/finalizer/generation/code/*.cpp"
            "lib/grammar/backend/finalizer/generation/code/scheme/*.h"
            "lib/grammar/backend/finalizer/generation/code/scheme/*.cpp"
            "lib/util/*.cpp"
            "lib/util/zip/*.cpp"
            "lib/core/*.cpp"
            "lib/core/*.h"
            "lib/core/opcode/*.cpp"
            "lib/core/opcode/*.h"
    )


    set(SOURCE_FILES main.cpp stdimports.h ${compiler_SRC})

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
            "lib/core/*.cpp"
            "lib/core/*.h"
            "lib/core/opcode/*.cpp"
            "lib/core/opcode/*.h"
            )


    set(SOURCE_FILES main.cpp stdimports.h ${compiler_SRC})

    add_executable(LegacySharpc ${SOURCE_FILES})
else()
    #virtual machine
    add_definitions(-DDEBUGGING)

    file(GLOB runtime_SRC
            "lib/runtime/error/*.h"
            "lib/runtime/error/*.cpp"
            "lib/runtime/exe/*.h"
            "lib/runtime/exe/*.cpp"
            "lib/runtime/memory/*.h"
            "lib/runtime/memory/*.cpp"
            "lib/runtime/reflect/*.h"
            "lib/runtime/reflect/*.cpp"
            "lib/runtime/multitasking/*/*.h"
            "lib/runtime/multitasking/*/*.cpp"
            "lib/runtime/types/*.h"
            "lib/runtime/types/*.cpp"
            "lib/runtime/*.h"
            "lib/runtime/*.cpp"
            "lib/core/*.cpp"
            "lib/core/*.h"
            "lib/core/opcode/*.cpp"
            "lib/core/opcode/*.h"
            "lib/util/*.cpp"
            "lib/util/*.h"
            )

    set(SOURCE_FILES main.cpp stdimports.h ${runtime_SRC})
    if (POSIX)
        set(LRT_LIB -lrt -ldl)
    endif ()

    add_executable(Sharp ${SOURCE_FILES})

    if (POSIX)
        target_link_libraries(Sharp ${LRT_LIB})
    endif ()
endif ()
