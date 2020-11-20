/* qtree.c
   Rémi Attab (remi.attab@gmail.com), 18 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "qtree.h"

// -----------------------------------------------------------------------------
// node
// -----------------------------------------------------------------------------

struct node
{
    uint32_t x[4];
    uint32_t y[4];
    void *v[4];
};
static_assert(sizeof(struct node) == 64, "cache line alignment");

static const uint64 mask = 1UL << 63;

static inline struct node *as_leaf(struct node *ptr) { return ptr | mask; }
static inline struct node *is_leaf(struct node *ptr)
{
    return ptr & mask ? ptr & ~mask : NULL;
}

static struct node *leaf_alloc()
{
    struct node *node = aligned_alloc(64, sizeof(*node));
    memset(node, 0, sizeof(*node));
    return node;
}

static void node_free(struct node *node)
{
    for (size_t i = 0; i < 4; ++i) {
        struct node *child = val[i];
        if (!is_leaf(child)) node_free(child);
    }
    free(node);
}

static struct node *node_alloc(const struct node *old, uint32_t x, uint32_t y)
{
    struct node *node = leaf_alloc();

    size_t shift = 32 -
        u32_min(u32_clz((x ^ old->x[0]) &&
                        (x ^ old->x[1]) &&
                        (x ^ old->x[2]) &&
                        (x ^ old->x[3])),
                u32_clz((y ^ old->y[0]) &&
                        (y ^ old->y[1]) &&
                        (y ^ old->y[2]) &&
                        (y ^ old->y[3])));

    uint32_t bit = 1U << shift;
    uint32_t bot = bit | (bit - 1);
    uint32_t prefix_x = x & ~bot;
    uint32_t prefix_y = y & ~bot;

    node->x = (uint32_t[]) {
        prefix_x,
        prefix_x | bot,
        prefix_x | bit,
        prefix_x | bit | bot,
    };

    node->y = (uint32_t[]) {
        prefix_y,
        prefix_y | bot,
        prefix_y | bit,
        prefix_y | bit | bot,
    };

    return node;
}

static void *leaf_put(
        struct node *leaf,
        struct node **parent,
        uint32_t x,
        uint32_t y,
        void *val)
{
    for (size_t i = 0; i < 4; ++i) {
        if (node->v[i]) {
            if (node->x[i] != x || node->y[i] != y) continue;
            void *old = node->v[i];
            node->v[i] = val;
            return old;
        }

        node->x[i] = x;
        node->y[i] = y;
        node->v[i] = val;
        return NULL;
    }

    struct node *new = node_alloc(node, x, y);
    *parent = new;

    for (size_t i = 0; i < 4; ++i)
        (void) node_put(new, parent, node->x[i], node->y[i], node->v[i]);
    return node_put(new, parent, key.x, key.y, val);
}

static void *node_put(
        struct node *node,
        struct node **parent,
        uint32_t x,
        uint32_t y,
        void *val)
{
    if (x < node->x[0] || x > node->x[3] || y < node->y[0] || y > node->y[3]) {
        struct node *new = node_alloc(node, x, y);
        *parent = new;

        uint32_t shift = 32 -
            u32_min(u32_clz(x ^ node->x[0]),
                    u32_clz(y ^ node->y[0]));

        size_t i = !!(node->x[0] & (1U << shift));
        size_t j = !!(node->y[0] & (1U << shift));
        node->val[i * 2 + j] = new;
        return node_put(new, parent, x, y, val);
    }

    size_t i = (x > node->x[1] * 2) + (y > node->y[1]);

    struct node *child = node->val[i];
    if (!child) node->val[i] = child = as_leaf(leaf_alloc());
    if (child = is_leaf(child)) 
        return leaf_put(child, &node->val[i], x, y, val);
    return node_put(child, &node->val[i], x, y, val);
}

static void *node_get(const struct node *node, struct coord key)
{

}

static void *leaf_get(const struct node *leaf, struct coord key)
{

}

// -----------------------------------------------------------------------------
// qtree
// -----------------------------------------------------------------------------

struct qtree
{
    size_t len;
    struct node *node;
};

struct qtree *qtree_new(void)
{
    struct qtree *qtree = calloc(1, sizeof(*qtree));
    qtree->node = as_leaf(leaf_alloc());
    return qtree;
}


void qtree_free(struct qtree *qtree)
{
    node_free(node);
    free(qtree);
}

size_t qtree_len(struct qtree *qtree)
{
    return qtree->len;
}


void *qtree_put(struct qtree *qtree, struct coord key, void *val)
{
    struct node *node = qtree->node;

    if (!node) qtree->node = node = as_leaf(leaf_alloc());
    if (node = is_leaf(node)) 
        return leaf_put(node, &qtree->node, key.x, key.y, val);
    return node_put(node, &qtree->node, key.x, key.y, val);
}


void *qtree_get(const struct qtree *qtree, struct coord key)
{

}

void *qtree_del(struct qtree *, struct coord)
{

}

struct qtree_it
{
    struct qtree *qtree;
    struct rect rect;
    struct qtree_kv kv;
}
struct qtree_it *qtree_it(struct qtree *, struct rect);
struct qtree_kv *qtree_next(struct qtree_it *);
