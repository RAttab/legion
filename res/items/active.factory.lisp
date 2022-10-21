;; -----------------------------------------------------------------------------
;; t0
;; -----------------------------------------------------------------------------

(deploy
 (info (type active) (list factory))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 4) (energy 8) (host item-assembly)
  (in item-spinal
      item-nodule
      item-spinal)
  (out item-deploy)))

(extract
 (info (type active) (list factory))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 4) (energy 8) (host item-assembly)
  (in (item-muscle 2)
      item-nodule
      (item-muscle 2))
  (out item-extract)))

(printer
 (info (type active) (list factory))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 4) (energy 16) (host item-assembly)
  (in (item-vein 2)
      item-nodule
      (item-vein 2))
  (out item-printer)))

(assembly
 (info (type active) (list factory) (config printer))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 4) (energy 16) (host item-assembly)
  (in (item-limb 2)
      item-vein
      item-nodule
      item-vein
      (item-limb 2))
  (out item-assembly)))

(fusion
 (info (type active))
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

(lab
 (info (type active) (list factory))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 16) (energy 32) (host item-assembly)
  (in (item-limb 2)
      item-vein
      (item-nodule 2)
      item-vein
      (item-cortex 2))
  (out item-lab)))

;; -----------------------------------------------------------------------------
;; t1
;; -----------------------------------------------------------------------------

(storage
 (info (type active) (list factory))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 8) (energy 16) (host item-assembly)
  (in (item-bone 2)
      (item-vein 3)
      (item-bone 2))
  (out item-storage)))

(port
 (info (type active) (list factory))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 24) (energy 32) (host item-assembly)
  (in (item-field 5)
      item-nodule
      (item-limb 3)
      item-nodule
      item-storage)
  (out item-port)))

(condenser
 (info (type active) (list factory) (config extract))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 10) (energy 24) (host item-assembly)
  (in (item-lung 2)
      item-nodule
      (item-lung 3))
  (out item-condenser)))

;; -----------------------------------------------------------------------------
;; t2
;; -----------------------------------------------------------------------------

(collider
 (info (type active) (list factory))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 16) (energy 128) (host item-assembly)
  (in item-brain
      item-nodule
      (item-accelerator 2)
      item-nodule
      (item-accelerator 2)
      item-nodule
      item-storage)
  (out item-collider)))

(burner
 (info (type active) (list factory))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 20) (energy 112) (host item-assembly)
  (in item-biosteel
      (item-furnace 3)
      item-heat-exchange
      item-conductor
      (item-battery 3)
      item-biosteel)
  (out item-burner)))

(packer
 (info (type active) (list factory))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 20) (energy 112) (host item-assembly)
  (in item-biosteel
      item-neurosteel
      (item-freezer 3)
      item-worker
      (item-storage 3)
      item-neurosteel
      item-biosteel)
  (out item-packer)))

(nomad
 (info (type active) (list factory))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 42) (energy 256) (host item-assembly)
  (in item-biosteel
      item-pill
      item-brain
      item-memory
      item-pill
      item-packer
      (item-storage 3)
      item-pill
      item-biosteel
      (item-m-lung 2))
  (out item-nomad)))


;; -----------------------------------------------------------------------------
;; misc
;; -----------------------------------------------------------------------------

(test (info (type active)))
