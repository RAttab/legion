;; Goal is to create a spanning tree of world all running (mod os 2)
;; which communicates via antennas that are setup to form a spanning
;; tree.
;;
;; To accomplish this we must setup a basic factory infrastructure
;; that can create legions and antennas. Legion are propagated via the
;; (mod launch.2) module and research to get to antenna is done via
;; the (mod lab.2) module.

(defconst max-depth 4)

(defconst extract-count 2)
(defconst condenser-count 2)
(defconst printer-count 4)
(defconst assembly-count 2)
(defconst worker-count 20)
(defconst lab-count 2)
(defconst burner-count 2)
(defconst active-count 1)
(defconst energy-target 64)
(defconst port-item-count 255)

;; Sanity checks
(defconst prober-id (id !item-prober 1))
(assert (= (io !io-ping prober-id) !io-ok))
(defconst deploy-id (id !item-deploy 1))
(assert (= (io !io-ping deploy-id) !io-ok))


;; Home
(progn
  (when (> (count !item-brain) 1) (reset))
  (when (os.is-home) (io !io-name (self) ?Bob-The-Homeworld))
  (io !io-log (self) ?booting (ior !io-coord (self))))


;; Extract
(progn
  (set-tape 1 1 !item-extract !item-elem-a)
  (set-tape 2 1 !item-extract !item-elem-b)
  (set-tape 1 1 !item-printer !item-monobarex)
  (set-tape 2 1 !item-printer !item-monobararkon)

  (deploy-tape !item-extract !item-elem-b extract-count)
  (set-tape 1 2 !item-extract !item-elem-a)
  (deploy-tape !item-extract !item-elem-c extract-count)
  (deploy-tape !item-extract !item-elem-d extract-count))


;; Printers
(progn
  (set-tape 1 1 !item-printer !item-monobarex)
  (set-tape 2 1 !item-printer !item-monocharkoid)

  (deploy-tape !item-printer !item-monarkols printer-count)
  (deploy-tape !item-printer !item-monobarols printer-count)
  (deploy-tape !item-printer !item-monochate printer-count)
  (deploy-tape !item-printer !item-monocharkoid printer-count)
  (deploy-tape !item-printer !item-monobararkon printer-count)
  (deploy-tape !item-printer !item-duodylium printer-count)
  (deploy-tape !item-printer !item-duodylitil printer-count)

  (set-tape 1 2 !item-printer !item-monobarex))

;; Rod
(progn
  (set-tape 2 1 !item-assembly !item-tridylarkitil)
  (deploy-tape !item-assembly !item-extract assembly-count)
  (deploy-tape !item-assembly !item-rod assembly-count))
  (deploy-item !item-fusion 4)

;; Workers
(progn
  (set-tape 2 1 !item-assembly !item-tridylarkitil)
  (deploy-tape !item-assembly !item-printer assembly-count)
  (deploy-item !item-worker worker-count))


;; Assembly
(progn
  ;; Required to build assemblies
  (set-tape 2 1 !item-assembly !item-tridylarkitil)
  (deploy-tape !item-assembly !item-tridylarkitil assembly-count)
  (io !io-reset (id !item-assembly 2))

  ;; Passives
  (deploy-tape !item-printer !item-tridylate printer-count)
  (deploy-tape !item-assembly !item-duochium assembly-count)
  (deploy-tape !item-assembly !item-trichubarium assembly-count)
  (deploy-tape !item-assembly !item-tetradylchols-tribarsh assembly-count)
  (deploy-tape !item-assembly !item-pentadylchutor assembly-count)
  (deploy-tape !item-assembly !item-hexadylchate-pentabaron assembly-count)

  ;; Requirements
  (deploy-tape !item-assembly !item-memory assembly-count))


;; OS - we reserve the juicy brain and memory ids for os
(defconst brain-os-id (id !item-brain 2))
(defconst brain-exec-id (id !item-brain 3))
(progn
  (deploy-item !item-brain 2)
  (deploy-item !item-memory (os.memory-need))
  (assert (= (io !io-ping brain-os-id) !io-ok))
  (assert (= (io !io-ping brain-exec-id) !io-ok)))


;; Labs
(progn
  (deploy-tape !item-assembly !item-deploy assembly-count)
  (deploy-tape !item-assembly !item-brain assembly-count)
  (deploy-item !item-lab lab-count)
  (deploy-item !item-brain 1)

  (let ((id-brain (id !item-brain (count !item-brain))))
    (assert (= (io !io-send id-brain ?lab-count lab-count) !io-ok))
    (assert (= (io !io-mod id-brain (mod lab.2)) !io-ok))))


;; Condenser
(progn
  (deploy-tape-wait-tech !item-assembly !item-tetradylchitil-duobarate assembly-count)
  (deploy-tape-wait-tech !item-assembly !item-pentadylchate assembly-count)
  (wait-tech !item-condenser)

  (deploy-tape !item-condenser !item-elem-e condenser-count)
  (deploy-tape !item-condenser !item-elem-f condenser-count))


