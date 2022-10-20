;; -----------------------------------------------------------------------------
;; t2
;; -----------------------------------------------------------------------------

(elem-m
 (info (type synth))
 (specs (lab-bits var 16) (lab-work var 128) (lab-energy var 64))
 
 (tape
  (host item-collider)
  (work 16)
  (energy 128)
  (in (item-elem-a 3)
      (item-elem-d 2)
      (item-elem-g 1)
      (item-elem-d 2)
      (item-elem-a 3))
  (out item-elem-m
       item-elem-o)))

(elem-n
 (info (type synth))
 (specs (lab-bits var 16) (lab-work var 192) (lab-energy var 96))
 
 (tape
  (host item-collider)
  (work 24)
  (energy 256)
  (in (item-elem-b 5)
      (item-elem-d 4)
      (item-elem-e 3)
      (item-elem-d 4)
      (item-elem-b 5))
  (out item-elem-n
       item-elem-o)))

(elem-o
 (info (type synth))
 (specs (lab-bits var 128) (lab-work var 8) (lab-energy var 16))
 (tape (host item-dummy)))

 
;; -----------------------------------------------------------------------------
;; tbd
;; -----------------------------------------------------------------------------

(elem-l (info (type synth)))
(elem-p (info (type synth)))
(elem-q (info (type synth)))
(elem-r (info (type synth)))
(elem-s (info (type synth)))
(elem-t (info (type synth)))
(elem-u (info (type synth)))
(elem-v (info (type synth)))
(elem-w (info (type synth)))
(elem-x (info (type synth)))
(elem-y (info (type synth)))
(elem-z (info (type synth)))
