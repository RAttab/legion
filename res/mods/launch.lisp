;; Scans a sector for stars that don't contain any `&item_brain` and
;; launches any available `&item_legion` to that star programmed with
;; `(mod boot 2)`.


(defconst scan-star-ongoing -1)
(defconst scan-star-done -1)

(defconst mem-id (id &item_memory 1))
(assert (= (io &io_ping mem-id) &io_ok))

(defconst scanner-star (id &item_scanner 2))
(assert (= (io &io_ping scanner-star) &io_ok))

(defconst scanner-count (id &item_scanner 3))
(assert (= (io &io_ping scanner-count) &io_ok))


(let ((mem-index 0))
  (io &io_scan scanner-star (progn (io &io_coord (self)) (head)))

  (while 1
    (case (progn (io &io_scan_val scanner-star) (head))
      ((scan-star-ongoing 0)
       (scan-star-done
	(io &io_scan scanner-star
	    (coord-inc (progn (io &io_status scanner-star) (head))))))
      (star
       (when (check-star star)
	 (let ((legion-id (legion)))
	   (assert (= (io &io_ping legion-id) &io_ok))

	   (io &io_mod legion-id (mod boot 2))
	   (io &io_launch legion-id star)

	   (io &io_set mem-id mem-index star)
	   (set mem-index (+ mem-index 1))))))))


(defconst min-energy 2000)
(defconst min-elem-top 10000)
(defconst min-elem-mid 5000)
(defconst min-elem-low 2000)
(defun check-star (star)
  (if (> (count star &item_brain) 0) 0
    (if (< (count star &item_elem_a) min-elem-top) 0
      (if (< (count star &item_elem_b) min-elem-top) 0
	(if (< (count star &item_elem_c) min-elem-mid) 0
	  (if (< (count star &item_elem_d) min-elem-mid) 0
	    (if (< (count star &item_elem_e) min-elem-mid) 0
	      (if (< (count star &item_elem_f) min-elem-mid) 0
		(if (< (count star &item_elem_g) min-elem-low) 0
		  (if (< (count star &item_elem_h) min-elem-low) 0
		    (if (< (count star &item_energy) min-energy) 0
		      1)))))))))))


(defconst inc-y (bsl 1 (+ 16)))
(defconst inc-x (bsl 1 (+ 32 16)))
(defun coord-inc (coord)
  (case (rem (tsc) 4)
    ((0 (+ coord inc-x))
     (1 (- coord inc-x))
     (2 (+ coord inc-y))
     (3 (- coord inc-y)))))


(defun count (coord item)
  (io &io_scan scanner-count coord item)
  (let ((count -1))
    (while (< count 0)
      (set count (progn (io &io_scan_val scanner-count) (head))))
    count))


(defun legion ()
  (let ((coord-self (progn (io &io_coord (self)) (head))))
    (while (progn (io &io_scan coord-self &item_legion) (head)))
    (let ((id (id &item_legion 1)))
      (while (= (io &io_ping id) &io_fail) (set id (+ id 1)))
      id)))
