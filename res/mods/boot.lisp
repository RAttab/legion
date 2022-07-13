;; Goal is to create a spanning tree of world all running (mod os 2)
;; which communicates via antennas that are setup to form a spanning
;; tree.
;;
;; To accomplish this we must setup a basic factory infrastructure
;; that can create legions and antennas. Legion are propagated via the
;; (mod launch 2) module and research to get to antenna is done via
;; the (mod lab 2) module.

(defconst max-depth 1)

(defconst extract-count 2)
(defconst condenser-count 2)
(defconst printer-count 4)
(defconst assembly-count 2)
(defconst worker-count 20)
(defconst lab-count 4)
(defconst active-count 1)
(defconst storage-count 4)

(defconst energy-target 100)
(defconst specs-solar-div 1000)

;; Sanity checks
(defconst prober-id (id &item-prober 1))
(assert (= (io &io-ping prober-id) &io-ok))
(defconst deploy-id (id &item-deploy 1))
(assert (= (io &io-ping deploy-id) &io-ok))


;; Home
(progn
  (when (> (count &item-brain) 1) (reset))
  (when (call (os is-home)) (io &io-name (self) !Bob-The-Homeworld))
  (io &io-log (self) !booting (ior &io-coord (self))))


;; Elem - Extract
(progn
  (set-tape 1 1 &item-extract &item-elem-a)
  (set-tape 2 1 &item-extract &item-elem-b)
  (set-tape 1 1 &item-printer &item-muscle)
  (set-tape 2 1 &item-printer &item-nodule)

  (deploy-tape &item-extract &item-elem-b extract-count)
  (set-tape 1 2 &item-extract &item-elem-a)
  (deploy-tape &item-extract &item-elem-c extract-count)
  (deploy-tape &item-extract &item-elem-d extract-count))


;; Printers - T0
(progn
  (set-tape 1 1 &item-printer &item-nodule)
  (set-tape 2 1 &item-printer &item-vein)

  (deploy-tape &item-printer &item-muscle (- printer-count 2))
  (deploy-tape &item-printer &item-nodule printer-count)
  (deploy-tape &item-printer &item-vein printer-count)
  (deploy-tape &item-printer &item-bone printer-count)
  (deploy-tape &item-printer &item-tendon printer-count)
  (deploy-tape &item-printer &item-lens printer-count)
  (deploy-tape &item-printer &item-nerve printer-count)
  (deploy-tape &item-printer &item-neuron printer-count)
  (deploy-tape &item-printer &item-retina printer-count)

  (set-tape 1 2 &item-printer &item-muscle))


;; Workers
(progn
  (set-tape 2 1 &item-assembly &item-limb)

  (deploy-tape &item-assembly &item-limb assembly-count)
  (deploy-tape &item-assembly &item-stem assembly-count)
  (deploy-tape &item-assembly &item-lung assembly-count)

  (deploy-item &item-worker worker-count))


;; Assembly - T0 Passive
(progn
  (deploy-tape &item-assembly &item-spinal assembly-count)
  (deploy-tape &item-assembly &item-engram assembly-count)
  (deploy-tape &item-assembly &item-cortex assembly-count)
  (deploy-tape &item-assembly &item-eye assembly-count)

  ;; required for building brain
  (deploy-tape &item-assembly &item-memory assembly-count)

  (assert (= (io &io-reset (id &item-assembly 2)) &io-ok)))


;; OS - we reserve the juicy brain ids for os
(defconst brain-os-id (id &item-brain 2))
(defconst brain-exec-id (id &item-brain 3))
(progn
  (deploy-item &item-brain 2)
  (assert (= (io &io-ping brain-os-id) &io-ok))
  (assert (= (io &io-ping brain-exec-id) &io-ok)))


;; Labs
(progn
  (deploy-item &item-lab lab-count)
  (deploy-item &item-brain 1)

  (let ((id-brain (id &item-brain (count &item-brain))))
    (assert (= (io &io-mod id-brain (mod lab 2)) &io-ok))
    (assert (= (io &io-send id-brain !lab-count lab-count) &io-ok))))


;; Energy - Solar
(progn
  (wait-tech &item-semiconductor)
  (deploy-tape &item-printer &item-semiconductor printer-count)

  (wait-tech &item-photovoltaic)
  (deploy-tape &item-assembly &item-photovoltaic printer-count)

  (wait-tech &item-solar)
  (deploy-tape &item-assembly &item-solar assembly-count)
  (deploy-item &item-solar (+ (/ energy-target (/ (count &item-energy) specs-solar-div)) 1)))


