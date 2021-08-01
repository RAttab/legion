;; Bootstraps a basic factory infrastructure along a few instances of
;; !item_legion_1 that are sent to other stars in the sector via
;; (launch 2).


(boot/extract 2)
(boot/printer 2)
(boot/extract 4)
(boot/printer 4)
(boot/assembler 2)

(boot/launch)
(boot/legion 5)


;; -----------------------------------------------------------------------------
;; extract
;; -----------------------------------------------------------------------------

(defun boot/extract (n)
  (let ((items 6)
	(current (boot/count !item_extract_1)))
    (when (<= current 2)
      (io !io_tape (id !item_extract_1 1) !item_elem_a)
      (io !io_tape (id !item_extract_1 2) !item_elem_f)
      (io !io_tape (id !item_printer_1 1) !item_frame)
      (io !io_tape (id !item_printer_1 2) !item_circuit))

    (io !io_tape (id !item_assembly_1 1) !item_extract_1 (- (* items n) current))
    (io !io_item (id !item_deploy 1) !item_extract_1 (- (* items n) current))
    (boot/wait (id !item_deploy 1)))

  (let ((id 1))
    (set id (boot/extract-set id n !item_elem_a))
    (set id (boot/extract-set id n !item_elem_b))
    (set id (boot/extract-set id n !item_elem_c))
    (set id (boot/extract-set id n !item_elem_d))
    (set id (boot/extract-set id n !item_elem_f))
    (set id (boot/extract-set id n !item_elem_g))))

(defun boot/extract-set (id n item)
  (for (i 0) (< i n) (+ i 1) (io !io_tape (id !item_extract_1 (+ id i)) item))
  (+ id n))


;; -----------------------------------------------------------------------------
;; printer
;; -----------------------------------------------------------------------------

(defun boot/printer (n)
  (let ((items 6)
	(current (boot/count !item_printer_1)))
    (when (<= current 2)
      (io !io_tape (id !item_printer_1 1) !item_circuit)
      (io !io_tape (id !item_printer_1 2) !item_gear))

    (io !io_tape (id !item_assembly_1 1) !item_printer_1 (- (* items n) current))
    (io !io_item (id !item_deploy 1) !item_printer_1 (- (* items n) current))
    (boot/wait (id !item_deploy 1)))

  (let ((id 1))
    (set id (boot/printer-set id n !item_frame))
    (set id (boot/printer-set id n !item_gear))
    (set id (boot/printer-set id n !item_circuit))
    (set id (boot/printer-set id n !item_neural))
    (set id (boot/printer-set id n !item_fuel))
    (set id (boot/printer-set id n !item_bonding))))

(defun boot/printer-set (id n item)
  (for (i 0) (< i n) (+ i 1) (io !io_tape (id !item_printer_1 (+ id i)) item))
  (+ id n))


;; -----------------------------------------------------------------------------
;; assembler
;; -----------------------------------------------------------------------------

(defun boot/assembler (n)
  (let ((items 9)
	(current-asm (- (boot/count !item_assembly_1) 2)) ; we always reserve 2 assemblers for our internal needs
	(current-work (boot/count !item_worker)))
    (when (<= current-asm 2)
      (io !io_tape (id !item_assembly_1 1) !item_assembly_1 3)
      (io !io_tape (id !item_assembly_1 2) !item_servo)
      (io !io_item (id !item_deploy 1) !item_assembly_1 3)
      (boot/wait (id !item_deploy 1))

      (io !io_tape (id !item_assembly_1 3) !item_servo)
      (io !io_tape (id !item_assembly_1 4) !item_thruster)
      (io !io_tape (id !item_assembly_1 5) !item_core))

    (when (< current-asm (* n items))
      (io !io_tape (id !item_assembly_1 1) !item_worker (- (* n 10) current-work))
      (io !io_item (id !item_deploy 1) !item_worker (- (* n 10) current-work))
      (boot/wait (id !item_deploy 1))

      (io !io_tape (id !item_assembly_1 1) !item_assembly_1 (- (* n items) current-asm 3))
      (io !io_item (id !item_deploy 1) !item_assembly_1 (- (* n items) current-asm 3))
      (boot/wait (id !item_deploy 1))))

  (let ((id 3))
    (set id (boot/assembly-set id n !item_servo))
    (set id (boot/assembly-set id n !item_thruster))
    (set id (boot/assembly-set id n !item_propulsion))
    (set id (boot/assembly-set id n !item_plate))
    (set id (boot/assembly-set id n !item_shielding))
    (set id (boot/assembly-set id n !item_hull_1))
    (set id (boot/assembly-set id n !item_core))
    (set id (boot/assembly-set id n !item_matrix))
    (set id (boot/assembly-set id n !item_databank))))

(defun boot/assembly-set (id n item)
  (for (i 0) (< i n) (+ i 1) (io !io_tape (id !item_assembly_1 (+ id i)) item))
  (+ id n))


;; -----------------------------------------------------------------------------
;; launch
;; -----------------------------------------------------------------------------

(defun boot/launch ()
  (io !io_tape (id !item_assembly_1 1) !item_scanner_1 2)
  (io !io_item (id !item_deploy 1) !item_scanner_1 2)
  (boot/wait (id !item_deploy 1))

  (io !io_tape (id !item_assembly_1 1) !item_brain_1 1)
  (io !io_item (id !item_deploy 1) !item_brain_1 1)
  (boot/wait (id !item_deploy 1))

  (assert (= (io !io_mod (id !item_brain_1 2) (mod launch 2)) !io_ok)))


;; -----------------------------------------------------------------------------
;; legion
;; -----------------------------------------------------------------------------

(defun boot/legion (n)
  (let ((items 8)
	(id (+ (boot/count !item_assembly_1) 1)))

    (io !io_tape (id !item_assembly_1 1) !item_assembly_1 items)
    (io !io_item (id !item_deploy 1) !item_assembly_1 items)
    (boot/wait (id !item_deploy 1))

    (set id (boot/assembly-set id 1 !item_deploy))
    (set id (boot/assembly-set id 1 !item_extract_1))
    (set id (boot/assembly-set id 1 !item_printer_1))
    (set id (boot/assembly-set id 1 !item_assembly_1))
    (set id (boot/assembly-set id 1 !item_scanner_1))
    (set id (boot/assembly-set id 1 !item_db_1))
    (set id (boot/assembly-set id 1 !item_brain_1))
    (set id (boot/assembly-set id 1 !item_worker)))

  (io !io_tape (id !item_assembly_1 1) !item_legion_1 n)
  (io !io_item (id !item_deploy 1) !item_legion_1 n)
  (boot/wait (id !item_deploy 1)))


;; -----------------------------------------------------------------------------
;; utils
;; -----------------------------------------------------------------------------

(defun boot/wait (id)
  (while (if (= (io !io_status id) !io_ok) (/= (head) 0))
    (yield)))

(defun boot/coord ()
  (assert (= (io !io_coord 0) !io_ok))
  (head))

(defun boot/count (item)
  (let ((coord (boot/coord)))
    (io !io_scan (id !item_scanner_1 1) coord item)
    (io !io_scan_val (id !item_scanner_1 1))
    (head)))
