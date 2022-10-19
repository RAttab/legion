(fusion

 (info
  (type active)
  (order last)
  (config im_fusion_config))

 (specs
  (input-item var !item-rod)

  (energy-out var 20)
  (energy-rod var 128)
  (energy-cap var 16384)

  (lab-bits var 4)
  (lab-work var 8)
  (lab-energy var 16))

 (tape
  (work 16)
  (energy 16)
  (host !item-assembly)

  (in !item-torus
      (!item-rod 2)
      !item-torus
      !item-stem
      !item-cortex
      !item-stem
      !item-torus
      (!item-rod 2)
      !item-torus)
  (out !item-fusion)))
