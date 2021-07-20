;; ##########################################################
;; fn

(misc/id
 ()
 ((id 1 2)
  (yield))
 (sp:1 #0:0x01000002))


;; ##########################################################
;; io

(misc/io-0
 ()
 ((io !io_nil (id 1 2)))
 (flags:0x1 io:1 ior:0xff sp:1 #0:0x4000000001000002))

(misc/io-1
 ()
 ((io !io_nil (id 1 2) 1))
 (flags:0x1 io:2 ior:0xff sp:2 #0:1 #1:0x4000000001000002))

(misc/io-2
 ()
 ((io !io_nil (id 1 2) 1 2))
 (flags:0x1 io:3 ior:0xff sp:3 #0:1 #1:2 #2:0x4000000001000002))


;; ##########################################################
;; ops

(misc/reset
 ()
 ((asm (PUSH 1)
       (POPR $0)
       (PUSH 1))
  (reset))
 (sp:0 $0:0 flags:0 ip:0))

(misc/tsc
 ()
 ((tsc)
  (yield))
 (sp:1))

(misc/fault
 ()
 ((fault))
 (flags:0x04))

(misc/pack
 ()
 ((pack 1 2)
  (yield))
 (sp:1 #0:0x0000000200000001))

(misc/unpack
 ()
 ((unpack (pack 1 2))
  (yield))
 (sp:2 #0:1 #1:2))
