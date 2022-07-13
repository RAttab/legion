;; -----------------------------------------------------------------------------
;; extract
;; -----------------------------------------------------------------------------

(!item-extract
 (!item-elem-a (out !item-elem-a))
 (!item-elem-b (out !item-elem-b))
 (!item-elem-c (out !item-elem-c))
 (!item-elem-d (out !item-elem-d)))


;; -----------------------------------------------------------------------------
;; printer
;; -----------------------------------------------------------------------------

(!item-printer

 (!item-muscle
  (in !item-elem-a
      !item-elem-a
      !item-elem-a)
  (out !item-muscle))

 (!item-nodule
  (in !item-elem-a
      !item-elem-b
      !item-elem-b
      !item-elem-a)
  (out !item-nodule))

 (!item-vein
  (in !item-elem-a
      !item-elem-c
      !item-elem-a)
  (out !item-vein))

 (!item-bone
  (in !item-elem-b
      !item-elem-b
      !item-elem-b
      !item-elem-b
      !item-elem-b)
  (out !item-bone))

 (!item-tendon
  (in !item-elem-b
      !item-elem-a
      !item-elem-a
      !item-elem-a
      !item-elem-b)
  (out !item-tendon))

 (!item-lens
  (in !item-elem-b
      !item-elem-c
      !item-elem-c
      !item-elem-b)
  (out !item-lens))

 (!item-nerve
  (in !item-elem-c
      !item-elem-c
      !item-elem-c)
  (out !item-nerve))

 (!item-neuron
  (in !item-elem-c
      !item-elem-a
      !item-elem-c
      !item-elem-a
      !item-elem-c)
  (out !item-neuron))

 (!item-retina
  (in !item-elem-c
      !item-elem-b
      !item-elem-b
      !item-elem-c)
  (out !item-retina)))


;; -----------------------------------------------------------------------------
;; assembly - passives
;; -----------------------------------------------------------------------------

(!item-assembly

 (!item-limb
  (in !item-bone
      !item-bone
      !item-tendon
      !item-tendon
      !item-muscle
      !item-muscle)
  (out !item-limb))

 (!item-spinal
  (in !item-nerve
      !item-vein
      !item-nerve
      !item-vein
      !item-nerve)
  (out !item-spinal))

 (!item-stem
  (in !item-neuron
      !item-neuron
      !item-vein
      !item-neuron
      !item-neuron)
  (out !item-stem))

 (!item-lung
  (in !item-stem
      !item-nerve
      !item-muscle
      !item-muscle
      !item-muscle
      !item-nerve)
  (out !item-lung))

 (!item-engram
  (in !item-nerve
      !item-neuron
      !item-neuron
      !item-nerve
      !item-neuron
      !item-neuron
      !item-nerve)
  (out !item-engram))

 (!item-cortex
  (in !item-stem
      !item-vein
      !item-neuron
      !item-neuron
      !item-neuron
      !item-vein
      !item-neuron
      !item-neuron
      !item-neuron
      !item-vein)
  (out !item-cortex))

 (!item-eye
  (in !item-retina
      !item-lens
      !item-lens
      !item-lens
      !item-retina)
  (out !item-eye)))


;; -----------------------------------------------------------------------------
;; assembly - active
;; -----------------------------------------------------------------------------

(!item-assembly

 (!item-deploy
  (in !item-spinal
      !item-nodule
      !item-spinal)
  (out !item-deploy))

 (!item-extract
  (in !item-muscle
      !item-muscle
      !item-nodule
      !item-muscle
      !item-muscle)
  (out !item-extract))

 (!item-printer
  (in !item-vein
      !item-vein
      !item-nodule
      !item-vein
      !item-vein)
  (out !item-printer))

 (!item-assembly
  (in !item-limb
      !item-limb
      !item-vein
      !item-nodule
      !item-vein
      !item-limb
      !item-limb)
  (out !item-assembly))

 (!item-worker
  (in !item-lung
      !item-lung
      !item-stem
      !item-nodule
      !item-stem
      !item-limb
      !item-limb)
  (out !item-worker))

 (!item-memory
  (in !item-engram
      !item-engram
      !item-engram
      !item-nodule
      !item-engram
      !item-engram
      !item-engram)
  (out !item-memory))

 (!item-brain
  (in !item-memory
      !item-cortex
      !item-cortex
      !item-nodule
      !item-cortex
      !item-cortex
      !item-memory)
  (out !item-brain))

 (!item-prober
  (in !item-lung
      !item-nodule
      !item-eye
      !item-eye
      !item-eye)
  (out !item-prober))

 (!item-scanner
  (in !item-lung
      !item-engram
      !item-nodule
      !item-eye
      !item-eye
      !item-eye
      !item-eye
      !item-eye)
  (out !item-scanner))

 (!item-legion
  (in !item-lung
      !item-lung
      !item-lung
      !item-nodule
      !item-worker
      !item-worker
      !item-extract
      !item-extract
      !item-printer
      !item-printer
      !item-assembly
      !item-assembly
      !item-deploy
      !item-deploy
      !item-memory
      !item-brain
      !item-prober
      !item-nodule
      !item-cortex
      !item-cortex)
  (out !item-legion))

 (!item-lab
  (in !item-limb
      !item-limb
      !item-vein
      !item-nodule
      !item-nodule
      !item-vein
      !item-cortex
      !item-cortex)
  (out !item-lab)))
