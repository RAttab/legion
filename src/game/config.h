/* config.h
   Rémi Attab (remi.attab@gmail.com), 03 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/item.h"
#include "vm/vm.h"

struct chunk;


// -----------------------------------------------------------------------------
// item_config
// -----------------------------------------------------------------------------

typedef void (*init_fn_t) (void *state, id_t id, struct chunk *);
typedef void (*step_fn_t) (void *state, struct chunk *);
typedef void (*cmd_fn_t) (
        void *state, struct chunk *,
        enum atom_io cmd, id_t src, size_t len, const word_t *args);

struct item_config
{
    size_t size;
    init_fn_t init;
    step_fn_t step;
    cmd_fn_t cmd;
};

const struct item_config *item_config(item_t);


// -----------------------------------------------------------------------------
// impl
// -----------------------------------------------------------------------------

const struct item_config *worker_config(void);
const struct item_config *printer_config(void);
const struct item_config *miner_config(void);
const struct item_config *deployer_config(void);
const struct item_config *brain_config(item_t);
const struct item_config *db_config(item_t);

