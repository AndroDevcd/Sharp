#!/bin/bash

# Detect OS
if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS-specific flags (no -lrt and no --no-as-needed)
    CXX_COMPILER="g++-15"
    PLATFORM_FLAGS=""
    INSTALL_BIN_DIR="/usr/local/bin"
    INSTALL_INCLUDE_DIR="/usr/local/include/sharp"
else
    # default flags
    CXX_COMPILER="g++"
    PLATFORM_FLAGS="-Wl,--no-as-needed -lrt -mno-red-zone"
    INSTALL_BIN_DIR="/bin"
    INSTALL_INCLUDE_DIR="/usr/include/sharp"
fi

# Function to build the Sharp compiler
build_compiler() {
    echo "Building Sharp compiler..."
    $CXX_COMPILER -o bin/sharpc -std=c++11 -pthread -lstdc++ -O3 -Ofast -g main.cpp stdimports.h -DMAKE_COMPILER \
        lib/grammar/*.h lib/grammar/*.cpp \
        lib/grammar/json/*.h lib/grammar/json/*.cpp \
        lib/grammar/settings/*.h lib/grammar/settings/*.cpp \
        lib/grammar/frontend/tokenizer/*.h lib/grammar/frontend/tokenizer/*.cpp \
        lib/grammar/frontend/parser/*.h lib/grammar/frontend/parser/*.cpp \
        lib/grammar/frontend/*.h lib/grammar/frontend/*.cpp \
        lib/grammar/taskdelegator/*.h lib/grammar/taskdelegator/*.cpp \
        lib/grammar/backend/dependency/*.h lib/grammar/backend/dependency/*.cpp \
        lib/grammar/backend/preprocessor/*.h lib/grammar/backend/preprocessor/*.cpp \
        lib/grammar/backend/postprocessor/*.h lib/grammar/backend/postprocessor/*.cpp \
        lib/grammar/backend/astparser/*.h lib/grammar/backend/astparser/*.cpp \
        lib/grammar/backend/context/*.h lib/grammar/backend/context/*.cpp \
        lib/grammar/backend/compiler/*.h lib/grammar/backend/compiler/*.cpp \
        lib/grammar/backend/compiler/expressions/*.h lib/grammar/backend/compiler/expressions/*.cpp \
        lib/grammar/backend/compiler/expressions/binary/*.h lib/grammar/backend/compiler/expressions/binary/*.cpp \
        lib/grammar/backend/compiler/expressions/primary/*.h lib/grammar/backend/compiler/expressions/primary/*.cpp \
        lib/grammar/backend/compiler/expressions/unary/*.h lib/grammar/backend/compiler/expressions/unary/*.cpp \
        lib/grammar/backend/operation/*.h lib/grammar/backend/operation/*.cpp \
        lib/grammar/backend/*.h lib/grammar/backend/*.cpp \
        lib/grammar/backend/types/*.h lib/grammar/backend/types/*.cpp \
        lib/grammar/backend/compiler/di/*.h lib/grammar/backend/compiler/di/*.cpp \
        lib/grammar/backend/compiler/functions/*.h lib/grammar/backend/compiler/functions/*.cpp \
        lib/grammar/backend/compiler/functions/statements/*.h lib/grammar/backend/compiler/functions/statements/*.cpp \
        lib/grammar/backend/finalizer/*.h lib/grammar/backend/finalizer/*.cpp \
        lib/grammar/backend/finalizer/optimization/*.h lib/grammar/backend/finalizer/optimization/*.cpp \
        lib/grammar/backend/finalizer/optimization/codemotion/*.h lib/grammar/backend/finalizer/optimization/codemotion/*.cpp \
        lib/grammar/backend/finalizer/optimization/analyzer/*.h lib/grammar/backend/finalizer/optimization/analyzer/*.cpp \
        lib/grammar/backend/finalizer/optimization/processor/*.h lib/grammar/backend/finalizer/optimization/processor/*.cpp \
        lib/grammar/backend/finalizer/generation/*.h lib/grammar/backend/finalizer/generation/*.cpp \
        lib/grammar/backend/finalizer/generation/code/*.h lib/grammar/backend/finalizer/generation/code/*.cpp \
        lib/grammar/backend/finalizer/generation/obfuscation/*.h lib/grammar/backend/finalizer/generation/obfuscation/*.cpp \
        lib/grammar/backend/finalizer/generation/code/scheme/*.h lib/grammar/backend/finalizer/generation/code/scheme/*.cpp \
        lib/util/*.cpp lib/util/zip/*.cpp \
        lib/core/*.cpp lib/core/*.h \
        lib/core/opcode/*.cpp lib/core/opcode/*.h

    if [ $? -eq 0 ]; then
        echo "Sharp compiler built successfully!"
    else
        echo "Error: Failed to build Sharp compiler"
        exit 1
    fi
}

# Function to build the Sharp runtime
build_runtime() {
    echo "Building Sharp runtime..."
    $CXX_COMPILER -o bin/sharp -std=c++17 -pthread $PLATFORM_FLAGS -DASMJIT_STATIC -lm -lstdc++ -O3 -ldl -g -Ofast main.cpp stdimports.h \
        lib/runtime/error/*.h lib/runtime/error/*.cpp \
        lib/runtime/exe/*.h lib/runtime/exe/*.cpp \
        lib/runtime/memory/*.h lib/runtime/memory/*.cpp \
        lib/runtime/reflect/*.h lib/runtime/reflect/*.cpp \
        lib/runtime/snb/*.h lib/runtime/snb/*.cpp \
        lib/runtime/multitasking/*/*.h lib/runtime/multitasking/*/*.cpp \
        lib/runtime/types/*.h lib/runtime/types/*.cpp \
        lib/runtime/jit/*.h lib/runtime/jit/*.cpp \
        lib/runtime/jit/arm/*.h lib/runtime/jit/arm/*.cpp \
        lib/runtime/jit/arm/opcode/*.cpp \
        lib/runtime/jit/arm/sections/*.cpp \
        lib/runtime/*.h lib/runtime/*.cpp \
        lib/core/*.cpp lib/core/*.h \
        lib/core/opcode/*.cpp lib/core/opcode/*.h \
        lib/util/*.cpp lib/util/*.h \
        lib/util/zip/*.cpp lib/util/zip/*.h \
        lib/Modules/math/*.h lib/Modules/math/*.cpp \
        lib/Modules/std/*.h lib/Modules/std/*.cpp \
        lib/Modules/std.io/*.h lib/Modules/std.io/*.cpp \
        lib/util/jit/asmjit/src/asmjit/core/*.h \
        lib/util/jit/asmjit/src/asmjit/arm/*.h \
        lib/util/jit/asmjit/src/asmjit/x86/*.cpp \
        lib/util/jit/asmjit/src/asmjit/x86/*.h \
        lib/util/jit/asmjit/src/asmjit/core/*.cpp \
        lib/util/jit/asmjit/src/asmjit/arm/*.cpp \
        lib/util/jit/asmjit/src/asmjit/*.h

    if [ $? -eq 0 ]; then
        echo "Sharp runtime built successfully!"
    else
        echo "Error: Failed to build Sharp runtime"
        exit 1
    fi
}

# Function to copy files and install standard library
install_files() {
    echo "Installing Sharp files..."

    # Copy binaries if they exist
    if [ -f "bin/sharpc" ]; then
        sudo cp bin/sharpc $INSTALL_BIN_DIR
        echo "Sharp compiler installed to $INSTALL_BIN_DIR/sharpc"
    fi

    if [ -f "bin/sharp" ]; then
        sudo cp bin/sharp $INSTALL_BIN_DIR
        echo "Sharp runtime installed to $INSTALL_BIN_DIR/sharp"
    fi

    # Install standard library
    echo "Installing standard library..."
    sudo rm -rf $INSTALL_INCLUDE_DIR
    sudo mkdir -p $INSTALL_INCLUDE_DIR
    sudo cp -r lib/support/0.3.0/ $INSTALL_INCLUDE_DIR

    echo "Sharp installed!"
}

# Main script logic
case "$1" in
    "sharpc")
        echo "Building Sharp compiler only..."
        build_compiler
        install_files
        ;;
    "sharp")
        echo "Building Sharp runtime only..."
        build_runtime
        install_files
        ;;
    "")
        echo "Building Sharp (compiler and runtime)..."
        build_compiler
        build_runtime
        install_files
        ;;
    *)
        echo "Usage: $0 [sharpc|sharp]"
        echo "  sharpc  - Build only the Sharp compiler"
        echo "  sharp   - Build only the Sharp runtime"
        echo "  (no args) - Build both compiler and runtime"
        exit 1
        ;;
esac