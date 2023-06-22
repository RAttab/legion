(elem-a
  (info (type natural))
  (specs (lab-bits u8 1) (lab-work work 1) (lab-energy energy 1))
  (tape (work 1) (energy 1) (host item-extract)
    (out (item-elem-a 1)))
  (dbg
    (info (id 1) (layer 0))
    (work (min 1) (total 1))
    (energy 1)
    (children 0)
    (needs 1
      (01 elem-a 1))))

(elem-b
  (info (type natural))
  (specs (lab-bits u8 1) (lab-work work 1) (lab-energy energy 1))
  (tape (work 2) (energy 2) (host item-extract)
    (out (item-elem-b 1)))
  (dbg
    (info (id 2) (layer 0))
    (work (min 2) (total 2))
    (energy 4)
    (children 0)
    (needs 1
      (02 elem-b 1))))

(elem-c
  (info (type natural))
  (specs (lab-bits u8 1) (lab-work work 1) (lab-energy energy 1))
  (tape (work 2) (energy 4) (host item-extract)
    (out (item-elem-c 1)))
  (dbg
    (info (id 3) (layer 0))
    (work (min 2) (total 2))
    (energy 8)
    (children 0)
    (needs 1
      (03 elem-c 1))))

(elem-o
  (info (type synth))
  (specs (lab-bits u8 1) (lab-work work 1) (lab-energy energy 1))
  (tape (work 1) (energy 1) (host item-dummy)
    (out (item-elem-o 1)))
  (dbg
    (info (id 4) (layer 0))
    (work (min 1) (total 1))
    (energy 1)
    (children 0)
    (needs 1
      (04 elem-o 1))))

(elem-g
  (info (type natural))
  (specs (lab-bits u8 1) (lab-work work 1) (lab-energy energy 1))
  (tape (work 16) (energy 32) (host item-extract)
    (out (item-elem-g 1)))
  (dbg
    (info (id 5) (layer 0))
    (work (min 16) (total 16))
    (energy 512)
    (children 0)
    (needs 1
      (05 elem-g 1))))

(elem-h
  (info (type natural))
  (specs (lab-bits u8 1) (lab-work work 1) (lab-energy energy 1))
  (tape (work 16) (energy 42) (host item-condenser)
    (out (item-elem-h 1)))
  (dbg
    (info (id 6) (layer 0))
    (work (min 16) (total 16))
    (energy 672)
    (children 0)
    (needs 1
      (06 elem-h 1))))

(elem-d
  (info (type natural))
  (specs (lab-bits u8 4) (lab-work work 13) (lab-energy energy 1))
  (tape (work 4) (energy 6) (host item-extract)
    (in (item-elem-b 2))
    (out (item-elem-d 1)))
  (dbg
    (info (id 10) (layer 1))
    (work (min 4) (total 4))
    (energy 24)
    (children 1
      (02 elem-b 2))
    (needs 2
      (02 elem-b 2)
      (10 elem-d 1))))

(monobarex
  (info (type passive))
  (specs (lab-bits u8 5) (lab-work work 18) (lab-energy energy 2))
  (tape (work 3) (energy 3) (host item-printer)
    (in (item-elem-a 5)
        (item-elem-b 1))
    (out (item-monobarex 1)))
  (dbg
    (info (id 11) (layer 1))
    (work (min 3) (total 10))
    (energy 18)
    (children 2
      (01 elem-a 5)
      (02 elem-b 1))
    (needs 2
      (01 elem-a 5)
      (02 elem-b 1))))

(monobararkon
  (info (type passive))
  (specs (lab-bits u8 2) (lab-work work 12) (lab-energy energy 2))
  (tape (work 3) (energy 3) (host item-printer)
    (in (item-elem-b 9)
        (item-elem-a 1)
        (item-elem-b 1)
        (item-elem-a 2))
    (out (item-monobararkon 1)))
  (dbg
    (info (id 12) (layer 1))
    (work (min 3) (total 26))
    (energy 52)
    (children 2
      (01 elem-a 3)
      (02 elem-b 10))
    (needs 2
      (01 elem-a 3)
      (02 elem-b 10))))

(monocharkoid
  (info (type passive))
  (specs (lab-bits u8 3) (lab-work work 11) (lab-energy energy 1))
  (tape (work 3) (energy 6) (host item-printer)
    (in (item-elem-c 2)
        (item-elem-a 1)
        (item-elem-c 1)
        (item-elem-a 1)
        (item-elem-c 2))
    (out (item-monocharkoid 1)))
  (dbg
    (info (id 13) (layer 1))
    (work (min 3) (total 15))
    (energy 60)
    (children 2
      (01 elem-a 2)
      (03 elem-c 5))
    (needs 2
      (01 elem-a 2)
      (03 elem-c 5))))

(elem-e
  (info (type natural))
  (specs (lab-bits u8 3) (lab-work work 10) (lab-energy energy 1))
  (tape (work 8) (energy 4) (host item-condenser)
    (in (item-elem-a 3)
        (item-elem-c 1))
    (out (item-elem-e 1)))
  (dbg
    (info (id 14) (layer 1))
    (work (min 8) (total 8))
    (energy 32)
    (children 2
      (01 elem-a 3)
      (03 elem-c 1))
    (needs 3
      (01 elem-a 3)
      (03 elem-c 1)
      (14 elem-e 1))))

(monochate
  (info (type passive))
  (specs (lab-bits u8 5) (lab-work work 16) (lab-energy energy 2))
  (tape (work 3) (energy 5) (host item-printer)
    (in (item-elem-c 32))
    (out (item-monochate 1)))
  (dbg
    (info (id 1c) (layer 1))
    (work (min 3) (total 67))
    (energy 271)
    (children 1
      (03 elem-c 32))
    (needs 1
      (03 elem-c 32))))

(monochury
  (info (type passive))
  (specs (lab-bits u8 3) (lab-work work 16) (lab-energy energy 1))
  (tape (work 3) (energy 6) (host item-printer)
    (in (item-elem-c 32))
    (out (item-monochury 1)))
  (dbg
    (info (id 1d) (layer 1))
    (work (min 3) (total 67))
    (energy 274)
    (children 1
      (03 elem-c 32))
    (needs 1
      (03 elem-c 32))))

(monobarols
  (info (type passive))
  (specs (lab-bits u8 5) (lab-work work 20) (lab-energy energy 2))
  (tape (work 3) (energy 3) (host item-printer)
    (in (item-elem-b 7))
    (out (item-monobarols 1)))
  (dbg
    (info (id 1e) (layer 1))
    (work (min 3) (total 17))
    (energy 37)
    (children 1
      (02 elem-b 7))
    (needs 1
      (02 elem-b 7))))

(monarkols
  (info (type passive))
  (specs (lab-bits u8 3) (lab-work work 17) (lab-energy energy 2))
  (tape (work 2) (energy 2) (host item-printer)
    (in (item-elem-a 17))
    (out (item-monarkols 1)))
  (dbg
    (info (id 1f) (layer 1))
    (work (min 2) (total 19))
    (energy 21)
    (children 1
      (01 elem-a 17))
    (needs 1
      (01 elem-a 17))))

