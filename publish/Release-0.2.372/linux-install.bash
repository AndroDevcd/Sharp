
cd bin/64bit/linux/

echo "installing sharp..."
sudo cp sharpc /bin

cd bin/64bit/linux/
sudo cp sharp /bin

cd ../../../
echo "installing standard lbrary..."

cd lib/support/0.2.422/
sudo mkdir /usr/include/sharp/
sudo cp -r */ /usr/include/sharp/
sudo rm -r /usr/include/sharp/bootstrap/

echo "sharp installed!"