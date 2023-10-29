/* ui_tree.h
   Rémi Attab (remi.attab@gmail.com), 23 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once


// -----------------------------------------------------------------------------
// tree
// -----------------------------------------------------------------------------

typedef uint32_t ui_tree_node;
constexpr ui_tree_node ui_tree_node_nil = UINT32_MAX;

struct ui_tree_node
{
    bool open, leaf;
    uint8_t depth;
    struct ui_str str;
    uint64_t user;
    ui_tree_node parent;
};


struct ui_tree_style
{
    struct {
        enum render_font font;
        struct rgba fg, bg;
    } idle, hover, selected;
};

void ui_tree_style_default(struct ui_style *);


struct ui_tree
{
    ui_widget w;
    struct ui_tree_style s;

    struct ui_scroll scroll;
    struct ui_str str;

    ui_tree_node len, cap;
    struct ui_tree_node *nodes;

    uint64_t selected;
    struct hset *open, *path;
};


struct ui_tree ui_tree_new(struct dim, size_t chars);
void ui_tree_free(struct ui_tree *);

struct ui_tree_node *ui_tree_at(struct ui_tree *, ui_tree_node);
ui_tree_node ui_tree_user(struct ui_tree *, uint64_t user);

void ui_tree_clear(struct ui_tree *);
ui_tree_node ui_tree_select(struct ui_tree *, uint64_t user);

void ui_tree_reset(struct ui_tree *);
ui_tree_node ui_tree_index(struct ui_tree *);
struct ui_str *ui_tree_add(struct ui_tree *, ui_tree_node parent, uint64_t user);

uint64_t ui_tree_event(struct ui_tree *);
void ui_tree_render(struct ui_tree *, struct ui_layout *);
