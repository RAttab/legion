(starter
 (hue 0)
 (weight 50)
 (rolls
  (one item_energy 30000)
  (one item_elem_k 100)
  (all_of item_elem_a item_elem_d 60000)
  (all_of item_elem_g item_elem_h 30000)))

(barren
 (hue 270)
 (weight 200)
 (rolls
  (one item_energy 100)
  (one item_elem_k 100)
  (rng item_elem_a item_elem_j 100)))

(extract
 (hue 180)
 (weight 350)
 (rolls
  (one item_energy 10000)
  (one item_elem_k 100)
  (rng item_elem_a item_elem_j 1000)
  (one_of item_elem_a item_elem_f 65000)))

(condenser
 (hue 120)
 (weight 250)
 (rolls
  (one item_energy 10000)
  (one item_elem_k 100)
  (rng item_elem_a item_elem_j 1000)
  (one_of item_elem_g item_elem_j 65000)))

(power
 (hue 60)
 (weight 150)
 (rolls
  (one item_elem_k 65000)
  (one item_energy 65000)))
