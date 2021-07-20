;; ##########################################################
;; let

(flow/let-0
 ()
 ((let () 1)
  (yield))
 (sp:1 #0:1))

(flow/let-2
 ()
 ((let ((a 1) (b 2)) (+ a b))
  (yield))
 (sp:1 #0:3))

(flow/let-stmts
 ()
 ((let ((a 1)) 0 a)
  (yield))
 (sp:1 #0:1))


;; ##########################################################
;; if

(flow/if-true
 ()
 ((if 1 1)
  (yield))
 (sp:1 #0:1))

(flow/if-false
 ()
 ((if 0 1)
  (yield))
 (sp:1 #0:0))

(flow/if-else-true
 ()
 ((if 1 1 2)
  (yield))
 (sp:1 #0:1))

(flow/if-else-false
 ()
 ((if 0 1 2)
  (yield))
 (sp:1 #0:2))


;; ##########################################################
;; while

(flow/while-basic
 ()
 ((while (< (tsc) 30) 1)
  (yield))
 (sp:1 #0:1))

(flow/while-false
 ()
 ((while 0 1)
  (yield))
 (sp:1 #0:0))


;; ##########################################################
;; for

(flow/for-nil
 ()
 ((for (i (tsc)) (< i 30) (tsc) 1)
  (yield))
 (sp:1 #0:1))

(flow/for-false
 ()
 ((for (i 0) 0 0 1)
  (yield))
 (sp:1 #0:0))

(flow/for-n
 ()
 ((for (i 0) (< i 10) (+ i 1) i)
  (yield))
 (sp:1 #0:9))
