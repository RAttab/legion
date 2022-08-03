;; [ EL ]-----------------------------------------------------------------------

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
;;
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
  (prober  (nodule eye lung))
  (scanner (nodule eye engram lung))
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
;;
;; Collider: every tape outputs garbage element o as well as the
;; desired element. Burner required to get rid of garbage
;; element. First unlocked element required to unlock nomad and then
;; the other three unlocked by nomad which unlocks all but one of the
;; remaining elements.
;;
;; Burner: Used to make collider work. Should probably have a duration
;; based on the item so that more then one is required. Energy output
;; and burn time should depend on the item being burned.
;;
;; Packer: Allows to make active items passive again. Can either take
;; an id or a a type and a count. If count is zero then all of type
;; are packed.
;;
;; Nomad: Main purpose is to exploit stars that can't sustain a legion
;; which will unlocks all but one of the natural
;; elements. Multi-function item with many io ops:
;;
;; - IO_ITEM: stores active items to be unloaded at
;;   destination. Storage has a cap on number of types and on stack
;;   size. On arrival all stored items are activated. Typical cargo:
;;   > brain / teleio
;;   > worker + solar
;;   > extract + condenser
;;   > port + pills
;;   > transmit + receive
;;
;; - IO_MOD: Can be configured with a mod that will be executed on the
;;   first activated brain. Required to make nomad
;;   automatable. Alternative would be to rely only teleio but that
;;   sounds annoying.
;;
;;  - IO_GET/IO_SET: Has a few words of memory and these IO ops will
;;    act exactly like memory. Required to hold things like the origin
;;    coord and whatever else state would be useful for
;;    automation. Alternative would again to rely exclusively on
;;    teleio.
;;
;;  - IO_LAUNCH: Send nomad to target and activate all stored
;;    items. Can take an id as argument to load the last brain before
;;    launching; solves the chicken-egg problem of the brain not being
;;    able to load itself into nomad while also sending the command to
;;    launch it.
;;
;; Teleio: Remote IO execution but can't return any data. Only
;; supports channel 0 and should probably be omni-directional to
;; simplify it's configuration when dealing with remote worlds. It's
;; main role is to give an alternative method of using Nomad or remote
;; colonies created by nomad: centralized control vs independent
;; function.
;;
;; Charge: Will take in battery as input and output charged
;; battery. Discharge will invert. charged batteries can't be deployed
;; but plain batteries can.
;;
;; Energy: Power is the limiting factor for how much you can do in a
;; single star. Otherwise, you can single star + nomad the entire game
;; which removes a lot of the fun of infinit expansion. As such once
;; T2 is done, need to spend time devising a diminishing return energy
;; scheme.
;;
;; T3 will probably ramp up the costs of tapes quite a bit given that
;; we'll have infinit access to resources.

(composite
 (collider
  (m o (a d g))) ;; a b c d g h -> m o

 (printer
  (biosteel o (b h m))
  (neurosteel o (d h m))
  (reactor o (a c h m)))

 (assembly
  (accelerator (conductor nerve field))
  (collider (brain nodule accelerator storage))

  (heat-exchange (biosteel conductor))

  ;; Relliance on m will require a bootstrap to store o before we can
  ;; burn it.
  (furnace (heat-exchange))
  (burner (furnace worker battery))

  (freezer (heat-exchange))
  (packer (furnace worker))

  (m-reactor (biosteel neurosteel heat-exhange))
  (m-condenser (biosteel field))
  (m-release (biosteel))
  (m-lung (m-reactor m-condenser m-release))

  (nomad (storage memory packer pill m-reactor m-lung))

  (teleio ())))

(nomad
 (collider
  (n o (b d e))
  (l o (h m n)))

 (printer
  ;; battery
  ;; charger/discharger - something conductive
  ;; library -
  )

 (assembly
  (battery)
  (charger)
  (discharger)
  (library)
  (eidetic-brain (brain library))) ;; printer item for (dis)charger


;; [ T3 ]-----------------------------------------------------------------------
 ;;
;; Major focus here is on eelem K which is omni present in small
;; quantities and very slowly replenishes over time.
 ;;
 ;; Tech unlocks are mostly around setting up the factories for K
 ;; which are required for everything in T4. Will also introduce
 ;; overseer which is a big automation layer for mass ressources
 ;; production and harvesting.
 ;;
 ;; Will also introduce the concept of channels for communication as
 ;; well as more memory/brain upgrades.

(siphon
 (assembly
  (siphon))) ;; l


;; [ T4 ]-----------------------------------------------------------------------
;;
;; This is the end game with big and weird projects to build which
;; should ultimately culminate into building Z which... ends the game?
