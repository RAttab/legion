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


(defconst cycle-delay 1000)

(defconst port-elem-id (id &item-port 1))
(assert (= (io &io-ping port-elem-id) &io-ok))

(defconst port-child-id (id &item-port 2))
(assert (= (io &io-ping port-child-id) &io-ok))


(unless (call (os is-home))
  (io &io-target port-elem-id (call (os home))))

(let ((next 0))
  (while 1
    (when (> (now) next)
      (set next (+ (now) cycle-delay))
      (unless (call (os is-home)) (cycle-elem))
      (cycle-child))))


;; -----------------------------------------------------------------------------
;; utils
;; -----------------------------------------------------------------------------

(defun now () (ior &io-tick (self)))

(defun cycle-elem ()
  (io &io-item port-elem-id
      (case (rem (now) 6)
	((0 &item-elem-a)
	 (1 &item-elem-b)
	 (2 &item-elem-c)
	 (3 &item-elem-d)
	 (4 &item-elem-g)
	 (5 &item-elem-h)))))


(defun cycle-child ()
  (let ((n (call (os child-count))))
    (when n
      (io &io-target port-child-id
	  (call (os child) (rem (now) n))))))
