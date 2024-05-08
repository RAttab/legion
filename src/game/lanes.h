/* lanes.h
   Rémi Attab (remi.attab@gmail.com), 03 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

struct hset;


// -----------------------------------------------------------------------------
// lanes
// -----------------------------------------------------------------------------

struct lanes
{
    struct world *world;

    struct htable lanes;
    struct htable index;
    struct heap data;
};

void lanes_init(struct lanes *, struct world *);
void lanes_free(struct lanes *);

bool lanes_load(struct lanes *, struct world *, struct save *);
void lanes_save(struct lanes *, struct save *);

world_ts_delta lanes_travel(size_t speed, struct coord src, struct coord dst);

struct lanes_packet
{
    user_id owner;
    enum item item;
    uint8_t len;
    uint32_t speed;
    struct coord src, dst;
    const vm_word *data;
};

void lanes_launch(struct lanes *, struct lanes_packet);
void lanes_step(struct lanes *);

// -----------------------------------------------------------------------------
// lanes list
// -----------------------------------------------------------------------------

const struct hset *lanes_set(struct lanes *, struct coord);

struct lanes_list_item { struct coord src, dst; };
struct lanes_list { uint32_t len, cap; struct lanes_list_item items[]; };

void lanes_list_free(struct lanes_list *);
void lanes_list_save(const struct lanes *, struct save *, struct world *, user_set);
bool lanes_list_load(struct lanes_list **, struct save *);

struct lanes_list_it {
    struct coord_rect rect;
    const struct lanes_list_item *it, *end;
};

struct lanes_list_it lanes_list_begin(const struct lanes_list *, struct coord_rect);
const struct lanes_list_item *lanes_list_next(struct lanes_list_it *);
