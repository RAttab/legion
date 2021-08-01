;; ##########################################################
;; assert

(misc/assert-false
 ()
 ((assert 0))
 (flags:04))

(misc/assert-true
 ()
 ((assert 1)
  (yield))
 (flags:00))


;; ##########################################################
;; fn

(misc/id
 ()
 ((assert (= 0x01000002 (id 1 2)))
  (yield))
 (sp:0 flags:0))


;; ##########################################################
;; io

(misc/io-0
 ()
 ((io !io_nil (id 1 2)))
 (flags:0x1 io:1 ior:0xff sp:1 #0:0x4000000001000002))

(misc/io-1
 ()
 ((io !io_nil (id 1 2) 1))
 (flags:0x1 io:2 ior:0xff sp:2 #0:0x4000000001000002 #1:1))

(misc/io-2
 ()
 ((io !io_nil (id 1 2) 1 2))
 (flags:0x1 io:3 ior:0xff sp:3 #0:0x4000000001000002 #1:1 #2:2))


;; ##########################################################
;; set

(misc/set
 ()
 ((assert (= 2 (let ((a 1)) (set a 2) a)))
  (yield))
 ($0:2 sp:0 flags:0))


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
 ((assert (> (tsc) 0))
  (yield))
 (sp:0 flags:0))

(misc/fault
 ()
 ((fault))
 (flags:0x04))

(misc/pack
 ()
 ((assert (= 0x0000000200000001 (pack 1 2)))
  (yield))
 (sp:0 flags:0))

(misc/unpack
 ()
 ((let ((top (unpack (pack 1 2)))
	(bot (head)))
    (assert (= top 1))
    (assert (= bot 2)))
  (yield))
 (sp:0 flags:0))

(misc/mod-self
 ()
 ((assert (= 2 (band (mod) 0xFFFF))))
 (sp:0 flags:0))

(misc/mod-other
 ()
 ((assert (= 2 (band (mod misc/mod-self) 0xFFFF))))
 (sp:0 flags:0))
