;; Scans a sector for stars that don't contain any `!item_brain` and
;; launches any available `!item_legion` to that star programmed with
;; `(mod boot.2)`.


(defconst scan-star-ongoing -1)
(defconst scan-star-done 0)

(defconst scanner-id (id !item-scanner 1))
(assert (= (io !io-ping scanner-id) !io-ok))

(defconst prober-id (id !item-prober 3))
(assert (= (io !io-ping prober-id) !io-ok))


(io !io-scan scanner-id (coord-inc (ior !io-coord (self))))

(while 1
  (case (ior !io-value scanner-id)
    ((scan-star-ongoing 0)
     (scan-star-done
      (io !io-scan scanner-id
	  (coord-inc (ior !io-state scanner-id !io-target)))))
    (star
     (when (check-star star)
       (let ((legion-id (legion)))
	 (assert (= (io !io-ping legion-id) !io-ok))

	 (io !io-mod legion-id (mod boot.2))
	 (io !io-launch legion-id star)
	 (os.net-child star))))))


(defun check-star (star)
  (if (> (count star !item-brain) 0) 0
    (if (< (count star !item-elem-a) 30000) 0
      (if (< (count star !item-elem-b) 30000) 0
	(if (< (count star !item-elem-c) 20000) 0
	  (if (< (count star !item-elem-d) 5000) 0
	    (if (< (count star !item-elem-e) 5000) 0
	      (if (< (count star !item-elem-f) 5000) 0
		(if (< (count star !item-energy) 8000) 0
		  1)))))))))


;; We're incrementing the packed coord
(defconst inc-y (bsl 1 (+ 16)))
(defconst inc-x (bsl 1 (+ 32 16)))
(defun coord-inc (coord)
  (case (rem (ior !io-tick (self)) 5)
    ((0 (+ coord inc-x))
     (1 (- coord inc-x))
     (2 (+ coord inc-y))
     (3 (- coord inc-y))
     (4 coord))))


(defun count (coord item)
  (io !io-probe prober-id item coord)
  (let ((count -1))
    (while (< count 0) (set count (ior !io-value prober-id)))
    count))


(defun legion ()
  (let ((coord-self (ior !io-coord (self))))
    (while (= (count coord-self !item-legion) 0))
    (let ((id (id !item-legion 1)))
      (while (= (io !io-ping id) !io-fail) (set id (+ id 1)))
      id)))
