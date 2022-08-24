;; Research all the T0 items.
;;
;; Number of active lab is received through the message queue. While
;; there are easier ways to get it, we need to test that functionality
;; somewhere and this seems as good a time as any.

(let ((n (recv-count)))
  ;; Elems
  (lab n &item-elem-a)
  (lab n &item-elem-b)
  (lab n &item-elem-c)
  (lab n &item-elem-d)

  ;; T0
  (lab n &item-muscle)
  (lab n &item-nodule)
  (lab n &item-vein)
  (lab n &item-bone)
  (lab n &item-tendon)
  (lab n &item-lens)
  (lab n &item-nerve)
  (lab n &item-neuron)
  (lab n &item-retina)
  (lab n &item-limb)
  (lab n &item-stem)
  (lab n &item-lung)
  (lab n &item-spinal)
  (lab n &item-engram)
  (lab n &item-cortex)
  (lab n &item-eye)

  ;; T1
  (lab n &item-semiconductor)
  (lab n &item-photovoltaic)
  (lab n &item-elem-g)
  (lab n &item-conductor)
  (lab n &item-antenna)
  (lab n &item-magnet)
  (lab n &item-field)
  (lab n &item-storage)
  (lab n &item-ferrofluid)

  ;; T2
  (lab n &item-brain)
  (lab n &item-elem-h)
  (lab n &item-accelerator)
  (lab n &item-galvanic)
  (lab n &item-battery)
  (lab n &item-elem-m)
  (lab n &item-elem-o)
  (lab n &item-biosteel)
  (lab n &item-heat-exchange)
  (lab n &item-furnace)
  (lab n &item-pill)
  (lab n &item-neurosteel)
  (lab n &item-m-reactor)
  (lab n &item-m-condenser)
  (lab n &item-m-lung))


(defun lab (n item)
  (assert (= (io &io-ping (id &item-lab 1)) &io-ok))
  (for (id 1) (<= id n) (+ id 1) (io &io-item (id &item-lab id) item))
  (for (known 0) (not known) (ior &io-item-known (id &item-lab 1) item)))


;; This is technically unecessary but it lets me test the send/recv
;; mechanism.
(defun recv-count ()
  (let ((n 0))
    (while (= n 0)
       (set n (when (= (io &io-recv 0) &io-ok)
		(assert (= (head) 2))
		(assert (= (head) !lab-count))
		(head))))
    n))
