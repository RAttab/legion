;; -----------------------------------------------------------------------------
;; call
;; -----------------------------------------------------------------------------

(call/defun-before-call
 (mod
  (defun bob-the-fn () 1)
  (assert (= 1 (bob-the-fn))))
 (check))

(call/defun-after-call
 (mod
  (assert (= 1 (bob-the-fn)))
  (defun bob-the-fn () 1))
 (check))


;; -----------------------------------------------------------------------------
;; args
;; -----------------------------------------------------------------------------

(call/args-1
 (mod
  (defun bob-the-fn (a) a)
  (assert (= 10 (bob-the-fn 10))))
 (check))

(call/args-2
 (mod
  (defun bob-the-fn (a b) (- a b))
  (assert (= 8 (bob-the-fn 10 2))))
 (check))

(call/args-4
 (mod
  (defun bob-the-fn (a b c d) (+ a b c d))
  (assert (= 10 (bob-the-fn 1 2 3 4))))
 (check))


;; -----------------------------------------------------------------------------
;; context
;; -----------------------------------------------------------------------------

(call/ctx-save-inner
 (mod
  (defun inner (a b) (- a b))
  (defun outer (a b) (inner a b))
  (assert (= 8 (outer 10 2))))
 (check))

(call/ctx-save-outer
 (mod
  (defun inner (a b))
  (defun outer (a b) (inner 1 1) (- a b))
  (assert (= 8 (outer 10 2))))
 (check))


;; -----------------------------------------------------------------------------
;; mod
;; -----------------------------------------------------------------------------

(call/mod
 (mod (defun fn (a) a)))

(call/mod-basic
 (mod
  (assert (= (call (call/mod fn) 1) 1)))
 (check))
