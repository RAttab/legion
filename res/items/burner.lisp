(burner

 (info
  (type active)
  (order first)
  (config im_burner_config))

 (specs
  (lab-bits var 4)
  (lab-work var 8)
  (lab-energy var 16))

 (tape
  (work 20)
  (energy 112)
  (host !item-assembly)
  (in !item-biosteel
      !item-furnace
      !item-furnace
      !item-furnace
      !item-heat-exchange
      !item-conductor
      !item-battery
      !item-battery
      !item-battery
      !item-biosteel)
  (out !item-burner)))
