  .org 0x8000

  ; syntax:
  ;
  ; read.
  ;   if ': ', read 4 hex bytes and write hex bytes given. stop at a semicolon.
  ;   if otherwise, interpret as a hex memory address. interpret it.
  ;
  ; in-memory subroutine format:
  ; (bools are just 1 if true & 0 if false)
  ;
  ; opcode byte:
  .const OP_BRK 0x00
  ;   -> no operands. returns to repl
  .const OP_PSH 0x01 ; x
  ;   -> pushes byte
  .const OP_DRP 0x02
  ;   -> drops the top of the stack
  .const OP_SWP 0x03
  ;   -> swap the top two stack items
  .const OP_OVR 0x04
  ;   -> copy second item to top
  .const OP_ADD 0x05
  ;   -> add top two stack items and push result
  .const OP_SUB 0x06
  ;   -> subtract top two stack items and push result
  .const OP_EQU 0x07
  ;   -> push whether top two items are equal (bool)
  .const OP_GTR 0x08
  ;   -> push b and then a. push whether a > b
  .const OP_BCH 0x09
  ;   -> pop (word) c then (word) b and then a. if a == 0, call subroutine c else call b
  .const OP_LOD 0x0a
  ;   -> pop a. push byte at address a
  .const OP_STO 0x0b
  ;   -> pop b and then a. store byte a into address b
  .const OP_OUT 0x0c
  ;   -> pop a and output it as ascii
  .const OP_HEX 0x0d
  ;   -> pop a and output it as hex
  .const OP_KEY 0x0e
  ;   -> push character; non blocking
  .const OP_INP 0x0f
  ;   -> push character; blocking
  .const OP_JSR 0x10
  ;   -> pop (word) a call it
  .const OP_RET 0x11
  ;   -> returns from subroutine
  .const OP_WRD 0x12 ; (word) x
  ;   -> pushes word

  .include <stdlib>
  .include <stdascii>
  .include <stdstack>
  .include <stdhex>
  .include <stdio>

  .res 256 buf
  .res 2 parse
  .res 2 c ; we read up to 4 chars at a time
  .res 2 dsp ; data stack
  .res 2 addr
  .res 2 word
  .res 2 word2
  .res 1 byte
  .res 1 byte2

  .macro readc x y
retry:
    ld x y
    incw x
    ceq y #SPACE retry
  .endmacro

  .macro read_rest_as_hexw
    readc parse (c 1 +)
    from_hex c addr fail
    readc parse c
    readc parse (c 1 +)
    from_hex c (addr 1 +) fail
  .endmacro

  .macro read_rest_as_hex
    readc parse (c 1 +)
    from_hex c byte fail
  .endmacro

  .macro read_hexw
    readc parse c
    read_rest_as_hexw
  .endmacro

  ; (ptr) addr
  .macro call_bytecode addr
    pushw addr sp
    call interpret
    pullw sp addr
  .endmacro

  .macro read_binw x
    readc addr (x)
    readc addr (x 1 +)
  .endmacro

start:
  lptr 0x4000 dsp
  print welcome
repl:
  out #LF
  gets buf
  lptr buf parse
@token:
  readc parse c
  ceq c #':' @write
  ceq c #NUL repl
@exec:
  read_rest_as_hexw
  jmp interpret
@write:
  read_hexw
@loop:
  readc parse c
  ceq c #SEMICOLON @token
  read_rest_as_hex
  st byte addr
  incw addr
  jmp @loop

; executes the subroutine in addr
interpret:
  readc addr byte
  ceq byte #OP_BRK repl
  ceq byte #OP_PSH @psh
  ceq byte #OP_DRP @drp
  ceq byte #OP_SWP @swp
  ceq byte #OP_OVR @ovr
  ceq byte #OP_ADD @add
  ceq byte #OP_SUB @sub
  ceq byte #OP_EQU @equ
  ceq byte #OP_GTR @gtr
  ceq byte #OP_BCH @bch
  ceq byte #OP_LOD @lod
  ceq byte #OP_STO @sto
  ceq byte #OP_OUT @out
  ceq byte #OP_HEX @hex
  ceq byte #OP_KEY @key
  ceq byte #OP_INP @inp
  ceq byte #OP_JSR @jsr
  ceq byte #OP_RET  ret
  ceq byte #OP_WRD @wrd
  jmp fail
@psh:
  readc addr byte
  push byte dsp
  jmp interpret
@drp:
  ; pull dsp _
  incw dsp ; this is faster
  jmp interpret
@swp:
  pull dsp   byte
  pull dsp   byte2
  push byte  dsp
  push byte2 dsp
  jmp interpret
@ovr:
  pull dsp   byte
  pull dsp   byte2
  push byte2  dsp
  push byte  dsp
  push byte2  dsp
  jmp interpret
@add:
  pull dsp   byte
  pull dsp   byte2
  add  byte2 byte
  push byte dsp
  jmp interpret
@sub:
  pull dsp   byte
  pull dsp   byte2
  sub  byte  byte2
  push byte2 dsp
  jmp interpret
@equ:
  pull dsp   byte
  pull dsp   byte2
  ceq  byte  byte2 @not_equ
  push #1 byte
  jmp interpret
@not_equ:
  push #0 byte
  jmp interpret
@gtr:
  pull dsp   byte
  pull dsp   byte2
  cgt  byte2 byte @not_gtr
  push #1 byte
  jmp interpret
@not_gtr:
  push #0 byte
  jmp interpret
@bch:
  pullw dsp word2
  pullw dsp word
  pull dsp byte
  beq byte @not_bch
  call_bytecode word
  jmp interpret
@not_bch:
  call_bytecode word2
  jmp interpret
@lod:
  pullw dsp word
  ld word byte
  push byte dsp
  jmp interpret
@sto:
  pull dsp byte
  pullw dsp word2
  st byte word2
  jmp interpret
@out:
  pull dsp byte
  out byte
  jmp interpret
@hex:
  pull dsp byte
  hex_out byte
  jmp interpret
@key:
  in byte
  push byte dsp
  jmp interpret
@inp:
  pin byte
  push byte dsp
  jmp interpret
@jsr:
  pullw dsp word
  call_bytecode word
  jmp interpret
@wrd:
  readc addr word
  readc addr (word 1 +)
  pushw word dsp
  jmp interpret

fail:
  print @msg
  jmp repl
@msg:
  .asciiz "SORRY?"

welcome:
  .string "S6CVM 0.1.0"
  .byte LF
  .byte LF
  .asciiz "OK"
