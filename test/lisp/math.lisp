;; ##########################################################
;; logical

(math/not
 ()
 ((asm (not 0xFF)
       (not 1)
       (not 0))
  (yield))
 (sp:3 #0:0
       #1:0
       #2:1))

(math/and
 ()
 ((asm (and 0 1)
       (and 1 1)
       (and 1 2)
       (and 2 0)
       (and 1 1 1)
       (and 1 1 0)
       (and 1 0 1)
       (and 0 1 1))
  (yield))
 (sp:8 #0:0
       #1:1
       #2:1
       #3:0
       #4:1
       #5:0
       #6:0
       #7:0))

(math/or
 ()
 ((asm (or 0 1)
       (or 0 0)
       (or 1 2)
       (or 2 0)
       (or 0 0 0)
       (or 0 0 1)
       (or 0 1 0)
       (or 1 0 0))
  (yield))
 (sp:8 #0:1
       #1:0
       #2:1
       #3:1
       #4:0
       #5:1
       #6:1
       #7:1))

(math/xor
 ()
 ((asm (xor 0 1)
       (xor 0 0)
       (xor 1 2)
       (xor 2 0)
       (xor 0 0 0)
       (xor 0 1 0)
       (xor 1 1 1)
       (xor 1 0 0))
  (yield))
 (sp:8 #0:1
       #1:0
       #2:0
       #3:1
       #4:0
       #5:1
       #6:1
       #7:1))


;; ##########################################################
;; binary

(math/bnot
 ()
 ((bnot 0xFF00FF00FF00FF00)
  (yield))
 (sp:1 #0:0x00ff00ff00ff00ff))

(math/band
 ()
 ((asm (band 0xFF00FF00FF00FF00
	     0xF0F0F0F0F0F0F0F0)
       (band 0xFF00FF00FF00FF00
	     0xF0F0F0F0F0F0F0F0
	     0x6060606060606060))
  (yield))
 (sp:2 #0:0xF000F000F000F000
       #1:0x6000600060006000))

(math/bor
 ()
 ((asm (bor 0xFF00FF00FF00FF00
	    0xF0F0F0F0F0F0F0F0)
       (bor 0xFF00FF00FF00FF00
	    0xF0F0F0F0F0F0F0F0
	    0x0C0C0C0C0C0C0C0C))
  (yield))
 (sp:2 #0:0xfff0fff0fff0fff0
       #1:0xfffcfffcfffcfffc))

(math/bxor
 ()
 ((asm (bxor 0xFF00FF00FF00FF00
	     0xF0F0F0F0F0F0F0F0)
       (bxor 0xFF00FF00FF00FF00
	     0xF0F0F0F0F0F0F0F0
	     0x0C0C0C0C0C0C0C0C))
  (yield))
 (sp:2 #0:0x0ff00ff00ff00ff0
       #1:0x03fc03fc03fc03fc))

(math/bsl
 ()
 ((bsl 0xFF00FF00FF00FF00 4)
  (yield))
 (sp:1 #0:0xf00ff00ff00ff000))

(math/bsr
 ()
 ((bsr 0xFF00FF00FF00FF00 4)
  (yield))
 (sp:1 #0:0x0ff00ff00ff00ff0))


;; ##########################################################
;; arithmetic

(math/neg
 ()
 ((asm (- 1)
       (- 0)
       (- -1))
  (yield))
 (sp:3 #0:-1 #1:0 #2:1))

(math/add
 ()
 ((asm (+ 1 2)
       (+ 1 2 3))
  (yield))
 (sp:2 #0:3 #1:6))

(math/sub
 ()
 ((asm (- 1 2)
       (- 2 1))
  (yield))
 (sp:2 #0:-1 #1:1))

(math/mul
 ()
 ((asm (* 2 3)
       (* 2 3 4))
  (yield))
 (sp:2 #0:6 #1:24))

(math/lmul
 ()
 ((asm (lmul 0x7FFFFFFFFFFFFFFF 0xF))
  (yield))
 (sp:2 #0:0x7FFFFFFFFFFFFFF1 #1:0x7))

(math/div
 ()
 ((asm (/ 12 4))
  (yield))
 (sp:1 #0:3))

(math/rem
 ()
 ((asm (rem 5 3))
  (yield))
 (sp:1 #0:2))


;; ##########################################################
;; comparaison

(math/eq
 ()
 ((asm (= 1 1)
       (= 1 0))
  (yield))
 (sp:2 #0:1 #1:0))

(math/ne
 ()
 ((asm (/= 1 1)
       (/= 1 0))
  (yield))
 (sp:2 #0:0 #1:1))

(math/gt
 ()
 ((asm (> 1 1)
       (> 1 0)
       (> 0 1))
  (yield))
 (sp:3 #0:0 #1:1 #2:0))

(math/ge
 ()
 ((asm (>= 1 1)
       (>= 1 0)
       (>= 0 1))
  (yield))
 (sp:3 #0:1 #1:1 #2:0))

(math/lt
 ()
 ((asm (< 1 1)
       (< 1 0)
       (< 0 1))
  (yield))
 (sp:3 #0:0 #1:0 #2:1))

(math/le
 ()
 ((asm (<= 1 1)
       (<= 1 0)
       (<= 0 1))
  (yield))
 (sp:3 #0:1 #1:0 #2:1))

(math/cmp
 ()
 ((asm (cmp 3 3)
       (cmp 3 1)
       (cmp 1 3))
  (yield))
 (sp:3 #0:0 #1:2 #2:-2))
