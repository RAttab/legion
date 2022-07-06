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
  (io &io_get net-id net-len)
  (head))

(defun depth ()
  (io &io_get state-id state-depth)
  (head))

(defun home ()
  (io &io_get state-id state-home)
  (head))

(defun is-home ()
  (io &io_get net-id net-parent)
  (not (head)))

(defun packet (ix)
  (io &io_get packet-id (+ packet-data ix))
  (head))

(defun exec (mod)
  (io &io_mod (progn (io &io_get state-id state-exec) (head)) mod))

(defun child (index)
  (assert (< index net-child-cap))
  (io &io_get net-id (+ net-child index))
  (head))



;; -----------------------------------------------------------------------------
;; net
;; -----------------------------------------------------------------------------

(defun net-connect ()
  (assert (= (io &io_ping (id &item_transmit 1)) &io_ok))
  (assert (= (io &io_ping (id &item_receive 1)) &io_ok))

  (case (progn (io &io_get net-id net-parent) (head))
    ((0 (io &io_set state-id state-home (progn (io &io_coord (self)) (head)))))

    (parent
     (io &io_target (id &item_transmit 1) parent)
     (io &io_target (id &item_receive 1) parent)
     (io &io_transmit (id &item_transmit 1) !os-connect)
     (while (not (progn (io &io_get state-id state-home) (head)))
       (when (progn (io &io_receive (id &item_receive 1)) (head))
	 (assert (= (head) !os-accept))
	 (let ((home (head))) (io &io_set state-id state-home home))
	 (let ((depth (head))) (io &io_set state-id state-depth depth)))))))

(defun net-accept ()
  (io &io_transmit
      (id &item_transmit (progn (io &io_get packet-id packet-src) (head)))
      !os-accept
      (progn (io &io_get state-id state-home) (head))
      (+ (progn (io &io_get state-id state-depth) (head)) 1)))

(defun net-child (coord)
  (assert coord)
  (let ((child-ix (progn (io &io_get net-id net-len) (head)))
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

    (let ((to-poll (+ (progn (io &io_get net-id net-len) (head)) 1)))
      (for (i 1) (and (<= i to-poll) (not recv-len)) (+ i 1)
	   (set recv-len (progn (io &io_receive (id &item_receive i)) (head)))
	   (when recv-len (io &io_set packet-id packet-src i))))

    (io &io_set packet-id packet-len recv-len)
    (for (i 0) (< i recv-len) (+ i 1)
	 (let ((part (head)))
	   (io &io_set packet-id (+ packet-data i) part)))

    recv-len))


;; Propagates the packet stored in packet id to the network; skipping
;; packet-src.
(defun propagate ()
  (assert (> (progn (io &io_get packet-id packet-len) (head)) 0))

  (let ((to-send (+ (progn (io &io_get net-id net-len) (head)) 1))
	(to-skip (progn (io &io_get packet-id packet-src) (head))))

    (for (i 1) (<= i to-send) (+ i 1)
	 (when (/= i to-skip)
	   (case (progn (io &io_get packet-id packet-len) (head))

	     ((1
	       (io &io_transmit (id &item_transmit i)
		   (progn (io &io_get packet-id (+ packet-data 0)) (head))))

	      (2
	       (io &io_transmit (id &item_transmit i)
		   (progn (io &io_get packet-id (+ packet-data 0)) (head))
		   (progn (io &io_get packet-id (+ packet-data 1)) (head))))

	      (3
	       (io &io_transmit (id &item_transmit i)
		   (progn (io &io_get packet-id (+ packet-data 0)) (head))
		   (progn (io &io_get packet-id (+ packet-data 1)) (head))
		   (progn (io &io_get packet-id (+ packet-data 2)) (head))))))))))
