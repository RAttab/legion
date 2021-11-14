;; -----------------------------------------------------------------------------
;; progn
;; -----------------------------------------------------------------------------

(flow/progn-1
 (mod (assert (= 1 (progn 1))))
 (check))

(flow/progn-n
 (mod (assert (= 5 (progn 1 2 3 4 5))))
 (check))


;; -----------------------------------------------------------------------------
;; let
;; -----------------------------------------------------------------------------

(flow/let-0
 (mod (assert (= 1 (let () 1))))
 (check))

(flow/let-2
 (mod (assert (= 3 (let ((a 1) (b 2)) (+ a b)))))
 (check))

(flow/let-stmts
 (mod (assert (= 1 (let ((a 1)) 0 a))))
 (check))


;; -----------------------------------------------------------------------------
;; branch
;; -----------------------------------------------------------------------------

(flow/if-true
 (mod (assert (= 1 (if 1 1))))
 (check))

(flow/if-false
 (mod (assert (= 0 (if 0 1))))
 (check))

(flow/if-else-true
 (mod (assert (= 1 (if 1 1 2))))
 (check))

(flow/if-else-false
 (mod (assert (= 2 (if 0 1 2))))
 (check))

(flow/when-true
 (mod (assert (= 1 (when 1 0 1))))
 (check))

(flow/when-false
 (mod (assert (= 0 (when 0 0 1))))
 (check))

(flow/unless-true
 (mod (assert (= 0 (unless 1 0 1))))
 (check))

(flow/unless-false
 (mod (assert (= 1 (unless 0 0 1))))
 (check))


;; -----------------------------------------------------------------------------
;; while
;; -----------------------------------------------------------------------------

(flow/while-nil
 (mod (assert (= 0 (while (< (tsc) 30)))))
 (check))

(flow/while-basic
 (mod (assert (= 1 (while (< (tsc) 30) 1))))
 (check))

(flow/while-false
 (mod (assert (= 0 (while 0 1))))
 (check))


;; -----------------------------------------------------------------------------
;; for
;; -----------------------------------------------------------------------------

(flow/for-nil
 (mod (assert (= 0 (for (i (tsc)) (< i 30) (tsc)))))
 (check))

(flow/for-true
 (mod (assert (= 1 (for (i (tsc)) (< i 30) (tsc) 1))))
 (check))

(flow/for-false
 (mod (assert (= 0 (for (i 0) 0 0 1))))
 (check))

(flow/for-n
 (mod (assert (= 4 (for (i 0) (< i 5) (+ i 1) i))))
 (check))


;; -----------------------------------------------------------------------------
;; case
;; -----------------------------------------------------------------------------

(flow/case-simplest
 (mod (assert (= 5 (case 5 ()))))
 (check))

(flow/case-true
 (mod (assert (= 2 (case 5 ((0 1) (5 2) (2 3)) (x 4)))))
 (check))

(flow/case-default
 (mod (assert (= 10 (case 5 ((0 1) (1 2) (1 3)) (x (* x 2))))))
 (check))

(flow/case-stmts
 (mod (assert (= 1 (case 0 ((0 1 1)) (x 2 2)))))
 (check))
