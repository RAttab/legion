/* test.h
   Rémi Attab (remi.attab@gmail.com), 26 Aug 2021
   FreeBSD-style copyright and disclaimer apply

   This is not a real item but a helper for test which can record the return
   value of io operations executed with it as the source.
*/

#pragma once

#include "common.h"
#include "game/id.h"
#include "db/io.h"
#include "db/items.h"
#include "vm/vm.h"

struct im_config;


// -----------------------------------------------------------------------------
// test
// -----------------------------------------------------------------------------

struct legion_packed im_test
{
    im_id src;
    enum io io;
    uint8_t len;
    legion_pad(3);
    vm_word args[7];
};

static_assert(sizeof(struct im_test) == 64);


bool im_test_check(const struct im_test *,
        enum io io, im_id src, const vm_word *args, size_t len);

void im_test_config(struct im_config *);
