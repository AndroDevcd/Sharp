
echo "building compiler..."
g++ main.cpp -DMAKE_COMPILER lib/grammar/*.cpp lib/runtime/Opcode.cpp lib/grammar/optimizer/*.cpp lib/grammar/generator/*.cpp lib/grammar/frontend/*.cpp lib/grammar/frontend/*/*.cpp lib/grammar/backend/*.cpp lib/grammar/backend/*/*.cpp lib/util/*.cpp lib/util/zip/*.cpp -o bin/sharpc -std=c++11 -pthread -lstdc++ -g

echo "building runtime engine..."
#gcc -fsanitize=address main.cpp lib/runtime/*.cpp lib/Modules/*/*.cpp lib/runtime/*/*.cpp lib/util/*.cpp lib/util/jit/asmjit/src/asmjit/x86/*.cpp lib/util/jit/asmjit/src/asmjit/core/*.cpp lib/util/zip/*.cpp lib/util/fmt/src/*.cc -o bin/sharp -std=c++11 -pthread -lstdc++ -g  -lm -lrt

gcc  main.cpp lib/runtime/*.cpp lib/Modules/*/*.cpp lib/runtime/*/*.cpp lib/util/*.cpp lib/util/jit/asmjit/src/asmjit/x86/*.cpp lib/util/jit/asmjit/src/asmjit/core/*.cpp lib/util/zip/*.cpp lib/util/fmt/src/*.cc -fno-omit-frame-pointer -O0 -o bin/sharp -std=c++11 -pthread -lstdc++ -g -lm -lrt

echo "building runtime engine profiler..."
#g++ main.cpp -DSHARP_PROF_ lib/runtime/*.cpp lib/Modules/*/*.cpp lib/runtime/*/*.cpp lib/util/*.cpp lib/util/zip/*.cpp lib/util/fmt/src/*.cc lib/util/jit/asmjit/src/asmjit/x86/*.cpp lib/util/jit/asmjit/src/asmjit/base/*.cpp -o bin/tanto -std=c++11 -pthread -lstdc++ -g

echo "sharp build finished!"