(extract
  (info (type active) (list factory))
  (specs (lab-bits u8 5) (lab-work work 35) (lab-energy energy 5))
  (tape (work 4) (energy 4) (host item-assembly)
    (in (item-monobararkon 1)
        (item-monobarex 3))
    (out (item-extract 1)))
  (dbg
    (info (id 20) (layer 2))
    (work (min 4) (total 60))
    (energy 122)
    (children 2
      (11 monobarex 3)
      (12 monobararkon 1))
    (needs 2
      (01 elem-a 18)
      (02 elem-b 13))))

(printer
  (info (type active) (list factory))
  (specs (lab-bits u8 9) (lab-work work 18) (lab-energy energy 5))
  (tape (work 4) (energy 7) (host item-assembly)
    (in (item-monocharkoid 1)
        (item-monobarex 3)
        (item-monocharkoid 1))
    (out (item-printer 1)))
  (dbg
    (info (id 21) (layer 2))
    (work (min 4) (total 64))
    (energy 202)
    (children 2
      (11 monobarex 3)
      (13 monocharkoid 2))
    (needs 3
      (01 elem-a 19)
      (02 elem-b 3)
      (03 elem-c 10))))

(duodylium
  (info (type passive))
  (specs (lab-bits u8 6) (lab-work work 33) (lab-energy energy 2))
  (tape (work 5) (energy 8) (host item-printer)
    (in (item-elem-c 2)
        (item-elem-d 2)
        (item-elem-c 3))
    (out (item-duodylium 1)))
  (dbg
    (info (id 22) (layer 2))
    (work (min 5) (total 23))
    (energy 128)
    (children 2
      (03 elem-c 5)
      (10 elem-d 2))
    (needs 3
      (02 elem-b 4)
      (03 elem-c 5)
      (10 elem-d 2))))

(elem-f
  (info (type natural))
  (specs (lab-bits u8 4) (lab-work work 31) (lab-energy energy 5))
  (tape (work 12) (energy 8) (host item-condenser)
    (in (item-elem-e 1)
        (item-elem-d 1))
    (out (item-elem-f 1)))
  (dbg
    (info (id 23) (layer 2))
    (work (min 12) (total 12))
    (energy 96)
    (children 2
      (10 elem-d 1)
      (14 elem-e 1))
    (needs 6
      (01 elem-a 3)
      (02 elem-b 2)
      (03 elem-c 1)
      (10 elem-d 1)
      (14 elem-e 1)
      (23 elem-f 1))))

(duerlex
  (info (type passive))
  (specs (lab-bits u8 7) (lab-work work 40) (lab-energy energy 2))
  (tape (work 10) (energy 9) (host item-printer)
    (in (item-elem-e 6)
        (item-elem-d 12)
        (item-elem-e 6)
        (item-elem-d 4)
        (item-elem-e 6)
        (item-elem-d 4)
        (item-elem-e 2)
        (item-elem-d 12)
        (item-elem-e 12))
    (out (item-duerlex 1)))
  (dbg
    (info (id 2a) (layer 2))
    (work (min 10) (total 394))
    (energy 1882)
    (children 2
      (10 elem-d 32)
      (14 elem-e 32))
    (needs 5
      (01 elem-a 96)
      (02 elem-b 64)
      (03 elem-c 32)
      (10 elem-d 32)
      (14 elem-e 32))))

(duodylitil
  (info (type passive))
  (specs (lab-bits u8 6) (lab-work work 21) (lab-energy energy 3))
  (tape (work 6) (energy 8) (host item-printer)
    (in (item-elem-b 29)
        (item-elem-d 32)
        (item-elem-b 3))
    (out (item-duodylitil 1)))
  (dbg
    (info (id 2b) (layer 2))
    (work (min 6) (total 198))
    (energy 944)
    (children 2
      (02 elem-b 32)
      (10 elem-d 32))
    (needs 2
      (02 elem-b 96)
      (10 elem-d 32))))

(duerltor
  (info (type passive))
  (specs (lab-bits u8 9) (lab-work work 42) (lab-energy energy 5))
  (tape (work 12) (energy 6) (host item-printer)
    (in (item-monobarols 1)
        (item-elem-e 3)
        (item-monobarex 2)
        (item-elem-e 15)
        (item-monobarex 2)
        (item-elem-e 3)
        (item-monobarols 1))
    (out (item-duerltor 1)))
  (dbg
    (info (id 2c) (layer 2))
    (work (min 12) (total 254))
    (energy 890)
    (children 3
      (11 monobarex 4)
      (14 elem-e 21)
      (1e monobarols 2))
    (needs 4
      (01 elem-a 83)
      (02 elem-b 18)
      (03 elem-c 21)
      (14 elem-e 21))))

(duerlry
  (info (type passive))
  (specs (lab-bits u8 10) (lab-work work 42) (lab-energy energy 5))
  (tape (work 11) (energy 8) (host item-printer)
    (in (item-monocharkoid 2)
        (item-monobarex 2)
        (item-monobarols 1)
        (item-elem-e 2)
        (item-monobararkon 1)
        (item-elem-e 19)
        (item-monocharkoid 2))
    (out (item-duerlry 1)))
  (dbg
    (info (id 2d) (layer 2))
    (work (min 11) (total 302))
    (energy 1125)
    (children 5
      (11 monobarex 2)
      (12 monobararkon 1)
      (13 monocharkoid 4)
      (14 elem-e 21)
      (1e monobarols 1))
    (needs 4
      (01 elem-a 84)
      (02 elem-b 19)
      (03 elem-c 41)
      (14 elem-e 21))))

(duerldylon-monochols
  (info (type passive))
  (specs (lab-bits u8 4) (lab-work work 23) (lab-energy energy 5))
  (tape (work 9) (energy 9) (host item-printer)
    (in (item-monocharkoid 7)
        (item-monobarols 1)
        (item-elem-d 10)
        (item-monobarex 1)
        (item-elem-e 21)
        (item-elem-d 10)
        (item-monobarols 1))
    (out (item-duerldylon-monochols 1)))
  (dbg
    (info (id 2e) (layer 2))
    (work (min 9) (total 406))
    (energy 1745)
    (children 5
      (10 elem-d 20)
      (11 monobarex 1)
      (13 monocharkoid 7)
      (14 elem-e 21)
      (1e monobarols 2))
    (needs 5
      (01 elem-a 82)
      (02 elem-b 55)
      (03 elem-c 56)
      (10 elem-d 20)
      (14 elem-e 21))))

