;; Bootstraps a basic factory along a few instances of `&item_legion`
;; that are sent to other stars in the sector via `(mod launch 2)`

(build-extract 2)
(build-printer 2)
(build-extract 4)
(build-printer 4)
(build-assembly 2)

(launch-legion)
(build-legion 3)


;; -----------------------------------------------------------------------------
;; extract
;; -----------------------------------------------------------------------------

(defun build-extract (n)
  (let ((items 6)
	(current (count &item_extract)))
    (when (<= current 2)
      (io &io_tape (id &item_extract 1) &item_elem_a)
      (io &io_tape (id &item_extract 2) &item_elem_b)
      (io &io_tape (id &item_printer 1) &item_frame)
      (io &io_tape (id &item_printer 2) &item_logic))

    (io &io_tape (id &item_assembly 1) &item_extract (- (* items n) current))
    (io &io_item (id &item_deploy 1) &item_extract (- (* items n) current))
    (wait (id &item_deploy 1)))

  (let ((id 1))
    (set id (set-extract id n &item_elem_a))
    (set id (set-extract id n &item_elem_b))
    (set id (set-extract id n &item_elem_c))
    (set id (set-extract id n &item_elem_d))
    (set id (set-extract id n &item_elem_e))
    (set id (set-extract id n &item_elem_f))))

(defun set-extract (id n item)
  (for (i 0) (< i n) (+ i 1) (io &io_tape (id &item_extract (+ id i)) item))
  (+ id n))


;; -----------------------------------------------------------------------------
;; printer
;; -----------------------------------------------------------------------------

(defun build-printer (n)
  (let ((items 7)
	(current (count &item_printer)))
    (when (<= current 2)
      (io &io_tape (id &item_printer 1) &item_logic)
      (io &io_tape (id &item_printer 2) &item_gear))

    (io &io_tape (id &item_assembly 1) &item_printer (- (* items n) current))
    (io &io_item (id &item_deploy 1) &item_printer (- (* items n) current))
    (wait (id &item_deploy 1)))

  (let ((id 1))
    (set id (set-printer id n &item_frame))
    (set id (set-printer id n &item_logic))
    (set id (set-printer id n &item_gear))
    (set id (set-printer id n &item_neuron))
    (set id (set-printer id n &item_bond))
    (set id (set-printer id n &item_magnet))
    (set id (set-printer id n &item_nuclear))))

(defun set-printer (id n item)
  (for (i 0) (< i n) (+ i 1) (io &io_tape (id &item_printer (+ id i)) item))
  (+ id n))


;; -----------------------------------------------------------------------------
;; assembler
;; -----------------------------------------------------------------------------

(defun build-assembly (n)
  (let ((items 7)
	(current-asm (- (count &item_assembly) 2)) ; we always reserve 2 assemblies for our internal needs
	(current-work (count &item_worker)))

    (when (< current-asm (* n items))
      (io &io_tape (id &item_assembly 2) &item_core)
      (io &io_tape (id &item_assembly 1) &item_worker (- (* n 10) current-work))
      (io &io_item (id &item_deploy 1) &item_worker (- (* n 10) current-work))
      (wait (id &item_deploy 1))

      (io &io_tape (id &item_assembly 2) &item_robotics)
      (io &io_tape (id &item_assembly 1) &item_assembly (- (* n items) current-asm))
      (io &io_item (id &item_deploy 1) &item_assembly (- (* n items) current-asm))
      (wait (id &item_deploy 1))))

  (let ((id 3))
    (set id (set-assembly id n &item_robotics))
    (set id (set-assembly id n &item_core))
    (set id (set-assembly id n &item_capacitor))
    (set id (set-assembly id n &item_matrix))
    (set id (set-assembly id n &item_magnet_field))
    (set id (set-assembly id n &item_hull))
    (set id (set-assembly id n &item_memory))))

(defun set-assembly (id n item)
  (for (i 0) (< i n) (+ i 1) (io &io_tape (id &item_assembly (+ id i)) item))
  (+ id n))


;; -----------------------------------------------------------------------------
;; launch
;; -----------------------------------------------------------------------------

(defun launch-legion ()
  (io &io_tape (id &item_assembly 1) &item_scanner 2)
  (io &io_item (id &item_deploy 1) &item_scanner 2)
  (wait (id &item_deploy 1))

  (io &io_tape (id &item_assembly 1) &item_brain 1)
  (io &io_item (id &item_deploy 1) &item_brain 1)
  (wait (id &item_deploy 1))

  (assert (= (io &io_mod (id &item_brain 2) (mod launch 2)) &io_ok)))


;; -----------------------------------------------------------------------------
;; legion
;; -----------------------------------------------------------------------------

(defun build-legion (n)
  (let ((items 7)
	(id (+ (count &item_assembly) 1)))

    (io &io_tape (id &item_assembly 1) &item_assembly items)
    (io &io_item (id &item_deploy 1) &item_assembly items)
    (wait (id &item_deploy 1))

    (set id (set-assembly id 1 &item_deploy))
    (set id (set-assembly id 1 &item_extract))
    (set id (set-assembly id 1 &item_printer))
    (set id (set-assembly id 1 &item_assembly))
    (set id (set-assembly id 1 &item_scanner))
    (set id (set-assembly id 1 &item_brain))
    (set id (set-assembly id 1 &item_worker)))

  (io &io_tape (id &item_assembly 1) &item_legion n)
  (io &io_item (id &item_deploy 1) &item_legion n)
  (wait (id &item_deploy 1)))


;; -----------------------------------------------------------------------------
;; utils
;; -----------------------------------------------------------------------------

(defun wait (id)
  (while (if (= (io &io_status id) &io_ok) (/= (head) 0))
    (yield)))

(defun coord ()
  (assert (= (io &io_coord 0) &io_ok))
  (head))

(defun count (item)
  (let ((coord (coord)))
    (io &io_scan (id &item_scanner 1) coord item)
    (io &io_scan_val (id &item_scanner 1))
    (head)))
