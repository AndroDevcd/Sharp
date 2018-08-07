
## Getting Started

This file will aid in getting you started with using sharp.

#### Downloading

To download Sharp go to ``releases`` and download the latest release of Sharp. Unpack the colder revealing the contents inside. Sharp is currently supporting 64 bit linux and windows operating systems, below covers how to install sharp on both systems:

#### Installing on Windows

Navigate to the folder ``{YOUR_DIR}\sharp-v{VERSION}\bin\64bit\win``.  You can move the binary to a folder of your choice. Add Sharp to your PATH:

```
{SHARP_PATH}\Sharp.exe
{SHARP_PATH}\Sharpc.exe
{SHARP_PATH}\Shprof.exe
```

Windows Powershell is the preferred method to use sharp but you may use windows CMD as well.
Next cd into the directory of your sharp build to the folder lib and run the commands below using powershell

```
mkdir C:/Sharp/include
cd {SHARP_INSTALL_PATH}/lib/support/0.2.0/
cp -r * C:/Sharp/include
rm -r C:/Sharp/include/bootstrap/
Sharpc Test.sharp -o test -s -R
Sharp test
```

After running the commands above if you receive the final output as ``Hello, World!``` Then Sharp has been installed properly and your good to go!

#### Installing on linux

Navigate to the folder ``{YOUR_DIR}\sharp-v{VERSION}\``.  Installing sharp on linux is very simple simply run the command below:

```
chmod +x linux-install.bash
./linux-install.bash
sharpc Test.sharp -o test -s -R
sharp test
```

After running the commands above if you receive the final output as ``Hello, World!``` Then Sharp has been installed properly and your good to go!
