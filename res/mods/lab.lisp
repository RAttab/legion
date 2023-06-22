;; Research all the T0 items.
;;
;; Number of active lab is received through the message queue. While
;; there are easier ways to get it, we need to test that functionality
;; somewhere and this seems as good a time as any.

(io !io-recv (self))
(assert (= (head) 2))
(assert (= (head) ?lab-count))
(let ((n (head)))

  ;; In-order list of all the deploy-tape function calls in boot.lisp
  ;; plus a few tweaks here and there.

  ;; Extract
  (lab n !item-elem-a)
  (lab n !item-elem-b)
  (lab n !item-elem-c)
  (lab n !item-elem-d)

  ;; Printers
  (lab n !item-monobarex)
  (lab n !item-monarkols)
  (lab n !item-monobarols)
  (lab n !item-monochate)
  (lab n !item-monocharkoid)
  (lab n !item-monobararkon)
  (lab n !item-duodylium)
  (lab n !item-duodylitil)

  ;; Workers
  (lab n !item-printer)

  ;; Assembly
  (lab n !item-tridylarkitil)
  (lab n !item-extract)
  (lab n !item-rod)
  (lab n !item-duochium)
  (lab n !item-tridylate)
  (lab n !item-trichubarium)
  (lab n !item-tetradylchols-tribarsh)
  (lab n !item-pentadylchutor)
  (lab n !item-hexadylchate-pentabaron)
  (lab n !item-memory)
  (lab n !item-deploy)

  ;; Condenser
  (lab n !item-tetradylchitil-duobarate)
  (lab n !item-pentadylchate)
  (lab n !item-elem-e)
  (lab n !item-elem-f)

  ;; Solar
  (lab n !item-duerltor)
  (lab n !item-duerldylon-monochols)
  (lab n !item-trifimbarsh)
  (lab n !item-solar)

  ;; OS
  (lab n !item-receive)
  (lab n !item-trifimate)
  (lab n !item-tetrafimry)
  (lab n !item-transmit)

  ;; Legion
  (lab n !item-fusion)
  (lab n !item-assembly)
  (lab n !item-worker)
  (lab n !item-prober)
  (lab n !item-legion)
  (lab n !item-brain)
  (lab n !item-prober)

  ;; Battery
  (lab n !item-duerldylon-monochols)
  (lab n !item-monochury)
  (lab n !item-trifimbarsh)
  (lab n !item-duerlry)
  (lab n !item-trerlchury-duobargen)
  (lab n !item-storage)
  (lab n !item-battery)

  ;; Port
  (lab n !item-duerlex)
  (lab n !item-tetrafimalt)
  (lab n !item-pentafimchex-monobarsh)
  (lab n !item-tetrerlbargen)
  (lab n !item-penterltor)

  ;; Collider
  (lab n !item-accelerator)
  (lab n !item-elem-m)
  (lab n !item-elem-n)
  (lab n !item-elem-l)
  (lab n !item-pentalofchols)
  (lab n !item-pill)

  ;; Nomad
  (lab n !item-pentamoxate)
  (lab n !item-hexamoxchoid-monobary)
  (lab n !item-packer)
  (lab n !item-condenser)
  (lab n !item-transmit)
  (lab n !item-prober)
  (lab n !item-port)
  (lab n !item-elem-g)
  (lab n !item-elem-h))


(defun lab (n item)
  (assert (= (io !io-ping (id !item-lab 1)) !io-ok))
  (for (id 1) (<= id n) (+ id 1) (io !io-item (id !item-lab id) item))
  (for (known 0) (not known) (ior !io-item-known (id !item-lab 1) item)))
