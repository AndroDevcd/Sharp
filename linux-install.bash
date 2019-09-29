cd Sharp

echo "getting latest..."
git pull

cd lib/grammar/

echo "installing sharp..."
#g++ -DMAKE_COMPILER *.cpp ../../main.cpp ../util/*.cpp parser/*.cpp parser/*/*.cpp -lstdc++ -s -O3 -Ofast -o sharpc -std=c++11

#sudo cp sharpc /bin

echo "installing runtime support..."

cd ../runtime/
gcc *.cpp -g ../../main.cpp ../Modules/*/*.cpp ../util/*.cpp ../util/fmt/src/*.cc ../util/jit/asmjit/src/asmjit/x86/*.cpp */*.cpp ../util/jit/asmjit/src/asmjit/base/*.cpp -lstdc++  -o sharp -std=c++11 -pthread
sudo cp sharp /bin

echo "sharp installed!"
