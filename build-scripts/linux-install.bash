
echo "getting latest..."
git pull

echo "installing sharp..."
g++ main.cpp -DMAKE_COMPILER lib/grammar/*.cpp lib/runtime/Opcode.cpp lib/grammar/optimizer/*.cpp lib/grammar/generator/*.cpp lib/grammar/frontend/*.cpp lib/grammar/frontend/*/*.cpp lib/grammar/backend/*.cpp lib/grammar/backend/*/*.cpp lib/util/*.cpp lib/util/zip/*.cpp -o bin/sharpc -std=c++11 -pthread -lstdc++ -O3
sudo cp sharpc /bin

echo "installing runtime support..."
g++ main.cpp lib/runtime/*.cpp lib/Modules/*/*.cpp lib/runtime/*/*.cpp lib/util/*.cpp lib/util/zip/*.cpp -o bin/sharp -std=c++11 -pthread -lstdc++ -lm -lrt -lstdc++ -O3 -ldl
sudo cp sharp /bin

echo -n "Installing  standard Lib"
rm -r /usr/include/sharp/
cp -r lib/support/0.2.9/ /usr/include/sharp/

echo "sharp installed!"
