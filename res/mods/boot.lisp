;; Goal is to create a spanning tree of stars of depth `max-depth` all
;; of which are running the os mod and have ports linking back to
;; home. This mod uses almost all the features in legion and
;; essentially acts as the integration test for the game.


;; -----------------------------------------------------------------------------
;; Config
;; -----------------------------------------------------------------------------

(defconst max-depth 4)
(defconst energy-target 64)
(defconst port-item-count 255)

(defconst deploy-id (id !item-deploy 1))
(assert (= (io !io-ping deploy-id) !io-ok))

(defconst assembly-id (id !item-assembly 1))
(assert (= (io !io-ping assembly-id) !io-ok))

(defconst library-id (id !item-library 1))
(assert (= (io !io-ping library-id) !io-ok))

(defconst prober-id (id !item-prober 1))
(assert (= (io !io-ping prober-id) !io-ok))


;; -----------------------------------------------------------------------------
;; Boot
;; -----------------------------------------------------------------------------

;; Home
(progn
  (when (> (ior !io-count prober-id !item-brain) 1) (reset))
  (unless (ior !io-get (id !item-memory 1) 0)
    (io !io-name (self) ?Bob-The-Homeworld))
  (io !io-log (self) ?booting (ior !io-coord (self))))


;; Setup
(progn
  ;; Elems
  (io !io-tape (id !item-extract 1) !item-elem-a)
  (io !io-tape (id !item-extract 2) !item-elem-b)
  (io !io-tape (id !item-printer 1) !item-monobarex)
  (io !io-tape (id !item-printer 2) !item-monobararkon)
  (deploy-tape !item-elem-a 2)
  (deploy-tape !item-elem-b 4)
  (deploy-tape !item-elem-c 4)
  (deploy-tape !item-elem-d 4)
  (io !io-tape (id !item-extract 2) !item-elem-a)

  ;; Passives
  (io !io-tape (id !item-printer 1) !item-monobarex)
  (io !io-tape (id !item-printer 2) !item-monocharkoid)
  (deploy-tape !item-monobarex 2)
  (deploy-tape !item-monobararkon 2)
  (deploy-tape !item-monocharkoid 2)
  (deploy-tape !item-duodylium 2)
  (io !io-tape (id !item-assembly 2) !item-tridylarkitil)
  (deploy-tape !item-tridylarkitil 1)

  ;; Logistics
  (deploy-requirements !item-rod)
  (deploy-tape !item-rod 2)
  (deploy-item !item-fusion 2)
  (deploy-item !item-worker 18))


;; OS - we reserve the juicy brain and memory ids for os
(defconst brain-os-id (id !item-brain 2))
(defconst brain-exec-id (id !item-brain 3))
(progn
  (deploy-item !item-brain 3)
  (deploy-item !item-memory (os.memory-need))

  ;; Copy our parent coord to the OS memory.
  (io !io-set (id !item-memory 2) 0 (ior !io-get (id !item-memory 1) 0))

  (assert (= (io !io-ping brain-os-id) !io-ok))
  (assert (= (io !io-ping brain-exec-id) !io-ok)))


;; Research
(progn
  (deploy-item !item-lab 2)
  (deploy-item !item-brain 1)
  (deploy-item !item-library 1)

  (let ((lab-brain-id (last-id !item-brain)))
    (io !io-send lab-brain-id ?lab-boot (last-id !item-library))
    (io !io-mod lab-brain-id (mod lab.2))))


;; OS
(progn
  ;; Condenser - Required to be able to unlock e & f
  (deploy-requirements !item-condenser)
  (deploy-tape !item-condenser 2)

  ;; Energy - Solar
  (let ((energy-per (specs !spec-solar-energy (ior !io-count prober-id !item-energy))))
    (deploy-item !item-solar (+ (/ energy-target energy-per) 1)))

  ;; Parent network - won't be used in homeworld but still needed to
  ;; make the ids align.
  (deploy-item !item-receive 1)
  (deploy-item !item-transmit 1)

  ;; Boot
  (os.boot brain-os-id brain-exec-id))


