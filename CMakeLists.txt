cmake_minimum_required(VERSION 3.6)
project(Sharp)

set(CMAKE_CXX_STANDARD 11)

set(BUILD_SHARED_LIBS OFF)
set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-static")

#set(CMAKE_CXX_FLAGS_DEBUG  " ${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-s -Os -O3 -Ofast ${CMAKE_CXX_FLAGS_MINSIZEREL}")

# compiler
add_definitions(-DMAKE_COMPILER)
set(SOURCE_FILES main.cpp)
#virtual machine
#set(SOURCE_FILES main.cpp)

add_executable(Sharp ${SOURCE_FILES})