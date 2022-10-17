;; -----------------------------------------------------------------------------
;; extract / condenser
;; -----------------------------------------------------------------------------

(!item-extract

 (!item-elem-e
  (work 8)
  (energy 32)
  (out !item-elem-e))

 (!item-elem-f
  (work 12)
  (energy 48)
  (out !item-elem-f)))


(!item-condenser

 (!item-elem-i
  (work 16)
  (energy 64)
  (out !item-elem-i))

 (!item-elem-j
  (work 20)
  (energy 128)
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
  (work 16)
  (energy 128)
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
  (work 24)
  (energy 256)
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
  (work 32)
  (energy 1024)
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
;; dummy
;; -----------------------------------------------------------------------------

(!item-dummy
 (!item-elem-o))


;; -----------------------------------------------------------------------------
;; printer
;; -----------------------------------------------------------------------------

(!item-printer

 (!item-biosteel
  (work 16)
  (energy 64)
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
  (work 16)
  (energy 72)
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
       !item-elem-o)))


;; -----------------------------------------------------------------------------
;; assembly - passive
;; -----------------------------------------------------------------------------

(!item-assembly

 (!item-accelerator
  (work 16)
  (energy 64)
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
  (work 16)
  (energy 72)
  (in !item-biosteel
      !item-conductor
      !item-conductor
      !item-conductor
      !item-biosteel)
  (out !item-heat-exchange))

 (!item-furnace
  (work 20)
  (energy 76)
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
  (work 20)
  (energy 76)
  (in !item-heat-exchange
      !item-heat-exchange
      !item-neurosteel
      !item-elem-m
      !item-neurosteel
      !item-heat-exchange
      !item-heat-exchange)
  (out !item-freezer))

 (!item-m-reactor
  (work 32)
  (energy 128)
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
  (work 32)
  (energy 128)
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
  (work 32)
  (energy 144)
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
  (work 40)
  (energy 256)
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
  (work 16)
  (energy 128)
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
  (work 20)
  (energy 112)
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
  (work 20)
  (energy 112)
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
  (work 42)
  (energy 256)
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
