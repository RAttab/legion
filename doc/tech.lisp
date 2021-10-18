(elems
 ((a b c d) extract solids)
 ((e f)     extract solids-nomad)

 ((g h)     condenser gas)
 ((i j)     condenser gas-nomad)

 ((k)       siphon)

 ((l m n o) collider composite)
 ((p q)     collider composite-nomad)

 ((r s t)   k-easy)
 ((u v)     k-hard)
 ((w x y)   meld)
 ((z)       eog))


;; [ T0 ]-----------------------------------------------------------------------
;;
;; Goal is to achive self-replication via production of a legion. Acts
;; as the tutorial and worlds won't be connected until T1.


;; This phase is all about booting until you're able to make all
;; available tapes. It's pretty rigid due to imposed limitations of
;; the starting set.
(boot-factory
 (printer
  (muscle (a))
  (nodule (a b))
  (vein   (a c))
  (bone   (b))
  (tendon (b a)))

 (assembly
  (limb (bone tendon muscle))
  (spinal (nerve vein))

  (extract ;; 2-ex 2-pr 1-as
   (nodule muscle))
  (printer ;; 2+ex 2-pr 1-as
   (nodule vein))
  (assembly ;; 2+ex 2+pr, 2-as
   (nodule limb vein))
  (deploy ;; Simple. intro to logic passives
   (nodule spinal))))


;; Now that we have a basic factory going we can start working towards
;; producing legions and expanding to other stars.
(boot-legion
 (printer
  (lens   (b c))
  (nerve  (c))
  (neuron (c a))
  (retina (c b)))

 (assembly
  (stem    (neuron nerve))
  (lung    (stem nerve muscle))
  (engram  (neuron nerve))
  (cortex  (stem neuron vein))
  (eye     (retina lens))

  (worker  (nodule stem limb lung))
  (memory  (nodule engram))
  (brain   (nodule cortex memory))
  (scanner (nodule eye lung))
  (legion  (nodule lung cortex ...))
  (lab     (nodule cortex limb vein))))


;; [ T1 ]-----------------------------------------------------------------------
;;
;; Goal is to introduce new mechanics:
;; - Research
;; - Energy logistics
;; - Pill logistics
;; - Data layer
;;
;; Enables the connection of various stars together and forms the
;; basis of the higher order logisitcs network.

(expand-lab
 (printer
  (magnet (d b))
  (ferrofluid ;; magnetic fluid - https://en.wikipedia.org/wiki/Ferrofluid
   (d c))
  (semiconductor (d a c)))

 (assembly
  (photovoltaic (semiconductor bone nerve))
  (field        (magnet nerve))

  (solar     (nodule photovoltaic nerve))
  (storage   (bone vein))
  (port      (nodule field storage limb))
  (pill      (bone vein ferrofluid))
  (condenser (nodule lung))))

(expand-gas
 (printer
  (conductor (g d))
  (galvanic  (g h)))

 (assembly
  (antenna     (conductor bone nerve))
  (accelerator (conductor field nerve))

  (battery   (galvanic bone nerve)) ;; can be used as an active and also a passive
  (receive   (nodule antenna engram))
  (transmit  (nodule antenna eye))
  (collider  (nodule accelerator brain))))


;; [ T2 ]-----------------------------------------------------------------------
;;
;; Goal is to start increasing factory depth and cost to break-out of
;; single star phase. Main new mechanic is nomad which allows for the
;; exploitation of stars not viable for a legion boot.

(composite
 (collider)
 (assembly
  (nomad ())))

(nomad)