(duochium
  (info (type passive))
  (specs (lab-bits u8 9) (lab-work work 33) (lab-energy energy 2))
  (tape (work 4) (energy 7) (host item-assembly)
    (in (item-monocharkoid 1)
        (item-monobarex 3)
        (item-monobararkon 1)
        (item-monocharkoid 4))
    (out (item-duochium 1)))
  (dbg
    (info (id 2f) (layer 2))
    (work (min 4) (total 135))
    (energy 434)
    (children 3
      (11 monobarex 3)
      (12 monobararkon 1)
      (13 monocharkoid 5))
    (needs 3
      (01 elem-a 28)
      (02 elem-b 13)
      (03 elem-c 25))))

(tridylarkitil
  (info (type passive))
  (specs (lab-bits u8 16) (lab-work work 66) (lab-energy energy 4))
  (tape (work 6) (energy 10) (host item-assembly)
    (in (item-monocharkoid 1)
        (item-duodylium 5)
        (item-monocharkoid 1))
    (out (item-tridylarkitil 1)))
  (dbg
    (info (id 30) (layer 3))
    (work (min 6) (total 151))
    (energy 820)
    (children 2
      (13 monocharkoid 2)
      (22 duodylium 5))
    (needs 4
      (01 elem-a 4)
      (02 elem-b 20)
      (03 elem-c 35)
      (10 elem-d 10))))

(deploy
  (info (type active) (list factory))
  (specs (lab-bits u8 13) (lab-work work 57) (lab-energy energy 10))
  (tape (work 8) (energy 11) (host item-assembly)
    (in (item-monocharkoid 3)
        (item-duodylium 1))
    (out (item-deploy 1)))
  (dbg
    (info (id 31) (layer 3))
    (work (min 8) (total 76))
    (energy 396)
    (children 2
      (13 monocharkoid 3)
      (22 duodylium 1))
    (needs 4
      (01 elem-a 6)
      (02 elem-b 4)
      (03 elem-c 20)
      (10 elem-d 2))))

(rod
  (info (type logistics))
  (specs (lab-bits u8 10) (lab-work work 48) (lab-energy energy 4))
  (tape (work 7) (energy 12) (host item-assembly)
    (in (item-duodylium 3)
        (item-extract 1)
        (item-monarkols 1)
        (item-duodylium 5))
    (out (item-rod 1)))
  (dbg
    (info (id 32) (layer 3))
    (work (min 7) (total 270))
    (energy 1251)
    (children 3
      (1f monarkols 1)
      (20 extract 1)
      (22 duodylium 8))
    (needs 4
      (01 elem-a 35)
      (02 elem-b 45)
      (03 elem-c 40)
      (10 elem-d 16))))

(tridylate
  (info (type passive))
  (specs (lab-bits u8 7) (lab-work work 44) (lab-energy energy 6))
  (tape (work 6) (energy 9) (host item-printer)
    (in (item-elem-d 17))
    (out (item-tridylate 1)))
  (dbg
    (info (id 3b) (layer 3))
    (work (min 6) (total 74))
    (energy 462)
    (children 1
      (10 elem-d 17))
    (needs 2
      (02 elem-b 34)
      (10 elem-d 17))))

(trifimate
  (info (type passive))
  (specs (lab-bits u8 9) (lab-work work 56) (lab-energy energy 11))
  (tape (work 16) (energy 11) (host item-printer)
    (in (item-elem-f 11)
        (item-elem-c 6)
        (item-elem-f 6)
        (item-elem-c 1)
        (item-elem-f 1)
        (item-elem-c 2)
        (item-elem-f 2)
        (item-elem-c 23)
        (item-elem-f 12))
    (out (item-trifimate 1)))
  (dbg
    (info (id 3c) (layer 3))
    (work (min 16) (total 464))
    (energy 3504)
    (children 2
      (03 elem-c 32)
      (23 elem-f 32))
    (needs 6
      (01 elem-a 96)
      (02 elem-b 64)
      (03 elem-c 64)
      (10 elem-d 32)
      (14 elem-e 32)
      (23 elem-f 32))))

(trifimbarsh
  (info (type passive))
  (specs (lab-bits u8 11) (lab-work work 57) (lab-energy energy 11))
  (tape (work 17) (energy 10) (host item-printer)
    (in (item-duodylium 1)
        (item-duerldylon-monochols 1)
        (item-duodylium 1)
        (item-duerltor 1)
        (item-elem-f 32)
        (item-duodylium 1))
    (out (item-trifimbarsh 1)))
  (dbg
    (info (id 3d) (layer 3))
    (work (min 17) (total 1130))
    (energy 6261)
    (children 4
      (22 duodylium 3)
      (23 elem-f 32)
      (2c duerltor 1)
      (2e duerldylon-monochols 1))
    (needs 6
      (01 elem-a 261)
      (02 elem-b 149)
      (03 elem-c 124)
      (10 elem-d 58)
      (14 elem-e 74)
      (23 elem-f 32))))

(trerlchury-duobargen
  (info (type passive))
  (specs (lab-bits u8 10) (lab-work work 69) (lab-energy energy 8))
  (tape (work 15) (energy 10) (host item-assembly)
    (in (item-duerldylon-monochols 1)
        (item-duerlry 1))
    (out (item-trerlchury-duobargen 1)))
  (dbg
    (info (id 3e) (layer 3))
    (work (min 15) (total 723))
    (energy 3020)
    (children 2
      (2d duerlry 1)
      (2e duerldylon-monochols 1))
    (needs 5
      (01 elem-a 166)
      (02 elem-b 74)
      (03 elem-c 97)
      (10 elem-d 20)
      (14 elem-e 42))))

(trichubarium
  (info (type passive))
  (specs (lab-bits u8 17) (lab-work work 24) (lab-energy energy 9))
  (tape (work 6) (energy 8) (host item-assembly)
    (in (item-duochium 1)
        (item-monocharkoid 8)
        (item-monarkols 1))
    (out (item-trichubarium 1)))
  (dbg
    (info (id 3f) (layer 3))
    (work (min 6) (total 280))
    (energy 983)
    (children 3
      (13 monocharkoid 8)
      (1f monarkols 1)
      (2f duochium 1))
    (needs 3
      (01 elem-a 61)
      (02 elem-b 13)
      (03 elem-c 65))))

(assembly
  (info (type active) (config printer) (list factory))
  (specs (lab-bits u8 13) (lab-work work 89) (lab-energy energy 14))
  (tape (work 8) (energy 12) (host item-assembly)
    (in (item-monobararkon 1)
        (item-monobarex 2)
        (item-tridylarkitil 3)
        (item-monobarex 1)
        (item-monobararkon 6)
        (item-monobarex 2))
    (out (item-assembly 1)))
  (dbg
    (info (id 40) (layer 4))
    (work (min 8) (total 693))
    (energy 3010)
    (children 3
      (11 monobarex 5)
      (12 monobararkon 7)
      (30 tridylarkitil 3))
    (needs 4
      (01 elem-a 58)
      (02 elem-b 135)
      (03 elem-c 105)
      (10 elem-d 30))))

