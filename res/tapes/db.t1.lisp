;; -----------------------------------------------------------------------------
;; condenser
;; -----------------------------------------------------------------------------

(!item-condenser

 (!item-elem-g (energy 1) (out !item-elem-g))
 (!item-elem-h (energy 1) (out !item-elem-h)))


;; -----------------------------------------------------------------------------
;; printer
;; -----------------------------------------------------------------------------

(!item-printer

 (!item-magnet
  (in !item-elem-d
      !item-elem-d
      !item-elem-b
      !item-elem-b
      !item-elem-d
      !item-elem-d)
  (out !item-magnet))

 (!item-ferrofluid
  (in !item-elem-d
      !item-elem-c
      !item-elem-d
      !item-elem-c
      !item-elem-d)
  (out !item-ferrofluid))

 (!item-semiconductor
  (in !item-elem-d
      !item-elem-d
      !item-elem-a
      !item-elem-c
      !item-elem-c
      !item-elem-a
      !item-elem-d
      !item-elem-d)
  (out !item-semiconductor))

 (!item-conductor
  (in !item-elem-g
      !item-elem-g
      !item-elem-g
      !item-elem-d
      !item-elem-d
      !item-elem-g
      !item-elem-g
      !item-elem-g)
  (out !item-conductor))

 (!item-galvanic
  (in !item-elem-g
      !item-elem-h
      !item-elem-h
      !item-elem-g
      !item-elem-g
      !item-elem-h
      !item-elem-h
      !item-elem-g)
  (out !item-galvanic)))


;; -----------------------------------------------------------------------------
;; assembly - passive
;; -----------------------------------------------------------------------------

(!item-assembly

 (!item-photovoltaic
  (in !item-semiconductor
      !item-semiconductor
      !item-nerve
      !item-bone
      !item-bone
      !item-nerve
      !item-semiconductor
      !item-semiconductor)
  (out !item-photovoltaic))

 (!item-field
  (in !item-magnet
      !item-magnet
      !item-magnet
      !item-nerve
      !item-nerve
      !item-magnet
      !item-magnet
      !item-magnet)
  (out !item-field))

 (!item-antenna
  (in !item-bone
      !item-bone
      !item-nerve
      !item-conductor
      !item-conductor
      !item-nerve
      !item-conductor
      !item-conductor
      !item-nerve
      !item-bone
      !item-bone)
  (out !item-antenna))

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

 (!item-solar
  (in !item-photovoltaic
      !item-nerve
      !item-nodule
      !item-nerve
      !item-photovoltaic)
  (out !item-solar))

 (!item-storage
  (in !item-bone
      !item-bone
      !item-vein
      !item-vein
      !item-vein
      !item-bone
      !item-bone)
  (out !item-storage))

 (!item-port
  (in !item-field
      !item-field
      !item-field
      !item-field
      !item-field
      !item-nodule
      !item-limb
      !item-limb
      !item-limb
      !item-nodule
      !item-storage)
  (out !item-port))

 (!item-pill
  (in !item-vein
      !item-ferrofluid
      !item-vein
      !item-bone
      !item-bone
      !item-vein
      !item-ferrofluid
      !item-vein)
  (out !item-pill))

 (!item-condenser
  (in !item-lung
      !item-lung
      !item-nodule
      !item-lung
      !item-lung
      !item-lung)
  (out !item-condenser))

 (!item-battery
  (energy 1)
  (in !item-bone
      !item-nerve
      !item-galvanic
      !item-galvanic
      !item-galvanic
      !item-nerve
      !item-bone)
  (out !item-battery))

 ;; (!item-auto-deploy
 ;;  (energy 1)
 ;;  (in )
 ;;  (out !item-auto-deploy))

 (!item-transmit
  (energy 20)
  (in !item-eye
      !item-eye
      !item-eye
      !item-nodule
      !item-antenna
      !item-antenna
      !item-antenna)
  (out !item-transmit))

 (!item-receive
  (energy 20)
  (in !item-antenna
      !item-antenna
      !item-antenna
      !item-nodule
      !item-engram
      !item-engram
      !item-engram)
  (out !item-receive))

 (!item-collider
  (energy 100)
  (in !item-brain
      !item-nodule
      !item-accelerator
      !item-accelerator
      !item-nodule
      !item-accelerator
      !item-accelerator
      !item-nodule)
  (out !item-collider)))
