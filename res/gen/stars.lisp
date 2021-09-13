(starter
 (weight 50)
 (rolls
  (one item_energy 30000)
  (one item_elem_k 100)
  (all_of item_elem_a item_elem_f 60000)
  (all_of item_elem_g item_elem_h 30000)
  (all_of item_elem_h item_elem_i 10000)))

(barren
 (weight 200)
 (rolls
  (one item_energy 100)
  (one item_elem_k 100)
  (rng item_elem_a item_elem_j 100)))

(extract
 (weight 450)
 (rolls
  (one item_energy 10000)
  (one item_elem_k 100)
  (rng item_elem_a item_elem_j 1000)
  (one_of item_elem_a item_elem_f 65000)))

(condenser
 (weight 250)
 (rolls
  (one item_energy 10000)
  (one item_elem_k 100)
  (rng item_elem_a item_elem_j 1000)
  (one_of item_elem_g item_elem_j 65000)))

(power
 (weight 50)
 (rolls
  (one item_elem_k 65000)
  (one item_energy 65000)))
