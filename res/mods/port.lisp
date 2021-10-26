;; Given that ports are processed in sequential order, using pre-setup
;; ports for each element or children would to the earliest element
;; getting preferential treatment which ultimately would lead to
;; starvation.
;;
;; We fix this by only using two ports and randomly switching their
;; targets around every once in a while. giving a fairer distribution
;; of pills across the elements and our spanning tree.
;;
;; \todo need to revisit the port mechanism as it should hopefully not
;; be this complex to get something working that is also fair.


(defconst switch-wait 1000)

(defconst mem-child 1)
(defconst mem-id (id &item_memory 1))
(assert (= (io &io_ping mem-id) &io_ok))

(defconst port-elem-id (id &item_port 1))
(assert (= (io &io_ping port-elem-id) &io_ok))

(defconst port-child-id (id &item_port 2))
(assert (= (io &io_ping port-child-id) &io_ok))



(unless (is-home)
  (io &io_target port-elem-id (call (os os-home))))

(let ((child-count (recv-count))
      (prev 0))
  (while 1
    (when (> (now) (+ prev switch-wait))
      (set prev (now))
      (unless (is-home)
	(io &io_item port-elem-id (pick-elem)))
      (io &io_target port-child-id (pick-child child-count)))))


;; -----------------------------------------------------------------------------
;; utils
;; -----------------------------------------------------------------------------

(defun now ()
  (progn (io &io_tick (self)) (head)))

(defun is-home ()
  (if (progn (io &io_get mem-id 0) (head)) 0 1))

(defun recv-count ()
  (let ((n 0))
    (while (= n 0)
      (set n (when (= (io &io_recv 0) &io_ok)
	       (assert (= (head) 2))
	       (assert (= (head) !child_count))
	       (head))))
    n))

(defun pick-elem ()
  (case (rem (now) 6)
    ((0 &item_elem_a)
     (1 &item_elem_b)
     (2 &item_elem_c)
     (3 &item_elem_d)
     (4 &item_elem_g)
     (5 &item_elem_h))))

(defun pick-child (n)
  (let ((index (+ mem-child (rem (now) n))))
    (io &io_get mem-id index) (head)))
