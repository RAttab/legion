;; -----------------------------------------------------------------------------
;; layer 0
;; -----------------------------------------------------------------------------

(elem-a
 (info (tier 0) (type natural) (syllable ark))
 (tape (layer 0) (host extract) (work 1) (energy 1)))

(elem-b
 (info (tier 0) (type natural) (syllable bar))
 (tape (layer 0) (host extract) (work 2) (energy 2)))

(elem-c
 (info (tier 0) (type natural) (syllable chu))
 (tape (layer 0) (host extract) (work 2) (energy 4)))

(elem-o
 (info (tier 2) (type synth) (syllable oth))
 (tape (layer 0) (host dummy) (work 1) (energy 1)))

(elem-g
 (info (tier 2) (type natural) (syllable gho))
 (tape (layer 0) (host extract) (work 16) (energy 32)))

(elem-h
 (info (tier 2) (type natural) (syllable hra))
 (tape (layer 0) (host condenser) (work 16) (energy 42)))

;; -----------------------------------------------------------------------------
;; layer 1
;; -----------------------------------------------------------------------------

(elem-d
 (info (tier 0) (type natural) (syllable dyl))
 (tape (layer 1) (host extract) (work 4) (energy 6)
       (in (elem-b 2))))

(monobarex
 (info (tier 0) (type passive))
 (tape (layer 1) (host printer)
       (in (elem-a 5)
	   (elem-b 1))))

(monobararkon
 (info (tier 0) (type passive))
 (tape (layer 1) (host printer)
       (in (elem-a 3)
	   (elem-b 10))))

(monocharkoid
 (info (tier 0) (type passive))
 (tape (layer 1) (host printer)
       (in (elem-a 2)
	   (elem-c 5))))

(elem-e
 (info (tier 1) (type natural) (syllable erl))
 (tape (layer 1) (host condenser) (work 8) (energy 4)
       (in (elem-a 3) (elem-c 1))))

;; -----------------------------------------------------------------------------
;; layer 2
;; -----------------------------------------------------------------------------

(extract
 (info (tier 0) (type active) (list factory))
 (tape (layer 2)
       (in (monobarex 3)
	   (monobararkon 1))))

(printer
 (info (tier 0) (type active) (list factory))
 (tape (layer 2)
       (in (monobarex 3)
	   (monocharkoid 2))))

(duodylium
 (info (tier 0) (type passive))
 (tape (layer 2) (host printer)
       (in (elem-c 5)
	   (elem-d 2))))

(elem-f
 (info (tier 1) (type natural) (syllable fim))
 (tape (layer 2) (host condenser) (work 12) (energy 8)
       (in (elem-d 1)
	   (elem-e 1))))

;; -----------------------------------------------------------------------------
;; layer 3
;; -----------------------------------------------------------------------------

(tridylarkitil
 (info (tier 0) (type passive))
 (tape (layer 3) (host assembly)
       (in (monocharkoid 2)
	   (duodylium 5))))

(deploy
 (info (tier 0) (type active) (list factory))
 (tape (layer 3)
       (needs (elem-a 10)
	      (elem-b 4)
	      (elem-c 20)
	      (elem-d 2))))

(rod
 (info (tier 0) (type logistics))
 (tape (layer 3)
       (needs (elem-a 50)
	      (elem-b 60)
	      (elem-c 40)
	      (elem-d 20))))

;; -----------------------------------------------------------------------------
;; layer 4
;; -----------------------------------------------------------------------------

(assembly
 (info (tier 0) (type active) (list factory) (config printer))
 (tape (layer 4)
       (in (monobarex 5)
	   (monobararkon 7)
	   (tridylarkitil 3))))

(fusion
 (info (tier 0) (type active) (list factory))
 (specs (input-item item !item-rod)
	(energy-output energy 20)
	(energy-rod energy 4096)
	(energy-cap energy 65536))
 (tape (layer 4)
       (needs (elem-a 80)
	      (elem-b 160)
	      (elem-c 80)
	      (elem-d 50))))

(worker
 (info (tier 0) (type logistics))
 (tape (layer 4)
       (needs (elem-a 80)
	      (elem-b 80)
	      (elem-c 100)
	      (elem-d 20))))

(memory
 (info (tier 0) (type active) (list control))
 (tape (layer 4)
       (needs (elem-a 200)
	      (elem-b 20)
	      (elem-c 80))))

(storage
 (info (tier 1) (type active) (list factory))
 (specs (max u16 4096))
 (tape (layer 4)
       (needs
	(elem-a 300)
	(elem-b 100)
	(elem-c 200)
	(elem-d 20)
	(elem-e 50))))

