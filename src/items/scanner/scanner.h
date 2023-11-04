/* scanner.h
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

struct im_config;


// -----------------------------------------------------------------------------
// scan
// -----------------------------------------------------------------------------

struct legion_packed im_scanner
{
    im_id id;

    struct { im_work left, cap; } work;

    legion_pad(4);

    struct world_scan_it it;
    vm_word result;
};

static_assert(sizeof(struct im_scanner) == 32);

void im_scanner_config(struct im_config *);

im_work im_scanner_work_cap(struct coord origin, struct coord target);
