;; Scans a sector for stars that don't contain any !item_brain_1 in
;; them and launches any available legion to that star programmed with
;; (boot 2).

(let ((scanner-star (id !item_scanner_1 2))
      (coord-self (progn (io !io_coord 0) (head)))
      (coord-star -1)
      (legion 0))

  (io !io_scan scanner-star coord-self)

  (while 1
    (set legion (launch/legion coord-self))
    (set coord-star (progn (io !io_scan_val scanner-star) (head)))
    (when (and (> legion 0) (/= coord-star -1))
      (when (= (launch/count coord-star !item_brain_1) 0)
	(assert (= (io !io_mod legion (mod boot 2)) !io_ok))
	(assert (= (io !io_launch legion coord-star) !io_ok))))))

(defun launch/count (coord item)
  (let ((val -1) (scanner (id !item_scanner_1 3)))
    (io !io_scan scanner coord item)
    (while (= val -1) (set val (progn (io !io_scan_val scanner) (head))))
    val))

(defun launch/legion (coord-self)
  (when (launch/count coord-self !item_legion_1)
    (let ((id 1))
      (while (= (io !io_ping (id !item_legion_1 id)) !io_fail) (set id (+ id 1)))
      id)))