(solar
 (info (tier 1) (type logistics))
 (specs (energy-div energy 4096) (energy fn))
 (tape (layer 4)
       (needs
	(elem-a 600)
	(elem-b 400)
	(elem-c 200)
	(elem-d 90)
	(elem-e 100)
	(elem-f 50))))

(elem-l
 (info (tier 2) (type synth) (syllable lof))
 (tape (layer 4) (host collider) (work 20) (energy 50)
       (out (elem-l 5) (elem-o 5))
       (in (elem-d 20) (elem-e 15))))

(elem-m
 (info (tier 2) (type synth) (syllable mox))
 (tape (layer 4) (host collider) (work 36) (energy 50)
       (out (elem-m 5) (elem-o 5))
       (in (elem-e 15) (elem-f 10))))

;; -----------------------------------------------------------------------------
;; layer 5
;; -----------------------------------------------------------------------------

(library
 (info (tier 0) (type active) (list control))
 (tape (layer 5)
       (in (memory 1))
       (needs (elem-a 250)
	      (elem-b 300)
	      (elem-c 250)
	      (elem-d 100))))

(brain
 (info (tier 0) (type active) (list control))
 (tape (layer 5)
       (in (memory 1))
       (needs (elem-a 250)
	      (elem-b 300)
	      (elem-c 250)
	      (elem-d 100))))

(prober
 (info (tier 0) (type active) (list control))
 (specs (work-energy energy 8) (work-cap fn))
 (tape (layer 5)
       (in (memory 2))
       (needs (elem-a 500)
	      (elem-b 450)
	      (elem-c 350)
	      (elem-d 150))))

(battery
 (info (tier 1) (type logistics))
 (specs (storage-cap energy 8))
 (tape (layer 5)
       (in (storage 1))
       (needs
	(elem-a 1000)
	(elem-b 580)
	(elem-c 700)
	(elem-d 150)
	(elem-e 180)
	(elem-f 80))))

(receive
 (info (tier 1) (type active) (list control))
 (specs (buffer-max enum 1))
 (tape (layer 5)
       (in (memory 2))
       (needs
	(elem-a 800)
	(elem-b 600)
	(elem-c 300)
	(elem-d 180)
	(elem-e 90))))

(elem-n
 (info (tier 2) (type synth) (syllable nuk))
 (tape (layer 5) (host collider) (work 42) (energy 50)
       (out (elem-n 5) (elem-o 5))
       (in (elem-l 10) (elem-m 5))))

;; -----------------------------------------------------------------------------
;; layer 6
;; -----------------------------------------------------------------------------

(lab
 (info (tier 0) (type active) (list factory))
 (tape (layer 6)
       (in (brain 1))
       (needs (elem-a 1000)
	      (elem-b 1500)
	      (elem-c 800)
	      (elem-d 400))))

(scanner
 (info (tier 0) (type active) (list control))
 (specs (work-energy energy 8) (work-cap fn))
 (tape (layer 6)
       (in (prober 2)
	   (brain 1))
       (needs (elem-a 1400)
	      (elem-b 1350)
	      (elem-c 1100)
	      (elem-d 450))))

(condenser
 (info (tier 1) (type active) (list factory) (config extract))
 (tape (layer 6)
       (in (extract 10))
       (needs (elem-a 800)
	      (elem-b 500)
	      (elem-c 500)
	      (elem-d 50))))

(pill
 (info (tier 1) (type logistics))
 (tape (layer 6)
       (in (storage 1))
       (needs
	(elem-a 700)
	(elem-b 550)
	(elem-c 400)
	(elem-d 100)
	(elem-e 100))))

(transmit
 (info (tier 1) (type active) (list control))
 (specs (launch-energy energy 32)
	(launch-speed u16 500))
 (tape (layer 6)
       (in (receive 1))
       (needs
	(elem-a 1200)
	(elem-b 900)
	(elem-c 700)
	(elem-d 250)
	(elem-e 150)
	(elem-f 50))))

(port
 (info (tier 1) (type active) (list factory))
 (specs (dock-energy energy 64)
	   (launch-energy energy 128)
	   (launch-speed u16 100))
 (tape (layer 6)
       (in
	(worker 2)
	(storage 2))
       (needs
	(elem-a 1760)
	(elem-b 1140)
	(elem-c 1000)
	(elem-d 320)
	(elem-e 300)
	(elem-f 100))))

(accelerator
 (info (tier 2) (type logistics))
 (tape (layer 6)
       (needs
	(elem-a 1000)
	(elem-b 1300)
	(elem-c 1000)
	(elem-d 500)
	(elem-e 150)
	(elem-f 50))))

(burner
 (info (tier 2) (type active) (list factory))
 (specs (energy fn) (work-cap fn))
 (tape (layer 6)
       (needs
	(elem-a 3200)
	(elem-b 2500)
	(elem-c 1100)
	(elem-d 650)
	(elem-e 540)
	(elem-f 120)
	(elem-l 20))))

