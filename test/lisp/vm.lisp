; ##########################################################
; Systemy tests

(vm/preempt
 (S:1 C:1)
 ((asm (PUSH 1)
       (PUSH 2)
       (YIELD)))
 (sp:1 #0:1))


; ##########################################################
; push/pop

(vm/push
 (S:1)
 ((asm (PUSH 1)
       (YIELD)))
 (sp:1 $0:0 #0:1))

(vm/pushr
 (S:1 $0:12)
 ((asm (PUSHR $0)
       (YIELD)))
 (sp:1 $0:12 #0:12))

(vm/pushf
 (S:1 flags:1)
 ((asm (PUSHF)
       (YIELD)))
 (flags:1 sp:1 #0:1))

(vm/pop
 (S:1)
 ((asm (PUSH 1)
       (POP)
       (YIELD)))
 (sp:0))

(vm/popr
 (S:1)
 ((asm (PUSH 1)
       (POPR $0)
       (YIELD)))
 (sp:0 $0:1))

(vm/dupe
 (S:2)
 ((asm (PUSH 1)
       (DUPE)
       (YIELD)))
 (sp:2 #0:1 #1:1))

(vm/swap
 (S:2)
 ((asm (PUSH 1)
       (PUSH 2)
       (SWAP)
       (YIELD)))
 (sp:2 #0:2 #1:1))


; ##########################################################
; boolean

(vm/not
 (S:2)
 ((asm (PUSH 0x123456789aBcDeF0)
       (NOT)
       (PUSH 0)
       (NOT)
       (YIELD)))
 (sp:2 #0:0 #1:1))

(vm/and
 (S:5)
 ((asm (PUSH 1)
       (PUSH 1)
       (AND)
       (PUSH 1)
       (PUSH 0)
       (AND)
       (PUSH 0)
       (PUSH 1)
       (AND)
       (PUSH 0)
       (PUSH 0)
       (AND)
       (YIELD)))
 (sp:4 #0:1 #1:0 #2:0 #3:0))

(vm/or
 (S:5)
 ((asm (PUSH 1)
       (PUSH 1)
       (OR)
       (PUSH 1)
       (PUSH 0)
       (OR)
       (PUSH 0)
       (PUSH 1)
       (OR)
       (PUSH 0)
       (PUSH 0)
       (OR)
       (YIELD)))
 (sp:4 #0:1 #1:1 #2:1 #3:0))

(vm/xor
  (S:5)
  ((asm (PUSH 1)
	(PUSH 1)
	(XOR)
	(PUSH 1)
	(PUSH 0)
	(XOR)
	(PUSH 0)
	(PUSH 1)
	(XOR)
	(PUSH 0)
	(PUSH 0)
	(XOR)
	(YIELD)))
  (#0:0 #1:1 #2:1 #3:0))


; ##########################################################
; binary

(vm/bnot
 (S:1)
 ((asm (PUSH 0xFF00FF00FF00FF00)
       (BNOT)
       (YIELD)))
 (sp:1 #0:0x00ff00ff00ff00ff))

(vm/band
 (S:2)
 ((asm (PUSH 0xFF00FF00FF00FF00)
       (PUSH 0xF0F0F0F0F0F0F0F0)
       (BAND)
       (YIELD)))
 (sp:1 #0:0xf000f000f000f000))

(vm/bor
 (S:2)
 ((asm (PUSH 0xFF00FF00FF00FF00)
       (PUSH 0xF0F0F0F0F0F0F0F0)
       (BOR)
       (YIELD)))
 (sp:1 #0:0xfff0fff0fff0fff0))

(vm/bxor
 (S:2)
 ((asm (PUSH 0xFF00FF00FF00FF00)
       (PUSH 0xF0F0F0F0F0F0F0F0)
       (BXOR)
       (YIELD)))
 (sp:1 #0:0x0ff00ff00ff00ff0))

(vm/bsl
 (S:2)
 ((asm (PUSH 0xFF00FF00FF00FF00)
       (PUSH 4)
       (BSL)
       (YIELD)))
 (sp:1 #0:0xf00ff00ff00ff000))

(vm/bsr
 (S:2)
 ((asm (PUSH 0xFF00FF00FF00FF00)
       (PUSH 4)
       (BSR)
       (YIELD)))
 (sp:1 #0:0x0ff00ff00ff00ff0))


; ##########################################################
; math

(vm/neg
 (S:2)
 ((asm (PUSH -1)
       (NEG)
       (PUSH 1)
       (NEG)
       (YIELD)))
 (sp:2 #0:1 #1:-1))

(vm/add
 (S:2)
 ((asm (PUSH 1)
       (PUSH 1)
       (ADD)
       (YIELD)))
 (sp:1 #0:2))

(vm/sub
 (S:2)
 ((asm (PUSH 3)
       (PUSH 2)
       (SUB)
       (YIELD)))
 (sp:1 #0:1))

(vm/mul
 (S:2)
 ((asm (PUSH 2)
       (PUSH 3)
       (MUL)
       (YIELD)))
 (sp:1 #0:6))

(vm/lmul
 (S:2)
 ((asm (PUSH 0x7FFFFFFFFFFFFFFF)
       (PUSH 0xF)
       (LMUL)
       (YIELD)))
 (sp:2 #0:0x7FFFFFFFFFFFFFF1 #1:0x7))

(vm/div
 (S:2)
 ((asm (PUSH 6)
       (PUSH 3)
       (DIV)
       (YIELD)))
 (sp:1 #0:2))

(vm/rem
 (S:2)
 ((asm (PUSH 5)
       (PUSH 3)
       (REM)
       (YIELD)))
 (sp:1 #0:2))


; ##########################################################
; comp

(vm/eq
 (S:3)
 ((asm (PUSH 1)
       (PUSH 1)
       (EQ)
       (PUSH 1)
       (PUSH 2)
       (EQ)
       (YIELD)))
 (sp:2 #0:1 #1:0))

(vm/ne
 (S:3)
 ((asm (PUSH 1)
       (PUSH 1)
       (NE)
       (PUSH 1)
       (PUSH 2)
       (NE)
       (YIELD)))
 (sp:2 #0:0 #1:1))

(vm/gt
 (S:4)
 ((asm (PUSH 1)
       (PUSH 1)
       (GT)
       (PUSH 1)
       (PUSH 2)
       (GT)
       (PUSH 2)
       (PUSH 1)
       (GT)
       (YIELD)))
 (sp:3 #0:0 #1:1 #2:0))

(vm/ge
 (S:4)
 ((asm (PUSH 1)
       (PUSH 1)
       (GE)
       (PUSH 1)
       (PUSH 2)
       (GE)
       (PUSH 2)
       (PUSH 1)
       (GE)
       (YIELD)))
 (sp:3 #0:1 #1:1 #2:0))

(vm/lt
 (S:4)
 ((asm (PUSH 1)
       (PUSH 1)
       (LT)
       (PUSH 1)
       (PUSH 2)
       (LT)
       (PUSH 2)
       (PUSH 1)
       (LT)
       (YIELD)))
 (sp:3 #0:0 #1:0 #2:1))

(vm/le
 (S:4)
 ((asm (PUSH 1)
       (PUSH 1)
       (LE)
       (PUSH 1)
       (PUSH 2)
       (LE)
       (PUSH 2)
       (PUSH 1)
       (LE)
       (YIELD)))
 (sp:3 #0:1 #1:0 #2:1))

(vm/cmp
 (S:4)
 ((asm (PUSH 1)
       (PUSH 1)
       (CMP)
       (PUSH 1)
       (PUSH 3)
       (CMP)
       (PUSH 3)
       (PUSH 1)
       (CMP)
       (YIELD)))
 (sp:3 #0:0 #1:2 #2:-2))


; ##########################################################
; jmp

(vm/call
 (S:1)
 ((asm (CALL fn)
       (@ fn)
       (YIELD)))
 (sp:1 #0:0x05))

(vm/ret
 (S:1)
 ((asm (CALL fn)
       (YIELD)
       (@ fn)
       (RET)))
 (sp:0))

(vm/jmp
 (S:1)
 ((asm (JMP out)
       (PUSH 0xffffffff)
       (@ out)
       (YIELD)))
 (sp:0))

(vm/jz
 (S:1)
 ((asm (PUSH 1)
       (JZ fail)
       (PUSH 0)

       (JZ ok)
       (JMP fail)

       (@ fail)
       (PUSH 0xffffffff)
       (@ ok)
       (YIELD)))
 (sp:0))

(vm/jnz
 (S:1)
 ((asm (PUSH 0)
       (JNZ fail)

       (PUSH 1)
       (JNZ ok)
       (JMP fail)

       (@ fail)
       (PUSH 0xffffffff)
       (@ ok)
       (YIELD)))
 (sp:0))

(vm/back-ref
 (S:1)
 ((asm (JMP next)
       (@ back)
       (PUSH 1)
       (YIELD)
       (@ next)
       (JMP back)))
 (sp:1 #0:1))


; ##########################################################
; io

(vm/io
 (S:2)
 ((asm (PUSH 1)
       (PUSH !io_nil)
       (IO 2)
       (YIELD)))
 (flags:0x1 io:2 ior:0xff sp:2 #0:1 #1:!io_nil))

(vm/ios
 (S:3)
 ((asm (PUSH 1)
       (PUSH !io_nil)
       (PUSH 2)
       (IOS)
       (YIELD)))
 (flags:0x1 io:2 ior:0 sp:2 #0:1 #1:!io_nil))

(vm/ior
 (S:3)
 ((asm (PUSH 1)
       (PUSH !io_nil)
       (PUSH 2)
       (POPR $0)
       (IOR $0)
       (YIELD)))
 (flags:0x1 io:2 ior:1 $0:2 sp:2 #0:1 #1:!io_nil))


; ##########################################################
; misc

(vm/fault
 (S:0)
 ((asm (FAULT)))
 (flags:0x4 ret:0xFFFFFFFF))

(vm/pack
 (S:2)
 ((asm (PUSH 1)
       (PUSH 2)
       (PACK)
       (YIELD)))
 (sp:1 #0:0x200000001))

(vm/unpack
 (S:2)
 ((asm (PUSH 0x200000001)
       (UNPACK)
       (YIELD)))
 (sp:2 #0:1 #1:2))
