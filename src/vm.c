/* vm.c
   Rémi Attab (remi.attab@gmail.com), 28 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "db.h"
#include "vm.h"
#include "game.h"
#include "engine.h"

#include "utils/fs.h"
#include "utils/bits.h"
#include "utils/err.h"
#include "utils/vec.h"
#include "utils/str.h"
#include "utils/save.h"
#include "utils/hset.h"
#include "utils/token.h"
#include "utils/htable.h"
#include "utils/config.h"

#include <stdarg.h>

#include "vm/vm.c"
#include "vm/op.c"
#include "vm/mod.c"
#include "vm/lisp.c"
#include "vm/atoms.c"
#include "vm/asm.c"
#include "vm/ast.c"
#include "vm/code.c"
