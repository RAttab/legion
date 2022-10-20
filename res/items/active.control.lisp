;; -----------------------------------------------------------------------------
;; t0
;; -----------------------------------------------------------------------------

(memory
 (info (type active))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 8) (energy 16) (host item-assembly)
  (in (item-engram 3)
      item-nodule
      (item-engram 3))
  (out item-memory)))

(brain
 (info (type active))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 10) (energy 18) (host item-assembly)
  (in item-memory
      (item-cortex 2)
      item-nodule
      (item-cortex 2)
      item-memory)
  (out item-brain)))

(prober
 (info (type active))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 4) (energy 18) (host item-assembly)
  (in item-lung
      item-nodule
      (item-eye 3))
  (out item-prober)))

(scanner
 (info (type active))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 6) (energy 18) (host item-assembly)
  (in item-lung
      item-engram
      item-nodule
      (item-eye 5))
  (out item-scanner)))

(legion
 (info (type active))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 24) (energy 32) (host item-assembly)
  (in (item-lung 3)
      item-nodule
      (item-worker 2)
      item-fusion
      (item-extract 2)
      (item-printer 2)
      (item-assembly 2)
      (item-deploy 2)
      item-memory
      item-brain
      item-prober
      item-nodule
      (item-cortex 2))
  (out item-legion)))

;; -----------------------------------------------------------------------------
;; t1
;; -----------------------------------------------------------------------------

(transmit
 (info (type active))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 16) (energy 20) (host item-assembly)
  (in (item-eye 3)
      item-nodule
      (item-antenna 3))
  (out item-transmit)))

(receive
 (info (type active))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 16) (energy 20) (host item-assembly)
  (in (item-antenna 3)
      item-nodule
      (item-engram 3))
  (out item-receive)))

;; -----------------------------------------------------------------------------
;; t2
;; -----------------------------------------------------------------------------

(nomad
 (info (type active))
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
