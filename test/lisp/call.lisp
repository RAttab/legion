;; ##########################################################
;; call

(call/defun-before-call
 ()
 ((defun bob-the-fn () 1)
  (assert (= 1 (bob-the-fn)))
  (yield))
 (sp:0 flags:0))

(call/defun-after-call
 ()
 ((assert (= 1 (bob-the-fn)))
  (yield)
  (defun bob-the-fn () 1))
 (sp:0 flags:0))


;; ##########################################################
;; args

(call/args-1
 ()
 ((defun bob-the-fn (a) a)
  (assert (= 10 (bob-the-fn 10)))
  (yield))
 (sp:0 flags:0))

(call/args-2
 ()
 ((defun bob-the-fn (a b) (- a b))
  (assert (= 8 (bob-the-fn 10 2)))
  (yield))
 (sp:0 flags:0))

(call/args-4
 ()
 ((defun bob-the-fn (a b c d) (+ a b c d))
  (assert (= 10 (bob-the-fn 1 2 3 4)))
  (yield))
 (sp:0 flags:0))


;; ##########################################################
;; context

(call/ctx-save-inner
 ()
 ((defun inner (a b) (- a b))
  (defun outer (a b) (inner a b))
  (assert (= 8 (outer 10 2)))
  (yield))
 (sp:0 flags:0))

(call/ctx-save-outer
 ()
 ((defun inner (a b))
  (defun outer (a b) (inner 1 1) (- a b))
  (assert (= 8 (outer 10 2)))
  (yield))
 (sp:0 flags:0))
