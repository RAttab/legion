;; -----------------------------------------------------------------------------
;; t0
;; -----------------------------------------------------------------------------

(muscle
 (info (type passive))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 2) (energy 2) (host item-printer)
       (in (item-elem-a 3))
       (out item-muscle)))

(nodule
 (info (type passive))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 2) (energy 2) (host item-printer)
       (in item-elem-a
	   (item-elem-b 2)
	   item-elem-a)
       (out item-nodule)))

(vein
 (info (type passive))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 2) (energy 2) (host item-printer)
       (in item-elem-a
	   item-elem-c
	   item-elem-a)
       (out item-vein)))

(bone
 (info (type passive))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 2) (energy 2) (host item-printer)
       (in (item-elem-b 5))
       (out item-bone)))

(tendon
 (info (type passive))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 4) (energy 2) (host item-printer)
       (in item-elem-b
	   (item-elem-a 3)
	   item-elem-b)
       (out item-tendon)))

(rod
 (info (type passive))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 8) (energy 8) (host item-printer)
       (in item-elem-b
	   (item-elem-a 5)
	   item-elem-b
	   (item-elem-a 5)
	   item-elem-b)
       (out item-rod)))

(torus
 (info (type passive))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 4) (energy 4) (host item-printer)
       (in item-elem-b
	   (item-elem-a 2)
	   (item-elem-b 2)
	   (item-elem-a 2)
	   item-elem-b)
       (out item-torus)))

(lens
 (info (type passive))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 2) (energy 4) (host item-printer)
       (in item-elem-b
	   (item-elem-c 2)
	   item-elem-b)
       (out item-lens)))

(nerve
 (info (type passive))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 2) (energy 4) (host item-printer)
       (in (item-elem-c 3))
       (out item-nerve)))

(neuron
 (info (type passive))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 4) (energy 4) (host item-printer)
       (in item-elem-c
	   item-elem-a
	   item-elem-c
	   item-elem-a
	   item-elem-c)
       (out item-neuron)))

(retina
 (info (type passive))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 4) (energy 4) (host item-printer)
       (in item-elem-c
	   (item-elem-b 2)
	   item-elem-c)
       (out item-retina)))

;; -----------------------------------------------------------------------------
;; t1
;; -----------------------------------------------------------------------------

(magnet
 (info (type passive))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 8) (energy 16) (host item-printer)
       (in (item-elem-d 2)
	   (item-elem-b 2)
	   (item-elem-d 2))
       (out item-magnet)))

(ferrofluid
 (info (type passive))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 8) (energy 16) (host item-printer)
       (in item-elem-d
	   item-elem-c
	   item-elem-d
	   item-elem-c
	   item-elem-d)
       (out item-ferrofluid)))

(semiconductor
 (info (type passive))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 10) (energy 24) (host item-printer)
       (in (item-elem-d 2)
	   item-elem-a
	   (item-elem-c 2)
	   item-elem-a
	   (item-elem-d 2))
       (out item-semiconductor)))

(conductor
 (info (type passive))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 10) (energy 24) (host item-printer)
       (in (item-elem-g 3)
	   (item-elem-d 2)
	   (item-elem-g 3))
       (out item-conductor)))

(galvanic
 (info (type passive))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 10) (energy 24) (host item-printer)
       (in item-elem-g
	   (item-elem-h 2)
	   (item-elem-g 2)
	   (item-elem-h 2)
	   item-elem-g)
       (out item-galvanic)))


;; -----------------------------------------------------------------------------
;; t2
;; -----------------------------------------------------------------------------

(biosteel
 (info (type passive))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 16) (energy 64) (host item-printer)
       (in (item-elem-b 2)
	   item-elem-h
	   (item-elem-b 2)
	   item-elem-m
	   (item-elem-b 2)
	   item-elem-h
	   (item-elem-b 2))
       (out item-elem-o
	    item-biosteel
	    item-elem-o)))

(neurosteel
 (info (type passive))
 (specs (lab-bits var 8) (lab-work var 8) (lab-energy var 8))
 (tape (work 16) (energy 72) (host item-printer)
       (in (item-elem-d 2)
	   item-elem-h
	   (item-elem-d 2)
	   item-elem-m
	   (item-elem-d 2)
	   item-elem-h
	   (item-elem-d 2))
       (out item-elem-o
	    item-neurosteel
	    item-elem-o)))
