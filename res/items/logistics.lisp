;; -----------------------------------------------------------------------------
;; t0
;; -----------------------------------------------------------------------------

(worker
 (info (type logistics))
 (specs (lab-bits u8 8) (lab-work work 16) (lab-energy energy 8))
 (tape (work 6) (energy 16) (host item-assembly)
       (in (item-lung 2)
	   item-stem
	   item-nodule
	   item-stem
	   (item-limb 2))
       (out item-worker)))


;; -----------------------------------------------------------------------------
;; t1
;; -----------------------------------------------------------------------------

(solar
 (info (type logistics))

 (specs
  (lab-bits u8 16) (lab-work work 32) (lab-energy energy 16)
  (energy-div energy 4096)
  (energy fn))

 (tape (work 6) (energy 16) (host item-assembly)
       (in item-photovoltaic
	   item-nerve
	   item-nodule
	   item-nerve
	   item-photovoltaic)
       (out item-solar)))


(pill
 (info (type logistics))
 (specs (lab-bits u8 16) (lab-work work 32) (lab-energy energy 32))
 (tape (work 8) (energy 24) (host item-assembly)
       (in item-vein
	   item-ferrofluid
	   item-vein
	   (item-bone 2)
	   item-vein
	   item-ferrofluid
	   item-vein)
       (out item-pill)))


(battery
 (info (type logistics))
 (specs (lab-bits u8 16) (lab-work work 24) (lab-energy energy 32)
	(storage-cap energy 8))
 (tape (work 4) (energy 8) (host item-assembly)
       (in item-bone
	   item-nerve
	   (item-galvanic 3)
	   item-nerve
	   item-bone)
       (out item-battery)))


;; -----------------------------------------------------------------------------
;; t3
;; -----------------------------------------------------------------------------

(kwheel
 (info (type logistics))
 (specs (lab-bits u8 64) (lab-work work 128) (lab-energy energy 256)
	(energy-div energy 100)))
