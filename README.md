# insdump

The insdump command disassembles and prints the machine instructions
a program executes.

### Quickstart

Building insdump requires the library and header files of *bfd* and *opcodes*,
provided by the [GNU binutils](www.gnu.org/s/binutils/).
If you have these files installed, run

    make

To print the machine instructions executed by a command,
say `echo "Hello World"`, run

    ./insdump echo "Hello World"

### License

insdump is released under MIT license.
You can find a copy of the MIT License in the [LICENSE](./LICENSE) file.
