/* items.c
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "ux/ux.h"
#include "game/game.h"
#include "items/items.h"

#include "items/checks.c"
#include "items/ui_tape.c"

#include "items/fusion/fusion.c"
#include "items/brain/brain.c"
#include "items/deploy/deploy.c"
#include "items/extract/extract.c"
#include "items/lab/lab.c"
#include "items/legion/legion.c"
#include "items/memory/memory.c"
#include "items/library/library.c"
#include "items/port/port.c"
#include "items/printer/printer.c"
#include "items/receive/receive.c"
#include "items/prober/prober.c"
#include "items/scanner/scanner.c"
#include "items/storage/storage.c"
#include "items/test/test.c"
#include "items/transmit/transmit.c"
#include "items/collider/collider.c"
#include "items/burner/burner.c"
#include "items/packer/packer.c"
#include "items/nomad/nomad.c"
