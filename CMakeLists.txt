cmake_minimum_required(VERSION 3.6)
project(Sharp)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp lib/parser/tokenizer/tokenentity.h lib/parser/tokenizer/tokenizer.h lib/parser/tokenizer/tokentype.h stdimports.h lib/parser/tokenizer/tokenentity.cpp lib/parser/tokenizer/tokenizer.cpp lib/parser/parseerrors.h lib/util/keypair.h lib/parser/parseerrors.cpp lib/parser/parser.h lib/parser/ast.h lib/parser/ast.cpp lib/parser/parser.cpp lib/util/file.h lib/util/file.cpp)
add_executable(Sharp ${SOURCE_FILES})