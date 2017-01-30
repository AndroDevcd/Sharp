cmake_minimum_required(VERSION 3.6)
project(Sharp)

set(CMAKE_CXX_STANDARD 11)

include_directories(/local/lib)

set(SOURCE_FILES main.cpp lib/grammar/parser/tokenizer/tokenentity.h lib/grammar/parser/tokenizer/tokenizer.h lib/grammar/parser/tokenizer/tokentype.h stdimports.h lib/grammar/parser/tokenizer/tokenentity.cpp lib/grammar/parser/tokenizer/tokenizer.cpp lib/grammar/parser/parseerrors.h lib/util/keypair.h lib/grammar/parser/parseerrors.cpp lib/grammar/parser/parser.h lib/grammar/parser/ast.h lib/grammar/parser/ast.cpp lib/grammar/parser/parser.cpp lib/util/file.h lib/util/file.cpp lib/grammar/parser/parserstate.h lib/grammar/runtime.h lib/grammar/runtime.cpp)
add_executable(Sharp ${SOURCE_FILES} lib/grammar/parser/parserstate.h lib/grammar/runtime.cpp)