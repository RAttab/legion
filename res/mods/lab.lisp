;; Research all the T0 items.
;;
;; Number of active lab is received through the message queue. While
;; there are easier ways to get it, we need to test that functionality
;; somewhere and this seems as good a time as any.

(let ((n (recv-count)))
  ;; T0 - Elem
  (lab n &item_elem_a)
  (lab n &item_elem_b)
  (lab n &item_elem_c)
  (lab n &item_elem_d)
  (lab n &item_elem_e)
  (lab n &item_elem_f)

  ;; T0 - Passives
  (lab n &item_frame)
  (lab n &item_logic)
  (lab n &item_gear)
  (lab n &item_neuron)
  (lab n &item_bond)
  (lab n &item_magnet)
  (lab n &item_nuclear)
  (lab n &item_robotics)
  (lab n &item_core)
  (lab n &item_capacitor)
  (lab n &item_matrix)
  (lab n &item_magnet_field)
  (lab n &item_hull)

  ;; T1
  (lab n &item_captor)
  (lab n &item_solar)
  (lab n &item_worker)
  (lab n &item_elem_g)
  (lab n &item_elem_h)
  (lab n &item_liquid_frame)
  (lab n &item_radiation)
  (lab n &item_antenna)
  (lab n &item_storage)
  (lab n &item_accelerator)
  (lab n &item_lab))


(defun lab (n item)
  (for (id 1) (<= id n) (+ id 1) (io &io_item (id &item_lab id) item))
  (for (known 0) (not known)
       (progn (assert (= (io &io_item_known (id &item_lab 1) item) &io_ok))
	      (head))))


;; This is technically unecessary but it lets me test the send/recv
;; mechanism.
(defun recv-count ()
  (let ((n 0))
    (while (= n 0)
       (set n (when (= (io &io_recv 0) &io_ok)
		(assert (= (head) 2))
		(assert (= (head) !lab_count))
		(head))))
    n))
