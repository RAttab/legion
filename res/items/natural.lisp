;; -----------------------------------------------------------------------------
;; t0
;; -----------------------------------------------------------------------------

(elem-a
 (info (type natural))
 (specs (lab-bits u8 8) (lab-work work 1) (lab-energy energy 1))
 (tape (host item-extract) (work 1) (energy 1) (out item-elem-a)))

(elem-b
 (info (type natural))
 (specs (lab-bits u8 8) (lab-work work 2) (lab-energy energy 2))
 (tape (host item-extract) (work 2) (energy 1) (out item-elem-b)))

(elem-c
 (info (type natural))
 (specs (lab-bits u8 12) (lab-work work 4) (lab-energy energy 4))
 (tape (host item-extract) (work 2) (energy 2) (out item-elem-c)))

(elem-d
 (info (type natural))
 (specs (lab-bits u8 12) (lab-work work 8) (lab-energy energy 8))
 (tape (host item-extract) (work 4) (energy 4) (out item-elem-d)))


;; -----------------------------------------------------------------------------
;; t1
;; -----------------------------------------------------------------------------

(elem-g
 (info (type natural))
 (specs (lab-bits u8 16) (lab-work work 16) (lab-energy energy 16))
 (tape (host item-condenser) (work 4) (energy 8) (out item-elem-g)))

(elem-h
 (info (type natural))
 (specs (lab-bits u8 24) (lab-work work 32) (lab-energy energy 24))
 (tape (host item-condenser) (work 8) (energy 12) (out item-elem-h)))


;; -----------------------------------------------------------------------------
;; t2
;; -----------------------------------------------------------------------------

(elem-e
 (info (type natural))
 (specs (lab-bits u8 32) (lab-work work 64) (lab-energy energy 32))
 (tape (host item-extract) (work 8) (energy 32) (out item-elem-e)))

(elem-f
 (info (type natural))
 (specs (lab-bits u8 32) (lab-work work 64) (lab-energy energy 48))
 (tape (host item-condenser) (work 16) (energy 64) (out item-elem-i)))


;; -----------------------------------------------------------------------------
;; t3
;; -----------------------------------------------------------------------------

(elem-k (info (type natural)))


;; -----------------------------------------------------------------------------
;; t4
;; -----------------------------------------------------------------------------

(elem-i (info (type natural)))
(elem-j (info (type natural)))
