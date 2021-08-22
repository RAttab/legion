;; Bootstraps a basic factory along a few instances of `&item_legion`
;; that are sent to other stars in the sector via `(mod launch 2)`


(boot/extract 2)
(boot/printer 2)
(boot/extract 4)
(boot/printer 4)
(boot/assembler 2)

(boot/launch)
(boot/legion 3)


;; -----------------------------------------------------------------------------
;; extract
;; -----------------------------------------------------------------------------

(defun boot/extract (n)
  (let ((items 6)
	(current (boot/count &item_extract)))
    (when (<= current 2)
      (io &io_tape (id &item_extract 1) &item_elem_a)
      (io &io_tape (id &item_extract 2) &item_elem_b)
      (io &io_tape (id &item_printer 1) &item_frame)
      (io &io_tape (id &item_printer 2) &item_logic))

    (io &io_tape (id &item_assembly 1) &item_extract (- (* items n) current))
    (io &io_item (id &item_deploy 1) &item_extract (- (* items n) current))
    (boot/wait (id &item_deploy 1)))

  (let ((id 1))
    (set id (boot/extract-set id n &item_elem_a))
    (set id (boot/extract-set id n &item_elem_b))
    (set id (boot/extract-set id n &item_elem_c))
    (set id (boot/extract-set id n &item_elem_d))
    (set id (boot/extract-set id n &item_elem_e))
    (set id (boot/extract-set id n &item_elem_f))))

(defun boot/extract-set (id n item)
  (for (i 0) (< i n) (+ i 1) (io &io_tape (id &item_extract (+ id i)) item))
  (+ id n))


;; -----------------------------------------------------------------------------
;; printer
;; -----------------------------------------------------------------------------

(defun boot/printer (n)
  (let ((items 7)
	(current (boot/count &item_printer)))
    (when (<= current 2)
      (io &io_tape (id &item_printer 1) &item_logic)
      (io &io_tape (id &item_printer 2) &item_gear))

    (io &io_tape (id &item_assembly 1) &item_printer (- (* items n) current))
    (io &io_item (id &item_deploy 1) &item_printer (- (* items n) current))
    (boot/wait (id &item_deploy 1)))

  (let ((id 1))
    (set id (boot/printer-set id n &item_frame))
    (set id (boot/printer-set id n &item_logic))
    (set id (boot/printer-set id n &item_gear))
    (set id (boot/printer-set id n &item_neuron))
    (set id (boot/printer-set id n &item_bond))
    (set id (boot/printer-set id n &item_magnet))
    (set id (boot/printer-set id n &item_nuclear))))

(defun boot/printer-set (id n item)
  (for (i 0) (< i n) (+ i 1) (io &io_tape (id &item_printer (+ id i)) item))
  (+ id n))


;; -----------------------------------------------------------------------------
;; assembler
;; -----------------------------------------------------------------------------

(defun boot/assembler (n)
  (let ((items 7)
	(current-asm (- (boot/count &item_assembly) 2)) ; we always reserve 2 assemblies for our internal needs
	(current-work (boot/count &item_worker)))

    (when (< current-asm (* n items))
      (io &io_tape (id &item_assembly 2) &item_core)
      (io &io_tape (id &item_assembly 1) &item_worker (- (* n 10) current-work))
      (io &io_item (id &item_deploy 1) &item_worker (- (* n 10) current-work))
      (boot/wait (id &item_deploy 1))

      (io &io_tape (id &item_assembly 2) &item_robotics)
      (io &io_tape (id &item_assembly 1) &item_assembly (- (* n items) current-asm))
      (io &io_item (id &item_deploy 1) &item_assembly (- (* n items) current-asm))
      (boot/wait (id &item_deploy 1))))

  (let ((id 3))
    (set id (boot/assembly-set id n &item_robotics))
    (set id (boot/assembly-set id n &item_core))
    (set id (boot/assembly-set id n &item_capacitor))
    (set id (boot/assembly-set id n &item_matrix))
    (set id (boot/assembly-set id n &item_magnet_field))
    (set id (boot/assembly-set id n &item_hull))
    (set id (boot/assembly-set id n &item_memory))))

(defun boot/assembly-set (id n item)
  (for (i 0) (< i n) (+ i 1) (io &io_tape (id &item_assembly (+ id i)) item))
  (+ id n))


;; -----------------------------------------------------------------------------
;; launch
;; -----------------------------------------------------------------------------

(defun boot/launch ()
  (io &io_tape (id &item_assembly 1) &item_scanner 2)
  (io &io_item (id &item_deploy 1) &item_scanner 2)
  (boot/wait (id &item_deploy 1))

  (io &io_tape (id &item_assembly 1) &item_brain 1)
  (io &io_item (id &item_deploy 1) &item_brain 1)
  (boot/wait (id &item_deploy 1))

  (assert (= (io &io_mod (id &item_brain 2) (mod launch 2)) &io_ok)))


;; -----------------------------------------------------------------------------
;; legion
;; -----------------------------------------------------------------------------

(defun boot/legion (n)
  (let ((items 7)
	(id (+ (boot/count &item_assembly) 1)))

    (io &io_tape (id &item_assembly 1) &item_assembly items)
    (io &io_item (id &item_deploy 1) &item_assembly items)
    (boot/wait (id &item_deploy 1))

    (set id (boot/assembly-set id 1 &item_deploy))
    (set id (boot/assembly-set id 1 &item_extract))
    (set id (boot/assembly-set id 1 &item_printer))
    (set id (boot/assembly-set id 1 &item_assembly))
    (set id (boot/assembly-set id 1 &item_scanner))
    (set id (boot/assembly-set id 1 &item_brain))
    (set id (boot/assembly-set id 1 &item_worker)))

  (io &io_tape (id &item_assembly 1) &item_legion n)
  (io &io_item (id &item_deploy 1) &item_legion n)
  (boot/wait (id &item_deploy 1)))


;; -----------------------------------------------------------------------------
;; utils
;; -----------------------------------------------------------------------------

(defun boot/wait (id)
  (while (if (= (io &io_status id) &io_ok) (/= (head) 0))
    (yield)))

(defun boot/coord ()
  (assert (= (io &io_coord 0) &io_ok))
  (head))

(defun boot/count (item)
  (let ((coord (boot/coord)))
    (io &io_scan (id &item_scanner 1) coord item)
    (io &io_scan_val (id &item_scanner 1))
    (head)))
