;; Manages the booting and roaming operations of a nomad

(defconst extract-count 4)
(defconst pill-count 4)
(defconst pill-elem-count 100)


(defconst ix-home 0)
(defconst ix-elem 1)

(defconst ix-nomad 0)
(defconst ix-prober 1)
(defconst ix-scanner 2)
(defconst ix-target 6)

;; The boot mod places sends a message to the brain before loading the
;; mod. Using this message we can easily tell whether we're booting in
;; the homeworld or whether we're roaming.
(if (ior !io-recv (self))
    (boot (head))
    (roam))


;; -----------------------------------------------------------------------------
;; booting
;; -----------------------------------------------------------------------------

(defun boot (state-id)
  (let ((nomad-id (ior !io-get state-id ix-nomad)))
    (load-item nomad-id !item-memory 1)
    (load-item nomad-id !item-worker 4)
    (load-item nomad-id !item-solar 20)
    (load-item nomad-id !item-battery 4)
    (load-item nomad-id !item-port 1)
    (load-item nomad-id !item-pill pill-count)
    (load-item nomad-id !item-transmit 1)
    (load-item nomad-id !item-receive 1)
    (if (>= (ior !io-get nomad-id ix-elem) !item-elem-i)
	(load-item nomad-id !item-condenser extract-count)
	(load-item nomad-id !item-extract extract-count)))
  (scan state-id)
  (launch state-id))

(defun load-item (nomad-id item n)
  (io !io-load nomad-id item n)
  (while (/= (ior !io-state nomad-id !io-item) 0)))


;; -----------------------------------------------------------------------------
;; roaming
;; -----------------------------------------------------------------------------

(defun roam ()
  (io !io-log (self) ?nomad-arrive (ior !io-get (id !item-nomad 1) ix-home))

  ;; Setup
  (let ((elem (ior !io-get (id !item-nomad 1) ix-elem)))
    (let ((extract (if (>= elem !item-elem-i) !item-condenser !item-extract)))
      (for (i 1) (<= i extract-count) (+ i 1)
	   (io !io-tape (id extract i) elem)))

    (let ((home (ior !io-get (id !item-nomad 1) ix-home)))
      (io !io-target (id !item-transmit 1) home)
      (io !io-target (id !item-receive 1) home)
      (io !io-send (id !item-transmit 1)
	  ?os-roam-unpack (ior !io-coord (self)) elem)

      (io !io-target (id !item-port 1) home)
      (io !io-item (id !item-port 1) elem pill-elem-count)
      (io !io-activate (id !item-port 1))))

  ;; Extract + Scan
  (progn
    (io !io-set (id !item-memory 1) ix-nomad (id !item-nomad 1))
    (io !io-set (id !item-memory 1) ix-prober (id !item-prober 1))
    (io !io-set (id !item-memory 1) ix-scanner (id !item-scanner 1))
    (io !io-set (id !item-memory 1) ix-target 0)
    (scan (id !item-memory 1))

    (let ((elem (ior !io-get (id !item-nomad 1) ix-elem)))
      (while (> (ior !io-probe (id !item-prober 1) elem) 0)))

    (io !io-send (id !item-transmit 1)
	?os-roam-pack (ior !io-get (id !item-memory 1) ix-target))
    (while (not (ior !io-receive (id !item-receive 1))))
    (assert (= (head) ?os-roam-next))

    ;; Wait for all the pills to have returned before moving on.
    (io !io-reset (id !item-port 1)) ;; undock any pending pills
    (while (< (ior !io-probe (id !item-prober 1) !item-pill) pill-count)))

  ;; Pack
  (progn
    (pack-item (id !item-nomad 1) !item-worker)
    (pack-item (id !item-nomad 1) !item-solar)
    (pack-item (id !item-nomad 1) !item-transmit)
    (pack-item (id !item-nomad 1) !item-receive)
    (pack-item (id !item-nomad 1) !item-port)
    (pack-item (id !item-nomad 1) !item-pill)
    (if (>= (ior !io-get (id !item-nomad 1) ix-elem) !item-elem-i)
	(pack-item (id !item-nomad 1) !item-condenser)
	(pack-item (id !item-nomad 1) !item-extract)))

  ;; Launch
  (launch (id !item-memory 1)))

(defun pack-item (nomad-id item)
  (io !io-pack nomad-id item)
  (while (/= (ior !io-state nomad-id !io-item) 0)))


;; -----------------------------------------------------------------------------
;; launch
;; -----------------------------------------------------------------------------

(defun launch (state-id)
  (let ((nomad-id (ior !io-get state-id ix-nomad))
	(target (ior !io-get state-id ix-target)))
    (io !io-log (self) ?nomad-launch target)
    (pack nomad-id (ior !io-get state-id ix-prober))
    (pack nomad-id (ior !io-get state-id ix-scanner))
    (pack nomad-id state-id)
    (io !io-mod nomad-id (mod nomad.2))
    (io !io-launch nomad-id target (ior !io-id (self))))
  (fault))


;; -----------------------------------------------------------------------------
;; scan
;; -----------------------------------------------------------------------------

(defconst scan-ongoing -1)
(defconst scan-done 0)

(defun scan (state-id)
  (io !io-scan
      (ior !io-get state-id ix-scanner)
      (ior !io-coord (self)))

  (while (not (ior !io-get state-id ix-target))

    (case (ior !io-value (ior !io-get state-id ix-scanner))
      ((scan-ongoing 0)
       (scan-done
	(io !io-scan
	    (ior !io-get state-id ix-scanner)
	    (coord-inc (ior !io-state (ior !io-get state-id ix-scanner) !io-target)))))

      (star
       (when (> (probe star
		    (ior !io-get state-id ix-prober)
		    (ior !io-get (ior !io-get state-id ix-nomad) ix-elem))
		2000)
	 (io !io-set state-id ix-target star))))))


(defun probe (coord prober-id elem)
  (io !io-probe prober-id elem coord)
  (let ((count scan-ongoing))
    (while (= count scan-ongoing) (set count (ior !io-value prober-id)))
    count))

;; We're incrementing the packed coord
(defconst inc-y (bsl 1 (+ 16)))
(defconst inc-x (bsl 1 (+ 32 16)))
(defun coord-inc (coord)
  (case (rem (ior !io-tick (self)) 5)
    ((0 (+ coord inc-x))
     (1 (- coord inc-x))
     (2 (+ coord inc-y))
     (3 (- coord inc-y))
     (4 coord))))
