/* tree.c
   Rémi Attab (remi.attab@gmail.com), 15 Sep 2021
   FreeBSD-style copyright and disclaimer apply
*/


#include "tree.h"


// -----------------------------------------------------------------------------
// style
// -----------------------------------------------------------------------------

void ui_tree_style_default(struct ui_style *s)
{
    s->tree = (struct ui_tree_style) {
        .idle = {
            .font = font_base,
            .fg = s->rgba.fg,
            .bg = s->rgba.bg,
        },

        .hover = {
            .font = font_base,
            .fg = s->rgba.fg,
            .bg = s->rgba.list.hover,
        },

        .selected = {
            .font = font_bold,
            .fg = s->rgba.fg,
            .bg = s->rgba.list.selected,
        },
    };
}


// -----------------------------------------------------------------------------
// tree
// -----------------------------------------------------------------------------

struct ui_tree ui_tree_new(struct dim dim, size_t chars)
{
    const struct ui_tree_style *s = &ui_st.tree;

    return (struct ui_tree) {
        .w = make_ui_widget(dim),
        .s = *s,

        .scroll = ui_scroll_new(dim, engine_cell()),
        .str = ui_str_v(chars),

        .len = 0,
        .cap = 0,
        .nodes = NULL,

        .selected = 0,
        .open = NULL,
        .path = NULL,
    };
}

void ui_tree_free(struct ui_tree *tree)
{
    ui_scroll_free(&tree->scroll);

    ui_str_free(&tree->str);
    for (size_t i = 0; i < tree->cap; ++i)
        ui_str_free(&tree->nodes[i].str);

    free(tree->nodes);
    hset_free(tree->open);
    hset_free(tree->path);
}


struct ui_tree_node *ui_tree_at(struct ui_tree *tree, ui_tree_node index)
{
    if (!tree->len && !index) return NULL;
    assert(index < tree->len);
    return tree->nodes + index;
}

ui_tree_node ui_tree_user(struct ui_tree *tree, uint64_t user)
{
    for (ui_tree_node i = 0; i < tree->len; ++i) {
        if (tree->nodes[i].user == user) return i;
    }
    return ui_tree_node_nil;
}

void ui_tree_clear(struct ui_tree *tree)
{
    hset_clear(tree->path);
    tree->selected = 0;
}

ui_tree_node ui_tree_select(struct ui_tree *tree, uint64_t user)
{
    hset_clear(tree->path);
    ui_tree_node index = ui_tree_user(tree, user);
    if (index == ui_tree_node_nil) { tree->selected = 0; return index; }

    struct ui_tree_node *node = tree->nodes + index;
    tree->selected = user;

    while (true) {
        if (!node->leaf) {
            node->open = true;
            tree->open = hset_put(tree->open, node->user);
        }

        if (node->parent == ui_tree_node_nil) break;
        node = tree->nodes + node->parent;
    }

    return index;
}

void ui_tree_reset(struct ui_tree *tree)
{
    tree->len = 0;
}

ui_tree_node ui_tree_index(struct ui_tree *tree)
{
    return tree->len;
}

struct ui_str *ui_tree_add(
        struct ui_tree *tree, ui_tree_node parent, uint64_t user)
{
    assert(user);
    assert(parent == ui_tree_node_nil || parent < tree->len);

    if (tree->len == tree->cap) {
        size_t old = tree->cap;
        tree->cap = tree->cap ? tree->cap * 2 : 8;
        tree->nodes = realloc_zero(tree->nodes, old, tree->cap, sizeof(*tree->nodes));
    }

    struct ui_tree_node *node = tree->nodes + tree->len;
    tree->len++;

    node->leaf = true;
    node->open = hset_test(tree->open, user);
    node->depth = 0;
    node->user = user;
    node->parent = parent;

    if (!node->str.cap)
        node->str = ui_str_clone(&tree->str);

    if (parent != ui_tree_node_nil) {
        struct ui_tree_node *parent_node = tree->nodes + parent;
        parent_node->leaf = false;
        node->depth = parent_node->depth + 1;
    }

    hset_clear(tree->path);
    return &node->str;
}

static ui_tree_node ui_tree_next(struct ui_tree *tree, ui_tree_node index)
{
    if (index >= tree->len) return ui_tree_node_nil;

    struct ui_tree_node *parent = tree->nodes + index;
    index++;
    if (parent->open) return index;

    while (index < tree->len) {
        if (tree->nodes[index].depth <= parent->depth) break;
        index++;
    }

    return index < tree->len ? index : ui_tree_node_nil;
}

