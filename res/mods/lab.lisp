;; Research all the T0 items.
;;
;; Number of active lab is received through the message queue. While
;; there are easier ways to get it, we need to test that functionality
;; somewhere and this seems as good a time as any.

(let ((n (recv-count)))
  ;; Elems
  (lab n &item_elem_a)
  (lab n &item_elem_b)
  (lab n &item_elem_c)
  (lab n &item_elem_d)

  ;; T0
  (lab n &item_muscle)
  (lab n &item_nodule)
  (lab n &item_vein)
  (lab n &item_bone)
  (lab n &item_tendon)
  (lab n &item_lens)
  (lab n &item_nerve)
  (lab n &item_neuron)
  (lab n &item_retina)
  (lab n &item_limb)
  (lab n &item_stem)
  (lab n &item_lung)
  (lab n &item_spinal)
  (lab n &item_engram)
  (lab n &item_cortex)
  (lab n &item_eye)

  ;; T1
  (lab n &item_semiconductor)
  (lab n &item_photovoltaic)
  (lab n &item_elem_g)
  (lab n &item_conductor)
  (lab n &item_antenna)
  (lab n &item_magnet)
  (lab n &item_field)
  (lab n &item_storage)
  (lab n &item_ferrofluid))


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
