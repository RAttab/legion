;; Galactic OS

;; -----------------------------------------------------------------------------
;; consts
;; -----------------------------------------------------------------------------


(defconst net-id (id &item_memory 1))
(defconst net-parent 0)
(defconst net-len 1)
(defconst net-child 2)
(defconst net-child-cap (- 7 net-child))
(assert (= (io &io_ping net-id) &io_ok))

(defconst state-id (id &item_memory 2))
(defconst state-home 0)
(defconst state-depth 1)
(defconst state-exec 2)
(assert (= (io &io_ping state-id) &io_ok))

(defconst packet-id (id &item_memory 3))
(defconst packet-src 0)
(defconst packet-len 1)
(defconst packet-data 2)
(assert (= (io &io_ping packet-id) &io_ok))

(defconst memory-count 3)


;; -----------------------------------------------------------------------------
;; state
;; -----------------------------------------------------------------------------

(defun memory-need () memory-count)

(defun child-cap () net-child-cap)

(defun child-count ()
  (ior &io_get net-id net-len))

(defun depth ()
  (ior &io_get state-id state-depth))

(defun home ()
  (ior &io_get state-id state-home))

(defun is-home ()
  (not (ior &io_get net-id net-parent)))

(defun packet (ix)
  (ior &io_get packet-id (+ packet-data ix)))

(defun exec (mod)
  (io &io_mod (ior &io_get state-id state-exec) mod))

(defun child (index)
  (assert (< index net-child-cap))
  (ior &io_get net-id (+ net-child index)))



;; -----------------------------------------------------------------------------
;; net
;; -----------------------------------------------------------------------------

(defun net-connect ()
  (assert (= (io &io_ping (id &item_transmit 1)) &io_ok))
  (assert (= (io &io_ping (id &item_receive 1)) &io_ok))

  (case (ior &io_get net-id net-parent)
    ((0 (io &io_set state-id state-home (ior &io_coord (self)))))

    (parent
     (io &io_target (id &item_transmit 1) parent)
     (io &io_target (id &item_receive 1) parent)
     (io &io_transmit (id &item_transmit 1) !os-connect)
     (while (not (ior &io_get state-id state-home))
       (when (ior &io_receive (id &item_receive 1))
	 (assert (= (head) !os-accept))
	 (let ((home (head))) (io &io_set state-id state-home home))
	 (let ((depth (head))) (io &io_set state-id state-depth depth)))))))

(defun net-accept ()
  (io &io_transmit
      (id &item_transmit (ior &io_get packet-id packet-src))
      !os-accept
      (ior &io_get state-id state-home)
      (+ (ior &io_get state-id state-depth) 1)))

(defun net-child (coord)
  (assert coord)
  (let ((child-ix (ior &io_get net-id net-len))
	(child-id (+ child-ix 2)))
    (assert (= (io &io_ping (id &item_transmit child-id)) &io_ok))
    (assert (= (io &io_ping (id &item_receive child-id)) &io_ok))

    (io &io_target (id &item_transmit child-id) coord)
    (io &io_target (id &item_receive child-id) coord)
    (io &io_set net-id (+ net-child child-ix) coord)
    (io &io_set net-id net-len (+ child-ix 1))))


;; -----------------------------------------------------------------------------
;; os
;; -----------------------------------------------------------------------------

(defun boot (os-id exec-id)
  (assert (= (io &io_ping os-id) &io_ok))
  (assert (= (io &io_ping exec-id) &io_ok))

  (net-connect)
  (io &io_set state-id state-exec exec-id)
  (io &io_mod os-id (mod)))


(while 1
  (when (poll)
    (case (packet 0)
      ((!os-connect (net-accept))
       (!os-exec (propagate) (exec (packet 1)))
       (!os-update (propagate) (load (packet 1)))
       (!os-quit (propagate) (reset)))
      (data (propagate)))))


(defun poll ()
  (io &io_set packet-id packet-src 0)

  (let ((recv-len (if (= (io &io_recv (self)) &io_ok) (head) 0)))

    (let ((to-poll (+ (ior &io_get net-id net-len) 1)))
      (for (i 1) (and (<= i to-poll) (not recv-len)) (+ i 1)
	   (set recv-len (ior &io_receive (id &item_receive i)))
	   (when recv-len (io &io_set packet-id packet-src i))))

    (io &io_set packet-id packet-len recv-len)
    (for (i 0) (< i recv-len) (+ i 1)
	 (let ((part (head)))
	   (io &io_set packet-id (+ packet-data i) part)))

    recv-len))


;; Propagates the packet stored in packet id to the network; skipping
;; packet-src.
(defun propagate ()
  (assert (> (ior &io_get packet-id packet-len) 0))

  (let ((to-send (+ (ior &io_get net-id net-len) 1))
	(to-skip (ior &io_get packet-id packet-src)))

    (for (i 1) (<= i to-send) (+ i 1)
	 (when (/= i to-skip)
	   (case (ior &io_get packet-id packet-len)

	     ((1
	       (io &io_transmit (id &item_transmit i)
		   (ior &io_get packet-id (+ packet-data 0))))

	      (2
	       (io &io_transmit (id &item_transmit i)
		   (ior &io_get packet-id (+ packet-data 0))
		   (ior &io_get packet-id (+ packet-data 1))))

	      (3
	       (io &io_transmit (id &item_transmit i)
		   (ior &io_get packet-id (+ packet-data 0))
		   (ior &io_get packet-id (+ packet-data 1))
		   (ior &io_get packet-id (+ packet-data 2))))))))))
