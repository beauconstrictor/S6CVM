## S6CVM

S6CVM is Forth compiler designed to produce incredibly compact binaries for the [Ozpex S6](https://github.com/beauconstrictor/s6emu) platform.

S6CVM is split into two main parts:
- The compiler, which runs on a PC, and produces bytecode
- The interpreter, which runs on the S6 and loads this bytecode from tape and then interprets it.

The name stands for 'Compact Virtual Machine'.
