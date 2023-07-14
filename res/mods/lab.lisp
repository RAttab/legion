;; Goal of this mod is to research all the tech required by boot. It's
;; executed in its own brain as research takes a while and so it's
;; best to run it in parallel with boot.


;; -----------------------------------------------------------------------------
;; Setup
;; -----------------------------------------------------------------------------

(io !io-recv (self))
(assert (= (head) 2))
(assert (= (head) ?lab-boot))
(let ((library-id (head)))


;; -----------------------------------------------------------------------------
;; Research
;; -----------------------------------------------------------------------------

  ;; Setup
  (learn-item !item-extract library-id)
  (learn-item !item-printer library-id)
  (learn-item !item-assembly library-id)
  (learn-req !item-deploy library-id)
  (learn-req !item-rod library-id)
  (learn-req !item-fusion library-id)
  (learn-req !item-worker library-id)

  ;; Research
  (learn-req !item-brain library-id)
  (learn-req !item-memory library-id)
  (learn-req !item-lab library-id)
  (learn-req !item-library library-id)

  ;; OS & Legion
  (learn-req !item-condenser library-id)
  (learn-item !item-condenser library-id)
  (learn-req !item-solar library-id)
  (learn-req !item-receive library-id)
  (learn-req !item-transmit library-id)
  (learn-req !item-prober library-id)
  (learn-req !item-scanner library-id)
  (learn-req !item-legion library-id)

  ;; Port
  (learn-req !item-storage library-id)
  (learn-req !item-battery library-id)
  (learn-req !item-port library-id)
  (learn-req !item-pill library-id)

  ;; Collider
  (learn-req !item-accelerator library-id)
  (learn-item !item-collider library-id)
  (learn-req !item-elem-l library-id)
  (learn-req !item-elem-m library-id)
  (learn-req !item-elem-n library-id)
  (learn-req !item-elem-o library-id)
  (learn-req !item-burner library-id)

  ;; Nomad
  (learn-req !item-packer library-id)
  (learn-req !item-nomad library-id))


;; -----------------------------------------------------------------------------
;; Utils
;; -----------------------------------------------------------------------------

(defconst prober-id (id !item-prober 1))

(defun learn-req (item library-id)
  (for (req (ior !io-tape-tech library-id item)) req (ior !io-tape-tech library-id item)
       (when (not (ior !io-tape-learned library-id req))
	 (io !io-log (self) ?lab-research (pack item req))
	 (for (id (ior !io-count prober-id !item-lab)) (> id 0) (- id 1)
	      (io !io-item (id !item-lab id) req))
	 (while (not (ior !io-tape-learned library-id req))))))

(defun learn-item (item library-id)
  (when (not (ior !io-tape-learned library-id item))
    (io !io-log (self) ?lab-research (pack 0 item))
    (for (id (ior !io-count prober-id !item-lab)) (> id 0) (- id 1)
	 (io !io-item (id !item-lab id) item))
    (while (not (ior !io-tape-learned library-id item)))))

