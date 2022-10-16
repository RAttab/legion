;; -----------------------------------------------------------------------------
;; condenser
;; -----------------------------------------------------------------------------

(!item-condenser

 (!item-elem-g
  (work 4)
  (energy 8)
  (out !item-elem-g))

 (!item-elem-h
  (work 8)
  (energy 12)
  (in !item-elem-d)
  (out !item-elem-h)))


;; -----------------------------------------------------------------------------
;; printer
;; -----------------------------------------------------------------------------

(!item-printer

 (!item-magnet
  (work 8)
  (energy 16)
  (in !item-elem-d
      !item-elem-d
      !item-elem-b
      !item-elem-b
      !item-elem-d
      !item-elem-d)
  (out !item-magnet))

 (!item-ferrofluid
  (work 8)
  (energy 16)
  (in !item-elem-d
      !item-elem-c
      !item-elem-d
      !item-elem-c
      !item-elem-d)
  (out !item-ferrofluid))

 (!item-semiconductor
  (work 10)
  (energy 24)
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
  (work 10)
  (energy 24)
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
  (work 10)
  (energy 24)
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
  (work 8)
  (energy 16)
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
  (work 8)
  (energy 16)
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
  (work 12)
  (energy 24)
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
  (out !item-antenna)))


;; -----------------------------------------------------------------------------
;; assembly - active
;; -----------------------------------------------------------------------------

(!item-assembly

 (!item-solar
  (work 6)
  (energy 16)
  (in !item-photovoltaic
      !item-nerve
      !item-nodule
      !item-nerve
      !item-photovoltaic)
  (out !item-solar))

 (!item-storage
  (work 8)
  (energy 16)
  (in !item-bone
      !item-bone
      !item-vein
      !item-vein
      !item-vein
      !item-bone
      !item-bone)
  (out !item-storage))

 (!item-port
  (work 24)
  (energy 32)
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
  (work 8)
  (energy 24)
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
  (work 10)
  (energy 24)
  (in !item-lung
      !item-lung
      !item-nodule
      !item-lung
      !item-lung
      !item-lung)
  (out !item-condenser))

 (!item-battery
  (work 4)
  (energy 8)
  (in !item-bone
      !item-nerve
      !item-galvanic
      !item-galvanic
      !item-galvanic
      !item-nerve
      !item-bone)
  (out !item-battery))

 (!item-transmit
  (work 16)
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
  (work 16)
  (energy 20)
  (in !item-antenna
      !item-antenna
      !item-antenna
      !item-nodule
      !item-engram
      !item-engram
      !item-engram)
  (out !item-receive)))