(fusion
  (info (type active) (list factory))
  (specs
    (lab-bits u8 13) (lab-work work 70) (lab-energy energy 9)
    (input-item item !item-rod)
	(energy-output energy 20)
	(energy-rod energy 1024)
	(energy-cap energy 16384))
  (tape (work 7) (energy 13) (host item-assembly)
    (in (item-duodylium 1)
        (item-tridylarkitil 1)
        (item-extract 2)
        (item-tridylarkitil 1)
        (item-extract 2)
        (item-duodylium 1))
    (out (item-fusion 1)))
  (dbg
    (info (id 41) (layer 4))
    (work (min 7) (total 595))
    (energy 2475)
    (children 3
      (20 extract 4)
      (22 duodylium 2)
      (30 tridylarkitil 2))
    (needs 4
      (01 elem-a 80)
      (02 elem-b 100)
      (03 elem-c 80)
      (10 elem-d 24))))

(worker
  (info (type logistics))
  (specs (lab-bits u8 19) (lab-work work 59) (lab-energy energy 8))
  (tape (work 8) (energy 13) (host item-assembly)
    (in (item-printer 2)
        (item-monobarex 3)
        (item-tridylarkitil 2)
        (item-printer 1))
    (out (item-worker 1)))
  (dbg
    (info (id 42) (layer 4))
    (work (min 8) (total 532))
    (energy 2404)
    (children 3
      (11 monobarex 3)
      (21 printer 3)
      (30 tridylarkitil 2))
    (needs 4
      (01 elem-a 80)
      (02 elem-b 52)
      (03 elem-c 100)
      (10 elem-d 20))))

(memory
  (info (type active) (list control))
  (specs (lab-bits u8 16) (lab-work work 36) (lab-energy energy 15))
  (tape (work 8) (energy 12) (host item-assembly)
    (in (item-monarkols 3)
        (item-monocharkoid 1)
        (item-trichubarium 1)
        (item-monarkols 1)
        (item-monocharkoid 1)
        (item-monarkols 3))
    (out (item-memory 1)))
  (dbg
    (info (id 43) (layer 4))
    (work (min 8) (total 451))
    (energy 1346)
    (children 3
      (13 monocharkoid 2)
      (1f monarkols 7)
      (3f trichubarium 1))
    (needs 3
      (01 elem-a 184)
      (02 elem-b 13)
      (03 elem-c 75))))

(storage
  (info (type active) (list factory))
  (specs
    (lab-bits u8 8) (lab-work work 77) (lab-energy energy 20)
    (max u16 4096))
  (tape (work 16) (energy 15) (host item-assembly)
    (in (item-trerlchury-duobargen 1)
        (item-monarkols 1)
        (item-monobarex 1)
        (item-monocharkoid 7)
        (item-printer 2)
        (item-monocharkoid 7)
        (item-monobarex 1))
    (out (item-storage 1)))
  (dbg
    (info (id 44) (layer 4))
    (work (min 16) (total 1116))
    (energy 4561)
    (children 5
      (11 monobarex 2)
      (13 monocharkoid 14)
      (1f monarkols 1)
      (21 printer 2)
      (3e trerlchury-duobargen 1))
    (needs 5
      (01 elem-a 259)
      (02 elem-b 82)
      (03 elem-c 187)
      (10 elem-d 20)
      (14 elem-e 42))))

(solar
  (info (type logistics))
  (specs
    (lab-bits u8 12) (lab-work work 85) (lab-energy energy 23)
    (energy-div energy 4096) (energy fn))
  (tape (work 18) (energy 11) (host item-assembly)
    (in (item-monobarex 4)
        (item-monobarols 2)
        (item-monobarex 1)
        (item-monobarols 3)
        (item-monobarex 1)
        (item-monobarols 2)
        (item-trifimbarsh 1)
        (item-monobarex 12))
    (out (item-solar 1)))
  (dbg
    (info (id 45) (layer 4))
    (work (min 18) (total 1447))
    (energy 7042)
    (children 3
      (11 monobarex 18)
      (1e monobarols 7)
      (3d trifimbarsh 1))
    (needs 6
      (01 elem-a 351)
      (02 elem-b 216)
      (03 elem-c 124)
      (10 elem-d 58)
      (14 elem-e 74)
      (23 elem-f 32))))

(elem-l
  (info (type synth))
  (specs (lab-bits u8 20) (lab-work work 64) (lab-energy energy 19))
  (tape (work 20) (energy 50) (host item-collider)
    (in (item-elem-d 6)
        (item-elem-e 5)
        (item-elem-d 2)
        (item-elem-e 5)
        (item-elem-d 6)
        (item-elem-e 5)
        (item-elem-d 6))
    (out (item-elem-l 1)))
  (dbg
    (info (id 46) (layer 4))
    (work (min 20) (total 20))
    (energy 1000)
    (children 2
      (10 elem-d 20)
      (14 elem-e 15))
    (needs 6
      (01 elem-a 45)
      (02 elem-b 40)
      (03 elem-c 15)
      (10 elem-d 20)
      (14 elem-e 15)
      (46 elem-l 1))))

(elem-m
  (info (type synth))
  (specs (lab-bits u8 20) (lab-work work 35) (lab-energy energy 18))
  (tape (work 36) (energy 50) (host item-collider)
    (in (item-elem-e 4)
        (item-elem-f 3)
        (item-elem-e 2)
        (item-elem-f 3)
        (item-elem-e 2)
        (item-elem-f 4)
        (item-elem-e 7))
    (out (item-elem-m 1)))
  (dbg
    (info (id 47) (layer 4))
    (work (min 36) (total 36))
    (energy 1800)
    (children 2
      (14 elem-e 15)
      (23 elem-f 10))
    (needs 7
      (01 elem-a 75)
      (02 elem-b 20)
      (03 elem-c 25)
      (10 elem-d 10)
      (14 elem-e 25)
      (23 elem-f 10)
      (47 elem-m 1))))

(tetradylchols-tribarsh
  (info (type passive))
  (specs (lab-bits u8 13) (lab-work work 53) (lab-energy energy 17))
  (tape (work 8) (energy 14) (host item-assembly)
    (in (item-tridylate 1)
        (item-monobararkon 1)
        (item-monobarex 1)
        (item-monobarols 1)
        (item-monocharkoid 1)
        (item-monobarex 2)
        (item-monocharkoid 1)
        (item-duodylium 1)
        (item-monobarex 1))
    (out (item-tetradylchols-tribarsh 1)))
  (dbg
    (info (id 4b) (layer 4))
    (work (min 8) (total 218))
    (energy 983)
    (children 6
      (11 monobarex 4)
      (12 monobararkon 1)
      (13 monocharkoid 2)
      (1e monobarols 1)
      (22 duodylium 1)
      (3b tridylate 1))
    (needs 4
      (01 elem-a 27)
      (02 elem-b 59)
      (03 elem-c 15)
      (10 elem-d 19))))