;; Solar
(progn
  (deploy-tape-wait-tech !item-printer !item-duerltor printer-count)
  (deploy-tape-wait-tech !item-printer !item-duerldylon-monochols printer-count)
  (deploy-tape-wait-tech !item-printer !item-trifimbarsh printer-count)
  (deploy-tape-wait-tech !item-assembly !item-solar assembly-count)

  (deploy-item !item-solar
	       (+ (/ energy-target
		     (specs !spec-solar-energy (count !item-energy)))
		  1)))


;; OS
(progn
  ;; Requirements for data network
  (deploy-tape-wait-tech !item-assembly !item-receive assembly-count)
  (deploy-tape-wait-tech !item-printer !item-trifimate printer-count)
  (deploy-tape-wait-tech !item-assembly !item-tetrafimry assembly-count)
  (deploy-tape-wait-tech !item-assembly !item-transmit assembly-count)

  ;; Parent network - won't be used in homeworld but still needed to
  ;; make the ids align.
  (deploy-item !item-receive 1)
  (deploy-item !item-transmit 1)

  ;; Boot
  (os.boot brain-os-id brain-exec-id))


;; Legion
(when (<= (os.depth) max-depth)

  (deploy-item !item-brain 1)
  (deploy-item !item-prober 1)
  (deploy-item-wait-tech !item-scanner 1)

  (assert (= (count !item-scanner) 1))
  (assert (= (count !item-prober) 2))
  (let ((brain-id (id !item-brain (count !item-brain))))
    (io !io-mod brain-id (mod launch.2)) !io-ok)

  (deploy-tape !item-assembly !item-fusion assembly-count)
  (deploy-tape !item-assembly !item-assembly assembly-count)
  (deploy-tape !item-assembly !item-worker assembly-count)
  (deploy-tape !item-assembly !item-prober assembly-count)
  (deploy-tape !item-assembly !item-legion assembly-count)
  (deploy-tape !item-assembly !item-brain assembly-count)
  (deploy-tape !item-assembly !item-prober assembly-count)

  (let ((n (os.child-cap)))
    (deploy-item !item-transmit n)
    (deploy-item !item-receive n)
    (deploy-item !item-legion n)))


;; Battery
(progn
  (deploy-tape-wait-tech !item-printer !item-duerldylon-monochols printer-count)
  (deploy-tape-wait-tech !item-printer !item-monochury printer-count)
  (deploy-tape-wait-tech !item-printer !item-trifimbarsh printer-count)
  (deploy-tape-wait-tech !item-printer !item-duerlry printer-count)
  (deploy-tape-wait-tech !item-assembly !item-trerlchury-duobargen assembly-count)
  (deploy-tape-wait-tech !item-assembly !item-storage assembly-count)
  (deploy-tape-wait-tech !item-assembly !item-battery printer-count)

  (deploy-item !item-battery 8))


;; Port
;; Requires the spanning tree to figure out where home is.
(defconst elem-count 11)
(progn
  (deploy-tape-wait-tech !item-printer !item-duerlex printer-count)
  (deploy-tape-wait-tech !item-assembly !item-tetrafimalt assembly-count)
  (deploy-tape-wait-tech !item-assembly !item-pentafimchex-monobarsh assembly-count)
  (deploy-item-wait-tech !item-port elem-count)

  (when (os.is-home)
    (deploy-item !item-storage elem-count)
    (for (i 0) (< i elem-count) (+ i 1)
	 (io !io-input (id !item-port (+ i 1)) (+ !item-elem-a i))
	 (io !io-item (id !item-storage (+ i 1)) (+ !item-elem-a i))))


  (deploy-tape-wait-tech !item-assembly !item-tetrerlbargen assembly-count)
  (deploy-tape-wait-tech !item-assembly !item-penterltor assembly-count)
  (wait-tech !item-pill)

  (unless (os.is-home)
    (for (i 0) (< i elem-count) (+ i 1)
	 (io !io-item (id !item-port (+ i 1)) (+ !item-elem-a i) port-item-count)
	 (io !io-target (id !item-port i) (os.home)))
    (deploy-item !item-pill (* elem-count 2)))

  (for (id 1) (<= id elem-count) (+ id 1)
       (io !io-activate (id !item-port id))))


;; Collider
(defconst collider-size 16)
(defconst collider-id (id !item-collider 1))
(progn
  (deploy-item !item-battery 8)
  (deploy-item !item-solar
	       (+ (/ energy-target
		     (specs !spec-solar-energy (count !item-energy)))
		  1))

  (deploy-tape-wait-tech !item-assembly !item-accelerator assembly-count)
  (wait-tech !item-collider)

  (for (id 0) (< id 3) (+ id 1)
       (deploy-item !item-collider 1)

       (let ((elem-id (+ !item-elem-m id))
	     (collider-id (id !item-collider (count !item-collider))))
	 (io !io-grow collider-id collider-size)
	 (while (< (ior !io-state collider-id !io-size) collider-size))

	 (wait-tech elem-id)
	 (io !io-tape collider-id elem-id)

	 (deploy-item !item-storage 1)
	 (io !io-item (id !item-storage (count !item-storage)) elem-id)))

  ;; Until we have a burner we need to store the garbage o elements
  (deploy-item !item-storage 1)
  (io !io-item (id !item-storage (count !item-storage)) !item-elem-o)

  ;; Burner
  (deploy-tape-wait-tech !item-printer !item-pentalofchols printer-count)
  (deploy-item-wait-tech !item-burner burner-count)
  (set-item 1 burner-count !item-burner !item-elem-o))


