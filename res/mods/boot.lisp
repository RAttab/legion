(boot/extractors 2)
(boot/printers 2)
(boot/assemblers 2)
(boot/workers 20)

(defun boot/extractors (n)
  (io !io_prog (id !item_extract_1 1) !item_elem_a)
  (io !io_prog (id !item_extract_1 2) !item_elem_f)

  (io !io_prog (id !item_printer_1 1) !item_frame)
  (io !io_prog (id !item_printer_1 2) !item_circuit)

  (let ((items (+ 1 (- !item_elem_k !item_elem_a)))
	(total (* items n)))

    (io !io_prog (id !item_assembly_1 1) !item_extract_1 total)
    (io !io_item (id !item_deploy 1) !item_extract_1)

    (boot/wait (id !item_assembly_1 1))

    (for (i 0) (< i total) (+ i 1)
	 (io !io_prog (id !item_extract_1 (+ i 1)) (+ !item_elem_a (/ i n))))))

(defun boot/printers (n)
  (let ((items (+ 1 (- !item_neural !item_frame)))
	(total (* items n)))

    (io !io_prog (id !item_assembly_1 1) !item_printer_1 total)
    (io !io_item (id !item_deploy 1) !item_printer_1)

    (boot/wait (id !item_assembly_1 1))

    (for (i 0) (<= i total) (+ i 1)
	 (io !io_prog (id !item_printer_1 (+ i 1)) (+ !item_frame (/ i n))))))

(defun boot/assemblers (n)
  (let ((items (+ 1 (- !item_databank !item_servo)))
	(total (* items n)))

    (io !io_prog (id !item_assembly_1 2) !item_servo)

    (io !io_prog (id !item_assembly_1 1) !item_assembly_1 total)
    (io !io_item (id !item_deploy 1) !item_assembly_1)

    (boot/wait (id !item_assembly_1 1))

    (for (i 0) (<= i total) (+ i 1)
	 (io !io_prog (id !item_assembly_1 (+ i 2)) (+ !item_servo (/ i n))))))

(defun boot/workers (n)
  (io !io_prog (id !item_assembly_1 1) !item_worker n)
  (io !io_item (id !item_deploy 1) !item_worker)
  (boot/wait (id !item_assembly_1 1)))

(defun boot/wait (id)
  (while (if (= (io !io_status id) !io_ok) (/= (head) 0))
    (yield)))
