;; Goal is to create a spanning tree of world all running (mod os 2)
;; which communicates via antennas that are setup to form a spanning
;; tree.
;;
;; To accomplish this we must setup a basic factory infrastructure
;; that can create legions and antennas. Legion are propagated via the
;; (mod launch 2) module and research to get to antenna is done via
;; the (mod lab 2) module.

(defconst extract-count 2)
(defconst condenser-count 4)
(defconst printer-count 4)
(defconst assembly-count 2)
(defconst worker-count 20)
(defconst lab-count 4)
(defconst active-count 1)
(defconst legion-count 3)
(defconst antenna-count (+ legion-count 1))

(defconst energy-target 1000)
(defconst specs-solar-div 1000)

(defconst scan-id (id &item_scanner 1))
(assert (= (io &io_ping scan-id) &io_ok))

(defconst mem-id (id &item_memory 1))
(assert (= (io &io_ping mem-id) &io_ok))

;; Name
(unless (progn (io &io_get mem-id 0) (head))
  (io &io_name (self) !Bob-The-Homeworld))


;; Elem - Extract
(progn
  (set-tape 1 1 &item_extract &item_elem_a)
  (set-tape 2 1 &item_extract &item_elem_b)
  (set-tape 1 1 &item_printer &item_frame)
  (set-tape 2 1 &item_printer &item_logic)

  (deploy-tape &item_extract &item_elem_b extract-count)
  (set-tape 1 2 &item_extract &item_elem_a)
  (deploy-tape &item_extract &item_elem_c extract-count)
  (deploy-tape &item_extract &item_elem_d extract-count)
  (deploy-tape &item_extract &item_elem_e extract-count)
  (deploy-tape &item_extract &item_elem_f extract-count))


;; Printers - T0
(progn
  (set-tape 1 1 &item_printer &item_logic)
  (set-tape 2 1 &item_printer &item_gear)

  (deploy-tape &item_printer &item_frame (- printer-count 2))
  (deploy-tape &item_printer &item_logic printer-count)
  (deploy-tape &item_printer &item_gear printer-count)
  (deploy-tape &item_printer &item_neuron printer-count)
  (deploy-tape &item_printer &item_bond printer-count)
  (deploy-tape &item_printer &item_magnet printer-count)
  (deploy-tape &item_printer &item_nuclear printer-count)

  (set-tape 1 2 &item_printer &item_frame))


;; Workers
(progn
  (set-tape 1 2 &item_assembly &item_core)
  (deploy-item &item_worker worker-count))


;; Assembly - T0 Passive
(progn
  (set-tape 2 1 &item_assembly &item_robotics)

  (deploy-tape &item_assembly &item_robotics assembly-count)
  (deploy-tape &item_assembly &item_core assembly-count)
  (deploy-tape &item_assembly &item_capacitor assembly-count)
  (deploy-tape &item_assembly &item_matrix assembly-count)
  (deploy-tape &item_assembly &item_magnet_field assembly-count)
  (deploy-tape &item_assembly &item_hull assembly-count)
  (deploy-tape &item_assembly &item_memory assembly-count)
  (deploy-tape &item_assembly &item_worker assembly-count)

  (assert (= (io &io_reset (id &item_assembly 2)) &io_ok)))


;; Labs
(progn
  (deploy-item &item_lab lab-count)
  (deploy-item &item_brain 1)

  (let ((id-brain (id &item_brain (count &item_brain))))
    (assert (= (io &io_mod id-brain (mod lab 2)) &io_ok))
    (assert (= (io &io_send id-brain !lab_count lab-count) &io_ok))))


;; Legions
(progn
  (deploy-item &item_scanner 2)
  (deploy-item &item_brain 1)
  (let ((id-brain (id &item_brain (count &item_brain))))
    (assert (= (io &io_mod id-brain (mod launch 2)) &io_ok)))

  (deploy-tape &item_assembly &item_deploy active-count)
  (deploy-tape &item_assembly &item_extract active-count)
  (deploy-tape &item_assembly &item_printer active-count)
  (deploy-tape &item_assembly &item_assembly active-count)
  (deploy-tape &item_assembly &item_scanner active-count)
  (deploy-tape &item_assembly &item_brain active-count)
  (deploy-item &item_legion legion-count))


