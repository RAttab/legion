;; Scans a sector for stars that don't contain any `&item_brain` and
;; launches any available `&item_legion` to that star programmed with
;; `(mod boot 2)`.


(defconst scan-star-ongoing -1)
(defconst scan-star-done 0)

(defconst mem-id (id &item_memory 1))
(assert (= (io &io_ping mem-id) &io_ok))

(defconst scanner-id (id &item_scanner 1))
(assert (= (io &io_ping scanner-id) &io_ok))

(defconst prober-id (id &item_prober 2))
(assert (= (io &io_ping prober-id) &io_ok))


(let ((mem-index 1))
  (io &io_scan scanner-id
      (coord-inc (progn (io &io_coord (self)) (head))))

  (while 1
    (case (progn (io &io_value scanner-id) (head))
      ((scan-star-ongoing 0)
       (scan-star-done
	(io &io_scan scanner-id
	    (coord-inc (progn (io &io_state scanner-id &io_target) (head))))))
      (star
       (when (check-star star)
	 (let ((legion-id (legion)))
	   (assert (= (io &io_ping legion-id) &io_ok))

	   (io &io_mod legion-id (mod boot 2))
	   (io &io_launch legion-id star)

	   (io &io_set mem-id mem-index star)
	   (set mem-index (+ mem-index 1))))))))


(defconst min-energy 2000)
(defconst min-solid 15000)
(defconst min-gas 2000)
(defun check-star (star)
  (if (> (count star &item_brain) 0) 0
    (if (< (count star &item_elem_a) min-solid) 0
      (if (< (count star &item_elem_b) min-solid) 0
	(if (< (count star &item_elem_c) min-solid) 0
	  (if (< (count star &item_elem_d) min-solid) 0
	    (if (< (count star &item_elem_g) min-gas) 0
	      (if (< (count star &item_elem_h) min-gas) 0
		(if (< (count star &item_energy) min-energy) 0
		  1)))))))))


(defconst inc-y (bsl 1 (+ 16)))
(defconst inc-x (bsl 1 (+ 32 16)))
(defun coord-inc (coord)
  (case (rem (progn (io &io_tick (self)) (head)) 5)
    ((0 (+ coord inc-x))
     (1 (- coord inc-x))
     (2 (+ coord inc-y))
     (3 (- coord inc-y))
     (4 coord))))


(defun count (coord item)
  (io &io_probe prober-id item coord)
  (let ((count -1))
    (while (< count 0)
      (set count (progn (io &io_value prober-id) (head))))
    count))


(defun legion ()
  (let ((coord-self (progn (io &io_coord (self)) (head))))
    (while (= (count coord-self &item_legion) 0))
    (let ((id (id &item_legion 1)))
      (while (= (io &io_ping id) &io_fail) (set id (+ id 1)))
      id)))
