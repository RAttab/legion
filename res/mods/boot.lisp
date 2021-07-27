(boot/extract 2)
(boot/printer 2)
(boot/assembler 1)

(boot/extract 4)
(boot/printer 4)
(boot/assembler 2)


(defun boot/extract (n)
  (let ((current  (boot/count !item_extract_1)))
    (when (> 2 current) (set n (- n (/ current 6))))
    (when (<= 2 current)
      (io !io_prog (id !item_extract_1 1) !item_elem_a)
      (io !io_prog (id !item_extract_1 2) !item_elem_f)
      (io !io_prog (id !item_printer_1 1) !item_frame)
      (io !io_prog (id !item_printer_1 2) !item_circuit)))
  (io !io_item (id !item_deploy 1) !item_extract_1)

  (let ((items 6))
    (io !io_prog (id !item_assembly_1 1) !item_extract_1 (* items n)))

  (boot/wait (id !item_assembly_1 1))

  (let ((id 1))
    (set id (boot/extract-set id n !item_elem_a))
    (set id (boot/extract-set id n !item_elem_b))
    (set id (boot/extract-set id n !item_elem_c))
    (set id (boot/extract-set id n !item_elem_d))
    (set id (boot/extract-set id n !item_elem_f))
    (set id (boot/extract-set id n !item_elem_g))))

(defun boot/extract-set (id n item)
  (for (i 0) (< i n) (+ i 1) (io !io_prog (id !item_extract_1 (+ id i)) item))
  (+ id n))



(defun boot/printer (n)
  (let ((current  (boot/count !item_printer_1)))
    (when (> 2 current) (set n (- n (/ current 5)))
	  (when (<= 2 current)
	    (io !io_prog (id !item_printer_1 1) !item_circuit)
	    (io !io_prog (id !item_printer_1 2) !item_gear))))
  (io !io_item (id !item_deploy 1) !item_printer_1)

  (let ((items 5))
    (io !io_prog (id !item_assembly_1 1) !item_printer_1 (* items n)))

  (boot/wait (id !item_assembly_1 1))

  (let ((id 1))
    (set id (boot/printer-set id n !item_frame))
    (set id (boot/printer-set id n !item_gear))
    (set id (boot/printer-set id n !item_circuit))
    (set id (boot/printer-set id n !item_neural))
    (set id (boot/printer-set id n !item_fuel))))

(defun boot/printer-set (id n item)
  (for (i 0) (< i n) (+ i 1) (io !io_prog (id !item_printer_1 (+ id i)) item))
  (+ id n))



(defun boot/assembler (n)
  (let ((current  (boot/count !item_assembly_1)))
    (when (> 2 current) (set n (- n (/ current 9))))
    (when (<= 2 current)
      (io !io_prog (id !item_assembly_1 1) !item_deploy)
      (io !io_item (id !item_deploy 1) !item_deploy)
      (boot/wait (id !item_assembly_1 1))

      (io !io_prog (id !item_assembly_1 1) !item_assembly_1 2)
      (io !io_prog (id !item_assembly_1 2) !item_servo)
      (io !io_item (id !item_deploy 1) !item_assembly_1)
      (boot/wait (id !item_assembly_1 1))

      (io !io_prog (id !item_assembly_1 3) !item_servo)
      (io !io_prog (id !item_assembly_1 4) !item_thruster)))

  (io !io_prog (id !item_assembly_1 1) !item_worker (* n 20))
  (io !io_item (id !item_deploy 1) !item_worker)

  (io !io_prog (id !item_assembly_2 1) !item_assembly_1 (* n 9))
  (io !io_item (id !item_deploy 1) !item_assembly_1)

  (boot/wait (id !item_assembly_1 1))
  (boot/wait (id !item_assembly_1 2))

  (let ((id 5))
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
  (for (i 0) (< i n) (+ i 1) (io !io_prog (id !item_assembly_1 (+ id i)) item))
  (+ id n))


(defun boot/wait (id)
  (while (if (= (io !io_status id) !io_ok) (/= (head) 0))
    (yield)))

(defun boot/count (item)
  (assert (= !io_ok (io !io_coord 0)))
  (let ((coord (head))) (io !io_scan (id !item_scanner_1 1) coord item))
  (io !io_result (id !item_scanner_1 1))
  (head))
