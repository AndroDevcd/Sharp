
## Getting Started

#### Installing

Download the newest `sharp-<version>.zip` from the [`releases`](releases) folder and unpack it.
`START-HERE.txt` inside says the same as below.

**Windows** - double-click `windows\win-install.cmd` (or run it in a terminal).

**Linux (64-bit x86)**

```
bash linux/unix-install.sh
```

**Raspberry Pi, or any other Linux** - the VM is built on the machine itself:

```
git clone https://github.com/AndroDevcd/Sharp.git
cd Sharp
bash build-scripts/linux/install_from_source.sh
```

Then open a NEW terminal and try it:

```
sharpc hello.sharp -o hello
sharp hello
```

(`hello.sharp` is in the `examples` folder of the install.)

#### Building from source

[`build-scripts/README.md`](build-scripts/README.md) explains how to build Sharp, run its tests and
make a release. The short form, in Linux or WSL:

```
bash build-scripts/build.sh
bash build-scripts/test.sh
```

#### Programming Ideas
After you install Sharp if you are having trouble coming up with programming ideas then you can head over to the 'examples/' directory to see a list
of sample programs using various parts of the Sharp programming language.
