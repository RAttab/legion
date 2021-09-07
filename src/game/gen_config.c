/* gen_stars.c
   Rémi Attab (remi.attab@gmail.com), 08 Sep 2021
   FreeBSD-style copyright and disclaimer apply
*/

// included from games/gen.c

gen_star(starter, 50,
        gen_one(ITEM_ENERGY, 30000),
        gen_one(ITEM_ELEM_K, 100),
        gen_all_of(ITEM_ELEM_A, ITEM_ELEM_F, 60000),
        gen_all_of(ITEM_ELEM_G, ITEM_ELEM_H, 30000),
        gen_all_of(ITEM_ELEM_H, ITEM_ELEM_I, 10000)),

gen_star(barren, 200,
        gen_one(ITEM_ENERGY, 100),
        gen_one(ITEM_ELEM_K, 100),
        gen_rng(ITEM_ELEM_A, ITEM_ELEM_J, 100)),

gen_star(extract, 450,
        gen_one(ITEM_ENERGY, 10000),
        gen_one(ITEM_ELEM_K, 100),
        gen_rng(ITEM_ELEM_A, ITEM_ELEM_J, 1000),
        gen_one_of(ITEM_ELEM_A, ITEM_ELEM_F, UINT16_MAX)),

gen_star(condenser, 250,
        gen_one(ITEM_ENERGY, 10000),
        gen_one(ITEM_ELEM_K, 100),
        gen_rng(ITEM_ELEM_A, ITEM_ELEM_J, 1000),
        gen_one_of(ITEM_ELEM_G, ITEM_ELEM_J, UINT16_MAX)),

gen_star(power, 50,
        gen_one(ITEM_ELEM_K, UINT16_MAX),
        gen_one(ITEM_ENERGY, UINT16_MAX)),
