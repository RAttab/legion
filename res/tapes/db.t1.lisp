;; -----------------------------------------------------------------------------
;; extract
;; -----------------------------------------------------------------------------
(item_extract
 (item_elem_g (energy 1) (out item_elem_g))
 (item_elem_h (energy 1) (out item_elem_h)))


;; -----------------------------------------------------------------------------
;; printer
;; -----------------------------------------------------------------------------

(item_printer

 (item_captor
  (in item_elem_c
      item_elem_e
      item_elem_c
      item_elem_c
      item_elem_c
      item_elem_e
      item_elem_c)
  (out item_captor))

 (item_liquid_frame
  (energy 4)
  (in item_elem_g
      item_elem_g
      item_elem_a
      item_elem_g
      item_elem_g
      item_elem_a
      item_elem_g
      item_elem_g)
  (out item_liquid_frame))

 (item_radiation
  (energy 10)
  (in item_elem_f
      item_elem_h
      item_elem_f
      item_elem_h
      item_elem_h
      item_elem_h
      item_elem_f
      item_elem_h
      item_elem_f)
  (out item_radiation)))

;; -----------------------------------------------------------------------------
;; assembly
;; -----------------------------------------------------------------------------

(item_assembly

 (item_solar
  (in item_captor
      item_captor
      item_bond
      item_captor
      item_bond
      item_captor
      item_bond
      item_captor
      item_captor)
  (out item_solar))

 (item_storage
  (in item_magnet_field
      item_robotics
      item_robotics
      item_robotics
      item_magnet_field)
  (out item_storage))

 ;; (item_port
 ;;  (in item_magnet_field
 ;;      item_magnet_field
 ;;      item_captor
 ;;      item_frame
 ;;      item_storage
 ;;      item_storage
 ;;      item_frame
 ;;      item_captor
 ;;      item_magnet_field
 ;;      item_magnet_field)
 ;;  (out item_port))

 (item_bullet
  (in item_hull
      item_magnet
      item_magnet
      item_hull)
  (out item_bullet))

 (item_condenser
  (in item_worker
      item_radiation
      item_radiation
      item_worker)
  (out item_condenser))

 (item_energy_store
  (energy 1)
  (in item_captor
      item_liquid_frame
      item_capacitor
      item_liquid_frame
      item_captor)
  (out item_energy_store))

 ;; (item_auto_deploy
 ;;  (energy 1)
 ;;  (in item_liquid_frame
 ;;      item_core
 ;;      item_deploy
 ;;      item_core
 ;;      item_liquid_frame)
 ;;  (out item_auto_deploy))

 ;; (item_transmitor
 ;;  (energy 1)
 ;;  (in item_radiation
 ;;      item_radiation
 ;;      item_antenna
 ;;      item_antenna
 ;;      item_antenna
 ;;      item_antenna
 ;;      item_antenna)
 ;;  (out item_transmitor))

 ;; (item_receptor
 ;;  (energy 1)
 ;;  (in item_antenna
 ;;      item_antenna
 ;;      item_antenna
 ;;      item_antenna
 ;;      item_antenna
 ;;      item_radiation
 ;;      item_radiation)
 ;;  (out item_receptor))

 (item_accelerator
  (energy 1)
  (in item_liquid_frame
      item_magnet_field
      item_magnet_field
      item_magnet_field
      item_liquid_frame)
  (out item_accelerator))

 ;; (item_collider
 ;;  (energy 1)
 ;;  (in item_lab
 ;;      item_accelerator
 ;;      item_accelerator
 ;;      item_accelerator
 ;;      item_accelerator
 ;;      item_accelerator
 ;;      item_lab)
 ;;  (out item_collider))
 )