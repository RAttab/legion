/* gen.c
   Rémi Attab (remi.attab@gmail.com), 12 Jun 2023
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"

#include "utils/err.h"
#include "utils/fs.h"
#include "utils/rng.h"
#include "utils/str.h"
#include "utils/vec.h"
#include "utils/bits.h"
#include "utils/htable.h"
#include "utils/symbol.h"
#include "utils/config.h"

#include <sys/mman.h>

void usage(int code, const char *msg);

#include "gen/types.h"
#include "gen/db.c"
#include "gen/tech.c"

