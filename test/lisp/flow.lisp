;; ##########################################################
;; let

(flow/let-0
 ()
 ((assert (= 1 (let () 1)))
  (yield))
 (sp:0 flags:0))

(flow/let-2
 ()
 ((assert (= 3 (let ((a 1) (b 2)) (+ a b))))
  (yield))
 (sp:0 flags:0))

(flow/let-stmts
 ()
 ((assert (= 1 (let ((a 1)) 0 a)))
  (yield))
 (sp:0 flags:0))


;; ##########################################################
;; if

(flow/if-true
 ()
 ((assert (= 1 (if 1 1)))
  (yield))
 (sp:0 flags:0))

(flow/if-false
 ()
 ((assert (= 0 (if 0 1)))
  (yield))
 (sp:0 flags:0))

(flow/if-else-true
 ()
 ((assert (= 1 (if 1 1 2)))
  (yield))
 (sp:0 flags:0))

(flow/if-else-false
 ()
 ((assert (= 2 (if 0 1 2)))
  (yield))
 (sp:0 flags:0))


;; ##########################################################
;; while

(flow/while-nil
 ()
 ((assert (= 0 (while (< (tsc) 30))))
  (yield))
 (sp:0 flags:0))

(flow/while-basic
 ()
 ((assert (= 1 (while (< (tsc) 30) 1)))
  (yield))
 (sp:0 flags:0))

(flow/while-false
 ()
 ((assert (= 0 (while 0 1)))
  (yield))
 (sp:0 flags:0))


;; ##########################################################
;; for

(flow/for-nil
 ()
 ((assert (= 0 (for (i (tsc)) (< i 30) (tsc))))
  (yield))
 (sp:0 flags:0))

(flow/for-true
 ()
 ((assert (= 1 (for (i (tsc)) (< i 30) (tsc) 1)))
  (yield))
 (sp:0 flags:0))

(flow/for-false
 ()
 ((assert (= 0 (for (i 0) 0 0 1)))
  (yield))
 (sp:0 flags:0))

(flow/for-n
 ()
 ((assert (= 4 (for (i 0) (< i 5) (+ i 1) i)))
  (yield))
 (sp:0 flags:0))
