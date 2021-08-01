;; ==========================================================
;; logical

(math/not
 (mod
  (asm (not 0xFF)
       (not 1)
       (not 0)
       0)
  (yield))
 (check (sp 3)
	(s 0 0)
	(s 1 0)
	(s 2 1)))

(math/and
 (mod
  (asm (and 0 1)
       (and 1 1)
       (and 1 2)
       (and 2 0)
       (and 1 1 1)
       (and 1 1 0)
       (and 1 0 1)
       (and 0 1 1)
       0)
  (yield))
 (check (sp 8) (s 0 0)
	(s 1 1)
	(s 2 1)
	(s 3 0)
	(s 4 1)
	(s 5 0)
	(s 6 0)
	(s 7 0)))

(math/or
 (mod
  (asm (or 0 1)
       (or 0 0)
       (or 1 2)
       (or 2 0)
       (or 0 0 0)
       (or 0 0 1)
       (or 0 1 0)
       (or 1 0 0)
       0)
  (yield))
 (check (sp 8)
	(s 0 1)
	(s 1 0)
	(s 2 1)
	(s 3 1)
	(s 4 0)
	(s 5 1)
	(s 6 1)
	(s 7 1)))

(math/xor
 (mod
  (asm (xor 0 1)
       (xor 0 0)
       (xor 1 2)
       (xor 2 0)
       (xor 0 0 0)
       (xor 0 1 0)
       (xor 1 1 1)
       (xor 1 0 0)
       0)
  (yield))
 (check (sp 8)
	(s 0 1)
	(s 1 0)
	(s 2 0)
	(s 3 1)
	(s 4 0)
	(s 5 1)
	(s 6 1)
	(s 7 1)))


;; ==========================================================
;; binary

(math/bnot
 (mod
  (asm (bnot 0xFF00FF00FF00FF00)
       0)
  (yield))
 (check (sp 1)
	(s 0 0x00ff00ff00ff00ff)))

(math/band
 (mod
  (asm (band 0xFF00FF00FF00FF00
	     0xF0F0F0F0F0F0F0F0)
       (band 0xFF00FF00FF00FF00
	     0xF0F0F0F0F0F0F0F0
	     0x6060606060606060)
       0)
  (yield))
 (check (sp 2)
	(s 0 0xF000F000F000F000)
	(s 1 0x6000600060006000)))

(math/bor
 (mod
  (asm (bor 0xFF00FF00FF00FF00
	    0xF0F0F0F0F0F0F0F0)
       (bor 0xFF00FF00FF00FF00
	    0xF0F0F0F0F0F0F0F0
	    0x0C0C0C0C0C0C0C0C)
       0)
  (yield))
 (check (sp 2)
	(s 0 0xfff0fff0fff0fff0)
	(s 1 0xfffcfffcfffcfffc)))

(math/bxor
 (mod
  (asm (bxor 0xFF00FF00FF00FF00
	     0xF0F0F0F0F0F0F0F0)
       (bxor 0xFF00FF00FF00FF00
	     0xF0F0F0F0F0F0F0F0
	     0x0C0C0C0C0C0C0C0C)
       0)
  (yield))
 (check (sp 2)
	(s 0 0x0ff00ff00ff00ff0)
	(s 1 0x03fc03fc03fc03fc)))

(math/bsl
 (mod
  (asm (bsl 0xFF00FF00FF00FF00 4)
       0)
  (yield))
 (check (sp 1)
	(s 0 0xf00ff00ff00ff000)))

(math/bsr
 (mod
  (asm (bsr 0xFF00FF00FF00FF00 4)
       0)
  (yield))
 (check (sp 1)
	(s 0 0x0ff00ff00ff00ff0)))


;; ==========================================================
;; arithmetic

(math/neg
 (mod
  (asm (- 1)
       (- 0)
       (- -1)
       0)
  (yield))
 (check (sp 3)
	(s 0 -1)
	(s 1 0)
	(s 2 1)))

(math/add
 (mod
  (asm (+ 1 2)
       (+ 1 2 3)
       0)
  (yield))
 (check (sp 2)
	(s 0 3)
	(s 1 6)))

(math/sub
 (mod
  (asm (- 1 2)
       (- 2 1)
       0)
  (yield))
 (check (sp 2)
	(s 0 -1)
	(s 1 1)))

(math/mul
 (mod
  (asm (* 2 3)
       (* 2 3 4)
       0)
  (yield))
 (check (sp 2)
	(s 0 6)
	(s 1 24)))

(math/lmul
 (mod
  (asm (lmul 0x7FFFFFFFFFFFFFFF 0xF)
       0)
  (yield))
 (check (sp 2)
	(s 0 0x7FFFFFFFFFFFFFF1)
	(s 1 0x7)))

(math/div
 (mod
  (asm (/ 12 4)
       0)
  (yield))
 (check (sp 1)
	(s 0 3)))

(math/rem
 (mod
  (asm (rem 5 3)
       0)
  (yield))
 (check (sp 1)
	(s 0 2)))


;; ==========================================================
;; comparaison

(math/eq
 (mod
  (asm (= 1 1)
       (= 1 0)
       0)
  (yield))
 (check (sp 2)
	(s 0 1)
	(s 1 0)))

(math/ne
 (mod
  (asm (/= 1 1)
       (/= 1 0)
       0)
  (yield))
 (check (sp 2)
	(s 0 0)
	(s 1 1)))

(math/gt
 (mod
  (asm (> 1 1)
       (> 1 0)
       (> 0 1)
       0)
  (yield))
 (check (sp 3)
	(s 0 0)
	(s 1 1)
	(s 2 0)))

(math/ge
 (mod
  (asm (>= 1 1)
       (>= 1 0)
       (>= 0 1)
       0)
  (yield))
 (check (sp 3)
	(s 0 1)
	(s 1 1)
	(s 2 0)))

(math/lt
 (mod
  (asm (< 1 1)
       (< 1 0)
       (< 0 1)
       0)
  (yield))
 (check (sp 3)
	(s 0 0)
	(s 1 0)
	(s 2 1)))

(math/le
 (mod
  (asm (<= 1 1)
       (<= 1 0)
       (<= 0 1)
       0)
  (yield))
 (check (sp 3)
	(s 0 1)
	(s 1 0)
	(s 2 1)))

(math/cmp
 (mod
  (asm (cmp 3 3)
       (cmp 3 1)
       (cmp 1 3)
       0)
  (yield))
 (check (sp 3)
	(s 0 0)
	(s 1 2)
	(s 2 -2)))
