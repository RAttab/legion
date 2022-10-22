/* energy.h
   Rémi Attab (remi.attab@gmail.com), 23 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/sector.h"

struct save;


// -----------------------------------------------------------------------------
// energy
// -----------------------------------------------------------------------------

typedef uint32_t im_energy;

struct energy
{
    uint8_t solar, kwheel, battery;
    im_energy produced, consumed, need;

    struct {
        im_energy burner;
        struct { im_energy produced, stored; } battery;
        struct { im_energy produced, saved, next; } fusion;
    } item;
};


im_energy energy_battery_cap(const struct energy *en);
im_energy energy_solar_output(im_energy star, size_t solar);
im_energy energy_prod_solar(const struct energy *, const struct star *);
im_energy energy_kwheel_output(uint8_t elem_k, size_t kwheel);
im_energy energy_prod_kwheel(const struct energy *, const struct star *);
    
bool energy_consume(struct energy *, im_energy);
void energy_produce_burner(struct energy *, im_energy);

void energy_step_begin(struct energy *, const struct star *star);
// runs right before energy_step_end.
im_energy energy_step_fusion(struct energy *, im_energy produced, im_energy cap);
void energy_step_end(struct energy *);


void energy_save(const struct energy *, struct save *);
bool energy_load(struct energy *, struct save *);
