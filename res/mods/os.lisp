;; Galactic OS

;; -----------------------------------------------------------------------------
;; constants
;; -----------------------------------------------------------------------------

(defconst mem-id (id &item_memory 2))
(defconst mem-brain 0)
(defconst mem-pck-src 2)
(defconst mem-pck-len 3)
(defconst mem-pck-msg 4)
(assert (= (io &io_ping mem-id) &io_ok))

(defconst mem-tree-id (id &item_memory 1))
(defconst mem-tree-home 6)
(assert (= (io &io_ping mem-tree-id) &io_ok))


;; -----------------------------------------------------------------------------
;; api
;; -----------------------------------------------------------------------------

(defun os-home ()
  (io &io_transmit (id &item_transmit 1) !os_home)

  (let ((home (progn (io &io_get mem-tree-id mem-tree-home) (head))))
    (when (= home 1) (set home 0))

    (while (not home)
      (when (progn (io &io_receive (id &item_receive 1)) (head))
	(assert (= (head) !os_home))
	(set home (head))))

    (io &io_set mem-tree-id mem-tree-home home)
    home))


;; -----------------------------------------------------------------------------
;; loop
;; -----------------------------------------------------------------------------

(while 1
  (when (poll)
    (propagate)
    (case (msg 0)
      ((!os_run (run (msg 1)))
       (!os_home (home))
       (!os_update (load (msg 1)))
       (!os_quit (reset))))))


;; Look through all our reception points (the brain's msg buffer and
;; our &item_receive) for commands received. If there's something save
;; it to mem for further processing (keeping it on the stack would
;; basically guarantee stack overflows).
(defun poll ()
  (let ((msg-len 0))

    ;; Commands sent directly to the brain
    (when (= (io &io_recv (self)) &io_ok)
      (set msg-len (head))
      (io &io_set mem-id mem-pck-src 0))

    ;; Commands received from one of our &item_receive
    (let ((to-poll (count &item_receive)))
      (for (i 1) (and (<= i to-poll) (not msg-len)) (+ i 1)
	   (when (= (io &io_receive (id &item_receive i)) &io_ok) (set msg-len (head)))
	   (when msg-len (io &io_set mem-id mem-pck-src i))))

    ;; Save the rest of the message to mem
    (io &io_set mem-id mem-pck-len msg-len)
    (for (i 0) (< i msg-len) (+ i 1)
	 (let ((part (head))) (io &io_set mem-id (+ mem-pck-msg i) part)))

    msg-len))


;; Transmit the command in mem to all &item_transmit that are not
;; mem-pck-src (the source of the command)
(defun propagate ()
  (let ((len (count &item_transmit))
	(src (progn (io &io_get mem-id mem-pck-src) (head))))

    (for (i 1) (<= i len) (+ i 1)
	 (when (/= (id &item_receive i) (id &item_receive src))
	   (case (progn (io &io_get mem-id mem-pck-len) (head))
	     ((1
	       (io &io_transmit (id &item_transmit i)
		   (progn (io &io_get mem-id mem-pck-len) (head))
		   (progn (io &io_get mem-id (+ mem-pck-msg 0)) (head))))
	      (2
	       (io &io_transmit (id &item_transmit i)
		   (progn (io &io_get mem-id mem-pck-len) (head))
		   (progn (io &io_get mem-id (+ mem-pck-msg 0)) (head))
		   (progn (io &io_get mem-id (+ mem-pck-msg 1)) (head))))
	      (3
	       (io &io_transmit (id &item_transmit i)
		   (progn (io &io_get mem-id mem-pck-len) (head))
		   (progn (io &io_get mem-id (+ mem-pck-msg 0)) (head))
		   (progn (io &io_get mem-id (+ mem-pck-msg 1)) (head))
		   (progn (io &io_get mem-id (+ mem-pck-msg 2)) (head))))))))))


;; -----------------------------------------------------------------------------
;; commands
;; -----------------------------------------------------------------------------

;; Execute the given mod on our executor brain who was created by boot
;; and whose id is in mem-brain
(defun run (mod)
  (let ((brain-id (progn (io &io_get mem-id mem-brain) (head))))
    (assert (= (io &io_mod brain-id mod) &io_ok))))

(defun home ()
  (let ((home (progn (io &io_get mem-tree-id mem-tree-home) (head)))
	(src (progn (io &io_get mem-id mem-pck-src) (head))))
    (assert (= (io &io_transmit (id &item_transmit src) !os_home home) &io_ok))))

;; -----------------------------------------------------------------------------
;; misc
;; -----------------------------------------------------------------------------

(defun msg (index) (io &io_get mem-id (+ mem-pck-msg index)) (head))

(defun count (item)
  (let ((coord (progn (assert (= (io &io_coord (self)) &io_ok)) (head))))
    (assert (= (io &io_scan (id &item_scanner 1) coord item) &io_ok))
    (assert (= (io &io_scan_val (id &item_scanner 1)) &io_ok))
    (head)))