;; Nomad
;; Constants must match values in nomad mod
(defconst nomad-ix-home 0)
(defconst nomad-ix-elem 1)
(defconst nomad-ix-nomad 0)
(defconst nomad-ix-prober 1)
(defconst nomad-ix-scanner 2)
(progn
  (deploy-tape !item-assembly !item-pill assembly-count)

  (deploy-tape-wait-tech !item-assembly !item-pentamoxate assembly-count)
  (deploy-tape-wait-tech !item-assembly !item-hexamoxchoid-monobary assembly-count)
  (deploy-tape-wait-tech !item-assembly !item-packer assembly-count)

  (deploy-tape !item-assembly !item-condenser assembly-count)
  (deploy-tape !item-assembly !item-transmit assembly-count)
  (deploy-tape !item-assembly !item-prober assembly-count)
  (deploy-tape !item-assembly !item-port assembly-count)

  (deploy-item-wait-tech !item-nomad 2)
  (io !io-set (id !item-nomad 1) nomad-ix-elem !item-elem-g)
  (io !io-set (id !item-nomad 2) nomad-ix-elem !item-elem-h)

  (for (it 1) (<= it 2) (+ it 1)
       (io !io-set (id !item-nomad it) nomad-ix-home (ior !io-coord (self)))

       (deploy-item !item-memory 1)
       (let ((state-id (id !item-memory (count !item-memory))))
	 (io !io-set state-id nomad-ix-nomad (id !item-nomad it))
	 (deploy-item !item-prober 1)
	 (io !io-set state-id nomad-ix-prober (id !item-prober (count !item-prober)))
	 (deploy-item !item-scanner 1)
	 (io !io-set state-id nomad-ix-scanner (id !item-scanner (count !item-scanner)))

	 (deploy-item !item-brain 1)
	 (io !io-send (id !item-brain (count !item-brain)) state-id)
	 (io !io-mod (id !item-brain (count !item-brain)) (mod nomad.2)))))

(io !io-log (self) ?done (ior !io-coord (self)))


;; -----------------------------------------------------------------------------
;; utils
;; -----------------------------------------------------------------------------

(defun count (item)
  (io !io-probe prober-id item (ior !io-coord (self)))
  (let ((count -1))
    (while (< count 0) (set count (ior !io-value prober-id)))
    count))

(defun set-tape (id n host tape)
  (assert (> id 0))
  (set n (+ id n))
  (while (< id n)
    (assert (= (io !io-tape (id host id) tape) !io-ok))
    (set id (+ id 1))))

(defun set-item (id n host item)
  (assert (> id 0))
  (set n (+ id n))
  (while (< id n)
    (assert (= (io !io-item (id host id) item) !io-ok))
    (set id (+ id 1))))

(defun deploy-item (item n)
  (assert (= (io !io-tape (id !item-assembly 1) item n) !io-ok))
  (assert (= (io !io-item (id !item-deploy 1) item n) !io-ok))
  (while (/= (ior !io-state deploy-id !io-item) 0)))

(defun deploy-item-wait-tech (item n)
  (assert (= (io !io-ping (id !item-lab 1)) !io-ok))
  (while (= (ior !io-tape-known (id !item-lab 1) item) 0))

  (assert (= (io !io-tape (id !item-assembly 1) item n) !io-ok))
  (assert (= (io !io-item (id !item-deploy 1) item n) !io-ok))
  (while (/= (ior !io-state deploy-id !io-item) 0)))

(defun deploy-tape (host tape n)
  (let ((id (+ (count host) 1)))
    (assert (> id 0))
    (assert (= (io !io-tape (id !item-assembly 1) host n) !io-ok))
    (assert (= (io !io-item (id !item-deploy 1) host n) !io-ok))
    (while (/= (ior !io-state deploy-id !io-item) 0))
    (set-tape id n host tape)))

(defun deploy-tape-wait-tech (host tape n)
  (assert (= (io !io-ping (id !item-lab 1)) !io-ok))
  (while (= (ior !io-tape-known (id !item-lab 1) tape) 0))

  (let ((id (+ (count host) 1)))
    (assert (> id 0))
    (assert (= (io !io-tape (id !item-assembly 1) host n) !io-ok))
    (assert (= (io !io-item (id !item-deploy 1) host n) !io-ok))
    (while (/= (ior !io-state deploy-id !io-item) 0))
    (set-tape id n host tape)))

(defun wait-tech (item)
  (assert (= (io !io-ping (id !item-lab 1)) !io-ok))
  (while (= (ior !io-tape-known (id !item-lab 1) item) 0)))
