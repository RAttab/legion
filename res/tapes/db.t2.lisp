;; -----------------------------------------------------------------------------
;; extract / condenser
;; -----------------------------------------------------------------------------

(!item-extract
 (!item-elem-e (out !item-elem-e))
 (!item-elem-f (out !item-elem-f)))

(!item-condenser
 (!item-elem-i (energy 10) (out !item-elem-i))
 (!item-elem-j (energy 100) (out !item-elem-j)))


;; -----------------------------------------------------------------------------
;; collider
;; -----------------------------------------------------------------------------
;; Collider output is not part of the tape as it depends on the size
;; of the collider. Refer to collider_im.c for more details.

(!item-collider
 
 (!item-elem-m
  (energy 10)
  (work 100)
  (in !item-elem-a
      !item-elem-a
      !item-elem-a
      !item-elem-d
      !item-elem-d
      !item-elem-g
      !item-elem-d
      !item-elem-d
      !item-elem-a
      !item-elem-a
      !item-elem-a))
 
 (!item-elem-n
  (energy 100)
  (work 200)
  (in !item-elem-b
      !item-elem-b
      !item-elem-b
      !item-elem-b
      !item-elem-b
      !item-elem-d
      !item-elem-d
      !item-elem-d
      !item-elem-d
      !item-elem-e
      !item-elem-e
      !item-elem-e
      !item-elem-d
      !item-elem-d
      !item-elem-d
      !item-elem-b
      !item-elem-b
      !item-elem-b
      !item-elem-b
      !item-elem-b))
 
 (!item-elem-l
  (work 1000)
  (energy 1000)
  (in !item-elem-h
      !item-elem-h
      !item-elem-h
      !item-elem-h
      !item-elem-h
      !item-elem-m
      !item-elem-m
      !item-elem-m
      !item-elem-m
      !item-elem-n
      !item-elem-n
      !item-elem-n
      !item-elem-n
      !item-elem-m
      !item-elem-m
      !item-elem-m
      !item-elem-m
      !item-elem-h
      !item-elem-h
      !item-elem-h
      !item-elem-h
      !item-elem-h)))


;; -----------------------------------------------------------------------------
;; printer
;; -----------------------------------------------------------------------------

(!item-printer)

;; -----------------------------------------------------------------------------
;; assembly - passive
;; -----------------------------------------------------------------------------

(!item-assembly
 
 (!item-accelerator
  (in !item-conductor
      !item-conductor
      !item-nerve
      !item-field
      !item-field
      !item-nerve
      !item-conductor
      !item-conductor)
  (out !item-accelerator)))


;; -----------------------------------------------------------------------------
;; assembly - active
;; -----------------------------------------------------------------------------

(!item-assembly
 
 (!item-collider
  (energy 100)
  (in !item-brain
      !item-nodule
      !item-accelerator
      !item-accelerator
      !item-nodule
      !item-accelerator
      !item-accelerator
      !item-nodule
      !item-storage)
  (out !item-collider))

 (!item-nomad (out !item-nomad))
 
 (!item-packer (out !item-packer)))
