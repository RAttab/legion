;; -----------------------------------------------------------------------------
;; extract
;; -----------------------------------------------------------------------------

(!item-extract
 (!item-elem-a (work 1) (out !item-elem-a))
 (!item-elem-b (work 2) (out !item-elem-b))
 (!item-elem-c (work 2) (out !item-elem-c))
 (!item-elem-d (work 4) (out !item-elem-d)))


;; -----------------------------------------------------------------------------
;; printer
;; -----------------------------------------------------------------------------

(!item-printer

 (!item-muscle
  (work 2)
  (in !item-elem-a
      !item-elem-a
      !item-elem-a)
  (out !item-muscle))

 (!item-nodule
  (work 2)
  (in !item-elem-a
      !item-elem-b
      !item-elem-b
      !item-elem-a)
  (out !item-nodule))

 (!item-vein
  (work 2)
  (in !item-elem-a
      !item-elem-c
      !item-elem-a)
  (out !item-vein))

 (!item-bone
  (work 2)
  (in !item-elem-b
      !item-elem-b
      !item-elem-b
      !item-elem-b
      !item-elem-b)
  (out !item-bone))

 (!item-tendon
  (work 4)
  (in !item-elem-b
      !item-elem-a
      !item-elem-a
      !item-elem-a
      !item-elem-b)
  (out !item-tendon))

 (!item-rod
  (work 50)
  (in !item-elem-b
      !item-elem-a
      !item-elem-a
      !item-elem-a
      !item-elem-a
      !item-elem-a
      !item-elem-b
      !item-elem-a
      !item-elem-a
      !item-elem-a
      !item-elem-a
      !item-elem-a
      !item-elem-b)
  (out !item-rod))

 (!item-torus
  (work 4)
  (in !item-elem-b
      !item-elem-a
      !item-elem-a
      !item-elem-b
      !item-elem-b
      !item-elem-a
      !item-elem-a
      !item-elem-b)
  (out !item-torus))

 (!item-lens
  (work 2)
  (in !item-elem-b
      !item-elem-c
      !item-elem-c
      !item-elem-b)
  (out !item-lens))

 (!item-nerve
  (work 2)
  (in !item-elem-c
      !item-elem-c
      !item-elem-c)
  (out !item-nerve))

 (!item-neuron
  (work 4)
  (in !item-elem-c
      !item-elem-a
      !item-elem-c
      !item-elem-a
      !item-elem-c)
  (out !item-neuron))

 (!item-retina
  (work 4)
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
  (work 2)
  (in !item-bone
      !item-bone
      !item-tendon
      !item-tendon
      !item-muscle
      !item-muscle)
  (out !item-limb))

 (!item-spinal
  (work 2)
  (in !item-nerve
      !item-vein
      !item-nerve
      !item-vein
      !item-nerve)
  (out !item-spinal))

 (!item-stem
  (work 2)
  (in !item-neuron
      !item-neuron
      !item-vein
      !item-neuron
      !item-neuron)
  (out !item-stem))

 (!item-lung
  (work 3)
  (in !item-stem
      !item-nerve
      !item-muscle
      !item-muscle
      !item-muscle
      !item-nerve)
  (out !item-lung))

 (!item-engram
  (work 4)
  (in !item-nerve
      !item-neuron
      !item-neuron
      !item-nerve
      !item-neuron
      !item-neuron
      !item-nerve)
  (out !item-engram))

 (!item-cortex
  (work 8)
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
  (work 2)
  (in !item-retina
      !item-lens
      !item-lens
      !item-lens
      !item-retina)
  (out !item-eye))

 (!item-fusion
  (work 16)
  (in !item-torus
      !item-torus
      !item-stem
      !item-cortex
      !item-stem
      !item-torus
      !item-torus)
  (out !item-fusion)))


;; -----------------------------------------------------------------------------
;; assembly - active
;; -----------------------------------------------------------------------------

(!item-assembly

 (!item-deploy
  (work 4)
  (in !item-spinal
      !item-nodule
      !item-spinal)
  (out !item-deploy))

 (!item-extract
  (work 4)
  (in !item-muscle
      !item-muscle
      !item-nodule
      !item-muscle
      !item-muscle)
  (out !item-extract))

 (!item-printer
  (work 4)
  (in !item-vein
      !item-vein
      !item-nodule
      !item-vein
      !item-vein)
  (out !item-printer))

 (!item-assembly
  (work 4)
  (in !item-limb
      !item-limb
      !item-vein
      !item-nodule
      !item-vein
      !item-limb
      !item-limb)
  (out !item-assembly))

 (!item-worker
  (work 6)
  (in !item-lung
      !item-lung
      !item-stem
      !item-nodule
      !item-stem
      !item-limb
      !item-limb)
  (out !item-worker))

 (!item-memory
  (work 8)
  (in !item-engram
      !item-engram
      !item-engram
      !item-nodule
      !item-engram
      !item-engram
      !item-engram)
  (out !item-memory))

 (!item-brain
  (work 10)
  (in !item-memory
      !item-cortex
      !item-cortex
      !item-nodule
      !item-cortex
      !item-cortex
      !item-memory)
  (out !item-brain))

 (!item-prober
  (work 4)
  (in !item-lung
      !item-nodule
      !item-eye
      !item-eye
      !item-eye)
  (out !item-prober))

 (!item-scanner
  (work 6)
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
  (work 24)
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
  (work 16)
  (in !item-limb
      !item-limb
      !item-vein
      !item-nodule
      !item-nodule
      !item-vein
      !item-cortex
      !item-cortex)
  (out !item-lab)))
