## S6CBC

S6CBC is Forth compiler designed to produce incredibly compact binaries for the [Ozpex S6](https://github.com/beauconstrictor/s6emu) platform.

S6CBC is split into two main parts: one of which runs on a PC and produces bytecode, which is then execute by second part, a bytecode interpreter, resident in system ROM. This is an intentionally unsual design, because a full Forth system would be too large to fit it ROM.

The name stands for 'Compact ByteCode'.
