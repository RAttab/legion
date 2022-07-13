(starter
 (hue 0)
 (weight 50)
 (rolls
  (one !item-energy 30000)
  (one !item-elem-k 100)
  (all-of !item-elem-a !item-elem-d 60000)
  (all-of !item-elem-g !item-elem-h 30000)))

(barren
 (hue 270)
 (weight 200)
 (rolls
  (one !item-energy 100)
  (one !item-elem-k 100)
  (rng !item-elem-a !item-elem-j 100)))

(extract
 (hue 180)
 (weight 350)
 (rolls
  (one !item-energy 10000)
  (one !item-elem-k 100)
  (rng !item-elem-a !item-elem-j 1000)
  (one-of !item-elem-a !item-elem-f 65000)))

(condenser
 (hue 120)
 (weight 250)
 (rolls
  (one !item-energy 10000)
  (one !item-elem-k 100)
  (rng !item-elem-a !item-elem-j 1000)
  (one-of !item-elem-g !item-elem-j 65000)))

(power
 (hue 60)
 (weight 150)
 (rolls
  (one !item-elem-k 65000)
  (one !item-energy 65000)))
