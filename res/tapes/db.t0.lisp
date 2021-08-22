;; -----------------------------------------------------------------------------
;; extract
;; -----------------------------------------------------------------------------

(item_extract
 (item_elem_a (out item_elem_a))
 (item_elem_b (out item_elem_b))
 (item_elem_c (out item_elem_c))
 (item_elem_d (out item_elem_d))
 (item_elem_e (out item_elem_e))
 (item_elem_f (out item_elem_f)))


;; -----------------------------------------------------------------------------
;; printer
;; -----------------------------------------------------------------------------

(item_printer

 (item_frame
  (in item_elem_a
      item_elem_a
      item_elem_a
      item_elem_a
      item_elem_a)
  (out item_frame))

 (item_logic
  (in item_elem_a
      item_elem_b
      item_elem_b
      item_elem_a
      item_elem_b
      item_elem_b
      item_elem_a)
  (out item_logic))

 (item_gear
  (in item_elem_a
      item_elem_c
      item_elem_a
      item_elem_c
      item_elem_a)
  (out item_gear))

 (item_neuron
  (in item_elem_b
      item_elem_d
      item_elem_b
      item_elem_d
      item_elem_b)
  (out item_neuron))

 (item_bond
  (in item_elem_d
      item_elem_e
      item_elem_d
      item_elem_e
      item_elem_d)
  (out item_bond))

 (item_magnet
  (in item_elem_e
      item_elem_f
      item_elem_e
      item_elem_f
      item_elem_e)
  (out item_magnet))

 (item_nuclear
  (in item_elem_b
      item_elem_d
      item_elem_f
      item_elem_d
      item_elem_f
      item_elem_d
      item_elem_b)
  (out item_nuclear)))


;; -----------------------------------------------------------------------------
;; assembly - passives
;; -----------------------------------------------------------------------------

(item_assembly
 (item_robotics
  (in item_gear
      item_logic
      item_gear
      item_gear
      item_logic
      item_gear)
  (out item_robotics))

 (item_core
  (in item_logic
      item_neuron
      item_neuron
      item_logic
      item_neuron
      item_neuron
      item_logic)
  (out item_core))

 (item_capacitor
  (in item_gear
      item_neuron
      item_neuron
      item_neuron
      item_gear)
  (out item_capacitor))

 (item_matrix
  (in item_capacitor
      item_core
      item_bond
      item_core
      item_core
      item_bond
      item_core
      item_capacitor)
  (out item_matrix))

 (item_magnet_field
  (in item_magnet
      item_magnet
      item_robotics
      item_magnet
      item_magnet
      item_magnet
      item_robotics
      item_magnet
      item_magnet)
  (out item_magnet_field))

 (item_hull
  (in item_magnet_field
      item_magnet_field
      item_bond
      item_nuclear
      item_nuclear
      item_nuclear
      item_bond
      item_magnet_field
      item_magnet_field)
  (out item_hull)))


;; -----------------------------------------------------------------------------
;; assembly - active
;; -----------------------------------------------------------------------------

(item_assembly

 (item_deploy
  (in item_frame
      item_logic
      item_logic
      item_frame
      item_logic
      item_logic
      item_frame)
  (out item_deploy))

 (item_extract
  (in item_frame
      item_frame
      item_logic
      item_logic
      item_logic
      item_frame
      item_frame)
  (out item_extract))

 (item_printer
  (in item_gear
      item_logic
      item_logic
      item_gear
      item_logic
      item_logic
      item_gear)
  (out item_printer))

 (item_assembly
  (in item_frame
      item_robotics
      item_neuron
      item_robotics
      item_robotics
      item_neuron
      item_robotics
      item_frame)
  (out item_assembly))

 (item_worker
  (in item_magnet
      item_frame
      item_frame
      item_core
      item_frame
      item_frame
      item_magnet)
  (out item_worker))

 (item_memory
  (in item_capacitor
      item_capacitor
      item_capacitor
      item_bond
      item_core
      item_bond
      item_capacitor
      item_capacitor
      item_capacitor)
  (out item_memory))

 (item_brain
  (in item_memory
      item_memory
      item_matrix
      item_matrix
      item_matrix
      item_memory
      item_memory)
  (out item_brain))

 (item_scanner
  (in item_magnet
      item_magnet
      item_magnet
      item_capacitor
      item_capacitor
      item_magnet
      item_magnet
      item_magnet)
  (out item_scanner))

 (item_legion
  (in item_hull
      item_hull
      item_worker
      item_worker
      item_extract
      item_extract
      item_printer
      item_printer
      item_assembly
      item_assembly
      item_deploy
      item_deploy
      item_memory
      item_brain
      item_scanner
      item_hull
      item_hull)
  (out item_legion))

 (item_lab
  (in item_robotics
      item_magnet_field
      item_nuclear
      item_magnet_field
      item_robotics)
  (out item_lab)))
