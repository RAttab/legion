;; -----------------------------------------------------------------------------
;; t0
;; -----------------------------------------------------------------------------

(memory
 (info (type active) (list control))
 (specs (lab-bits u8 8) (lab-work work 8) (lab-energy energy 8))
 (tape (work 8) (energy 16) (host item-assembly)
  (in (item-engram 3)
      item-nodule
      (item-engram 3))
  (out item-memory)))

(brain
 (info (type active) (list control))
 (specs (lab-bits u8 8) (lab-work work 8) (lab-energy energy 8))
 (tape (work 10) (energy 18) (host item-assembly)
  (in item-memory
      (item-cortex 2)
      item-nodule
      (item-cortex 2)
      item-memory)
  (out item-brain)))

(prober
 (info (type active) (list control))
 (specs (lab-bits u8 8) (lab-work work 8) (lab-energy energy 8)
	(work-cap fn))
 (tape (work 4) (energy 18) (host item-assembly)
  (in item-lung
      item-nodule
      (item-eye 3))
  (out item-prober)))

(scanner
 (info (type active) (list control))
 (specs (lab-bits u8 8) (lab-work work 8) (lab-energy energy 8)
	(work-cap fn))
 (tape (work 6) (energy 18) (host item-assembly)
  (in item-lung
      item-engram
      item-nodule
      (item-eye 5))
  (out item-scanner)))

(legion
 (info (type active) (list control))
 (specs (lab-bits u8 8) (lab-work work 8) (lab-energy energy 8)
	(travel-speed u16 100))
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
 (info (type active) (list control))
 (specs (lab-bits u8 8) (lab-work work 8) (lab-energy energy 8)
	(launch-speed u16 500))
 (tape (work 16) (energy 20) (host item-assembly)
  (in (item-eye 3)
      item-nodule
      (item-antenna 3))
  (out item-transmit)))

(receive
 (info (type active) (list control))
 (specs (lab-bits u8 8) (lab-work work 8) (lab-energy energy 8)
	(buffer-max enum 1))
 (tape (work 16) (energy 20) (host item-assembly)
  (in (item-antenna 3)
      item-nodule
      (item-engram 3))
  (out item-receive)))

;; -----------------------------------------------------------------------------
;; t2
;; -----------------------------------------------------------------------------
