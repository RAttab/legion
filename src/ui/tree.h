/* tree.h
   Rémi Attab (remi.attab@gmail.com), 23 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "types.h"
#include "str.h"
#include "scroll.h"

#include "utils/hset.h"


// -----------------------------------------------------------------------------
// tree
// -----------------------------------------------------------------------------

typedef uint32_t ui_node;
static legion_unused ui_node ui_node_nil = -1;

struct ui_node
{
    bool open, leaf;
    uint8_t depth;
    struct ui_str str;
    uint64_t user;
    ui_node parent;
};

struct ui_tree_style
{
    struct {
        const struct font *font;
        struct rgba fg, bg;
    } idle, hover, selected;
};

struct ui_tree
{
    struct ui_widget w;
    struct ui_tree_style s;

    struct ui_scroll scroll;
    struct ui_str str;

    ui_node len, cap;
    struct ui_node *nodes;

    uint64_t hover, selected;
    struct hset *open, *path;
};


struct ui_tree ui_tree_new(struct dim, size_t chars);
void ui_tree_free(struct ui_tree *);

struct ui_node *ui_tree_node(struct ui_tree *, ui_node);
ui_node ui_tree_user(struct ui_tree *, uint64_t user);

void ui_tree_clear(struct ui_tree *);
ui_node ui_tree_select(struct ui_tree *, uint64_t user);

void ui_tree_reset(struct ui_tree *);
ui_node ui_tree_index(struct ui_tree *);
struct ui_str *ui_tree_add(struct ui_tree *, ui_node parent, uint64_t user);

enum ui_ret ui_tree_event(struct ui_tree *, const SDL_Event *);
void ui_tree_render(struct ui_tree *, struct ui_layout *, SDL_Renderer *);
