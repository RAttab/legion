(io io-nil
    io-ok
    io-fail

    ;; phase
    io-step
    io-arrive
    io-return

    ;; common
    io-ping
    io-pong
    io-state
    io-activate
    io-reset
    io-id
    io-item
    io-tape
    io-mod
    io-loop
    io-value
    io-target

    ;; brain
    io-log
    io-tick
    io-coord
    io-name
    io-send
    io-recv
    io-dbg-attach
    io-dbg-detach
    io-dbg-break
    io-dbg-step
    io-specs

    ;; memory
    io-get
    io-set
    io-cas

    ;; library
    io-tape-in
    io-tape-out
    io-tape-tech
    io-tape-host
    io-tape-work
    io-tape-energy

    ;; lab
    io-tape-at
    io-tape-known
    io-item-bits
    io-item-known

    ;; transmit / receive
    io-channel
    io-transmit
    io-receive

    ;; scanner / probe
    io-scan
    io-probe
    io-count

    ;; packer nomad
    io-pack
    io-load
    io-unload

    io-launch ;; legion / nomad
    io-grow   ;; collider
    io-input  ;; port

    ;; State
    io-has-item
    io-has-loop
    io-size
    io-rate
    io-work
    io-output
    io-cargo
    io-energy
    io-active)

(ioe ioe-missing-arg
     ioe-invalid-state
     ioe-vm-fault
     ioe-starved
     ioe-out-of-range
     ioe-out-of-space
     ioe-invalid-spec

     ioe-a0-invalid
     ioe-a0-unknown
     ioe-a1-invalid
     ioe-a1-unknown)
