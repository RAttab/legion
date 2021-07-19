; ##########################################################
; Systemy tests

(vm-preempt
 (S:1 C:0)
 ((asm (PUSH 1)
       (PUSH 2)
       (YIELD)))
 (S:1 sp:1 s0:1))


; ##########################################################
; push/pop

(vm-push
 (S:1)
 ((asm (PUSH 1)
      (YIELD)))
 (S:1 sp:1 r1:0 s0:1))

(vm-pushr
 (S:1 r1:12)
 ((asm (PUSHR $1)
       (YIELD)))
 (S:1 sp:1 r1:12 s0:12))

(vm-pushf
 (S:1 flags:1)
 ((asm (PUSHF)
       (YIELD)))
 (S:1 flags:1 sp:1 s0:1))

(vm-pop
 (S:1)
 ((asm (PUSH 1)
       (POP)
       (YIELD))
  (S:1 sp:0))

(vm-popr
 (S:1)
 ((asm (PUSH 1)
       (POPR $1)
       (YIELD)))
 (S:1 sp:0 r1:1))

(vm-dupe
 (S:1)
 ((asm (PUSH 1)
       (DUPE)
       (YIELD)))
 (S:1 sp:2 s0:1 s1:1))

(vm-swap
 (S:1)
 ((asm (PUSH 1)
       (PUSH 2)
       (SWAP)
       (YIELD)))
 (S:1 sp:2 s0:2 s1:1))


; ##########################################################
; boolean

(vm-not
 (S:1)
 ((asm (PUSH 0x123456789aBcDeF0)
       (NOT)
       (PUSH 0)
       (NOT)
       (YIELD)))
 (S:1 sp:2 s0:0 s1:1))

(vm-and
 (S:2)
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
 (S:2 sp:4 s0:1 s1:0 s2:0 s3:0))

(vm-or
 (S:2)
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
 (S:2 sp:4 s0:1 s1:1 s2:1 s3:0))

(vm-xor
  (S:2)
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
  (S:2 sp:4 s0:0 s1:1 s2:1 s3:0))


; ##########################################################
; binary

(vm-bnot
 (S:1)
 ((asm (PUSH 0xFF00FF00FF00FF00)
       (BNOT)
       (YIELD)))
 (S:1 sp:1 s0:0x00ff00ff00ff00ff))

(vm-band
 (S:1)
 ((asm (PUSH 0xFF00FF00FF00FF00)
       (PUSH 0xF0F0F0F0F0F0F0F0)
       (BAND)
       (YIELD)))
 (S:1 sp:1 s0:0xf000f000f000f000))

(vm-bor
 (S:1)
 ((asm (PUSH 0xFF00FF00FF00FF00)
       (PUSH 0xF0F0F0F0F0F0F0F0)
       (BOR)
       (YIELD)))
 (S:1 sp:1 s0:0xfff0fff0fff0fff0))

(vm-bxor
 (S:1)
 ((asm (PUSH 0xFF00FF00FF00FF00)
       (PUSH 0xF0F0F0F0F0F0F0F0)
       (BXOR)
       (YIELD)))
 (S:1 sp:1 s0:0x0ff00ff00ff00ff0))

(vm-bsl
 (S:1)
 ((asm (PUSH 0xFF00FF00FF00FF00)
      (PUSH 4)
      (BSL)
      (YIELD)))
 (S:1 sp:1 s0:0xf00ff00ff00ff000))

(vm-bsr
 (S:1)
 ((asm (PUSH 0xFF00FF00FF00FF00)
       (PUSH 4)
       (BSR)
       (YIELD)))
 (S:1 sp:1 s0:0x0ff00ff00ff00ff0)


; ##########################################################
; math

(vm-neg
 (S:1)
 ((asm (PUSH -1)
       (NEG)
       (PUSH 1)
       (NEG)
       (YIELD)))
 (S:1 sp:2 s0:1 s1:-1))

(vm-add
 (S:1)
 (asm (PUSH 1)
       (PUSH 1)
       (ADD)
       (YIELD)))
 (S:1 sp:1 s0:2))

(vm-sub
 (S:1)
 ((asm (PUSH 3)
       (PUSH 2)
       (SUB)
       (YIELD)))
 (S:1 sp:1 s0:1))

(vm-mul
 (S:1)
 ((asm (PUSH 2)
       (PUSH 3)
       (MUL)
       (YIELD)))
 (S:1 sp:1 s0:6))

(vm-lmul
 (S:1)
 ((asm (PUSH 0x7FFFFFFFFFFFFFFF)
       (PUSH 0xF)
       (LMUL)
       (YIELD)))
 (S:1 sp:2 s0:0x7FFFFFFFFFFFFFF1 s1:0x7))

(vm-div
 (S:1)
 ((asm (PUSH 6)
       (PUSH 3)
       (DIV)
       (YIELD)))
 (S:1 sp:1 s0:2))

(vm-rem
 (S:1)
 ((asm (PUSH 5)
       (PUSH 3)
       (REM)
       (YIELD)))
 (S:1 sp:1 s0:2))


; ##########################################################
; comp

(vm-eq
 (S:1)
 ((asm (PUSH 1)
       (PUSH 1)
       (EQ)
       (PUSH 1)
       (PUSH 2)
       (EQ)
       (YIELD)))
 (S:1 sp:2 s0:1 s1:0))

(vm-ne
 (S:1)
 ((asm (PUSH 1)
       (PUSH 1)
       (NE)
       (PUSH 1)
       (PUSH 2)
       (NE)
       (YIELD)))
 (S:1 sp:2 s0:0 s1:1))

(vm-gt
 (S:2)
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
 (S:2 sp:3 s0:0 s1:1 s2:0))

(vm-lt
 (S:2)
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
 (S:2 sp:3 s0:0 s1:0 s2:1))

(vm-cmp
 (S:2)
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
 (S:2 sp:3 s0:0 s1:2 s2:-2))


; ##########################################################
; jmp

(vm-call
 (S:1)
 ((asm (CALL fn)
       (@ fn)
       (YIELD)))
 (S:1 sp:1 s0:0x05))

(vm-ret
 (S:1)
 ((asm (CALL fn)
       (YIELD)
       (@ fn)
       (RET)))
 (S:1 sp:0))

(vm-jmp
 (S:1)
 ((asm (JMP out)
       (PUSH 0xffffffff)
       (@ out)
       (YIELD)))
 (S:1 sp:0))

(vm-jz
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
 (S:1 sp:0))

(vm-jnz
 (S:1)
 ((asm (PUSH 0)
       (JNZ fail)

       (PUSH 1)
       (JNZ ok)
       (JMP fail)

       (@ fail)
       (PUSH 0xffffffff)
       (@ OK)
       (YIELD)))
 (S:1 sp:0))

(vm-back-ref
 (S:1)
 ((asm (JMP next)
       (@ back)
       (PUSH 1)
       (YIELD)
       (@ next)
       (JMP back)))
 (S:1 sp:1 s0:1))


; ##########################################################
; io

(vm-io
 (S:1)
 ((asm (PUSH 1)
       (PUSH !io_nil)
       (IO 2)
       (YIELD)))
 (S:1 flags:0x1 io:2 ior:0xff sp:2 s0:1 s1:!io_nil))

(vm-ios
 (S:1)
 ((asm (PUSH 1)
       (PUSH !io_nil)
       (PUSH 2)
       !IOS)
       (YIELD))
 (S:1 flags:0x1 io:2 ior:0 sp:2 s0:1 s1:!io_nil))

(vm-ior
 (S:1)
 ((asm (PUSH 1)
       (PUSH !io_nil)
       (PUSH 2)
       (POPR $1)
       (IOR $1)
       (YIELD)))
 (S:1 flags:0x1 io:2 ior:1 r1:2 sp:2 s0:1 s1:!io_nil))


; ##########################################################
; misc

(vm-fault
 (S:1)
 ((asm (FAULT)))
 (S:1 flags:0x4 ret:0xFFFFFFFF))

(vm-pack
 (S:1)
 ((asm (PUSH 1)
       (PUSH 2)
       (PACK)
       (YIELD)))
 (S:1 sp:1 s0:0x200000001))

(vm-unpack
 (S:1)
 ((asm (PUSH 0x200000001)
       (UNPACK)
       (YIELD)))
 (S:1 sp:2 s0:1 s1:2))
