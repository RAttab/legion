/* qtree.h
   Rémi Attab (remi.attab@gmail.com), 18 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

struct qtree;
struct qtree_kv
{
    struct coord key;
    void *val;
};

struct qtree *qtree_new(void);
void qtree_free(struct qtree *);

size_t qtree_len(struct qtree *);

void *qtree_get(struct qtree *, struct coord);
void *qtree_del(struct qtree *, struct coord);
void *qtree_put(struct qtree *, struct coord, void *);

struct qtree_it;
struct qtree_it *qtree_it(struct qtree *, struct rect);
struct qtree_kv *qtree_next(struct qtree_it *);
