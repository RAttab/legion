;; ==========================================================
;; assert

(misc/assert-false
 (mod (assert 0))
 (check (flags 0x04)))

(misc/assert-true
 (mod (assert 1))
 (check))

;; ==========================================================
;; atoms

(misc/atoms
 (mod (assert (= ?alice 0x80000000))
      (assert (= !alice 0x80000000))
      (assert (= ?bob 0x80000001))
      (assert (= !bob 0x80000001))
      (assert (= !alice 0x80000000)))
 (check))


;; ==========================================================
;; fn

(misc/id
 (mod (assert (= 0x0102 (id 1 2))))
 (check))


;; ==========================================================
;; io

(misc/io-0
 (mod (io ?io-nil (id 1 2)))
 (check (flags 0x1) (io 1) (sp 1) (s 0 0x010000000102)))

(misc/io-1
 (mod (io ?io-nil (id 1 2) 1))
 (check (flags 0x1) (io 2)
	(sp 2) (s 0 0x010000000102) (s 1 1)))

(misc/io-2
 (mod (io ?io-nil (id 1 2) 1 2))
 (check (flags 0x1) (io 3)
	(sp 3) (s 0 0x010000000102) (s 1 1) (s 2 2)))


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
 (mod (assert (> (tsc) 0)))
 (check))

(misc/fault
 (mod (fault))
 (check (flags 0x04)))

(misc/pack
 (mod (assert (= 0x0000000200000001 (pack 1 2))))
 (check))

(misc/unpack
 (mod
  (let ((top (unpack (pack 1 2)))
	(bot (head)))
    (assert (= top 1))
    (assert (= bot 2))))
 (check))

(misc/mod-self
 (mod (assert (= 2 (band (mod) 0xFFFF))))
 (check))

(misc/mod-other
 (mod (assert (= 1 (band (mod misc/mod-self) 0xFFFF))))
 (check))

(misc/mod-version
 (mod (assert (= 1 (band (mod misc/mod-self.1) 0xFFFF))))
 (check))

(misc/specs-var
 (mod (specs !spec-test-var))
  (check (flags 0x1) (io 2)
	 (sp 2)
	 (s 0 0x0000011f00000000)
	 (s 1 0x0000000000001ff0)))

(misc/specs-fn
 (mod (specs !spec-test-fn 3 2))
  (check (flags 0x1) (io 4)
	 (sp 4)
	 (s 0 0x0000011f00000000)
	 (s 1 0x0000000000001ff1)
	 (s 2 0x0000000000000003)
	 (s 3 0x0000000000000002)))
