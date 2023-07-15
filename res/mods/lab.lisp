;; Goal of this mod is to research all the tech required by boot. It's
;; executed in its own brain as research takes a while and so it's
;; best to run it in parallel with boot.


;; -----------------------------------------------------------------------------
;; Setup
;; -----------------------------------------------------------------------------

(defconst prober-id (id !item-prober 2))
(assert (= (io !io-ping prober-id) !io-ok))

;; Must match boot.lisp
(defconst tech-memory-ix 1)
(defconst tech-memory-id (id !item-memory 1))
(assert (= (io !io-ping tech-memory-id) !io-ok))

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
  (learn-reqs !item-deploy library-id)
  (learn-reqs !item-rod library-id)
  (learn-reqs !item-fusion library-id)
  (learn-reqs !item-worker library-id)

  ;; Research
  (learn-reqs !item-brain library-id)
  (learn-reqs !item-memory library-id)
  (learn-reqs !item-lab library-id)
  (learn-reqs !item-library library-id)

  ;; OS & Legion
  (learn-reqs !item-condenser library-id)
  (learn-item !item-condenser library-id)
  (learn-reqs !item-solar library-id)
  (learn-reqs !item-receive library-id)
  (learn-reqs !item-transmit library-id)
  (learn-reqs !item-prober library-id)
  (learn-reqs !item-scanner library-id)
  (learn-reqs !item-legion library-id)

  ;; Port
  (learn-reqs !item-storage library-id)
  (learn-reqs !item-battery library-id)
  (learn-reqs !item-port library-id)
  (learn-reqs !item-pill library-id)

  ;; Collider
  (learn-reqs !item-accelerator library-id)
  (learn-item !item-collider library-id)
  (learn-reqs !item-elem-l library-id)
  (learn-reqs !item-elem-m library-id)
  (learn-reqs !item-elem-n library-id)
  (learn-reqs !item-elem-o library-id)
  (learn-reqs !item-burner library-id)

  ;; Nomad
  (learn-reqs !item-packer library-id)
  (learn-reqs !item-nomad library-id))


;; -----------------------------------------------------------------------------
;; Utils
;; -----------------------------------------------------------------------------

(defun learn-reqs (item library-id)
  (for (req (ior !io-tape-tech library-id item)) req (ior !io-tape-tech library-id item)
       (when (not (ior !io-tape-learned library-id req))
	 (while (not (band (ior !io-get tech-memory-id (+ (/ req 64) tech-memory-ix))
			   (bsl 1 (rem req 64)))))

	 (io !io-log (self) ?lab-research (pack item req))
	 (for (id (ior !io-count prober-id !item-lab)) (> id 0) (- id 1)
	      (io !io-item (id !item-lab id) req))

	 (while (not (ior !io-tape-learned library-id req))))))

(defun learn-item (item library-id)
  (when (not (ior !io-tape-learned library-id item))
    (while (not (band (ior !io-get tech-memory-id (+ (/ item 64) tech-memory-ix))
		      (bsl 1 (rem item 64)))))

    (io !io-log (self) ?lab-research (pack 0 item))
    (for (id (ior !io-count prober-id !item-lab)) (> id 0) (- id 1)
	 (io !io-item (id !item-lab id) item))

    (while (not (ior !io-tape-learned library-id item)))))
