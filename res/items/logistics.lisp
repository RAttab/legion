;; -----------------------------------------------------------------------------
;; t0
;; -----------------------------------------------------------------------------

(worker
 (info (type logistics))
 (specs (lab-bits var 8) (lab-work var 16) (lab-energy var 8))
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
  (lab-bits var 16) (lab-work var 32) (lab-energy var 16)
  (energy-div var 1024))

 (tape (work 6) (energy 16) (host item-assembly)
       (in item-photovoltaic
	   item-nerve
	   item-nodule
	   item-nerve
	   item-photovoltaic)
       (out item-solar)))


(pill
 (info (type logistics))
 (specs (lab-bits var 16) (lab-work var 32) (lab-energy var 32))
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
 (specs (lab-bits var 16) (lab-work var 24) (lab-energy var 32))
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

(kwheel (info (type logistics)))
