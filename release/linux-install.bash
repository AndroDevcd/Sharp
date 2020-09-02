
MACHINE_TYPE=`uname -m`
if [ ${MACHINE_TYPE} == 'x86_64' ]; then
  cd bin/x64/linux/
else
  cd bin/x32/linux/
fi

echo "installing sharp..."
sudo cp * /bin

cd ../../../
echo "installing standard lbrary..."

cd lib/support/0.2.9/
sudo mkdir /usr/include/sharp/
sudo cp  *.sharp /usr/include/sharp/
sudo cp -r */ /usr/include/sharp/

cd ../../../
sharpc test.sharp -o hello -s -R
sharp hello

echo ""
echo "sharp installed!"