## S6CVM

S6CVM is a development toolchain for the [Ozpex S6](https://github.com/beauconstrictor/s6emu) platform, which focuses on producing extremely small binaries. It achieves this by using a bytecode virtual machine, and a compiler that runs on a separate machine, targeting this VM.

S6CVM code is considerably slower than the other alternative toolchain for S6 development, [S6ASM](https://github.com/beauconstrictor/s6asm), which produces native code. However, it is impossible to write nontrivial programs in S6 assembly as a result of the inefficient instruction set and limited ROM size. S6CVM eschews this problem, by defining it's own instruction set that runs _on top_ of the S6.

which is written in S6 assembly. The virtual machine is resident in system ROM, and simply prompts you to insert a tape, containing a binary from the `s6cvm` compiler.

Here is a sample of the programming language:

```
{:fib <n : fib(n)>
    (if (gt 1), {
        (add (fib (sub dup, 1)), (fib (sub over, 2)))
        (drop swap)
    })
}

{:main (puti (fib 5))}
```

This is a simple `.cvm` (the file extension used for this language's source code) subroutine that computes the 5'th Fibonacci number.
