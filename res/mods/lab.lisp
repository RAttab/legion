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
  (research !item-extract library-id)
  (research !item-printer library-id)
  (research !item-assembly library-id)
  (research !item-deploy library-id)
  (research !item-rod library-id)
  (research !item-fusion library-id)
  (research !item-worker library-id)

  ;; Research
  (research !item-brain library-id)
  (research !item-memory library-id)
  (research !item-lab library-id)
  (research !item-library library-id)

  ;; OS & Legion
  (research !item-condenser library-id)
  (research !item-solar library-id)
  (research !item-receive library-id)
  (research !item-transmit library-id)
  (research !item-prober library-id)
  (research !item-scanner library-id)
  (research !item-legion library-id)

  ;; Port
  (research !item-storage library-id)
  (research !item-battery library-id)
  (research !item-port library-id)
  (research !item-pill library-id)

  ;; Collider
  (research !item-accelerator library-id)
  (research !item-collider library-id)
  (research !item-elem-l library-id)
  (research !item-elem-m library-id)
  (research !item-elem-n library-id)
  (research !item-elem-o library-id)
  (research !item-burner library-id)

  ;; Nomad
  (research !item-packer library-id)
  (research !item-nomad library-id))


;; -----------------------------------------------------------------------------
;; Utils
;; -----------------------------------------------------------------------------

(defconst prober-id (id !item-prober 1))
(defun research (item library-id)
  (for (req (ior !io-tape-tech library-id item)) req (ior !io-tape-tech library-id item)
       (when (not (ior !io-tape-learned library-id req))
	 (io !io-log (self) ?lab-research (pack item req))
	 (for (id (ior !io-count prober-id !item-lab)) (> id 0) (- id 1)
	      (io !io-item (id !item-lab id) req))
	 (while (not (ior !io-tape-learned library-id req))))))