(tetrafimalt
  (info (type passive))
  (specs (lab-bits u8 17) (lab-work work 33) (lab-energy energy 16))
  (tape (work 22) (energy 12) (host item-assembly)
    (in (item-tridylate 2)
        (item-trifimate 1)
        (item-duerltor 1))
    (out (item-tetrafimalt 1)))
  (dbg
    (info (id 4c) (layer 4))
    (work (min 22) (total 888))
    (energy 5582)
    (children 3
      (2c duerltor 1)
      (3b tridylate 2)
      (3c trifimate 1))
    (needs 6
      (01 elem-a 179)
      (02 elem-b 150)
      (03 elem-c 85)
      (10 elem-d 66)
      (14 elem-e 53)
      (23 elem-f 32))))

(tetrafimry
  (info (type passive))
  (specs (lab-bits u8 9) (lab-work work 89) (lab-energy energy 8))
  (tape (work 24) (energy 17) (host item-assembly)
    (in (item-trifimate 1)
        (item-monochate 2))
    (out (item-tetrafimry 1)))
  (dbg
    (info (id 4d) (layer 4))
    (work (min 24) (total 622))
    (energy 4454)
    (children 2
      (1c monochate 2)
      (3c trifimate 1))
    (needs 6
      (01 elem-a 96)
      (02 elem-b 64)
      (03 elem-c 128)
      (10 elem-d 32)
      (14 elem-e 32)
      (23 elem-f 32))))

(tetrerlbargen
  (info (type passive))
  (specs (lab-bits u8 18) (lab-work work 89) (lab-energy energy 14))
  (tape (work 19) (energy 11) (host item-assembly)
    (in (item-tridylarkitil 1)
        (item-trerlchury-duobargen 1)
        (item-duodylitil 1)
        (item-monarkols 2))
    (out (item-tetrerlbargen 1)))
  (dbg
    (info (id 4e) (layer 4))
    (work (min 19) (total 1129))
    (energy 5035)
    (children 4
      (1f monarkols 2)
      (2b duodylitil 1)
      (30 tridylarkitil 1)
      (3e trerlchury-duobargen 1))
    (needs 5
      (01 elem-a 204)
      (02 elem-b 190)
      (03 elem-c 132)
      (10 elem-d 62)
      (14 elem-e 42))))

(tetradylchitil-duobarate
  (info (type passive))
  (specs (lab-bits u8 22) (lab-work work 32) (lab-energy energy 9))
  (tape (work 8) (energy 15) (host item-assembly)
    (in (item-duodylium 2)
        (item-monobarols 7)
        (item-tridylarkitil 2)
        (item-monobarex 3)
        (item-duodylium 1)
        (item-monobarex 3)
        (item-tridylarkitil 1)
        (item-monobarex 11)
        (item-monobarols 5))
    (out (item-tetradylchitil-duobarate 1)))
  (dbg
    (info (id 4f) (layer 4))
    (work (min 8) (total 924))
    (energy 3750)
    (children 4
      (11 monobarex 19)
      (1e monobarols 12)
      (22 duodylium 3)
      (30 tridylarkitil 3))
    (needs 4
      (01 elem-a 107)
      (02 elem-b 175)
      (03 elem-c 120)
      (10 elem-d 36))))

(brain
  (info (type active) (list control))
  (specs (lab-bits u8 13) (lab-work work 108) (lab-energy energy 39))
  (tape (work 11) (energy 13) (host item-assembly)
    (in (item-tridylarkitil 1)
        (item-memory 1)
        (item-tridylarkitil 1)
        (item-monobarols 3)
        (item-duodylium 3)
        (item-monobarols 3)
        (item-duodylium 2)
        (item-monobarols 3)
        (item-tridylarkitil 1)
        (item-rod 1))
    (out (item-brain 1)))
  (dbg
    (info (id 50) (layer 5))
    (work (min 11) (total 1453))
    (energy 6173)
    (children 5
      (1e monobarols 9)
      (22 duodylium 5)
      (30 tridylarkitil 3)
      (32 rod 1)
      (43 memory 1))
    (needs 4
      (01 elem-a 231)
      (02 elem-b 201)
      (03 elem-c 245)
      (10 elem-d 56))))

(prober
  (info (type active) (list control))
  (specs
    (lab-bits u8 25) (lab-work work 115) (lab-energy energy 44)
    (work-energy energy 8) (work-cap fn))
  (tape (work 10) (energy 16) (host item-assembly)
    (in (item-tridylarkitil 1)
        (item-duodylitil 1)
        (item-tridylarkitil 1)
        (item-rod 1)
        (item-memory 1)
        (item-duodylium 1)
        (item-memory 1)
        (item-rod 1)
        (item-extract 1)
        (item-tridylarkitil 1))
    (out (item-prober 1)))
  (dbg
    (info (id 51) (layer 5))
    (work (min 10) (total 2186))
    (energy 9008)
    (children 6
      (20 extract 1)
      (22 duodylium 1)
      (2b duodylitil 1)
      (30 tridylarkitil 3)
      (32 rod 2)
      (43 memory 2))
    (needs 4
      (01 elem-a 468)
      (02 elem-b 289)
      (03 elem-c 340)
      (10 elem-d 96))))

(battery
  (info (type logistics))
  (specs
    (lab-bits u8 12) (lab-work work 108) (lab-energy energy 41)
    (storage-cap energy 8))
  (tape (work 23) (energy 23) (host item-assembly)
    (in (item-monochury 2)
        (item-tridylarkitil 1)
        (item-deploy 1)
        (item-trifimbarsh 1)
        (item-storage 1)
        (item-deploy 1)
        (item-tridylarkitil 1)
        (item-monochury 2))
    (out (item-battery 1)))
  (dbg
    (info (id 52) (layer 5))
    (work (min 23) (total 2991))
    (energy 14879)
    (children 5
      (1d monochury 4)
      (30 tridylarkitil 2)
      (31 deploy 2)
      (3d trifimbarsh 1)
      (44 storage 1))
    (needs 6
      (01 elem-a 540)
      (02 elem-b 279)
      (03 elem-c 549)
      (10 elem-d 102)
      (14 elem-e 116)
      (23 elem-f 32))))

(receive
  (info (type active) (list control))
  (specs
    (lab-bits u8 14) (lab-work work 41) (lab-energy energy 19)
    (buffer-max enum 1))
  (tape (work 16) (energy 13) (host item-assembly)
    (in (item-memory 2)
        (item-duerltor 1)
        (item-duodylitil 2)
        (item-duodylium 3)
        (item-duodylitil 2)
        (item-duerldylon-monochols 1)
        (item-duerltor 2))
    (out (item-receive 1)))
  (dbg
    (info (id 53) (layer 5))
    (work (min 16) (total 2947))
    (energy 11475)
    (children 5
      (22 duodylium 3)
      (2b duodylitil 4)
      (2c duerltor 3)
      (2e duerldylon-monochols 1)
      (43 memory 2))
    (needs 5
      (01 elem-a 699)
      (02 elem-b 531)
      (03 elem-c 284)
      (10 elem-d 154)
      (14 elem-e 84))))