;; -----------------------------------------------------------------------------
;; layer 7
;; -----------------------------------------------------------------------------

(legion
 (info (tier 0) (type active) (list control))
 (specs (travel-speed u16 100))
 (tape (layer 7)
       (in (worker 2)
	   (fusion 1)
	   (extract 2)
	   (printer 2)
	   (assembly 2)
	   (deploy 2)
	   (memory 1)
	   (library 1)
	   (brain 1)
	   (prober 1))
       (needs (elem-a 1750)
	      (elem-b 1900)
	      (elem-c 1550)
	      (elem-d 500))))

(collider
 (info (tier 2) (type active) (list factory))
 (specs (grow-max u8 64)
	(grow-item item !item-accelerator)
	(junk-item item !item-elem-o)
	(output-rate fn))
 (tape (layer 7)
       (in (accelerator 2))
       (needs
	(elem-a 5000)
	(elem-b 4200)
	(elem-c 3700)
	(elem-d 1400)
	(elem-e 900)
	(elem-f 250))))

(packer
 (info (tier 2) (type active) (list factory))
 (tape (layer 7)
       (needs
	(elem-a 4000)
	(elem-b 1800)
	(elem-c 1700)
	(elem-d 500)
	(elem-e 700)
	(elem-f 150)
	(elem-m 15))))

;; -----------------------------------------------------------------------------
;; layer 8
;; -----------------------------------------------------------------------------

(nomad
 (info (tier 2) (type active) (list factory))
 (specs (travel-speed u16 100)
	(memory-len enum 3)
	(cargo-len enum 12)
	(cargo-max u8 255))
 (tape (layer 8)
       (in (packer 1))
       (needs
	(elem-a 14000)
	(elem-b 6700)
	(elem-c 5300)
	(elem-d 1700)
	(elem-e 2400)
	(elem-f 800)
	(elem-m 50))))

;; -----------------------------------------------------------------------------
;; misc
;; -----------------------------------------------------------------------------

;; (kwheel
;;  (info (tier 3) (type logistics))
;;  (specs (energy-div energy 100))
;;  (tape (layer 8)))

;; (elem-k
;;  (info (tier 3) (type natural) (syllable ko))
;;  (tape (layer 0) (host siphon) (work 4) (energy 4)))

;; -----------------------------------------------------------------------------
;; Layer 15
;; -----------------------------------------------------------------------------

(user (info (type sys)) (tape (layer 15)))
(data (info (type sys)) (tape (layer 15)))
(dummy (info (type sys)) (tape (layer 15)))
(energy (info (type sys)) (tape (layer 15)))
(test (info (type active)) (tape (layer 15) (work 1) (energy 1)))

;; -----------------------------------------------------------------------------
;; Placeholders
;; -----------------------------------------------------------------------------

(elem-i
 (info (tier 4) (type natural) (syllable ist))
 (tape (layer 14) (host extract) (work 4) (energy 4)))
(elem-j
 (info (tier 5) (type natural) (syllable jat))
 (tape (layer 14) (host condenser) (work 4) (energy 4)))
(elem-k
 (info (tier 3) (type natural) (syllable ko))
 (tape (layer 14) (host extract) (work 4) (energy 4)))

(elem-p
 (info (type synth) (syllable abc))
 (tape (layer 14) (host extract) (work 1) (energy 1)))
(elem-q
 (info (type synth) (syllable abc))
 (tape (layer 14) (host extract) (work 1) (energy 1)))
(elem-r
 (info (type synth) (syllable abc))
 (tape (layer 14) (host extract) (work 1) (energy 1)))
(elem-s
 (info (type synth) (syllable abc))
 (tape (layer 14) (host extract) (work 1) (energy 1)))
(elem-t
 (info (type synth) (syllable abc))
 (tape (layer 14) (host extract) (work 1) (energy 1)))
(elem-u
 (info (type synth) (syllable abc))
 (tape (layer 14) (host extract) (work 1) (energy 1)))
(elem-v
 (info (type synth) (syllable abc))
 (tape (layer 14) (host extract) (work 1) (energy 1)))
(elem-w
 (info (type synth) (syllable abc))
 (tape (layer 14) (host extract) (work 1) (energy 1)))
(elem-x
 (info (type synth) (syllable abc))
 (tape (layer 14) (host extract) (work 1) (energy 1)))
(elem-y
 (info (type synth) (syllable abc))
 (tape (layer 14) (host extract) (work 1) (energy 1)))
(elem-z
 (info (type synth) (syllable abc))
 (tape (layer 14) (host extract) (work 1) (energy 1)))

(kwheel
 (info (tier 3) (type logistics))
 (specs (energy-div energy 100))
 (tape (layer 14) (work 1) (energy 1)))
