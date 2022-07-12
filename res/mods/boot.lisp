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
(defconst prober-id (id &item_prober 1))
(assert (= (io &io_ping prober-id) &io_ok))
(defconst deploy-id (id &item_deploy 1))
(assert (= (io &io_ping deploy-id) &io_ok))


;; Home
(progn
  (when (> (count &item_brain) 1) (reset))
  (when (call (os is-home)) (io &io_name (self) !Bob-The-Homeworld))
  (io &io_log (self) !booting (ior &io_coord (self))))


;; Elem - Extract
(progn
  (set-tape 1 1 &item_extract &item_elem_a)
  (set-tape 2 1 &item_extract &item_elem_b)
  (set-tape 1 1 &item_printer &item_muscle)
  (set-tape 2 1 &item_printer &item_nodule)

  (deploy-tape &item_extract &item_elem_b extract-count)
  (set-tape 1 2 &item_extract &item_elem_a)
  (deploy-tape &item_extract &item_elem_c extract-count)
  (deploy-tape &item_extract &item_elem_d extract-count))


;; Printers - T0
(progn
  (set-tape 1 1 &item_printer &item_nodule)
  (set-tape 2 1 &item_printer &item_vein)

  (deploy-tape &item_printer &item_muscle (- printer-count 2))
  (deploy-tape &item_printer &item_nodule printer-count)
  (deploy-tape &item_printer &item_vein printer-count)
  (deploy-tape &item_printer &item_bone printer-count)
  (deploy-tape &item_printer &item_tendon printer-count)
  (deploy-tape &item_printer &item_lens printer-count)
  (deploy-tape &item_printer &item_nerve printer-count)
  (deploy-tape &item_printer &item_neuron printer-count)
  (deploy-tape &item_printer &item_retina printer-count)

  (set-tape 1 2 &item_printer &item_muscle))


;; Workers
(progn
  (set-tape 2 1 &item_assembly &item_limb)

  (deploy-tape &item_assembly &item_limb assembly-count)
  (deploy-tape &item_assembly &item_stem assembly-count)
  (deploy-tape &item_assembly &item_lung assembly-count)

  (deploy-item &item_worker worker-count))


;; Assembly - T0 Passive
(progn
  (deploy-tape &item_assembly &item_spinal assembly-count)
  (deploy-tape &item_assembly &item_engram assembly-count)
  (deploy-tape &item_assembly &item_cortex assembly-count)
  (deploy-tape &item_assembly &item_eye assembly-count)

  ;; required for building brain
  (deploy-tape &item_assembly &item_memory assembly-count)

  (assert (= (io &io_reset (id &item_assembly 2)) &io_ok)))


;; OS - we reserve the juicy brain ids for os
(defconst brain-os-id (id &item_brain 2))
(defconst brain-exec-id (id &item_brain 3))
(progn
  (deploy-item &item_brain 2)
  (assert (= (io &io_ping brain-os-id) &io_ok))
  (assert (= (io &io_ping brain-exec-id) &io_ok)))


;; Labs
(progn
  (deploy-item &item_lab lab-count)
  (deploy-item &item_brain 1)

  (let ((id-brain (id &item_brain (count &item_brain))))
    (assert (= (io &io_mod id-brain (mod lab 2)) &io_ok))
    (assert (= (io &io_send id-brain !lab_count lab-count) &io_ok))))


;; Energy - Solar
(progn
  (wait-tech &item_semiconductor)
  (deploy-tape &item_printer &item_semiconductor printer-count)

  (wait-tech &item_photovoltaic)
  (deploy-tape &item_assembly &item_photovoltaic printer-count)

  (wait-tech &item_solar)
  (deploy-tape &item_assembly &item_solar assembly-count)
  (deploy-item &item_solar (+ (/ energy-target (/ (count &item_energy) specs-solar-div)) 1)))


;; Elem - Condenser
(progn
  (wait-tech &item_condenser)
  (deploy-tape &item_condenser &item_elem_g condenser-count)
  (deploy-tape &item_condenser &item_elem_h condenser-count))