(elem-n
  (info (type synth))
  (specs (lab-bits u8 17) (lab-work work 110) (lab-energy energy 30))
  (tape (work 42) (energy 50) (host item-collider)
    (in (item-elem-m 2)
        (item-elem-l 4)
        (item-elem-m 1)
        (item-elem-l 6)
        (item-elem-m 2))
    (out (item-elem-n 1)))
  (dbg
    (info (id 54) (layer 5))
    (work (min 42) (total 42))
    (energy 2100)
    (children 2
      (46 elem-l 10)
      (47 elem-m 5))
    (needs 9
      (01 elem-a 825)
      (02 elem-b 500)
      (03 elem-c 275)
      (10 elem-d 250)
      (14 elem-e 275)
      (23 elem-f 50)
      (46 elem-l 10)
      (47 elem-m 5)
      (54 elem-n 1))))

(pentamoxate
  (info (type passive))
  (specs (lab-bits u8 15) (lab-work work 87) (lab-energy energy 28))
  (tape (work 47) (energy 59) (host item-printer)
    (in (item-tridylate 1)
        (item-duodylium 1)
        (item-elem-m 13)
        (item-duodylitil 1)
        (item-duerltor 1)
        (item-duerldylon-monochols 1)
        (item-elem-m 2)
        (item-duerldylon-monochols 1)
        (item-duerlry 1))
    (out (item-pentamoxate 1)))
  (dbg
    (info (id 5a) (layer 5))
    (work (min 47) (total 2250))
    (energy 36812)
    (children 7
      (22 duodylium 1)
      (2b duodylitil 1)
      (2c duerltor 1)
      (2d duerlry 1)
      (2e duerldylon-monochols 2)
      (3b tridylate 1)
      (47 elem-m 15))
    (needs 7
      (01 elem-a 1456)
      (02 elem-b 581)
      (03 elem-c 554)
      (10 elem-d 241)
      (14 elem-e 459)
      (23 elem-f 150)
      (47 elem-m 15))))

(pentadylchutor
  (info (type passive))
  (specs (lab-bits u8 24) (lab-work work 79) (lab-energy energy 18))
  (tape (work 9) (energy 15) (host item-assembly)
    (in (item-tetradylchols-tribarsh 2)
        (item-duodylium 3))
    (out (item-pentadylchutor 1)))
  (dbg
    (info (id 5b) (layer 5))
    (work (min 9) (total 514))
    (energy 2485)
    (children 2
      (22 duodylium 3)
      (4b tetradylchols-tribarsh 2))
    (needs 4
      (01 elem-a 54)
      (02 elem-b 130)
      (03 elem-c 45)
      (10 elem-d 44))))

(pentalofchols
  (info (type passive))
  (specs (lab-bits u8 25) (lab-work work 45) (lab-energy energy 33))
  (tape (work 24) (energy 56) (host item-printer)
    (in (item-tridylate 2)
        (item-monobarols 13)
        (item-trifimbarsh 1)
        (item-tridylate 1)
        (item-elem-l 1)
        (item-trifimbarsh 1)
        (item-elem-l 19))
    (out (item-pentalofchols 1)))
  (dbg
    (info (id 5c) (layer 5))
    (work (min 24) (total 3127))
    (energy 35733)
    (children 4
      (1e monobarols 13)
      (3b tridylate 3)
      (3d trifimbarsh 2)
      (46 elem-l 20))
    (needs 7
      (01 elem-a 1422)
      (02 elem-b 1291)
      (03 elem-c 548)
      (10 elem-d 567)
      (14 elem-e 448)
      (23 elem-f 64)
      (46 elem-l 20))))

(pentafimchex-monobarsh
  (info (type passive))
  (specs (lab-bits u8 20) (lab-work work 65) (lab-energy energy 18))
  (tape (work 24) (energy 16) (host item-assembly)
    (in (item-tetrafimalt 1)
        (item-monarkols 2)
        (item-duodylitil 1))
    (out (item-pentafimchex-monobarsh 1)))
  (dbg
    (info (id 5d) (layer 5))
    (work (min 24) (total 1148))
    (energy 6952)
    (children 3
      (1f monarkols 2)
      (2b duodylitil 1)
      (4c tetrafimalt 1))
    (needs 6
      (01 elem-a 213)
      (02 elem-b 246)
      (03 elem-c 85)
      (10 elem-d 98)
      (14 elem-e 53)
      (23 elem-f 32))))

(penterltor
  (info (type passive))
  (specs (lab-bits u8 26) (lab-work work 67) (lab-energy energy 30))
  (tape (work 27) (energy 13) (host item-assembly)
    (in (item-monobarex 2)
        (item-monarkols 1)
        (item-monobarex 10)
        (item-monarkols 1)
        (item-monocharkoid 1)
        (item-tetrerlbargen 1)
        (item-monobarex 8))
    (out (item-penterltor 1)))
  (dbg
    (info (id 5e) (layer 5))
    (work (min 27) (total 1409))
    (energy 5848)
    (children 4
      (11 monobarex 20)
      (13 monocharkoid 1)
      (1f monarkols 2)
      (4e tetrerlbargen 1))
    (needs 5
      (01 elem-a 340)
      (02 elem-b 210)
      (03 elem-c 137)
      (10 elem-d 62)
      (14 elem-e 42))))

(pentadylchate
  (info (type passive))
  (specs (lab-bits u8 26) (lab-work work 51) (lab-energy energy 36))
  (tape (work 10) (energy 19) (host item-assembly)
    (in (item-monobarex 6)
        (item-monobarols 5)
        (item-duochium 2)
        (item-monobarex 5)
        (item-duochium 1)
        (item-monobarols 6)
        (item-monocharkoid 3)
        (item-monobarex 11)
        (item-tetradylchitil-duobarate 1))
    (out (item-pentadylchate 1)))
  (dbg
    (info (id 5f) (layer 5))
    (work (min 10) (total 1791))
    (energy 6225)
    (children 5
      (11 monobarex 22)
      (13 monocharkoid 3)
      (1e monobarols 11)
      (2f duochium 3)
      (4f tetradylchitil-duobarate 1))
    (needs 4
      (01 elem-a 307)
      (02 elem-b 313)
      (03 elem-c 210)
      (10 elem-d 36))))

(lab
  (info (type active) (list factory))
  (specs (lab-bits u8 26) (lab-work work 85) (lab-energy energy 92))
  (tape (work 14) (energy 20) (host item-assembly)
    (in (item-duodylitil 1)
        (item-extract 1)
        (item-deploy 1)
        (item-rod 10)
        (item-extract 14)
        (item-rod 3)
        (item-duodylium 2)
        (item-brain 1)
        (item-extract 1)
        (item-duodylitil 1))
    (out (item-lab 1)))
  (dbg
    (info (id 60) (layer 6))
    (work (min 14) (total 6455))
    (energy 27208)
    (children 6
      (20 extract 16)
      (22 duodylium 2)
      (2b duodylitil 2)
      (31 deploy 1)
      (32 rod 13)
      (50 brain 1))
    (needs 4
      (01 elem-a 980)
      (02 elem-b 1198)
      (03 elem-c 795)
      (10 elem-d 334))))

