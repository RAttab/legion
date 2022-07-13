;; Galactic OS

;; -----------------------------------------------------------------------------
;; consts
;; -----------------------------------------------------------------------------


(defconst net-id (id &item-memory 1))
(defconst net-parent 0)
(defconst net-len 1)
(defconst net-child 2)
(defconst net-child-cap (- 7 net-child))
(assert (= (io &io-ping net-id) &io-ok))

(defconst state-id (id &item-memory 2))
(defconst state-home 0)
(defconst state-depth 1)
(defconst state-exec 2)
(assert (= (io &io-ping state-id) &io-ok))

(defconst packet-id (id &item-memory 3))
(defconst packet-src 0)
(defconst packet-len 1)
(defconst packet-data 2)
(assert (= (io &io-ping packet-id) &io-ok))

(defconst memory-count 3)


;; -----------------------------------------------------------------------------
;; state
;; -----------------------------------------------------------------------------

(defun memory-need () memory-count)

(defun child-cap () net-child-cap)

(defun child-count ()
  (ior &io-get net-id net-len))

(defun depth ()
  (ior &io-get state-id state-depth))

(defun home ()
  (ior &io-get state-id state-home))

(defun is-home ()
  (not (ior &io-get net-id net-parent)))

(defun packet (ix)
  (ior &io-get packet-id (+ packet-data ix)))

(defun exec (mod)
  (io &io-mod (ior &io-get state-id state-exec) mod))

(defun child (index)
  (assert (< index net-child-cap))
  (ior &io-get net-id (+ net-child index)))



;; -----------------------------------------------------------------------------
;; net
;; -----------------------------------------------------------------------------

(defun net-connect ()
  (assert (= (io &io-ping (id &item-transmit 1)) &io-ok))
  (assert (= (io &io-ping (id &item-receive 1)) &io-ok))

  (case (ior &io-get net-id net-parent)
    ((0 (io &io-set state-id state-home (ior &io-coord (self)))))

    (parent
     (io &io-target (id &item-transmit 1) parent)
     (io &io-target (id &item-receive 1) parent)
     (io &io-transmit (id &item-transmit 1) !os-connect)
     (while (not (ior &io-get state-id state-home))
       (when (ior &io-receive (id &item-receive 1))
	 (assert (= (head) !os-accept))
	 (let ((home (head))) (io &io-set state-id state-home home))
	 (let ((depth (head))) (io &io-set state-id state-depth depth)))))))

(defun net-accept ()
  (io &io-transmit
      (id &item-transmit (ior &io-get packet-id packet-src))
      !os-accept
      (ior &io-get state-id state-home)
      (+ (ior &io-get state-id state-depth) 1)))

(defun net-child (coord)
  (assert coord)
  (let ((child-ix (ior &io-get net-id net-len))
	(child-id (+ child-ix 2)))
    (assert (= (io &io-ping (id &item-transmit child-id)) &io-ok))
    (assert (= (io &io-ping (id &item-receive child-id)) &io-ok))

    (io &io-target (id &item-transmit child-id) coord)
    (io &io-target (id &item-receive child-id) coord)
    (io &io-set net-id (+ net-child child-ix) coord)
    (io &io-set net-id net-len (+ child-ix 1))))


;; -----------------------------------------------------------------------------
;; os
;; -----------------------------------------------------------------------------

(defun boot (os-id exec-id)
  (assert (= (io &io-ping os-id) &io-ok))
  (assert (= (io &io-ping exec-id) &io-ok))

  (net-connect)
  (io &io-set state-id state-exec exec-id)
  (io &io-mod os-id (mod)))


(while 1
  (when (poll)
    (case (packet 0)
      ((!os-connect (net-accept))
       (!os-exec (propagate) (exec (packet 1)))
       (!os-update (propagate) (load (packet 1)))
       (!os-quit (propagate) (reset)))
      (data (propagate)))))


(defun poll ()
  (io &io-set packet-id packet-src 0)

  (let ((recv-len (if (= (io &io-recv (self)) &io-ok) (head) 0)))

    (let ((to-poll (+ (ior &io-get net-id net-len) 1)))
      (for (i 1) (and (<= i to-poll) (not recv-len)) (+ i 1)
	   (set recv-len (ior &io-receive (id &item-receive i)))
	   (when recv-len (io &io-set packet-id packet-src i))))

    (io &io-set packet-id packet-len recv-len)
    (for (i 0) (< i recv-len) (+ i 1)
	 (let ((part (head)))
	   (io &io-set packet-id (+ packet-data i) part)))

    recv-len))


;; Propagates the packet stored in packet id to the network; skipping
;; packet-src.
(defun propagate ()
  (assert (> (ior &io-get packet-id packet-len) 0))

  (let ((to-send (+ (ior &io-get net-id net-len) 1))
	(to-skip (ior &io-get packet-id packet-src)))

    (for (i 1) (<= i to-send) (+ i 1)
	 (when (/= i to-skip)
	   (case (ior &io-get packet-id packet-len)

	     ((1
	       (io &io-transmit (id &item-transmit i)
		   (ior &io-get packet-id (+ packet-data 0))))

	      (2
	       (io &io-transmit (id &item-transmit i)
		   (ior &io-get packet-id (+ packet-data 0))
		   (ior &io-get packet-id (+ packet-data 1))))

	      (3
	       (io &io-transmit (id &item-transmit i)
		   (ior &io-get packet-id (+ packet-data 0))
		   (ior &io-get packet-id (+ packet-data 1))
		   (ior &io-get packet-id (+ packet-data 2))))))))))
