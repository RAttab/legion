;; Scans a sector for stars that don't contain any `&item_brain` and
;; launches any available `&item_legion` to that star programmed with
;; `(mod boot 2)`.

(let ((scanner-star (id &item_scanner 2))
      (coord-star -1)
      (legion 0)
      (mem-index 1))

  (assert (= (io &io_scan scanner-star (progn (io &io_coord 0) (head))) &io_ok))

  (while (/= coord-star 0)

    (set legion (legion (progn (io &io_coord 0) (head))))
    (set coord-star (progn (assert (= (io &io_scan_val scanner-star) &io_ok))
			   (head)))

    (when (and (> legion 0) (and (/= coord-star -1) (/= coord-star 0)))
      (when (and (= (count coord-star &item_brain) 0)
		 (> (count coord-star &item_elem_a) 10000)
		 (> (count coord-star &item_elem_b) 10000)
		 (> (count coord-star &item_elem_c) 5000)
		 (> (count coord-star &item_elem_d) 5000)
		 (> (count coord-star &item_elem_e) 5000)
		 (> (count coord-star &item_elem_f) 5000))

	(assert (= (io &io_set (id &item_memory 1) mem-index coord-star) &io_ok))
	(set mem-index (+ mem-index 1))

	(assert (= (io &io_mod legion (mod boot 2)) &io_ok))
	(assert (= (io &io_launch legion coord-star) &io_ok))
	))))

(defun count (coord item)
  (let ((val -1) (scanner (id &item_scanner 3)))
    (assert (= (io &io_scan scanner coord item) &io_ok))
    (while (= val -1)
      (set val (progn (assert (= (io &io_scan_val scanner) &io_ok))
			      (head))))
    val))

(defun legion (coord-self)
  (when (count coord-self &item_legion)
    (let ((id (id &item_legion 1)))
      (while (= (io &io_ping id) &io_fail) (set id (+ id 1)))
      id)))
