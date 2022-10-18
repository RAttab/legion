;; -----------------------------------------------------------------------------
;; elem-a
;; -----------------------------------------------------------------------------

(item-elem-a
 (info (type natural))

 (specs
  (elem-a-lab-bits lab-bits 4)
  (elem-a-lab-work lab-work 1)
  (elem-a-lab-energy lab-energy 1))

 (tape (host !item-extract) (work 1) (energy 1) (out !item-elem-a)))

;; -----------------------------------------------------------------------------
;; elem-b
;; -----------------------------------------------------------------------------

(item-elem-b
 (info (type natural))

 (specs
  (elem-b-lab-bits lab-bits 4)
  (elem-b-lab-work lab-work 2)
  (elem-b-lab-energy lab-energy 2))

 (tape (host !item-extract) (work 2) (energy 1) (out !item-elem-b)))

;; -----------------------------------------------------------------------------
;; elem-c
;; -----------------------------------------------------------------------------

(item-elem-c
 (info (type natural))

 (specs
  (elem-b-lab-bits lab-bits 4)
  (elem-b-lab-work lab-work 2)
  (elem-b-lab-energy lab-energy 2))
 
 (tape (host !item-extract) (work 2) (energy 2) (out !item-elem-c)))
