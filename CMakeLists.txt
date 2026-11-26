cmake_minimum_required(VERSION 3.6)
project(Sharp)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Detect OS (matching build.sh logic)
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # macOS-specific flags (no -lrt and no --no-as-needed)
    set(CXX_COMPILER "g++-15")
    set(PLATFORM_FLAGS "")
    set(STDLIB_FLAG "")
    set(REDZONE_FLAG "")
else()
    # Default flags (Linux/other)
    set(CXX_COMPILER "g++")
    set(PLATFORM_FLAGS "-Wl,--no-as-needed -lrt -mno-red-zone -fsanitize=address")
    set(STDLIB_FLAG "-lstdc++")
    set(REDZONE_FLAG "-mno-red-zone")
    set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-static")
endif()

set(BUILD_SHARED_LIBS OFF)

# Set compiler flags using platform-specific variables
set(CMAKE_CXX_FLAGS_DEBUG  "-fno-omit-frame-pointer -DASMJIT_STATIC ${REDZONE_FLAG} -std=c++17 -O0 -pthread ${STDLIB_FLAG} -g -gdwarf-4 ${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_MINSIZEREL  "-Werror ${STDLIB_FLAG} ${REDZONE_FLAG} -s -O3 -Ofast ${CMAKE_CXX_FLAGS_MINSIZEREL}")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO   "-fsanitize=address ${REDZONE_FLAG} -pthread ${STDLIB_FLAG} -O3 -Ofast ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
set(CMAKE_CXX_FLAGS_RELEASE   "${REDZONE_FLAG} -pthread ${STDLIB_FLAG} -O3 -Ofast ${CMAKE_CXX_FLAGS_RELEASE}")

# For runtime builds, also set DASMJIT_STATIC flag
#set(CMAKE_RUNTIME_CXX_FLAGS_DEBUG  "-std=c++17 -g ${STDLIB_FLAG} -DASMJIT_STATIC ${PLATFORM_FLAGS} ${CMAKE_C_FLAGS_DEBUG}")
#set(CMAKE_RUNTIME_CXX_FLAGS_RELEASE   "-std=c++17 ${REDZONE_FLAG} -Werror ${STDLIB_FLAG} -DASMJIT_STATIC ${PLATFORM_FLAGS} -O3 -g ${CMAKE_C_FLAGS_RELEASE}")

# The VM. (The compiler is a Sharp program, in compiler/: build-scripts/build.sh builds both.)
add_definitions(-DDEBUGGING)

file(GLOB runtime_SRC
        "runtime/cffi/*.h"
        "runtime/cffi/*.cpp"
        "runtime/error/*.h"
        "runtime/error/*.cpp"
        "runtime/exe/*.h"
        "runtime/exe/*.cpp"
        "runtime/memory/*.h"
        "runtime/memory/*.cpp"
        "runtime/reflect/*.h"
        "runtime/reflect/*.cpp"
        "runtime/jit/*.h"
        "runtime/jit/*.cpp"
        "runtime/jit/arm/*.h"
        "runtime/jit/arm/*.cpp"
        "runtime/jit/arm/opcode/*.h"
        "runtime/jit/arm/opcode/*.cpp"
        "runtime/jit/arm/sections/*.h"
        "runtime/jit/arm/sections/*.cpp"
        "runtime/jit/x64/*.h"
        "runtime/jit/x64/*.cpp"
        "runtime/jit/x64/opcode/*.cpp"
        "runtime/jit/x64/sections/*.cpp"
        "runtime/telemetry/*.h"
        "runtime/telemetry/*.cpp"
        "runtime/sni/*.h"
        "runtime/sni/*.cpp"
        "runtime/multitasking/*/*.h"
        "runtime/multitasking/*/*.cpp"
        "runtime/types/*.h"
        "runtime/types/*.cpp"
        "runtime/interrupts/*.h"
        "runtime/interrupts/*.cpp"
        "runtime/*.h"
        "runtime/*.cpp"
        "runtime/opcode/*.cpp"
        "runtime/opcode/*.h"
        "runtime/util/*.cpp"
        "runtime/util/*.h"
        "runtime/jit/asmjit/src/asmjit/*.cpp"
        "runtime/jit/asmjit/src/asmjit/*.h"
        "runtime/jit/asmjit/src/asmjit/arm/*.cpp"
        "runtime/jit/asmjit/src/asmjit/arm/*.h"
        "runtime/jit/asmjit/src/asmjit/core/*.cpp"
        "runtime/jit/asmjit/src/asmjit/core/*.h"
        "runtime/jit/asmjit/src/asmjit/x86/*.cpp"
        "runtime/jit/asmjit/src/asmjit/x86/*.h"
        "runtime/interrupts/native/*.h"
        "runtime/interrupts/native/*.cpp"
        )

set(SOURCE_FILES runtime/entry/main.cpp ${runtime_SRC}
        )
add_executable(Sharp ${SOURCE_FILES})

# Link libraries based on platform  
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_link_libraries(Sharp stdc++ m dl)
else()
    target_link_libraries(Sharp stdc++ rt m dl)
endif()
