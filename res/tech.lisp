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
       (in (elem-a 2) (elem-c 1))))

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
       (needs (elem-a 12)
	      (elem-b 15)
	      (elem-c 10)
	      (elem-d 5))))

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
       (needs (elem-a 20)
	      (elem-b 40)
	      (elem-c 20)
	      (elem-d 12))))

(worker
 (info (tier 0) (type logistics))
 (tape (layer 4)
       (needs (elem-a 20)
	      (elem-b 20)
	      (elem-c 25)
	      (elem-d 5))))

(memory
 (info (tier 0) (type active) (list control))
 (tape (layer 4)
       (needs (elem-a 50)
	      (elem-b 25)
	      (elem-c 20))))

(storage
 (info (tier 1) (type active) (list factory))
 (specs (max u16 4096))
 (tape (layer 4)
       (needs
	(elem-a 75)
	(elem-b 25)
	(elem-c 50)
	(elem-d 5)
	(elem-e 12))))

(solar
 (info (tier 1) (type logistics))
 (specs (energy-div energy 4096) (energy fn))
 (tape (layer 4)
       (needs
	(elem-a 150)
	(elem-b 100)
	(elem-c 50)
	(elem-d 22)
	(elem-e 25)
	(elem-f 12))))

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
       (needs (elem-a 62)
	      (elem-b 75)
	      (elem-c 62)
	      (elem-d 25))))

(brain
 (info (tier 0) (type active) (list control))
 (tape (layer 5)
       (in (memory 1))
       (needs (elem-a 62)
	      (elem-b 75)
	      (elem-c 62)
	      (elem-d 25))))

(prober
 (info (tier 0) (type active) (list control))
 (specs (work-energy energy 8) (work-cap fn))
 (tape (layer 5)
       (in (memory 2))
       (needs (elem-a 125)
	      (elem-b 150)
	      (elem-c 87)
	      (elem-d 37))))

(battery
 (info (tier 1) (type logistics))
 (specs (storage-cap energy 8))
 (tape (layer 5)
       (in (storage 1))
       (needs
	(elem-a 250)
	(elem-b 145)
	(elem-c 175)
	(elem-d 37)
	(elem-e 45)
	(elem-f 20))))

(receive
 (info (tier 1) (type active) (list control))
 (specs (buffer-max enum 1))
 (tape (layer 5)
       (in (memory 2))
       (needs
	(elem-a 200)
	(elem-b 150)
	(elem-c 125)
	(elem-d 45)
	(elem-e 22))))

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
       (needs (elem-a 250)
	      (elem-b 375)
	      (elem-c 200)
	      (elem-d 100))))

(scanner
 (info (tier 0) (type active) (list control))
 (specs (work-energy energy 8) (work-cap fn))
 (tape (layer 6)
       (in (prober 2)
	   (brain 1))
       (needs (elem-a 350)
	      (elem-b 450)
	      (elem-c 275)
	      (elem-d 112))))

(condenser
 (info (tier 1) (type active) (list factory) (config extract))
 (tape (layer 6)
       (in (extract 10))
       (needs (elem-a 250)
	      (elem-b 200)
	      (elem-c 75)
	      (elem-d 20))))

(pill
 (info (tier 1) (type logistics))
 (tape (layer 6)
       (in (storage 1))
       (needs
	(elem-a 175)
	(elem-b 137)
	(elem-c 100)
	(elem-d 25)
	(elem-e 25))))

(transmit
 (info (tier 1) (type active) (list control))
 (specs (launch-energy energy 32)
	(launch-speed u16 500))
 (tape (layer 6)
       (in (receive 1))
       (needs
	(elem-a 300)
	(elem-b 225)
	(elem-c 175)
	(elem-d 62)
	(elem-e 37)
	(elem-f 12))))

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
	(elem-a 440)
	(elem-b 285)
	(elem-c 250)
	(elem-d 80)
	(elem-e 75)
	(elem-f 25))))

(accelerator
 (info (tier 2) (type logistics))
 (tape (layer 6)
       (needs
	(elem-a 500)
	(elem-b 650)
	(elem-c 500)
	(elem-d 250)
	(elem-e 75)
	(elem-f 25))))

(burner
 (info (tier 2) (type active) (list factory))
 (specs (energy fn) (work-cap fn))
 (tape (layer 6)
       (needs
	(elem-a 1600)
	(elem-b 1250)
	(elem-c 550)
	(elem-d 325)
	(elem-e 270)
	(elem-f 60)
	(elem-l 10))))

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
       (needs (elem-a 750)
	      (elem-b 850)
	      (elem-c 650)
	      (elem-d 125))))

(collider
 (info (tier 2) (type active) (list factory))
 (specs (grow-max u8 64)
	(grow-item item !item-accelerator)
	(junk-item item !item-elem-o)
	(output-rate fn))
 (tape (layer 7)
       (in (accelerator 2))
       (needs
	(elem-a 2500)
	(elem-b 2100)
	(elem-c 1850)
	(elem-d 700)
	(elem-e 250)
	(elem-f 125))))

(packer
 (info (tier 2) (type active) (list factory))
 (tape (layer 7)
       (needs
	(elem-a 2000)
	(elem-b 900)
	(elem-c 850)
	(elem-d 250)
	(elem-e 350)
	(elem-f 75)
	(elem-m 7))))

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
	(elem-a 7000)
	(elem-b 3350)
	(elem-c 2650)
	(elem-d 850)
	(elem-e 1200)
	(elem-f 400)
	(elem-m 25))))

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