(scanner
  (info (type active) (list control))
  (specs
    (lab-bits u8 27) (lab-work work 49) (lab-energy energy 38)
    (work-energy energy 8) (work-cap fn))
  (tape (work 13) (energy 21) (host item-assembly)
    (in (item-rod 1)
        (item-printer 1)
        (item-monobarex 2)
        (item-brain 1)
        (item-prober 1)
        (item-deploy 1)
        (item-prober 1)
        (item-monobarex 2)
        (item-rod 2))
    (out (item-scanner 1)))
  (dbg
    (info (id 61) (layer 6))
    (work (min 13) (total 6828))
    (energy 28885)
    (children 6
      (11 monobarex 4)
      (21 printer 1)
      (31 deploy 1)
      (32 rod 3)
      (50 brain 1)
      (51 prober 2))
    (needs 4
      (01 elem-a 1317)
      (02 elem-b 925)
      (03 elem-c 1075)
      (10 elem-d 298))))

(condenser
  (info (type active) (config extract) (list factory))
  (specs (lab-bits u8 14) (lab-work work 94) (lab-energy energy 42))
  (tape (work 11) (energy 24) (host item-assembly)
    (in (item-monocharkoid 23)
        (item-monarkols 3)
        (item-extract 5)
        (item-monocharkoid 8)
        (item-printer 1)
        (item-monobarex 1)
        (item-monocharkoid 1)
        (item-monochury 1)
        (item-extract 5)
        (item-printer 6)
        (item-monarkols 3)
        (item-pentadylchate 1))
    (out (item-condenser 1)))
  (dbg
    (info (id 62) (layer 6))
    (work (min 11) (total 3521))
    (energy 11461)
    (children 7
      (11 monobarex 1)
      (13 monocharkoid 32)
      (1d monochury 1)
      (1f monarkols 6)
      (20 extract 10)
      (21 printer 7)
      (5f pentadylchate 1))
    (needs 4
      (01 elem-a 791)
      (02 elem-b 465)
      (03 elem-c 472)
      (10 elem-d 36))))

(pill
  (info (type logistics))
  (specs (lab-bits u8 14) (lab-work work 53) (lab-energy energy 37))
  (tape (work 40) (energy 19) (host item-assembly)
    (in (item-monocharkoid 6)
        (item-storage 1)
        (item-penterltor 1)
        (item-monocharkoid 1)
        (item-monobarols 27)
        (item-monocharkoid 2))
    (out (item-pill 1)))
  (dbg
    (info (id 63) (layer 6))
    (work (min 40) (total 3159))
    (energy 12708)
    (children 4
      (13 monocharkoid 9)
      (1e monobarols 27)
      (44 storage 1)
      (5e penterltor 1))
    (needs 5
      (01 elem-a 617)
      (02 elem-b 481)
      (03 elem-c 369)
      (10 elem-d 82)
      (14 elem-e 84))))

(transmit
  (info (type active) (list control))
  (specs
    (lab-bits u8 17) (lab-work work 141) (lab-energy energy 87)
    (launch-energy energy 32)
	(launch-speed u16 500))
  (tape (work 35) (energy 18) (host item-assembly)
    (in (item-tetrafimry 1)
        (item-receive 1)
        (item-rod 1)
        (item-monochate 3)
        (item-deploy 2))
    (out (item-transmit 1)))
  (dbg
    (info (id 64) (layer 6))
    (work (min 35) (total 4227))
    (energy 19415)
    (children 5
      (1c monochate 3)
      (31 deploy 2)
      (32 rod 1)
      (4d tetrafimry 1)
      (53 receive 1))
    (needs 6
      (01 elem-a 842)
      (02 elem-b 648)
      (03 elem-c 588)
      (10 elem-d 206)
      (14 elem-e 116)
      (23 elem-f 32))))

(port
  (info (type active) (list factory))
  (specs
    (lab-bits u8 27) (lab-work work 121) (lab-energy energy 70)
    (dock-energy energy 64)
	   (launch-energy energy 128)
	   (launch-speed u16 100))
  (tape (work 34) (energy 22) (host item-assembly)
    (in (item-storage 1)
        (item-duerlex 1)
        (item-worker 2)
        (item-pentafimchex-monobarsh 1)
        (item-storage 1)
        (item-trifimate 1))
    (out (item-port 1)))
  (dbg
    (info (id 65) (layer 6))
    (work (min 34) (total 5336))
    (energy 27016)
    (children 5
      (2a duerlex 1)
      (3c trifimate 1)
      (42 worker 2)
      (44 storage 2)
      (5d pentafimchex-monobarsh 1))
    (needs 6
      (01 elem-a 1083)
      (02 elem-b 642)
      (03 elem-c 755)
      (10 elem-d 242)
      (14 elem-e 201)
      (23 elem-f 64))))

(accelerator
  (info (type logistics))
  (specs (lab-bits u8 30) (lab-work work 132) (lab-energy energy 48))
  (tape (work 31) (energy 24) (host item-assembly)
    (in (item-battery 1)
        (item-tridylate 9)
        (item-rod 3)
        (item-tridylate 1)
        (item-tridylarkitil 4)
        (item-deploy 1)
        (item-tridylate 2)
        (item-rod 2)
        (item-tridylate 3))
    (out (item-accelerator 1)))
  (dbg
    (info (id 66) (layer 6))
    (work (min 31) (total 6162))
    (energy 32484)
    (children 5
      (30 tridylarkitil 4)
      (31 deploy 1)
      (32 rod 5)
      (3b tridylate 15)
      (52 battery 1))
    (needs 6
      (01 elem-a 737)
      (02 elem-b 1098)
      (03 elem-c 909)
      (10 elem-d 479)
      (14 elem-e 116)
      (23 elem-f 32))))

(burner
  (info (type active) (list factory))
  (specs
    (lab-bits u8 27) (lab-work work 91) (lab-energy energy 37)
    (energy fn) (work-cap fn))
  (tape (work 36) (energy 77) (host item-assembly)
    (in (item-duodylium 1)
        (item-monarkols 5)
        (item-deploy 1)
        (item-pentalofchols 1)
        (item-monarkols 5))
    (out (item-burner 1)))
  (dbg
    (info (id 67) (layer 6))
    (work (min 36) (total 3452))
    (energy 39239)
    (children 4
      (1f monarkols 10)
      (22 duodylium 1)
      (31 deploy 1)
      (5c pentalofchols 1))
    (needs 7
      (01 elem-a 1598)
      (02 elem-b 1299)
      (03 elem-c 573)
      (10 elem-d 571)
      (14 elem-e 448)
      (23 elem-f 64)
      (46 elem-l 20))))

