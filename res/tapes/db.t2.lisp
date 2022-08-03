;; -----------------------------------------------------------------------------
;; extract / condenser
;; -----------------------------------------------------------------------------

(!item-extract

 (!item-elem-e
  (energy 1)
  (out !item-elem-e))

 (!item-elem-f
  (energy 10)
  (out !item-elem-f)))


(!item-condenser

 (!item-elem-i
  (energy 10)
  (out !item-elem-i))

 (!item-elem-j
  (energy 100)
  (in !item-elem-m)
  (out !item-elem-j)))


;; -----------------------------------------------------------------------------
;; collider
;; -----------------------------------------------------------------------------
;; The collider will not process the outputs of the tape and is only
;; included for the dependency graph. Which element is outputed and in
;; what quantity is determined within collider_im.c

(!item-collider

 (!item-elem-m
  (energy 10)
  (work 100)
  (in !item-elem-a
      !item-elem-a
      !item-elem-a
      !item-elem-d
      !item-elem-d
      !item-elem-g
      !item-elem-d
      !item-elem-d
      !item-elem-a
      !item-elem-a
      !item-elem-a)
  (out !item-elem-m
       !item-elem-o))

 (!item-elem-n
  (energy 100)
  (work 200)
  (in !item-elem-b
      !item-elem-b
      !item-elem-b
      !item-elem-b
      !item-elem-b
      !item-elem-d
      !item-elem-d
      !item-elem-d
      !item-elem-d
      !item-elem-e
      !item-elem-e
      !item-elem-e
      !item-elem-d
      !item-elem-d
      !item-elem-d
      !item-elem-b
      !item-elem-b
      !item-elem-b
      !item-elem-b
      !item-elem-b)
  (out !item-elem-n
       !item-elem-o))

 (!item-elem-l
  (work 1000)
  (energy 1000)
  (in !item-elem-h
      !item-elem-h
      !item-elem-h
      !item-elem-h
      !item-elem-h
      !item-elem-m
      !item-elem-m
      !item-elem-m
      !item-elem-m
      !item-elem-n
      !item-elem-n
      !item-elem-n
      !item-elem-n
      !item-elem-m
      !item-elem-m
      !item-elem-m
      !item-elem-m
      !item-elem-h
      !item-elem-h
      !item-elem-h
      !item-elem-h
      !item-elem-h)
  (out !item-elem-l
       !item-elem-o)))


;; -----------------------------------------------------------------------------
;; printer
;; -----------------------------------------------------------------------------

(!item-printer
 
 (!item-biosteel
  (in !item-elem-b
      !item-elem-b
      !item-elem-h
      !item-elem-b
      !item-elem-b
      !item-elem-m
      !item-elem-b
      !item-elem-b
      !item-elem-h
      !item-elem-b
      !item-elem-b)
  (out !item-elem-o
       !item-biosteel
       !item-elem-o))

 (!item-neurosteel
  (in !item-elem-d
      !item-elem-d
      !item-elem-h
      !item-elem-d
      !item-elem-d
      !item-elem-m
      !item-elem-d
      !item-elem-d
      !item-elem-h
      !item-elem-d
      !item-elem-d)
  (out !item-elem-o
       !item-neurosteel
       !item-elem-o))

 (!item-reactor
  (in !item-elem-a
      !item-elem-a
      !item-elem-a
      !item-elem-c
      !item-elem-c
      !item-elem-h
      !item-elem-m
      !item-elem-h
      !item-elem-m
      !item-elem-h
      !item-elem-m
      !item-elem-h
      !item-elem-c
      !item-elem-c
      !item-elem-a
      !item-elem-a
      !item-elem-a)
  (out !item-elem-o
       !item-elem-o
       !item-reactor
       !item-elem-o
       !item-elem-o)))


;; -----------------------------------------------------------------------------
;; assembly - passive
;; -----------------------------------------------------------------------------

(!item-assembly

 (!item-accelerator
  (in !item-conductor
      !item-conductor
      !item-nerve
      !item-field
      !item-field
      !item-nerve
      !item-conductor
      !item-conductor)
  (out !item-accelerator))

 (!item-heat-exchange
  (in !item-biosteel
      !item-conductor
      !item-conductor
      !item-conductor
      !item-biosteel)
  (out !item-heat-exchange))

 (!item-furnace
  (in !item-biosteel
      !item-heat-exchange
      !item-heat-exchange
      !item-elem-m
      !item-heat-exchange
      !item-heat-exchange
      !item-biosteel)
  (out !item-elem-o
       !item-furnace
       !item-elem-o))

 (!item-freezer
  (in !item-heat-exchange
      !item-heat-exchange
      !item-neurosteel
      !item-elem-m
      !item-neurosteel
      !item-heat-exchange
      !item-heat-exchange)
  (out !item-freezer))

 (!item-m-reactor
  (in !item-biosteel
      !item-heat-exchange
      !item-neurosteel
      !item-elem-m
      !item-elem-m
      !item-neurosteel
      !item-elem-m
      !item-neurosteel
      !item-elem-m
      !item-elem-m
      !item-neurosteel
      !item-heat-exchange
      !item-biosteel)
  (out !item-elem-o
       !item-m-reactor
       !item-elem-o))

 (!item-m-condenser
  (in !item-biosteel
      !item-field
      !item-elem-m
      !item-elem-m
      !item-field
      !item-elem-m
      !item-field
      !item-elem-m
      !item-elem-m
      !item-field
      !item-biosteel)
  (out !item-elem-o
       !item-m-condenser
       !item-elem-o))

 (!item-m-release
  (in !item-biosteel
      !item-ferrofluid
      !item-ferrofluid
      !item-ferrofluid
      !item-elem-m
      !item-ferrofluid
      !item-ferrofluid
      !item-ferrofluid
      !item-biosteel)
  (out !item-elem-o
       !item-m-release
       !item-elem-o))

 (!item-m-lung
  (in !item-biosteel
      !item-m-reactor
      !item-neurosteel
      !item-m-condenser
      !item-m-condenser
      !item-neurosteel
      !item-m-release
      !item-m-release
      !item-m-release)
  (out !item-m-lung)))


;; -----------------------------------------------------------------------------
;; assembly - active
;; -----------------------------------------------------------------------------

(!item-assembly

 (!item-collider
  (energy 100)
  (in !item-brain
      !item-nodule
      !item-accelerator
      !item-accelerator
      !item-nodule
      !item-accelerator
      !item-accelerator
      !item-nodule
      !item-storage)
  (out !item-collider))

 (!item-burner
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
  (out !item-burner))

 (!item-packer
  (in !item-biosteel
      !item-neurosteel
      !item-freezer
      !item-freezer
      !item-freezer
      !item-worker
      !item-storage
      !item-storage
      !item-storage
      !item-neurosteel
      !item-biosteel)
  (out !item-packer))

 (!item-nomad
  (in !item-biosteel
      !item-pill
      !item-brain
      !item-memory
      !item-pill
      !item-packer
      !item-storage
      !item-storage
      !item-storage
      !item-pill
      !item-biosteel
      !item-m-lung
      !item-m-lung)
  (out !item-nomad)))
