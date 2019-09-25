
cd bin/64bit/linux/

echo "installing sharp..."
sudo cp * /bin

cd ../../../
echo "installing standard lbrary..."

cd lib/support/0.2.581/
sudo mkdir /usr/include/sharp/
sudo cp  *.sharp /usr/include/sharp/
sudo cp -r */ /usr/include/sharp/

cd ../../../
sharpc Test.sharp -o hello -s -R
sharp hello

echo ""
echo "sharp installed!"