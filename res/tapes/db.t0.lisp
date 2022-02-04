;; -----------------------------------------------------------------------------
;; extract
;; -----------------------------------------------------------------------------

(!item_extract
 (!item_elem_a (out !item_elem_a))
 (!item_elem_b (out !item_elem_b))
 (!item_elem_c (out !item_elem_c))
 (!item_elem_d (out !item_elem_d)))


;; -----------------------------------------------------------------------------
;; printer
;; -----------------------------------------------------------------------------

(!item_printer

 (!item_muscle
  (in !item_elem_a
      !item_elem_a
      !item_elem_a)
  (out !item_muscle))

 (!item_nodule
  (in !item_elem_a
      !item_elem_b
      !item_elem_b
      !item_elem_a)
  (out !item_nodule))

 (!item_vein
  (in !item_elem_a
      !item_elem_c
      !item_elem_a)
  (out !item_vein))

 (!item_bone
  (in !item_elem_b
      !item_elem_b
      !item_elem_b
      !item_elem_b
      !item_elem_b)
  (out !item_bone))

 (!item_tendon
  (in !item_elem_b
      !item_elem_a
      !item_elem_a
      !item_elem_a
      !item_elem_b)
  (out !item_tendon))

 (!item_lens
  (in !item_elem_b
      !item_elem_c
      !item_elem_c
      !item_elem_b)
  (out !item_lens))

 (!item_nerve
  (in !item_elem_c
      !item_elem_c
      !item_elem_c)
  (out !item_nerve))

 (!item_neuron
  (in !item_elem_c
      !item_elem_a
      !item_elem_c
      !item_elem_a
      !item_elem_c)
  (out !item_neuron))

 (!item_retina
  (in !item_elem_c
      !item_elem_b
      !item_elem_b
      !item_elem_c)
  (out !item_retina)))


;; -----------------------------------------------------------------------------
;; assembly - passives
;; -----------------------------------------------------------------------------

(!item_assembly

 (!item_limb
  (in !item_bone
      !item_bone
      !item_tendon
      !item_tendon
      !item_muscle
      !item_muscle)
  (out !item_limb))

 (!item_spinal
  (in !item_nerve
      !item_vein
      !item_nerve
      !item_vein
      !item_nerve)
  (out !item_spinal))

 (!item_stem
  (in !item_neuron
      !item_neuron
      !item_vein
      !item_neuron
      !item_neuron)
  (out !item_stem))

 (!item_lung
  (in !item_stem
      !item_nerve
      !item_muscle
      !item_muscle
      !item_muscle
      !item_nerve)
  (out !item_lung))

 (!item_engram
  (in !item_nerve
      !item_neuron
      !item_neuron
      !item_nerve
      !item_neuron
      !item_neuron
      !item_nerve)
  (out !item_engram))

 (!item_cortex
  (in !item_stem
      !item_vein
      !item_neuron
      !item_neuron
      !item_neuron
      !item_vein
      !item_neuron
      !item_neuron
      !item_neuron
      !item_vein)
  (out !item_cortex))

 (!item_eye
  (in !item_retina
      !item_lens
      !item_lens
      !item_lens
      !item_retina)
  (out !item_eye)))


;; -----------------------------------------------------------------------------
;; assembly - active
;; -----------------------------------------------------------------------------

(!item_assembly

 (!item_deploy
  (in !item_spinal
      !item_nodule
      !item_spinal)
  (out !item_deploy))

 (!item_extract
  (in !item_muscle
      !item_muscle
      !item_nodule
      !item_muscle
      !item_muscle)
  (out !item_extract))

 (!item_printer
  (in !item_vein
      !item_vein
      !item_nodule
      !item_vein
      !item_vein)
  (out !item_printer))

 (!item_assembly
  (in !item_limb
      !item_limb
      !item_vein
      !item_nodule
      !item_vein
      !item_limb
      !item_limb)
  (out !item_assembly))

 (!item_worker
  (in !item_lung
      !item_lung
      !item_stem
      !item_nodule
      !item_stem
      !item_limb
      !item_limb)
  (out !item_worker))

 (!item_memory
  (in !item_engram
      !item_engram
      !item_engram
      !item_nodule
      !item_engram
      !item_engram
      !item_engram)
  (out !item_memory))

 (!item_brain
  (in !item_memory
      !item_cortex
      !item_cortex
      !item_nodule
      !item_cortex
      !item_cortex
      !item_memory)
  (out !item_brain))

 (!item_scanner
  (in !item_lung
      !item_nodule
      !item_eye
      !item_eye
      !item_eye
      !item_eye
      !item_eye)
  (out !item_scanner))

 (!item_legion
  (in !item_lung
      !item_lung
      !item_lung
      !item_nodule
      !item_worker
      !item_worker
      !item_extract
      !item_extract
      !item_printer
      !item_printer
      !item_assembly
      !item_assembly
      !item_deploy
      !item_deploy
      !item_memory
      !item_brain
      !item_scanner
      !item_nodule
      !item_cortex
      !item_cortex)
  (out !item_legion))

 (!item_lab
  (in !item_limb
      !item_limb
      !item_vein
      !item_nodule
      !item_nodule
      !item_vein
      !item_cortex
      !item_cortex)
  (out !item_lab)))
