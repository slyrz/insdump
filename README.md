# insdump

The insdump command disassembles and prints the machine instructions
a program executes.

### Quickstart

Building insdump requires the library and header files of *bfd* and *opcodes*,
provided by the [GNU binutils](http://www.gnu.org/s/binutils/).
If you have these files installed, run

    make

To print the machine instructions executed by a command,
say `echo "Hello World"`, run

    ./insdump echo "Hello World"

The output should look like

    ...
    7f6bbf1d38f9:    f7 45 10 fa ff ff ff     testl  $0xfffffffa,0x10(%rbp)
    7f6bbf1d3900:    0f 85 34 08 00 00        jne    0x000000000000083a
    7f6bbf1d3906:    48 83 7d 18 00           cmpq   $0x0,0x18(%rbp)
    7f6bbf1d390b:    48 8b 85 38 ff ff ff     mov    -0xc8(%rbp),%rax
    7f6bbf1d3912:    4c 8b 08                 mov    (%rax),%r9
    7f6bbf1d3915:    0f 85 05 02 00 00        jne    0x000000000000020b
    7f6bbf1d391b:    4d 85 c9                 test   %r9,%r9
    7f6bbf1d391e:    0f 84 35 08 00 00        je     0x000000000000083b
    7f6bbf1d3924:    49 89 c2                 mov    %rax,%r10
    7f6bbf1d3927:    48 8d 85 70 ff ff ff     lea    -0x90(%rbp),%rax
    7f6bbf1d392e:    4d 89 d6                 mov    %r10,%r14
    7f6bbf1d3931:    48 89 85 40 ff ff ff     mov    %rax,-0xc0(%rbp)
    7f6bbf1d3938:    48 8d 85 60 ff ff ff     lea    -0xa0(%rbp),%rax
    ...

The first column shows the instruction pointer value, the second column
shows the raw instruction bytes and the third column shows the disassembled
instruction.

### License

insdump is released under MIT license.
You can find a copy of the MIT License in the [LICENSE](./LICENSE) file.
