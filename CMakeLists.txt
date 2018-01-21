cmake_minimum_required(VERSION 3.6)
project(Sharp)

set(CMAKE_CXX_STANDARD 11)

set(BUILD_SHARED_LIBS OFF)
set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-static")

#set(CMAKE_CXX_FLAGS_DEBUG  " ${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-s -Os -O3 -Ofast ${CMAKE_CXX_FLAGS_MINSIZEREL}")

# compiler
add_definitions(-DMAKE_COMPILER)
set(SOURCE_FILES main.cpp lib/grammar/Runtime.cpp lib/grammar/Runtime.h lib/grammar/parser/tokenizer/tokenentity.cpp lib/grammar/parser/tokenizer/tokenentity.h lib/grammar/parser/tokenizer/tokentype.h lib/grammar/parser/tokenizer/tokenizer.cpp lib/grammar/parser/tokenizer/tokenizer.h lib/util/List2.h lib/grammar/parser/ParseError.cpp lib/grammar/parser/ParseError.h lib/util/keypair.h)
#virtual machine
#set(SOURCE_FILES main.cpp)

add_executable(Sharp ${SOURCE_FILES})