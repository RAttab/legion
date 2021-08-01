;; ==========================================================
;; progn

(flow/progn-1
 (mod (assert (= 1 (progn 1)))))

(flow/progn-n
 (mod (assert (= 5 (progn 1 2 3 4 5)))))


;; ==========================================================
;; let

(flow/let-0
 (mod (assert (= 1 (let () 1)))))

(flow/let-2
 (mod (assert (= 3 (let ((a 1) (b 2)) (+ a b))))))

(flow/let-stmts
 (mod (assert (= 1 (let ((a 1)) 0 a)))))


;; ==========================================================
;; branch

(flow/if-true
 (mod (assert (= 1 (if 1 1)))))

(flow/if-false
 (mod (assert (= 0 (if 0 1)))))

(flow/if-else-true
 (mod (assert (= 1 (if 1 1 2)))))

(flow/if-else-false
 (mod (assert (= 2 (if 0 1 2)))))

(flow/when-true
 (mod (assert (= 1 (when 1 0 1)))))

(flow/when-false
 (mod (assert (= 0 (when 0 0 1)))))

(flow/unless-true
 (mod (assert (= 0 (unless 1 0 1)))))

(flow/unless-false
 (mod (assert (= 1 (unless 0 0 1)))))


;; ==========================================================
;; while

(flow/while-nil
 (mod (assert (= 0 (while (< (tsc) 30))))))

(flow/while-basic
 (mod (assert (= 1 (while (< (tsc) 30) 1)))))

(flow/while-false
 (mod (assert (= 0 (while 0 1)))))


;; ==========================================================
;; for

(flow/for-nil
 (mod (assert (= 0 (for (i (tsc)) (< i 30) (tsc))))))

(flow/for-true
 (mod (assert (= 1 (for (i (tsc)) (< i 30) (tsc) 1)))))

(flow/for-false
 (mod (assert (= 0 (for (i 0) 0 0 1)))))

(flow/for-n
 (mod (assert (= 4 (for (i 0) (< i 5) (+ i 1) i)))))
