  .org 0x8000

  ; syntax:
  ;
  ; read char.
  ;   if :, jump to 'define'
  ;   if #, read 4 chars as hex and push word
  ;   if $, read 2 chars as hex and push byte
  ;   if ., pop byte and output in hex
  ;   if ,, pop word and output in hex
  ;   if null, read new line
  ;   else, read 3 chars, and interpret all 4 as hex. run 'interpret' on addr
  ;
  ; in-memory subroutine format:
  ;
  ; opcode byte:
  ;   00 - break
  ;     -> no operands. returns to repl
  ;   01 - push x
  ;     -> pushes byte
  ;   02 - drop
  ;     -> drops the top of the stack
  ;   03 - swap
  ;     -> swap the top two stack items
  ;   04 - over
  ;     -> copy second item to top
  ;   05 - add
  ;     -> add top two stack items and push result
  ;   06 - sub
  ;     -> subtract top two stack items and push result
  ;   06 - equal
  ;     -> push whether top two items are equal
  ;   07 - greater
  ;     -> push b and then a. push whether a > b
  ;   08 - branch x y
  ;     -> pop a. calls x if a != 0, else calls y
  ;   09 - load
  ;     -> pop a. push byte at address a
  ;   0a - store
  ;     -> pop b and then a. store byte a into address b
  ;   0b - out
  ;     -> pop a and output it as ascii
  ;   0c - hex out
  ;     -> pop a and output it as hex
  ;   0d - in
  ;     -> push character; non blocking
  ;   0e - call x
  ;     -> call subroutine x

  .include <stdlib>
  .include <stdstack>
  .include <stdhex>
  .include <stdio>

  .res 256 buf
  .res 2 parse
  .res 5 c ; we read up to 4 chars at a time
  .res 1 byte
  .res 2 word
  .res 2 dsp ; data stack
  .res 1 output ; bool

  .macro readc x y
    ld x y
    incw x
  .endmacro

  lptr 0x4000 dsp
  print welcome
readline:
  ble output @no_newline
  out #0x0a
  mov #0 output
@no_newline:
  gets buf
  lptr buf parse
token:
  readc parse c
  ceq c #0x20 token
  ceq c #0x00 readline
  ceq c #':' define
  ceq c #'$' push_byte
  ceq c #'#' push_word
  ceq c #'.' print_byte
  ceq c #',' print_word

  ; fallthrough
call:
  readc parse (c 1 +)
  from_hex c word fail
  readc parse c
  readc parse (c 1 +)
  from_hex c (word 1 +) fail
  jmp interpret

define:
  halt

push_byte:
  readc parse c
  readc parse (c 1 +)
  from_hex c byte fail
  push byte dsp
  jmp token

push_word:
  readc parse c
  readc parse (c 1 +)
  from_hex c word fail
  readc parse c
  readc parse (c 1 +)
  from_hex c (word 1 +) fail
  pushw word dsp
  jmp token

print_byte:
  pull dsp byte
  hex_out byte
  out #0x20
  mov #1 output
  jmp token

print_word:
  pullw dsp word
  hexw_out word
  out #0x20
  mov #1 output
  jmp token

; interprets the cbc subroutine in 'word'
interpret:
  halt

fail:
  print @msg
  jmp readline
@msg:
  .string "SORRY?"
  .word 0x0a00

welcome:
  .string "S6CBC 0.1.0"
  .word 0x0a0a
  .string "OK"
  .word 0x0a00
