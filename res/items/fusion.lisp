(item-fusion

 (info
  (type active)
  (order last)
  (config im_fusion_config))

 (specs
  (fusion-input-item nil !item-rod)

  (fusion-energy-out nil 20)
  (fusion-energy-rod nil 128)
  (fusion-energy-cap nil 16384)

  (fusion-lab-bits lab-bits 4)
  (fusion-lab-work lab-work 8)
  (fusion-lab-energy lab-energy 16))

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
