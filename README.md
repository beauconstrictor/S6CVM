# CVMC - CVM Compiler

`cvmc` is a compiler for the CVM (Compact Virtual Machine) programming language, designed to produce extremely small, cross-platform binaries targeting the Compact Virtual Machine. The CVM is a minimal bytecode VM that can quickly be implemented on custom, limited hardware, immediately providing an expressive, high-level programming language for that machine.

Here is a sample of the CVM language:

```forth
import std

{:fib <n -- fib(n)>
    (if (gt 1), {
        (add (fib (sub dup, 1)), (fib (sub over, 2)))
        (drop swap)
    })
}

{:main (puti (fib 5))}
```

The design of the language closely mimics the design of the virtual machine itself, making binary size easy to predict and minimise.

There is a reference manual in the works that will provide more thorough details on programming in the language itself.

## Installation

To install `cvmc`, you will need to install `clang` and `make`. Then, just run these commands to build a binary which you can move into your PATH:

```sh
$ git clone https://github.com/beauconstrictor/cvmc
$ make
```

The binary will be located in `cvmc/build/cvmc`. Once you have moved the binary into your PATH, verify that it is installed correctly:

```sh
$ cvmc -v
```

If you use Vim, there is also a syntax highlighter included in this repo, called `cvm.vim`.
