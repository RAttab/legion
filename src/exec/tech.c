/* tech.c
   Rémi Attab (remi.attab@gmail.com), 05 Feb 2023
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "vm/symbol.h"
#include "items/im_type.h"
#include "utils/fs.h"
#include "utils/rng.h"
#include "utils/str.h"
#include "utils/bits.h"
#include "utils/htable.h"
#include "utils/config.h"

#include <sys/mman.h>

// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

enum
{
    layer_cap = 16,
    index_cap = 16,
    edges_cap = 16,
    name_cap = symbol_cap - 8,

    threshold_div = 5,
    check_mult = 8,
    check_div = 10,

    check_show_ins_elems = false,
    check_show_ins_work = false,
    check_show_ins_energy = false,
};

// used in legion_min so not having it as an enum solves some casting headaches.
static const size_t child_count_cap = 32;

// -----------------------------------------------------------------------------
// implementation
// -----------------------------------------------------------------------------

#include "tech_tree.c"
#include "tech_parse.c"
#include "tech_check.c"
#include "tech_gen.c"
#include "tech_dump.c"

// -----------------------------------------------------------------------------
// main
// -----------------------------------------------------------------------------

bool tech_run(const char *path, const char *output)
{
    struct tree tree = tree_init();
    tech_parse(&tree, path);
    tech_check_inputs(&tree);
    tech_gen(&tree);
    tech_check_outputs(&tree);
    tech_dump(&tree, output);
    return true;
}
