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

static struct node *node_alloc(uint32_t x, uint32_t y, uint32_t pivot)
{
    struct node *node = leaf_alloc();

    uint32_t bot = pivot | (pivot - 1);
    uint32_t prefix_x = x & ~bot;
    uint32_t prefix_y = y & ~bot;

    node->x = (uint32_t[]) {
        prefix_x,
        prefix_x | bot,
        prefix_x | pivot,
        prefix_x | pivot | bot,
    };

    node->y = (uint32_t[]) {
        prefix_y,
        prefix_y | bot,
        prefix_y | pivot,
        prefix_y | pivot | bot,
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

    size_t shift = -1;
    shift = u32_min(shift, u32_clz(x ^ leaf->x[0]));
    shift = u32_min(shift, u32_clz(x ^ leaf->x[1]));
    shift = u32_min(shift, u32_clz(x ^ leaf->x[2]));
    shift = u32_min(shift, u32_clz(x ^ leaf->x[3]));
    shift = u32_min(shift, u32_clz(y ^ leaf->y[0]));
    shift = u32_min(shift, u32_clz(y ^ leaf->y[1]));
    shift = u32_min(shift, u32_clz(y ^ leaf->y[2]));
    shift = u32_min(shift, u32_clz(y ^ leaf->y[3]));
    shift = 32 - shift;

    struct node *new = node_alloc(x, y, 1U << shift);
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
        size_t shift = -1;
        shift = u32_min(shift, u32_clz(x ^ node->x[2]));
        shift = u32_min(shift, u32_clz(y ^ node->y[2]));
        shift = 32 - shift;
        uint32_t pivot = 1U << shift;

        struct node *new = node_alloc(x, y, pivot);
        *parent = new;

        size_t i = !!(node->x[0] & pivot);
        size_t j = !!(node->y[0] & pivot);
        node->val[j * 2 + i] = new;
        return node_put(new, parent, x, y, val);
    }

    size_t i = x > node->x[1];
    size_t j = y > node->y[1];
    size_t index = (j * 2) + i;

    struct node *child = node->val[index];
    if (!child) node->val[index] = child = as_leaf(leaf_alloc());
    if (child = is_leaf(child))
        return leaf_put(child, &node->val[index], x, y, val);
    return node_put(child, &node->val[index], x, y, val);
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

    struct leaf *leaf = node;
    if (leaf = is_leaf(node))
        return leaf_put(leaf, &qtree->node, key.x, key.y, val);
    return node_put(node, &qtree->node, key.x, key.y, val);
}


void *qtree_get(const struct qtree *qtree, struct coord key)
{
    struct node *node = qtree->node;
    while (node) {

        struct node *leaf = is_leaf(node);
        if (leaf) {
            for (size_t i = 0; i < 4; ++i) {
                if (leaf->x[i] == key.x && leaf->y[i] == key.y)
                    return leaf->val[i];
            }
            return NULL;
        }

        if (key.x < node->x[0] || key.x > node->x[3]) return NULL;
        if (key.y < node->y[0] || key.y > node->y[3]) return NULL;

        size_t i = key.x > node->x[1];
        size_t j = key.y > node->y[1];
        node = node->val[j * 2 + i];
    }

    return NULL;
}

// I currently don't shrink the tree as I expect dels and put to be intermingled
// quite a bit and I don't want needless shrinking and growing. But I can see a
// world where I could use both behaviours (UI with changing view points) then I
// could make the behaviour tweakable and support both.
void *qtree_del(struct qtree *qtree, struct coord key)
{
    struct node *node = qtree->node;
    while (node) {

        struct node *leaf = is_leaf(node);
        if (leaf) {
            for (size_t i = 0; i < 4; ++i) {
                if (leaf->x[i] == key.x && leaf->y[i] == key.y) {
                    void *old = leaf->val[i];
                    leaf->x[i] = leaf->y[i] = leaf->val[i] = 0;
                    return old;
                }
            }
            return NULL;
        }

        if (key.x < node->x[0] || key.x > node->x[3]) return NULL;
        if (key.y < node->y[0] || key.y > node->y[3]) return NULL;

        size_t i = key.x > node->x[1];
        size_t j = key.y > node->y[1];
        node = node->val[j * 2 + i];
    }

    return NULL;
}

struct qtree_it
{
    struct qtree *qtree;
    struct rect rect;
    struct qtree_kv kv;
}
struct qtree_it *qtree_it(struct qtree *, struct rect);
struct qtree_kv *qtree_next(struct qtree_it *);
