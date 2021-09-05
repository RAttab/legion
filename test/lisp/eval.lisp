;; -----------------------------------------------------------------------------
;; basics
;; -----------------------------------------------------------------------------

(eval/basic
 (mod
  (defconst X 1)
  (assert (= X 1))
  (defconst Y 0xFFFFFFFF)
  (assert (= X 1))
  (assert (= Y 0xFFFFFFFF)))
 (check))

(eval/const
 (mod
  (defconst X 5)
  (defconst Y X)
  (assert (= Y 5)))
 (check))


;; -----------------------------------------------------------------------------
;; boolean
;; -----------------------------------------------------------------------------

(eval/not
 (mod
  (defconst X (not 1))
  (assert (not X))
  (defconst Y (not 0))
  (assert Y))
 (check))

(eval/and
 (mod
  (defconst X (and 1 2 3))
  (assert X)
  (defconst Y (and 1 2 0))
  (assert (not Y)))
 (check))

(eval/or
 (mod
  (defconst X (or 0 1 2))
  (assert X)
  (defconst Y (or 0))
  (assert (not Y)))
 (check))

(eval/xor
 (mod
  (defconst X (xor 1 1 0))
  (assert (not X))
  (defconst Y (xor 0 1 0))
  (assert Y))
 (check))


;; -----------------------------------------------------------------------------
;; binary
;; -----------------------------------------------------------------------------

(eval/bnot
 (mod
  (defconst X (bnot 0xFF00FF00FF00FF00))
  (assert (= X 0x00FF00FF00FF00FF)))
 (check))

(eval/band
 (mod
  (defconst X (band 0xFF00FF00FF00FF00
		    0xF0F0F0F0F0F0F0F0))
  (assert (= X 0xF000F000F000F000)))
 (check))

(eval/bor
 (mod
  (defconst X (bor 0xFF00FF00FF00FF00
		   0xF0F0F0F0F0F0F0F0))
  (assert (= X 0xFFF0FFF0FFF0FFF0)))
 (check))

(eval/bxor
 (mod
  (defconst X (bxor 0xFF00FF00FF00FF00
		    0xF0F0F0F0F0F0F0F0))
  (assert (= X 0x0FF00FF00FF00FF0)))
 (check))

(eval/bsl
 (mod
  (defconst X (bsl 0xFF00FF00FF00FF00 4))
  (assert (= X 0xF00FF00FF00FF000)))
 (check))

(eval/bsr
 (mod
  (defconst X (bsr 0xFF00FF00FF00FF00 4))
  (assert (= X 0x0FF00FF00FF00FF0)))
 (check))


;; -----------------------------------------------------------------------------
;; arithmetics
;; -----------------------------------------------------------------------------

(eval/neg
 (mod
  (defconst X (- 1))
  (assert (= X -1)))
 (check))

(eval/add
 (mod
  (defconst X (+ 1 2 3))
  (assert (= X 6)))
 (check))

(eval/sub
 (mod
  (defconst X (- 6 2 1))
  (assert (= X 3)))
 (check))

(eval/mul
 (mod
  (defconst X (* 2 3 4))
  (assert (= X 24)))
 (check))

(eval/div
 (mod
  (defconst X (/ 5 2))
  (assert (= X 2)))
 (check))

(eval/rem
 (mod
  (defconst X (rem 5 3))
  (assert (= X 2)))
 (check))

;; -----------------------------------------------------------------------------
;; cmp
;; -----------------------------------------------------------------------------

(eval/eq
 (mod
  (defconst X (= 1 1))
  (assert X)
  (defconst Y (= 1 0))
  (assert (not Y)))
 (check))

(eval/ne
 (mod
  (defconst X (/= 1 0))
  (assert X)
  (defconst Y (/= 1 1))
  (assert (not Y)))
 (check))

(eval/gt
 (mod
  (defconst X (> 1 0))
  (assert X)
  (defconst Y (> 0 1))
  (assert (not Y)))
 (check))

(eval/ge
 (mod
  (defconst X (>= 1 0))
  (assert X)
  (defconst Y (>= 0 1))
  (assert (not Y)))
 (check))

(eval/lt
 (mod
  (defconst X (< 0 1))
  (assert X)
  (defconst Y (< 1 0))
  (assert (not Y)))
 (check))

(eval/le
 (mod
  (defconst X (<= 0 1))
  (assert X)
  (defconst Y (<= 1 0))
  (assert (not Y)))
 (check))

(eval/cmp
 (mod
  (defconst X (cmp 3 1))
  (assert (> X 0))
  (defconst Y (cmp 1 3))
  (assert (< Y 0))
  (defconst Z (cmp 3 3))
  (assert (= Z 0)))
 (check))


;; -----------------------------------------------------------------------------
;; flow
;; -----------------------------------------------------------------------------

(eval/if-single
 (mod
  (defconst X (if 1 3))
  (assert (= X 3))
  (defconst Y (if 0 3))
  (assert (= Y 0)))
 (check))

(eval/if-both
 (mod
  (defconst X (if 1 3 5))
  (assert (= X 3))
  (defconst Y (if 0 3 5))
  (assert (= Y 5)))
 (check))

(eval/when
 (mod
  (defconst X (when 1 3))
  (assert (= X 3))
  (defconst Y (when 0 3))
  (assert (= Y 0)))
 (check))

(eval/unless
 (mod
  (defconst X (unless 0 3))
  (assert (= X 3))
  (defconst Y (unless 1 3))
  (assert (= Y 0)))
 (check))


;; -----------------------------------------------------------------------------
;; misc
;; -----------------------------------------------------------------------------

(misc/pack
 (mod
  (defconst X (pack 1 2))
  (assert (= X 0x0000000200000001)))
 (check))

(misc/id
 (mod
  (defconst X (id &item_data 0xFFFFFF))
  (assert (= X 0xF0FFFFFF)))
 (check))