;; Legion
(when (<= (os.depth) max-depth)
  (deploy-item !item-brain 1)

  ;; We do it the hard way to avoid race conditions with lab.lisp
  (deploy-requirements !item-prober)
  (deploy-tape !item-prober 2)
  (deploy-item !item-prober 1)
  (assert (= (ior !io-count prober-id !item-prober) 2))

  (deploy-requirements !item-scanner)
  (deploy-tape !item-scanner 2)
  (deploy-item !item-scanner 1)
  (assert (= (ior !io-count prober-id !item-scanner) 1))

  (io !io-mod (last-id !item-brain) (mod launch.2))

  (let ((n (os.child-cap)))
    (deploy-item !item-transmit n)
    (deploy-item !item-receive n)
    (deploy-item !item-legion n)))


;; Port
;; Requires the spanning tree to figure out where home is.
(defconst port-elem-count 11)
(progn
  (deploy-requirements !item-storage)
  (deploy-tape !item-storage 2)
  (deploy-item !item-battery 8)
  (deploy-item !item-port port-elem-count)

  (when (os.is-home)
    (deploy-item !item-storage port-elem-count)
    (for (i 0) (< i port-elem-count) (+ i 1)
	 (io !io-input (last-id-offset !item-port i) (+ !item-elem-a i))
	 (io !io-item (last-id-offset !item-storage i) (+ !item-elem-a i))))

  (unless (os.is-home)
    (for (i 0) (< i port-elem-count) (+ i 1)
	 (io !io-item (last-id-offset !item-port i) (+ !item-elem-a i) port-item-count)
	 (io !io-target (last-id-offset !item-port i) (os.home)))
    (deploy-item !item-pill (* port-elem-count 2)))

  (for (i 0) (< i port-elem-count) (+ i 1)
       (io !io-activate (last-id-offset !item-port i))))


;; Collider
(defconst burner-count 2)
(defconst collider-size 16)
(defconst collider-elem-count 11)
(progn
  (deploy-item !item-storage collider-elem-count)
  (deploy-item !item-battery 8)
  (deploy-item !item-solar (ior !io-count prober-id !item-solar))
  (deploy-requirements !item-accelerator)
  (deploy-tape !item-accelerator 2)
  (deploy-item !item-collider collider-elem-count)

  (for (i 0) (< i collider-elem-count) (+ i 1)
       (let ((elem-id (+ !item-elem-l i))
	     (collider-id (last-id-offset !item-collider i)))

	 (io !io-grow collider-id collider-size)
	 (while (< (ior !io-state collider-id !io-size) collider-size))

	 (while (not (ior !io-tape-known library-id elem-id)))
	 (io !io-tape collider-id elem-id)
	 (io !io-item (last-id-offset !item-storage i) elem-id)))

  ;; Until we have a burner we need to store the garbage o elements
  (deploy-item !item-storage 2)
  (io !io-item (last-id !item-storage) !item-elem-o)

  ;; Burner
  (deploy-item !item-burner burner-count)
  (for (i 0) (< i burner-count) (+ i 1)
       (io !io-item (last-id !item-burner) !item-elem-o)))