(hexamoxchoid-monobary
  (info (type passive))
  (specs (lab-bits u8 22) (lab-work work 120) (lab-energy energy 80))
  (tape (work 50) (energy 87) (host item-assembly)
    (in (item-duerldylon-monochols 1)
        (item-duodylium 2)
        (item-duerldylon-monochols 1)
        (item-duodylium 1)
        (item-pentamoxate 1)
        (item-duerldylon-monochols 1))
    (out (item-hexamoxchoid-monobary 1)))
  (dbg
    (info (id 6e) (layer 6))
    (work (min 50) (total 3587))
    (energy 46781)
    (children 3
      (22 duodylium 3)
      (2e duerldylon-monochols 3)
      (5a pentamoxate 1))
    (needs 7
      (01 elem-a 1702)
      (02 elem-b 758)
      (03 elem-c 737)
      (10 elem-d 307)
      (14 elem-e 522)
      (23 elem-f 150)
      (47 elem-m 15))))

(hexadylchate-pentabaron
  (info (type passive))
  (specs (lab-bits u8 34) (lab-work work 80) (lab-energy energy 73))
  (tape (work 13) (energy 21) (host item-assembly)
    (in (item-monobarols 6)
        (item-pentadylchutor 2))
    (out (item-hexadylchate-pentabaron 1)))
  (dbg
    (info (id 6f) (layer 6))
    (work (min 13) (total 1143))
    (energy 5465)
    (children 2
      (1e monobarols 6)
      (5b pentadylchutor 2))
    (needs 4
      (01 elem-a 108)
      (02 elem-b 302)
      (03 elem-c 90)
      (10 elem-d 88))))

(legion
  (info (type active) (list control))
  (specs
    (lab-bits u8 20) (lab-work work 99) (lab-energy energy 144)
    (travel-speed u16 100))
  (tape (work 14) (energy 30) (host item-assembly)
    (in (item-memory 1)
        (item-brain 1)
        (item-duodylium 7)
        (item-assembly 1)
        (item-printer 1)
        (item-prober 1)
        (item-worker 2)
        (item-monochate 1)
        (item-fusion 1)
        (item-monochate 1)
        (item-rod 1)
        (item-assembly 1)
        (item-extract 2)
        (item-monocharkoid 1)
        (item-monobarols 1)
        (item-hexadylchate-pentabaron 1)
        (item-monobarols 1)
        (item-deploy 2)
        (item-printer 1)
        (item-duodylium 7))
    (out (item-legion 1)))
  (dbg
    (info (id 70) (layer 7))
    (work (min 14) (total 9467))
    (energy 40874)
    (children 15
      (13 monocharkoid 1)
      (1c monochate 2)
      (1e monobarols 2)
      (20 extract 2)
      (21 printer 2)
      (22 duodylium 14)
      (31 deploy 2)
      (32 rod 1)
      (40 assembly 2)
      (41 fusion 1)
      (42 worker 2)
      (43 memory 1)
      (50 brain 1)
      (51 prober 1)
      (6f hexadylchate-pentabaron 1))
    (needs 4
      (01 elem-a 1470)
      (02 elem-b 1434)
      (03 elem-c 1479)
      (10 elem-d 412))))

(collider
  (info (type active) (list factory))
  (specs
    (lab-bits u8 26) (lab-work work 145) (lab-energy energy 149)
    (grow-max u8 64)
	(grow-item item !item_accelerator)
	(junk-item item !item-elem-o)
	(output-rate fn))
  (tape (work 42) (energy 33) (host item-assembly)
    (in (item-trifimbarsh 4)
        (item-tridylarkitil 1)
        (item-trerlchury-duobargen 1)
        (item-trichubarium 1)
        (item-accelerator 1)
        (item-trichubarium 2)
        (item-trerlchury-duobargen 1)
        (item-accelerator 1)
        (item-trichubarium 1)
        (item-trerlchury-duobargen 4)
        (item-rod 1)
        (item-trichubarium 1))
    (out (item-collider 1)))
  (dbg
    (info (id 71) (layer 7))
    (work (min 42) (total 23045))
    (energy 116504)
    (children 6
      (30 tridylarkitil 1)
      (32 rod 1)
      (3d trifimbarsh 4)
      (3e trerlchury-duobargen 6)
      (3f trichubarium 5)
      (66 accelerator 2))
    (needs 6
      (01 elem-a 3858)
      (02 elem-b 3366)
      (03 elem-c 3296)
      (10 elem-d 1336)
      (14 elem-e 780)
      (23 elem-f 192))))

(packer
  (info (type active) (list factory))
  (specs (lab-bits u8 35) (lab-work work 63) (lab-energy energy 72))
  (tape (work 68) (energy 130) (host item-assembly)
    (in (item-rod 1)
        (item-printer 1)
        (item-deploy 1)
        (item-printer 1)
        (item-hexamoxchoid-monobary 1)
        (item-rod 1))
    (out (item-packer 1)))
  (dbg
    (info (id 72) (layer 7))
    (work (min 68) (total 4399))
    (energy 58923)
    (children 4
      (21 printer 2)
      (31 deploy 1)
      (32 rod 2)
      (6e hexamoxchoid-monobary 1))
    (needs 7
      (01 elem-a 1816)
      (02 elem-b 858)
      (03 elem-c 857)
      (10 elem-d 341)
      (14 elem-e 522)
      (23 elem-f 150)
      (47 elem-m 15))))

(nomad
  (info (type active) (list factory))
  (specs
    (lab-bits u8 36) (lab-work work 168) (lab-energy energy 361)
    (travel-speed u16 100)
	(memory-len enum 3)
	(cargo-len enum 12)
	(cargo-max u8 255))
  (tape (work 100) (energy 135) (host item-assembly)
    (in (item-monobarols 10)
        (item-trifimate 4)
        (item-monobarols 2)
        (item-trifimate 1)
        (item-monobarols 4)
        (item-packer 1)
        (item-pentamoxate 1)
        (item-monobarols 16))
    (out (item-nomad 1)))
  (dbg
    (info (id 80) (layer 8))
    (work (min 100) (total 11863))
    (energy 164751)
    (children 4
      (1e monobarols 32)
      (3c trifimate 5)
      (5a pentamoxate 2)
      (72 packer 1))
    (needs 7
      (01 elem-a 5208)
      (02 elem-b 2564)
      (03 elem-c 2285)
      (10 elem-d 983)
      (14 elem-e 1600)
      (23 elem-f 610)
      (47 elem-m 45))))

(user
  (info (type sys)))

(data
  (info (type sys)))

(dummy
  (info (type sys)))

(energy
  (info (type sys)))

(test
  (info (type active))
  (specs (lab-bits u8 42) (lab-work work 125) (lab-energy energy 35810))
  (tape (work 1) (energy 1) (host item-assembly)
    (out (item-test 1)))
  (dbg
    (info (id f4) (layer 15))
    (work (min 1) (total 1))
    (energy 1)
    (children 0)
    (needs 0)))

