;; -----------------------------------------------------------------------------
;; condenser
;; -----------------------------------------------------------------------------

(!item_condenser

 (!item_elem_g (energy 1) (out !item_elem_g))
 (!item_elem_h (energy 1) (out !item_elem_h)))


;; -----------------------------------------------------------------------------
;; printer
;; -----------------------------------------------------------------------------

(!item_printer

 (!item_magnet
  (in !item_elem_d
      !item_elem_d
      !item_elem_b
      !item_elem_b
      !item_elem_d
      !item_elem_d)
  (out !item_magnet))

 (!item_ferrofluid
  (in !item_elem_d
      !item_elem_c
      !item_elem_d
      !item_elem_c
      !item_elem_d)
  (out !item_ferrofluid))

 (!item_semiconductor
  (in !item_elem_d
      !item_elem_d
      !item_elem_a
      !item_elem_c
      !item_elem_c
      !item_elem_a
      !item_elem_d
      !item_elem_d)
  (out !item_semiconductor))

 (!item_conductor
  (in !item_elem_g
      !item_elem_g
      !item_elem_g
      !item_elem_d
      !item_elem_d
      !item_elem_g
      !item_elem_g
      !item_elem_g)
  (out !item_conductor))

 (!item_galvanic
  (in !item_elem_g
      !item_elem_h
      !item_elem_h
      !item_elem_g
      !item_elem_g
      !item_elem_h
      !item_elem_h
      !item_elem_g)
  (out !item_galvanic)))


;; -----------------------------------------------------------------------------
;; assembly - passive
;; -----------------------------------------------------------------------------

(!item_assembly

 (!item_photovoltaic
  (in !item_semiconductor
      !item_semiconductor
      !item_nerve
      !item_bone
      !item_bone
      !item_nerve
      !item_semiconductor
      !item_semiconductor)
  (out !item_photovoltaic))

 (!item_field
  (in !item_magnet
      !item_magnet
      !item_magnet
      !item_nerve
      !item_nerve
      !item_magnet
      !item_magnet
      !item_magnet)
  (out !item_field))

 (!item_antenna
  (in !item_bone
      !item_bone
      !item_nerve
      !item_conductor
      !item_conductor
      !item_nerve
      !item_conductor
      !item_conductor
      !item_nerve
      !item_bone
      !item_bone)
  (out !item_antenna))

 (!item_accelerator
  (in !item_conductor
      !item_conductor
      !item_nerve
      !item_field
      !item_field
      !item_nerve
      !item_conductor
      !item_conductor)
  (out !item_accelerator)))


;; -----------------------------------------------------------------------------
;; assembly - active
;; -----------------------------------------------------------------------------

(!item_assembly

 (!item_solar
  (in !item_photovoltaic
      !item_nerve
      !item_nodule
      !item_nerve
      !item_photovoltaic)
  (out !item_solar))

 (!item_storage
  (in !item_bone
      !item_bone
      !item_vein
      !item_vein
      !item_vein
      !item_bone
      !item_bone)
  (out !item_storage))

 (!item_port
  (in !item_field
      !item_field
      !item_field
      !item_field
      !item_field
      !item_nodule
      !item_limb
      !item_limb
      !item_limb
      !item_nodule
      !item_storage)
  (out !item_port))

 (!item_pill
  (in !item_vein
      !item_ferrofluid
      !item_vein
      !item_bone
      !item_bone
      !item_vein
      !item_ferrofluid
      !item_vein)
  (out !item_pill))

 (!item_condenser
  (in !item_lung
      !item_lung
      !item_nodule
      !item_lung
      !item_lung
      !item_lung)
  (out !item_condenser))

 (!item_battery
  (energy 1)
  (in !item_bone
      !item_nerve
      !item_galvanic
      !item_galvanic
      !item_galvanic
      !item_nerve
      !item_bone)
  (out !item_battery))

 ;; (!item_auto_deploy
 ;;  (energy 1)
 ;;  (in )
 ;;  (out !item_auto_deploy))

 (!item_transmit
  (energy 20)
  (in !item_eye
      !item_eye
      !item_eye
      !item_nodule
      !item_antenna
      !item_antenna
      !item_antenna)
  (out !item_transmit))

 (!item_receive
  (energy 20)
  (in !item_antenna
      !item_antenna
      !item_antenna
      !item_nodule
      !item_engram
      !item_engram
      !item_engram)
  (out !item_receive))

 (!item_collider
  (energy 100)
  (in !item_brain
      !item_nodule
      !item_accelerator
      !item_accelerator
      !item_nodule
      !item_accelerator
      !item_accelerator
      !item_nodule)
  (out !item_collider)))