;; OS - data network and boot
(progn
  ;; Antenna's for parent
  (wait-tech &item_conductor)
  (deploy-tape &item_printer &item_conductor printer-count)
  (wait-tech &item_antenna)
  (deploy-tape &item_assembly &item_antenna assembly-count)
  (wait-tech &item_transmit)
  (deploy-item &item_transmit 1)
  (wait-tech &item_receive)
  (deploy-item &item_receive 1)

  ;; This brain is used as the execution thread for os.
  (deploy-item &item_memory (call (os memory-need)))
  (call (os boot) brain-os-id brain-exec-id))


;; Legions
(when (<= (call (os depth)) max-depth)

  (wait-tech &item_scanner)
  (deploy-item &item_scanner 1)
  (deploy-item &item_prober 1)
  (deploy-item &item_brain 1)

  (assert (= (count &item_scanner) 1))
  (assert (= (count &item_prober) 2))
  (let ((brain-id (id &item_brain (count &item_brain))))
    (io &io_mod brain-id (mod launch 2)) &io_ok)

  (deploy-tape &item_assembly &item_worker active-count)
  (deploy-tape &item_assembly &item_extract active-count)
  (deploy-tape &item_assembly &item_printer active-count)
  (deploy-tape &item_assembly &item_assembly active-count)
  (deploy-tape &item_assembly &item_deploy active-count)
  (deploy-tape &item_assembly &item_brain active-count)
  (deploy-tape &item_assembly &item_prober active-count)
  (deploy-tape &item_assembly &item_scanner active-count)

  (let ((n (call (os child-cap))))
    (deploy-item &item_transmit n)
    (deploy-item &item_receive n)
    (deploy-item &item_legion n)))



;; Pill logistics
;; Requires the spanning tree to figure out where home is.
(progn
  (deploy-tape &item_printer &item_magnet printer-count)
  (deploy-tape &item_printer &item_ferrofluid printer-count)

  (wait-tech &item_storage)
  (deploy-tape &item_assembly &item_storage active-count)

  (wait-tech &item_field)
  (deploy-tape &item_assembly &item_field assembly-count)

  (wait-tech &item_port)
  (deploy-item &item_port 2)

  (when (call (os is-home))
    (deploy-item &item_storage (* 11 storage-count))
    (for (i 0) (< i 11) (+ i 1)
	 (set-item (+ (* i storage-count) 1)
		   storage-count
		   &item_storage
		   (+ &item_elem_a i))))

  (deploy-item &item_brain 1)
  (let ((brain-id (id &item_brain (count &item_brain))))
    (assert (= (io &io_mod brain-id (mod port 2)) &io_ok)))

  ;; We build it after to give (mod port) a chance to boot up and set
  ;; up the ports.
  (wait-tech &item_pill)
  (deploy-item &item_pill 10))

(io &io_log (self) !done (ior &io_coord (self)))


;; -----------------------------------------------------------------------------
;; utils
;; -----------------------------------------------------------------------------

(defun set-tape (id n host tape)
  (assert (> id 0))
  (set n (+ id n))
  (while (< id n)
    (assert (= (io &io_tape (id host id) tape) &io_ok))
    (set id (+ id 1))))

(defun set-item (id n host item)
  (assert (> id 0))
  (set n (+ id n))
  (while (< id n)
    (assert (= (io &io_item (id host id) item) &io_ok))
    (set id (+ id 1))))

(defun deploy-item (item n)
  (assert (= (io &io_tape (id &item_assembly 1) item n) &io_ok))
  (assert (= (io &io_item (id &item_deploy 1) item n) &io_ok))
  (while (/= (ior &io_state deploy-id &io_item) 0)))

(defun deploy-tape (host tape n)
  (let ((id (+ (count host) 1)))
    (assert (> id 0))
    (assert (= (io &io_tape (id &item_assembly 1) host n) &io_ok))
    (assert (= (io &io_item (id &item_deploy 1) host n) &io_ok))
    (while (/= (ior &io_state deploy-id &io_item) 0))
    (set-tape id n host tape)))

(defun count (item)
  (io &io_probe prober-id item (ior &io_coord (self)))
  (let ((count -1))
    (while (< count 0) (set count (ior &io_value prober-id)))
    count))

(defun wait-tech (item)
  (assert (= (io &io_ping (id &item_lab 1)) &io_ok))
  (while (= (ior &io_tape_known (id &item_lab 1) item) 0)))
