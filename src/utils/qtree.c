/* qtree.c
   Rémi Attab (remi.attab@gmail.com), 18 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "qtree.h"

#include "utils/bits.h"

// -----------------------------------------------------------------------------
// node
// -----------------------------------------------------------------------------

struct qtree_node
{
    uint32_t x[4];
    uint32_t y[4];
    void *v[4];
};
static_assert(sizeof(struct qtree_node) == 64, "cache line alignment");

static const uint64 qtree_node_mask = 1UL << 63;

static inline struct qtree_node *qtree_as_leaf(struct qtree_node *ptr)
{
    return ptr | qtree_node_mask;
}

static inline struct qtree_node *qtree_is_leaf(struct qtree_node *ptr)
{
    return ptr & qtree_node_pmask ? ptr & ~qtree_node_mask : NULL;
}

static struct qtree_node *qtree_leaf_alloc()
{
    struct qtree_node *node = aligned_alloc(64, sizeof(*node));
    memset(node, 0, sizeof(*node));
    return node;
}

static void qtree_node_free(struct qtree_node *node)
{
    for (size_t i = 0; i < 4; ++i) {
        struct qtree_node *child = val[i];
        if (!qtree_is_leaf(child)) qtree_node_free(child);
    }
    free(node);
}

static struct qtree_node *qtree_node_alloc(uint32_t x, uint32_t y, uint32_t pivot)
{
    struct qtree_node *node = qtree_leaf_alloc();

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

static void *qtree_leaf_put(
        struct qtree_node *leaf,
        struct qtree_node **parent,
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

    struct qtree_node *new = qtree_node_alloc(x, y, 1U << shift);
    *parent = new;

    for (size_t i = 0; i < 4; ++i)
        (void) node_put(new, parent, node->x[i], node->y[i], node->v[i]);
    return node_put(new, parent, key.x, key.y, val);
}

static void *qtree_node_put(
        struct qtree_node *node,
        struct qtree_node **parent,
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

        struct qtree_node *new = qtree_node_alloc(x, y, pivot);
        *parent = new;

        size_t i = !!(node->x[0] & pivot);
        size_t j = !!(node->y[0] & pivot);
        node->val[j * 2 + i] = new;
        return qtree_node_put(new, parent, x, y, val);
    }

    size_t i = x > node->x[1];
    size_t j = y > node->y[1];
    size_t index = (j * 2) + i;

    struct qtree_node *child = node->val[index];
    if (!child) node->val[index] = child = qtree_as_leaf(qtree_leaf_alloc());
    if (child = qtree_is_leaf(child))
        return qtree_leaf_put(child, &node->val[index], x, y, val);
    return qtree_node_put(child, &node->val[index], x, y, val);
}

static struct rect qtree_node_rect(struct qtree_node *node, size_t index)
{
    bool x = index & 1;
    bool y = (index >> 1) & 1;

    return (struct rect) {
        .top = (struct coord) {
            .x = x ? node->x[0] : node->x[2],
            .y = y ? node->y[0] : node->y[2],
        },
        .bot = (struct coord) {
            .x = x ? node->x[1] : node->x[3],
            .y = y ? node->y[1] : node->y[3],
        },
    };
}


// -----------------------------------------------------------------------------
// qtree
// -----------------------------------------------------------------------------

struct qtree
{
    size_t len;
    struct qtree_node *node;
};

struct qtree *qtree_new(void)
{
    struct qtree *qtree = calloc(1, sizeof(*qtree));
    qtree->node = qtree_as_leaf(qtree_leaf_alloc());
    return qtree;
}


void qtree_free(struct qtree *qtree)
{
    qtree_node_free(node);
    free(qtree);
}

size_t qtree_len(struct qtree *qtree)
{
    return qtree->len;
}


void *qtree_put(struct qtree *qtree, struct coord key, void *val)
{
    struct qtree_node *node = qtree->node;

    if (!node) qtree->node = node = qtree_as_leaf(qtree_leaf_alloc());

    struct leaf *leaf = node;
    if (leaf = qtree_is_leaf(node))
        return qtree_leaf_put(leaf, &qtree->node, key.x, key.y, val);
    return qtree_node_put(node, &qtree->node, key.x, key.y, val);
}


void *qtree_get(const struct qtree *qtree, struct coord key)
{
    struct qtree_node *node = qtree->node;
    while (node) {

        struct qtree_node *leaf = qtree_is_leaf(node);
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
    struct qtree_node *node = qtree->node;
    while (node) {

        struct qtree_node *leaf = qtree_is_leaf(node);
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

static const uintptr_t qtree_path_mask = 0x3;

static struct qtree_node *qtree_path(struct qtree_node *node, size_t index)
{
    assert(!(node & qtree_path_mask));
    return node | index;
}

static size_t qtree_path_index(struct qtree_node *node)
{
    return ((uintptr_t) node) & qtree_path_mask;
}

static struct qtree_node *qtree_path_ptr(struct qtree_node *node)
{
    return node & ~qtree_path_mask;
}

void qtree_it(struct qtree_it *it, struct qtree *qtree, struct rect rect)
{
    *it = (struct qtree_it) {
        .qtree = qtree,
        .rect = rect,
        .len = 1,
        .path = { qtree_path(qtree->node, 0) },
    };
}

struct qtree_kv *qtree_next(struct qtree_it *it)
{
    if (!it->len) return NULL;

    size_t index = qtree_path_index(it->path[it->len - 1]);
    struct qtree_node *node = qtree_path_ptr(it->path[it->len - 1]);
    struct qtree_leaf *leaf = qtree_is_leaf(node);

    while (true) {
        for (; index < 4; ++index) {
            if (!node->v[index]) continue;

            if (likely(leaf)) {
                struct coord coord = { .x = leaf->x[index], leaf->y[index] };
                if (rect_contains(&it->rect, coord)) {
                    it->path[it->len - 1] = qtree_path(node, index + 1);
                    it->kv = { coord, leav->v[index] };
                    return &it->kv;
                }
            }
            else {
                struct rect rect = qtree_node_rect(node, index);
                if (rect_intersect(&it->rect, &rect)) {
                    it->path[it->len - 1] = qtree_path(node, index + 1);
                    it->path[it->len++] = qtree_path(node = node->v[index], index = 0);
                    leaf = qtree_is_leaf(node);
                }
            }
        }

        if (it->len) { it->len--; } else { return NULL; }
        index = qtree_path_index(it->path[it->len - 1]);
        node = qtree_path_ptr(it->path[it->len - 1]);
        leaf = NULL;
    }

    assert(false);
}
