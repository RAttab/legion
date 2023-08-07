;; defconst
(defconst l 1)

;; defun
(defun blah (a b) (+ a b))

;; let
(let ((x 1)
      (y (+ l 1)))
  (blah x y))

;; for
(for (i l) (> i 0) (- i 1)
     (blah 1 $2))

;; case
(case 2
  ((l (blah l 2))
   (0 (blah 1 1)))
  (x (blah x x)))

;; mod
(mod)
(mod boot)
(mod boot.2)

;; fn
(blah 1)
;; (os.packet)
;; (os.packet.2 1)
