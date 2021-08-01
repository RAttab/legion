;; ==========================================================
;; Systemy tests

(vm/preempt
 (vm (S 1) (C 1))
 (mod
  (asm (PUSH 1)
       (PUSH 2)
       (YIELD)))
 (check (sp 1) (s 0 1)))


;; ==========================================================
;; push/pop

(vm/push
 (vm (S 1))
 (mod
  (asm (PUSH 1)
       (YIELD)))
 (check (sp 1) (s 0 1)))

(vm/pushr
 (vm (S 1) (r 0 12))
 (mod
  (asm (PUSHR $0)
       (YIELD)))
 (check (sp 1) (r 0 12) (s 0 12)))

(vm/pushf
 (vm (S 1) (flags 0x1))
 (mod
  (asm (PUSHF)
       (YIELD)))
 (check (flags 0x1) (sp 1) (s 0 1)))

(vm/pop
 (vm (S 1))
 (mod
  (asm (PUSH 1)
       (POP)
       (YIELD)))
 (check (sp 0)))

(vm/popr
 (vm (S 1))
 (mod
  (asm (PUSH 1)
       (POPR $0)
       (YIELD)))
 (check (sp 0) (r 0 1)))

(vm/dupe
 (vm (S 2))
 (mod
  (asm (PUSH 1)
       (DUPE)
       (YIELD)))
 (check (sp 2) (s 0 1) (s 1 1)))

(vm/swap
 (vm (S 2))
 (mod
  (asm (PUSH 1)
       (PUSH 2)
       (SWAP)
       (YIELD)))
 (check (sp 2) (s 0 2) (s 1 1)))


;; ==========================================================
;; boolean

(vm/not
 (vm (S 2))
 (mod
  (asm (PUSH 0x123456789aBcDeF0)
       (NOT)
       (PUSH 0)
       (NOT)
       (YIELD)))
 (check (sp 2) (s 0 0) (s 1 1)))