;; Nomad
;; Constants must match values in nomad mod
(defconst nomad-ix-home 0)
(defconst nomad-ix-elem 1)
(defconst nomad-ix-nomad 0)
(defconst nomad-ix-prober 1)
(defconst nomad-ix-scanner 2)
(progn
  (deploy-requirements !item-packer)
  (deploy-tape !item-packer 1)
  (deploy-item !item-nomad 2)
  (io !io-set (id !item-nomad 1) nomad-ix-elem !item-elem-g)
  (io !io-set (id !item-nomad 2) nomad-ix-elem !item-elem-h)

  (unless (is-deployed !item-extract)	(deploy-item !item-extract 1))
  (unless (is-deployed !item-memory)	(deploy-item !item-memory 1))
  (unless (is-deployed !item-worker)	(deploy-item !item-worker 1))
  (unless (is-deployed !item-condenser) (deploy-item !item-condenser 1))
  (unless (is-deployed !item-solar)	(deploy-item !item-solar 1))
  (unless (is-deployed !item-battery)	(deploy-item !item-battery 1))
  (unless (is-deployed !item-transmit)	(deploy-item !item-transmit 1))
  (unless (is-deployed !item-receive)	(deploy-item !item-receive 1))
  (unless (is-deployed !item-port)	(deploy-item !item-port 1))
  (unless (is-deployed !item-pill)	(deploy-item !item-pill 1))

  (for (i 0) (< i 2) (+ i 0)
       (io !io-set (last-id-offset !item-nomad i) nomad-ix-home (ior !io-coord (self)))

       (deploy-item !item-memory 1)
       (let ((nomad-state-id (last-id !item-memory)))
	 (io !io-set nomad-state-id nomad-ix-nomad (last-id-offset !item-nomad i))

	 (deploy-item !item-prober 1)
	 (io !io-set nomad-state-id nomad-ix-prober (last-id !item-prober))
	 (deploy-item !item-scanner 1)
	 (io !io-set nomad-state-id nomad-ix-scanner (last-id !item-scanner))

	 (deploy-item !item-brain 1)
	 (io !io-send (last-id !item-brain) nomad-state-id)
	 (io !io-mod (last-id !item-brain) (mod nomad.2)))))

(io !io-log (self) ?done (ior !io-coord (self)))


;; -----------------------------------------------------------------------------
;; Utils
;; -----------------------------------------------------------------------------

(defconst tech-memory-ix 1)
(defconst tech-memory-id (id !item-memory 1))
(assert (= (io !io-ping tech-memory-id) !io-ok))

(defun is-deployed (item)
  (band (ior !io-get tech-memory-id (+ (/ item 64) tech-memory-ix))
			(bsl 1 (rem item 64))))

(defun deploy-requirements (item)
  (for (req (ior !io-tape-tech library-id item)) req (ior !io-tape-tech library-id item)
       (let ((tech-index (+ (/ req 64) tech-memory-ix)))
	 (when (not (band (ior !io-get tech-memory-id tech-index)
			  (bsl 1 (rem req 64))))

	   (while (not (ior !io-tape-known library-id req)))

	   (let ((host (ior !io-tape-host library-id req)))
	     (io !io-tape assembly-id host 2)
	     (io !io-item deploy-id host 2)
	     (while (ior !io-state deploy-id !io-item))
	     (io !io-tape (id host (- (ior !io-count prober-id host) 0)) req)
	     (io !io-tape (id host (- (ior !io-count prober-id host) 1)) req))

	   ;; Need to pre-compute the value to minimize stack usage
	   (let ((value (bor (ior !io-get tech-memory-id tech-index)
			     (bsl 1 (rem req 64)))))
	     (io !io-set tech-memory-id tech-index value))))))

(defun deploy-item (item n)
  (assert n)
  (deploy-requirements item)

  (while (not (ior !io-tape-known library-id item)))
  (assert (= (ior !io-tape-host library-id item) !item-assembly))

  (io !io-tape assembly-id item n)
  (io !io-item deploy-id item n)
  (while (ior !io-state deploy-id !io-item)))


(defun deploy-tape (item n)
  (assert n)
  (while (not (ior !io-tape-known library-id item)))

  (let ((host (ior !io-tape-host library-id item)))
    (assert (=  (ior !io-tape-host library-id host) !item-assembly))

    (io !io-tape (id !item-assembly 1) host n)
    (io !io-item (id !item-deploy 1) host n)
    (while (ior !io-state deploy-id !io-item))

    (for (i 0) (< i n) (+ i 1)
	 (io !io-tape (last-id-offset host i) item)))

  (io !io-set tech-memory-id
      (+ (/ item 64) tech-memory-ix)
      (bor (ior !io-get tech-memory-id (+ (/ item 64) tech-memory-ix))
	   (bsl 1 (rem item 64)))))

(defun last-id (item)
  (id item (ior !io-count prober-id item)))

(defun last-id-offset (item offset)
  (id item (- (ior !io-count prober-id item) offset)))