static ui_tree_node ui_tree_row(struct ui_tree *tree, size_t row)
{
    ui_tree_node index = 0;
    for (size_t i = 1; i <= row; ++i)
        index = ui_tree_next(tree, index);
    return index;
}


// -----------------------------------------------------------------------------
// event
// -----------------------------------------------------------------------------

uint64_t ui_tree_event(struct ui_tree *tree)
{
    uint64_t ret = 0;
    struct dim cell = engine_cell();

    ui_scroll_event(&tree->scroll);

    for (auto ev = ev_next_button(nullptr); ev; ev = ev_next_button(ev)) {
        if (ev->button != ev_button_left) continue;
        if (ev->state != ev_state_up) continue;

        struct pos cursor = ev_mouse_pos();
        if (!rect_contains(tree->w, cursor)) continue;
        ev_consume_button(ev);

        size_t row = (cursor.y - tree->w.y) / cell.h;
        row += ui_scroll_first_row(&tree->scroll);

        ui_tree_node index = ui_tree_row(tree, row);
        if (index == ui_tree_node_nil) continue;

        struct ui_tree_node *node = tree->nodes + index;

        size_t col = (cursor.x - tree->w.x) / cell.w;
        if (col < node->depth) continue;

        if (!node->leaf && (!node->open || col == node->depth)) {
            node->open = !node->open;
            if (!node->open) hset_del(tree->open, node->user);
            else tree->open = hset_put(tree->open, node->user);
        }

        if (node->user && col > node->depth) {
            ret = tree->selected = node->user;
            hset_clear(tree->path);
            continue;
        }
    }

    return ret;
}


// -----------------------------------------------------------------------------
// render
// -----------------------------------------------------------------------------

static void ui_tree_update_path(struct ui_tree *tree)
{
    if (!tree->selected) return;
    if (tree->path && tree->path->len) return;

    for (ui_tree_node index = ui_tree_user(tree, tree->selected);
         index != ui_tree_node_nil; index = (tree->nodes + index)->parent)
        tree->path = hset_put(tree->path, index + 1);
}

void ui_tree_render(struct ui_tree *tree, struct ui_layout *layout)
{
    if (!tree->len) ui_scroll_update_rows(&tree->scroll, 0);
    else {
        size_t n = 1;
        ui_tree_node index = 0;
        while ((index = ui_tree_next(tree, index)) < tree->len) n++;
        ui_scroll_update_rows(&tree->scroll, n);
    }

    struct ui_layout inner = ui_scroll_render(&tree->scroll, layout);
    if (ui_layout_is_nil(&inner)) return;
    tree->w = tree->scroll.w;

    const render_layer layer_bg = render_layer_push(2);
    const render_layer layer_fg = layer_bg + 1;

    struct dim cell = engine_cell();
    struct pos pos = inner.base.pos;

    size_t last = ui_scroll_last_row(&tree->scroll);
    size_t first = ui_scroll_first_row(&tree->scroll);

    ui_tree_node index = ui_tree_row(tree, first);
    ui_tree_update_path(tree);

    for (size_t i = first; i < last; ++i, index = ui_tree_next(tree, index)) {
        struct ui_tree_node *node = tree->nodes + index;

        struct rect rect = {
            .x = pos.x, .y = pos.y,
            .w = inner.base.dim.w,
            .h = cell.h,
        };

        enum render_font font = font_nil;
        struct rgba fg = {0}, bg = {0};
        bool selected = hset_test(tree->path, index + 1);

        if (ev_mouse_in(rect)) {
            font = selected ? tree->s.selected.font : tree->s.hover.font;
            fg = tree->s.hover.fg;
            bg = tree->s.hover.bg;
        }
        else if (selected) {
            font = tree->s.selected.font;
            fg = tree->s.selected.fg;
            bg = tree->s.selected.bg;
        }
        else {
            font = tree->s.idle.font;
            fg = tree->s.idle.fg;
            bg = tree->s.idle.bg;
        }

        render_rect_fill(layer_bg, bg, rect);

        pos.x += (node->depth * cell.w);
        if (!node->leaf)
            render_font(layer_fg, font, fg, pos, node->open ? "-" : "+", 1);

        pos.x += cell.w;
        render_font(layer_fg, font, fg, pos, node->str.str, node->str.len);

        pos.x = inner.base.pos.x;
        pos.y += cell.h;
    }

    render_layer_pop();
}