(vm/and
 (vm (S 5))
 (mod
  (asm (PUSH 1)
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
 (check (sp 4)
	(s 0 1)
	(s 1 0)
	(s 2 0)
	(s 3 0)))

(vm/or
 (vm (S 5))
 (mod
  (asm (PUSH 1)
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
 (check (sp 4)
	(s 0 1)
	(s 1 1)
	(s 2 1)
	(s 3 0)))

(vm/xor
 (vm (S 5))
 (mod
  (asm (PUSH 1)
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
 (check (sp 4)
	(s 0 0)
	(s 1 1)
	(s 2 1)
	(s 3 0)))


;; ==========================================================
;; binary

(vm/bnot
 (vm (S 1))
 (mod
  (asm (PUSH 0xFF00FF00FF00FF00)
       (BNOT)
       (YIELD)))
 (check (sp 1) (s 0 0x00ff00ff00ff00ff)))

(vm/band
 (vm (S 2))
 (mod
  (asm (PUSH 0xFF00FF00FF00FF00)
       (PUSH 0xF0F0F0F0F0F0F0F0)
       (BAND)
       (YIELD)))
 (check (sp 1) (s 0 0xf000f000f000f000)))

(vm/bor
 (vm (S 2))
 (mod
  (asm (PUSH 0xFF00FF00FF00FF00)
       (PUSH 0xF0F0F0F0F0F0F0F0)
       (BOR)
       (YIELD)))
 (check (sp 1) (s 0 0xfff0fff0fff0fff0)))

(vm/bxor
 (vm (S 2))
 (mod
  (asm (PUSH 0xFF00FF00FF00FF00)
       (PUSH 0xF0F0F0F0F0F0F0F0)
       (BXOR)
       (YIELD)))
 (check (sp 1) (s 0 0x0ff00ff00ff00ff0)))

(vm/bsl
 (vm (S 2))
 (mod
  (asm (PUSH 0xFF00FF00FF00FF00)
       (PUSH 4)
       (BSL)
       (YIELD)))
 (check (sp 1) (s 0 0xf00ff00ff00ff000)))

(vm/bsr
 (vm (S 2))
 (mod
  (asm (PUSH 0xFF00FF00FF00FF00)
       (PUSH 4)
       (BSR)
       (YIELD)))
 (check (sp 1) (s 0 0x0ff00ff00ff00ff0)))


;; ==========================================================
;; math

(vm/neg
 (vm (S 2))
 (mod
  (asm (PUSH -1)
       (NEG)
       (PUSH 1)
       (NEG)
       (YIELD)))
 (check (sp 2) (s 0 1) (s 1 -1)))

(vm/add
 (vm (S 2))
 (mod
  (asm (PUSH 1)
       (PUSH 1)
       (ADD)
       (YIELD)))
 (check (sp 1) (s 0 2)))

(vm/sub
 (vm (S 2))
 (mod
  (asm (PUSH 3)
       (PUSH 2)
       (SUB)
       (YIELD)))
 (check (sp 1) (s 0 1)))

(vm/mul
 (vm (S 2))
 (mod
  (asm (PUSH 2)
       (PUSH 3)
       (MUL)
       (YIELD)))
 (check (sp 1) (s 0 6)))

(vm/lmul
 (vm (S 2))
 (mod
  (asm (PUSH 0x7FFFFFFFFFFFFFFF)
       (PUSH 0xF)
       (LMUL)
       (YIELD)))
 (check (sp 2) (s 0 0x7FFFFFFFFFFFFFF1) (s 1 0x7)))

(vm/div
 (vm (S 2))
 (mod
  (asm (PUSH 6)
       (PUSH 3)
       (DIV)
       (YIELD)))
 (check (sp 1) (s 0 2)))

(vm/rem
 (vm (S 2))
 (mod
  (asm (PUSH 5)
       (PUSH 3)
       (REM)
       (YIELD)))
 (check (sp 1) (s 0 2)))


;; ==========================================================
;; comp

(vm/eq
 (vm (S 3))
 (mod
  (asm (PUSH 1)
       (PUSH 1)
       (EQ)
       (PUSH 1)
       (PUSH 2)
       (EQ)
       (YIELD)))
 (check (sp 2)
	(s 0 1)
	(s 1 0)))

(vm/ne
 (vm (S 3))
 (mod
  (asm (PUSH 1)
       (PUSH 1)
       (NE)
       (PUSH 1)
       (PUSH 2)
       (NE)
       (YIELD)))
 (check (sp 2)
	(s 0 0)
	(s 1 1)))

(vm/gt
 (vm (S 4))
 (mod
  (asm (PUSH 1)
       (PUSH 1)
       (GT)
       (PUSH 1)
       (PUSH 2)
       (GT)
       (PUSH 2)
       (PUSH 1)
       (GT)
       (YIELD)))
 (check (sp 3)
	(s 0 0)
	(s 1 1)
	(s 2 0)))

(vm/ge
 (vm (S 4))
 (mod
  (asm (PUSH 1)
       (PUSH 1)
       (GE)
       (PUSH 1)
       (PUSH 2)
       (GE)
       (PUSH 2)
       (PUSH 1)
       (GE)
       (YIELD)))
 (check (sp 3)
	(s 0 1)
	(s 1 1)
	(s 2 0)))

(vm/lt
 (vm (S 4))
 (mod
  (asm (PUSH 1)
       (PUSH 1)
       (LT)
       (PUSH 1)
       (PUSH 2)
       (LT)
       (PUSH 2)
       (PUSH 1)
       (LT)
       (YIELD)))
 (check (sp 3)
	(s 0 0)
	(s 1 0)
	(s 2 1)))

(vm/le
 (vm (S 4))
 (mod
  (asm (PUSH 1)
       (PUSH 1)
       (LE)
       (PUSH 1)
       (PUSH 2)
       (LE)
       (PUSH 2)
       (PUSH 1)
       (LE)
       (YIELD)))
 (check (sp 3)
	(s 0 1)
	(s 1 0)
	(s 2 1)))

(vm/cmp
 (vm (S 4))
 (mod
  (asm (PUSH 1)
       (PUSH 1)
       (CMP)
       (PUSH 1)
       (PUSH 3)
       (CMP)
       (PUSH 3)
       (PUSH 1)
       (CMP)
       (YIELD)))
 (check (sp 3)
	(s 0 0)
	(s 1 2)
	(s 2 -2)))


;; ==========================================================
;; jmp

(vm/load
 (vm (S 1))
 (mod
  (asm (PUSH 1)
       (LOAD)))
 (check (ret 1) (ip 0) (sp 0)))

(vm/call
 (vm (S 1))
 (mod
  (asm (CALL fn)
       (@ fn)
       (YIELD)))
 (check (ret 0) (sp 1) (s 0 0x9)))

(vm/call-mod
 (vm (S 1))
 (mod
  (asm (CALL 0x100000002)))
 (check (ret 1) (sp 1)))

(vm/ret
 (vm (S 1))
 (mod
  (asm (CALL fn)
       (YIELD)
       (@ fn)
       (RET)))
 (check (ret 0) (sp 0)))

(vm/ret-mod
 (vm (S 1))
 (mod
  (asm (PUSH 0x100000002)
       (RET)))
 (check (ret 1) (ip 2) (sp 0)))

(vm/jmp
 (vm (S 1))
 (mod
  (asm (JMP out)
       (PUSH 0xffffffff)
       (@ out)
       (YIELD)))
 (check (ret 0) (sp 0)))

(vm/jz
 (vm (S 1))
 (mod
  (asm (PUSH 1)
       (JZ fail)
       (PUSH 0)

       (JZ ok)
       (JMP fail)

       (@ fail)
       (PUSH 0xffffffff)
       (@ ok)
       (YIELD)))
 (check (sp 0)))

(vm/jnz
 (vm (S 1))
 (mod
  (asm (PUSH 0)
       (JNZ fail)

       (PUSH 1)
       (JNZ ok)
       (JMP fail)

       (@ fail)
       (PUSH 0xffffffff)
       (@ ok)
       (YIELD)))
 (check (sp 0)))

(vm/back-ref
 (vm (S 1))
 (mod
  (asm (JMP next)
       (@ back)
       (PUSH 1)
       (YIELD)
       (@ next)
       (JMP back)))
 (check (sp 1) (s 0 1)))


;; ==========================================================
;; io

(vm/io
 (vm (S 2))
 (mod
  (asm (PUSH !io_nil)
       (PUSH 1)
       (IO 2)
       (YIELD)))
 (check (flags 0x1) (io 2) (ior 0xff)
	(sp 2) (s 0 !io_nil) (s 1 1)))

(vm/ios
 (vm (S 3))
 (mod
  (asm (PUSH !io_nil)
       (PUSH 1)
       (PUSH 2)
       (IOS)
       (YIELD)))
 (check (flags 0x1) (io 2) (ior 0)
	(sp 2) (s 0 !io_nil) (s 1 1)))

(vm/ior
 (vm (S 3))
 (mod
  (asm (PUSH !io_nil)
       (PUSH 1)
       (PUSH 2)
       (POPR $0)
       (IOR $0)
       (YIELD)))
 (check (flags 0x1) (io 2) (ior 1) (r 0 2) (sp 2) (s 0 !io_nil) (s 1 1)))


;; ==========================================================
;; misc

(vm/fault
 (vm (S 0))
 (mod
  (asm (FAULT)))
 (check (flags 0x4) (ret 0xFFFFFFFF)))

(vm/pack
 (vm (S 2))
 (mod
  (asm (PUSH 1)
       (PUSH 2)
       (PACK)
       (YIELD)))
 (check (sp 1) (s 0 0x200000001)))

(vm/unpack
 (vm (S 2))
 (mod
  (asm (PUSH 0x200000001)
       (UNPACK)
       (YIELD)))
 (check (sp 2) (s 0 2) (s 1 1)))
