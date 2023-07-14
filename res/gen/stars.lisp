(barren
 (hue 270)
 (weight 200)
 (rolls
  (one !item-energy 100)
  (one !item-elem-k 100)
  (rng !item-elem-a !item-elem-j 100)))

(starter
 (hue 0)
 (weight 50)
 (rolls
  (one !item-energy 30000)
  (one !item-elem-k 100)
  (all-of !item-elem-a !item-elem-c 60000)
  (all-of !item-elem-d !item-elem-f 30000)))

(nomad-elem-common
 (hue 180)
 (weight 350)
 (rolls
  (one !item-energy 10000)
  (one !item-elem-k 100)
  (rng !item-elem-a !item-elem-j 1000)
  (one-of !item-elem-g !item-elem-h 60000)))

(nomad-elem-rare
 (hue 120)
 (weight 10)
 (rolls
  (one !item-energy 10000)
  (one !item-elem-k 100)
  (rng !item-elem-a !item-elem-j 1000)
  (one-of !item-elem-i !item-elem-j 10000)))

(power
 (hue 60)
 (weight 150)
 (rolls
  (one !item-elem-k 60000)
  (one !item-energy 60000)))
