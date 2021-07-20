; ##########################################################
; logical

(math/not
 ()
 ((asm (not 0xFF)
       (not 1)
       (not 0))
  (yield))
 (sp:3 #0:0
       #1:0
       #2:1))

(math/and
 ()
 ((asm (and 0 1)
       (and 1 1)
       (and 1 2)
       (and 2 0)
       (and 1 1 1)
       (and 1 1 0)
       (and 1 0 1)
       (and 0 1 1))
  (yield))
 (sp:8 #0:0
       #1:1
       #2:1
       #3:0
       #4:1
       #5:0
       #6:0
       #7:0))

(math/or
 ()
 ((asm (or 0 1)
       (or 0 0)
       (or 1 2)
       (or 2 0)
       (or 0 0 0)
       (or 0 0 1)
       (or 0 1 0)
       (or 1 0 0))
  (yield))
 (sp:8 #0:1
       #1:0
       #2:1
       #3:1
       #4:0
       #5:1
       #6:1
       #7:1))

(math/xor
 ()
 ((asm (xor 0 1)
       (xor 0 0)
       (xor 1 2)
       (xor 2 0)
       (xor 0 0 0)
       (xor 0 1 0)
       (xor 1 1 1)
       (xor 1 0 0))
  (yield))
 (sp:8 #0:1
       #1:0
       #2:0
       #3:1
       #4:0
       #5:1
       #6:1
       #7:1))
