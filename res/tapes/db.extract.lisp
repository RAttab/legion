;; -----------------------------------------------------------------------------
;; extract_1
;; -----------------------------------------------------------------------------

(item_extract_1
 (item_elem_a (out item_elem_a))
 (item_elem_b (out item_elem_b))
 (item_elem_c (out item_elem_c))
 (item_elem_d (out item_elem_d))
 (item_elem_f (out item_elem_f))
 (item_elem_g (out item_elem_g)))

;; -----------------------------------------------------------------------------
;; extract_2
;; -----------------------------------------------------------------------------

(item_extract_2
 (item_elem_e (in item_elem_c)
	      (out item_elem_e))

 (item_elem_h (in item_elem_c)
	      (out item_elem_h))

 (item_elem_i (in item_elem_c
		  item_elem_c
		  item_elem_c)
	      (out item_elem_i)))


;; -----------------------------------------------------------------------------
;; extract_3
;; -----------------------------------------------------------------------------

(item_extract_3
 (item_elem_k (in item_elem_e
		  item_elem_e
		  item_elem_e
		  item_elem_e
		  item_elem_e)
	      (out item_elem_l
		   item_elem_l
		   item_elem_k
		   item_elem_l
		   item_elem_l)))
