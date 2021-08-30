;; Goal is to create a spanning tree of world all running (mod os 2)
;; which communicates via antennas that are setup to form a spanning
;; tree.
;;
;; To accomplish this we must setup a basic factory infrastructure
;; that can create legions and antennas. Legion are propagated via the
;; (mod launch 2) module and research to get to antenna is done via
;; the (mod lab 2) module.


;; Elem - Extract
(let ((n 2))
  (set-tape 1 1 &item_extract &item_elem_a)
  (set-tape 2 1 &item_extract &item_elem_b)
  (set-tape 1 1 &item_printer &item_frame)
  (set-tape 2 1 &item_printer &item_logic)

  (deploy-tape &item_extract &item_elem_b n)
  (set-tape 1 2 &item_extract &item_elem_a)
  (deploy-tape &item_extract &item_elem_c n)
  (deploy-tape &item_extract &item_elem_d n)
  (deploy-tape &item_extract &item_elem_e n)
  (deploy-tape &item_extract &item_elem_f n))


;; Printers - T0
(let ((n 4))
  (set-tape 1 1 &item_printer &item_logic)
  (set-tape 2 1 &item_printer &item_gear)

  (deploy-tape &item_printer &item_frame (- n 2))
  (deploy-tape &item_printer &item_logic n)
  (deploy-tape &item_printer &item_gear n)
  (deploy-tape &item_printer &item_neuron n)
  (deploy-tape &item_printer &item_bond n)
  (deploy-tape &item_printer &item_magnet n)
  (deploy-tape &item_printer &item_nuclear n)

  (set-tape 1 2 &item_printer &item_frame))


;; Workers
(progn
  (set-tape 1 2 &item_assembly &item_core)
  (deploy-item &item_worker 20))


;; Assembly - T0 Passive
(let ((n 2))
  (set-tape 2 1 &item_assembly &item_robotics)

  (deploy-tape &item_assembly &item_robotics n)
  (deploy-tape &item_assembly &item_core n)
  (deploy-tape &item_assembly &item_capacitor n)
  (deploy-tape &item_assembly &item_matrix n)
  (deploy-tape &item_assembly &item_magnet_field n)
  (deploy-tape &item_assembly &item_hull n)
  ;; Not passive but requirements in other recipies
  (deploy-tape &item_assembly &item_memory n)
  (deploy-tape &item_assembly &item_worker n)

  (assert (= (io &io_reset (id &item_assembly 2)) &io_ok)))


;; Research
(let ((n 4))
  (deploy-item &item_lab n)
  (deploy-item &item_brain 1)

  (let ((id-brain (id &item_brain (count &item_brain))))
    (assert (= (io &io_mod id-brain (mod lab 2)) &io_ok))
    (assert (= (io &io_send id-brain !lab_count n) &io_ok))))


;; Legion
(let ((n 2))
  (deploy-item &item_scanner 2)
  (deploy-item &item_brain 1)
  (let ((id-brain (id &item_brain (count &item_brain))))
    (assert (= (io &io_mod id-brain (mod launch 2)) &io_ok)))

  (deploy-tape &item_assembly &item_deploy 1)
  (deploy-tape &item_assembly &item_extract 1)
  (deploy-tape &item_assembly &item_printer 1)
  (deploy-tape &item_assembly &item_assembly 1)
  (deploy-tape &item_assembly &item_scanner 1)
  (deploy-tape &item_assembly &item_brain 1)
  (deploy-item &item_legion n))


;; Energy - Solar
(let ((target 1000))
  (wait-tech &item_captor)
  (deploy-tape &item_printer &item_captor 4)

  (wait-tech &item_solar)
  (deploy-tape &item_assembly &item_solar 2)
  (deploy-item &item_solar (/ target (/ (count &item_energy) 100))))


;; Elem - Condenser
(let ((n 4))

  (wait-tech &item_condenser)
  (deploy-tape &item_condenser &item_elem_g n)
  (deploy-tape &item_condenser &item_elem_h n)
  (deploy-tape &item_condenser &item_elem_i n)
  (deploy-tape &item_condenser &item_elem_j n))


;; Antenna
(let ((n 2)) ;; must match legion's count
  (wait-tech &item_liquid_frame)
  (deploy-tape &item_printer &item_liquid_frame 4)

  (wait-tech &item_radiation)
  (deploy-tape &item_printer &item_radiation 4)

  (wait-tech &item_antenna)
  (deploy-tape &item_assembly &item_antenna 2)

  (wait-tech &item_transmit)
  (deploy-item &item_transmit (+ n 1))

  (wait-tech &item_receive)
  (deploy-item &item_receive (+ n 1)))


;; Finish - T1
(progn
  (wait-tech &item_storage)
  (deploy-tape &item_assembly &item_storage 1)

  (wait-tech &item_accelerator)
  (deploy-tape &item_assembly &item_accelerator 2)

  (deploy-tape &item_assembly &item_lab 1))


;; Spanning Tree
(let ((n 2)) ;; must match antenna's count
  (while (count &item_legion))

  ;; Launch is responsible for filling (id &item_memory 1) with our
  ;; children stars while the parent is filled in automatically when
  ;; legion is deployed.
  (for (i 0) (< i (+ n 1)) (+ i 1)
       (let ((coord (progn (io &io_get (id &item_memory 1) i) (head))))
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
    (assert (= (io &io_tape (id &item_assembly 1) host n) &io_ok))
    (assert (= (io &io_item (id &item_deploy 1) host n) &io_ok))
    (while (progn (assert (= (io &io_status (id &item_deploy 1)) &io_ok))
		  (/= (head) 0)))
    (set-tape id n host tape)))

(defun count (item)
  (let ((coord (progn (assert (= (io &io_coord 0) &io_ok)) (head))))
    (assert (= (io &io_scan (id &item_scanner 1) coord item) &io_ok))
    (assert (= (io &io_scan_val (id &item_scanner 1)) &io_ok))
    (head)))

(defun wait-tech (item)
  (while
      (progn (assert (= (io &io_tape_known (id &item_lab 1) item) &io_ok))
	     (= (head) 0))))