;; Energy - Solar
(progn
  (wait-tech &item_captor)
  (deploy-tape &item_printer &item_captor printer-count)

  (wait-tech &item_solar)
  (deploy-tape &item_assembly &item_solar assembly-count)
  (deploy-item &item_solar (+ (/ energy-target (/ (count &item_energy) specs-solar-div)) 1)))


;; Elem - Condenser
(progn
  (wait-tech &item_condenser)
  (deploy-tape &item_condenser &item_elem_g condenser-count)
  (deploy-tape &item_condenser &item_elem_h condenser-count))


;; Antenna
(progn
  (wait-tech &item_liquid_frame)
  (deploy-tape &item_printer &item_liquid_frame printer-count)

  (wait-tech &item_radiation)
  (deploy-tape &item_printer &item_radiation printer-count)

  (wait-tech &item_antenna)
  (deploy-tape &item_assembly &item_antenna assembly-count)

  (wait-tech &item_transmit)
  (deploy-item &item_transmit antenna-count)

  (wait-tech &item_receive)
  (deploy-item &item_receive antenna-count))


;; Finish - T1
(progn
  (wait-tech &item_storage)
  (deploy-tape &item_assembly &item_storage active-count)

  (wait-tech &item_accelerator)
  (deploy-tape &item_assembly &item_accelerator assembly-count)

  (deploy-tape &item_assembly &item_lab active-count))


;; Spanning Tree
(progn
  (while (count &item_legion))

  ;; Launch is responsible for filling (id &item_memory 1) with our
  ;; children stars while the parent is filled in automatically when
  ;; legion is deployed.
  (for (i 0) (< i antenna-count) (+ i 1)
       (let ((coord (progn (io &io_get mem-id i) (head))))
	 (assert (= (io &io_target (id &item_transmit (+ i 1)) coord) &io_ok))
	 (assert (= (io &io_target (id &item_receive (+ i 1)) coord) &io_ok)))))


;; OS
(progn
  ;; This brain is used as the execution thread for os.
  (deploy-item &item_brain 1)
  (deploy-item &item_memory 1)
  (assert (= (io &io_set
		 (id &item_memory (count &item_memory)) 0
		 (id &item_brain (count &item_brain)))
	     &io_ok))

  ;; We're all done so time to switch
  (load (mod os 2)))


;; -----------------------------------------------------------------------------
;; utils
;; -----------------------------------------------------------------------------

(defun set-tape (id n host tape)
  (assert (> id 0))
  (set n (+ id n))
  (while (< id n)
    (assert (= (io &io_tape (id host id) tape) &io_ok))
    (set id (+ id 1))))

(defun deploy-item (item n)
  (assert (= (io &io_tape (id &item_assembly 1) item n) &io_ok))
  (assert (= (io &io_item (id &item_deploy 1) item n) &io_ok))
  (while (progn (assert (= (io &io_status (id &item_deploy 1)) &io_ok))
		(/= (head) 0))))

(defun deploy-tape (host tape n)
  (let ((id (+ (count host) 1)))
    (assert (> id 0))
    (assert (= (io &io_tape (id &item_assembly 1) host n) &io_ok))
    (assert (= (io &io_item (id &item_deploy 1) host n) &io_ok))
    (while (progn (assert (= (io &io_status (id &item_deploy 1)) &io_ok))
		  (/= (head) 0)))
    (set-tape id n host tape)))

(defun count (item)
  (io &io_scan scan-id (progn (io &io_coord (self)) (head)) item)
  (let ((count -1))
    (while (< count 0) (set count (progn (io &io_scan_val scan-id) (head))))
    count))

(defun wait-tech (item)
  (while
      (progn (assert (= (io &io_tape_known (id &item_lab 1) item) &io_ok))
	     (= (head) 0))))
