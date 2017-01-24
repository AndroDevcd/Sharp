cmake_minimum_required(VERSION 3.6)
project(Sharp)

set(CMAKE_CXX_STANDARD 11)

include_directories(/local/lib)

set(SOURCE_FILES main.cpp lib/grammer/parser/tokenizer/tokenentity.h lib/grammer/parser/tokenizer/tokenizer.h lib/grammer/parser/tokenizer/tokentype.h stdimports.h lib/grammer/parser/tokenizer/tokenentity.cpp lib/grammer/parser/tokenizer/tokenizer.cpp lib/grammer/parser/parseerrors.h lib/util/keypair.h lib/grammer/parser/parseerrors.cpp lib/grammer/parser/parser.h lib/grammer/parser/ast.h lib/grammer/parser/ast.cpp lib/grammer/parser/parser.cpp lib/util/file.h lib/util/file.cpp)
add_executable(Sharp ${SOURCE_FILES})