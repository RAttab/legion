;; ==========================================================
;; assert

(misc/assert-false
 (mod (assert 0))
 (check (flags 0x04)))

(misc/assert-true
 (mod (assert 1)))


;; ==========================================================
;; fn

(misc/id
 (mod (assert (= 0x01000002 (id 1 2)))))


;; ==========================================================
;; io

(misc/io-0
 (mod (io !io_nil (id 1 2)))
 (check (flags 0x1) (io 1) (ior 0xff) (sp 1) (s 0 0x4000000001000002)))

(misc/io-1
 (mod (io !io_nil (id 1 2) 1))
 (check (flags 0x1) (io 2) (ior 0xff)
	(sp 2) (s 0 0x4000000001000002) (s 1 1)))

(misc/io-2
 (mod (io !io_nil (id 1 2) 1 2))
 (check (flags 0x1) (io 3) (ior 0xff)
	(sp 3) (s 0 0x4000000001000002) (s 1 1) (s 2 2)))


;; ==========================================================
;; set

(misc/set
 (mod (assert (= 2 (let ((a 1)) (set a 2) a)))
      (yield))
 (check (r 0 2) (sp 0)))


;; ==========================================================
;; ops

(misc/reset
 (mod
  (asm (PUSH 1)
       (POPR $0)
       (PUSH 1))
  (reset))
 (check (sp 0) (r 0 0) (ip 0)))

(misc/tsc
 (mod (assert (> (tsc) 0))))

(misc/fault
 (mod (fault))
 (check (flags 0x04)))

(misc/pack
 (mod (assert (= 0x0000000200000001 (pack 1 2)))))

(misc/unpack
 (mod
  (let ((top (unpack (pack 1 2)))
	(bot (head)))
    (assert (= top 1))
    (assert (= bot 2)))))

(misc/mod-self
 (mod (assert (= 2 (band (mod) 0xFFFF)))))

(misc/mod-other
 (mod (assert (= 2 (band (mod misc/mod-self) 0xFFFF)))))
