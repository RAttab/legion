;; ##########################################################
;; call

(call/defun-before-call
 ()
 ((defun bob-the-fn () 1)
  (bob-the-fn)
  (yield))
 (sp:1 #0:1))

(call/defun-after-call
 ()
 ((bob-the-fn)
  (yield)
  (defun bob-the-fn () 1))
 (sp:1 #0:1))


;; ##########################################################
;; args

(call/args-1
 ()
 ((defun bob-the-fn (a) a)
  (bob-the-fn 10)
  (yield))
 (sp:1 #0:10))

(call/args-2
 ()
 ((defun bob-the-fn (a b) (- a b))
  (bob-the-fn 10 2)
  (yield))
 (sp:1 #0:8))

(call/args-4
 ()
 ((defun bob-the-fn (a b c d) (+ a b c d))
  (bob-the-fn 1 2 3 4)
  (yield))
 (sp:1 #0:10))


;; ##########################################################
;; context

(call/ctx-save
 ()
 ((defun inner (a b) (- a b))
  (defun outer (a b) (inner a b))
  (outer 10 2)
  (yield))
 (sp:1 #0:8))