;; Elem - Condenser
(progn
  (wait-tech &item-condenser)
  (deploy-tape &item-condenser &item-elem-g condenser-count)
  (deploy-tape &item-condenser &item-elem-h condenser-count))

;; OS - data network and boot
(progn
  ;; Antenna's for parent
  (wait-tech &item-conductor)
  (deploy-tape &item-printer &item-conductor printer-count)
  (wait-tech &item-antenna)
  (deploy-tape &item-assembly &item-antenna assembly-count)
  (wait-tech &item-transmit)
  (deploy-item &item-transmit 1)
  (wait-tech &item-receive)
  (deploy-item &item-receive 1)

  ;; This brain is used as the execution thread for os.
  (deploy-item &item-memory (call (os memory-need)))
  (call (os boot) brain-os-id brain-exec-id))


;; Legions
(when (<= (call (os depth)) max-depth)

  (wait-tech &item-scanner)
  (deploy-item &item-scanner 1)
  (deploy-item &item-prober 1)
  (deploy-item &item-brain 1)

  (assert (= (count &item-scanner) 1))
  (assert (= (count &item-prober) 2))
  (let ((brain-id (id &item-brain (count &item-brain))))
    (io &io-mod brain-id (mod launch 2)) &io-ok)

  (deploy-tape &item-assembly &item-worker active-count)
  (deploy-tape &item-assembly &item-extract active-count)
  (deploy-tape &item-assembly &item-printer active-count)
  (deploy-tape &item-assembly &item-assembly active-count)
  (deploy-tape &item-assembly &item-deploy active-count)
  (deploy-tape &item-assembly &item-brain active-count)
  (deploy-tape &item-assembly &item-prober active-count)
  (deploy-tape &item-assembly &item-scanner active-count)

  (let ((n (call (os child-cap))))
    (deploy-item &item-transmit n)
    (deploy-item &item-receive n)
    (deploy-item &item-legion n)))



;; Pill logistics
;; Requires the spanning tree to figure out where home is.
(progn
  (deploy-tape &item-printer &item-magnet printer-count)
  (deploy-tape &item-printer &item-ferrofluid printer-count)

  (wait-tech &item-storage)
  (deploy-tape &item-assembly &item-storage active-count)

  (wait-tech &item-field)
  (deploy-tape &item-assembly &item-field assembly-count)

  (wait-tech &item-port)
  (deploy-item &item-port 2)

  (when (call (os is-home))
    (deploy-item &item-storage (* 11 storage-count))
    (for (i 0) (< i 11) (+ i 1)
	 (set-item (+ (* i storage-count) 1)
		   storage-count
		   &item-storage
		   (+ &item-elem-a i))))

  (deploy-item &item-brain 1)
  (let ((brain-id (id &item-brain (count &item-brain))))
    (assert (= (io &io-mod brain-id (mod port 2)) &io-ok)))

  ;; We build it after to give (mod port) a chance to boot up and set
  ;; up the ports.
  (wait-tech &item-pill)
  (deploy-item &item-pill 10))

(io &io-log (self) !done (ior &io-coord (self)))


;; -----------------------------------------------------------------------------
;; utils
;; -----------------------------------------------------------------------------

(defun set-tape (id n host tape)
  (assert (> id 0))
  (set n (+ id n))
  (while (< id n)
    (assert (= (io &io-tape (id host id) tape) &io-ok))
    (set id (+ id 1))))

(defun set-item (id n host item)
  (assert (> id 0))
  (set n (+ id n))
  (while (< id n)
    (assert (= (io &io-item (id host id) item) &io-ok))
    (set id (+ id 1))))

(defun deploy-item (item n)
  (assert (= (io &io-tape (id &item-assembly 1) item n) &io-ok))
  (assert (= (io &io-item (id &item-deploy 1) item n) &io-ok))
  (while (/= (ior &io-state deploy-id &io-item) 0)))

(defun deploy-tape (host tape n)
  (let ((id (+ (count host) 1)))
    (assert (> id 0))
    (assert (= (io &io-tape (id &item-assembly 1) host n) &io-ok))
    (assert (= (io &io-item (id &item-deploy 1) host n) &io-ok))
    (while (/= (ior &io-state deploy-id &io-item) 0))
    (set-tape id n host tape)))

(defun count (item)
  (io &io-probe prober-id item (ior &io-coord (self)))
  (let ((count -1))
    (while (< count 0) (set count (ior &io-value prober-id)))
    count))

(defun wait-tech (item)
  (assert (= (io &io-ping (id &item-lab 1)) &io-ok))
  (while (= (ior &io-tape-known (id &item-lab 1) item) 0)))
