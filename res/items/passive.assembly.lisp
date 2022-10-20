;; -----------------------------------------------------------------------------
;; t0
;; -----------------------------------------------------------------------------

(limb
 (info (type passive))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 2) (energy 4) (host item-assembly)
  (in (item-bone 2)
      (item-tendon 2)
      (item-muscle 2))
  (out item-limb)))

(spinal
 (info (type passive))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 2) (energy 4) (host item-assembly)
  (in item-nerve
      item-vein
      item-nerve
      item-vein
      item-nerve)
  (out item-spinal)))

(stem
 (info (type passive))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 2) (energy 4) (host item-assembly)
  (in (item-neuron 2)
      item-vein
      (item-neuron 2))
  (out item-stem)))

(lung
 (info (type passive))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 3) (energy 4) (host item-assembly)
  (in item-stem
      item-nerve
      (item-muscle 3)
      item-nerve)
  (out item-lung)))

(engram
 (info (type passive))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 4) (energy 6) (host item-assembly)
  (in item-nerve
      (item-neuron 2)
      item-nerve
      (item-neuron 2)
      item-nerve)
  (out item-engram)))

(cortex
 (info (type passive))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 8) (energy 8) (host item-assembly)
  (in item-stem
      item-vein
      (item-neuron 3)
      item-vein
      (item-neuron 3)
      item-vein)
  (out item-cortex)))

(eye
 (info (type passive))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 2) (energy 6) (host item-assembly)
  (in item-retina
      (item-lens 3)
      item-retina)
  (out item-eye)))

(fusion
 (info (type passive))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 16) (energy 16) (host item-assembly)
  (in item-torus
      (item-rod 4)
      item-torus
      item-stem
      item-cortex
      item-stem
      item-torus
      (item-rod 4)
      item-torus)
  (out item-fusion)))

;; -----------------------------------------------------------------------------
;; t1
;; -----------------------------------------------------------------------------

(photovoltaic
 (info (type passive))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 8) (energy 16) (host item-assembly)
  (in (item-semiconductor 2)
      item-nerve
      (item-bone 2)
      item-nerve
      (item-semiconductor 2))
  (out item-photovoltaic)))

(field
 (info (type passive))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 8) (energy 16) (host item-assembly)
  (in (item-magnet 3)
      (item-nerve 2)
      (item-magnet 3))
  (out item-field)))

(antenna
 (info (type passive))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 12) (energy 24) (host item-assembly)
  (in (item-bone 2)
      item-nerve
      (item-conductor 2)
      item-nerve
      (item-conductor 2)
      item-nerve
      (item-bone 2))
  (out item-antenna)))


;; -----------------------------------------------------------------------------
;; t2
;; -----------------------------------------------------------------------------

(accelerator
 (info (type passive))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 16) (energy 64) (host item-assembly)
  (in (item-conductor 2)
      item-nerve
      (item-field 2)
      item-nerve
      (item-conductor 2))
  (out item-accelerator)))

(heat-exchange
 (info (type passive))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 16) (energy 72) (host item-assembly)
  (in item-biosteel
      (item-conductor 3)
      item-biosteel)
  (out item-heat-exchange)))

(furnace
 (info (type passive))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 20) (energy 76) (host item-assembly)
  (in item-biosteel
      (item-heat-exchange 2)
      item-elem-m
      (item-heat-exchange 2)
      item-biosteel)
  (out item-elem-o
       item-furnace
       item-elem-o)))

(freezer
 (info (type passive))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 20) (energy 76) (host item-assembly)
  (in (item-heat-exchange 2)
      item-neurosteel
      item-elem-m
      item-neurosteel
      (item-heat-exchange 2))
  (out item-freezer)))

(m-reactor
 (info (type passive))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 32) (energy 128) (host item-assembly)
  (in item-biosteel
      item-heat-exchange
      item-neurosteel
      (item-elem-m 2)
      item-neurosteel
      item-elem-m
      item-neurosteel
      (item-elem-m 2)
      item-neurosteel
      item-heat-exchange
      item-biosteel)
  (out item-elem-o
       item-m-reactor
       item-elem-o)))

(m-condenser
 (info (type passive))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 32) (energy 128) (host item-assembly)
  (in item-biosteel
      item-field
      (item-elem-m 2)
      item-field
      item-elem-m
      item-field
      (item-elem-m 2)
      item-field
      item-biosteel)
  (out item-elem-o
       item-m-condenser
       item-elem-o)))

(m-release
 (info (type passive))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 32) (energy 144) (host item-assembly)
  (in item-biosteel
      (item-ferrofluid 3)
      item-elem-m
      (item-ferrofluid 3)
      item-biosteel)
  (out item-elem-o
       item-m-release
       item-elem-o)))

(m-lung
 (info (type passive))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 40) (energy 256) (host item-assembly)
  (in item-biosteel
      item-m-reactor
      item-neurosteel
      (item-m-condenser 2)
      item-neurosteel
      (item-m-release 3))
  (out item-m-lung)))
