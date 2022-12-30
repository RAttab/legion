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
            .font = s->font.base,
            .fg = s->rgba.fg,
            .bg = s->rgba.bg,
        },

        .hover = {
            .font = s->font.base,
            .fg = s->rgba.fg,
            .bg = s->rgba.list.hover,
        },

        .selected = {
            .font = s->font.bold,
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
        .w = ui_widget_new(dim.w, dim.h),
        .s = *s,

        .scroll = ui_scroll_new(dim, s->idle.font->glyph_h),
        .str = ui_str_v(chars),

        .len = 0,
        .cap = 0,
        .nodes = NULL,

        .hover = 0,
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
}


struct ui_node *ui_tree_node(struct ui_tree *tree, ui_node index)
{
    if (!tree->len && !index) return NULL;
    assert(index < tree->len);
    return tree->nodes + index;
}

ui_node ui_tree_user(struct ui_tree *tree, uint64_t user)
{
    for (ui_node i = 0; i < tree->len; ++i) {
        if (tree->nodes[i].user == user) return i;
    }
    return ui_node_nil;
}

void ui_tree_clear(struct ui_tree *tree)
{
    hset_clear(tree->path);
    tree->selected = ui_node_nil;
}

ui_node ui_tree_select(struct ui_tree *tree, uint64_t user)
{
    hset_clear(tree->path);
    ui_node index = ui_tree_user(tree, user);
    if (index == ui_node_nil) { tree->selected = 0; return index; }

    struct ui_node *node = tree->nodes + index;
    tree->selected = user;

    while (true) {
        if (!node->leaf) {
            node->open = true;
            tree->open = hset_put(tree->open, node->user);
        }

        if (node->parent == ui_node_nil) break;
        node = tree->nodes + node->parent;
    }

    return index;
}

void ui_tree_reset(struct ui_tree *tree)
{
    tree->len = 0;
}

ui_node ui_tree_index(struct ui_tree *tree)
{
    return tree->len;
}

struct ui_str *ui_tree_add(
        struct ui_tree *tree, ui_node parent, uint64_t user)
{
    assert(user);
    assert(parent == ui_node_nil || parent < tree->len);

    if (tree->len == tree->cap) {
        size_t old = tree->cap;
        tree->cap = tree->cap ? tree->cap * 2 : 8;
        tree->nodes = realloc_zero(tree->nodes, old, tree->cap, sizeof(*tree->nodes));
    }

    struct ui_node *node = tree->nodes + tree->len;
    tree->len++;

    node->leaf = true;
    node->open = hset_test(tree->open, user);
    node->depth = 0;
    node->user = user;
    node->parent = parent;

    if (!node->str.cap)
        node->str = ui_str_clone(&tree->str);

    if (parent != ui_node_nil) {
        struct ui_node *parent_node = tree->nodes + parent;
        parent_node->leaf = false;
        node->depth = parent_node->depth + 1;
    }

    return &node->str;
}

static ui_node ui_tree_next(struct ui_tree *tree, ui_node index)
{
    if (index >= tree->len) return ui_node_nil;

    struct ui_node *parent = tree->nodes + index;
    index++;
    if (parent->open) return index;

    while (index < tree->len) {
        if (tree->nodes[index].depth <= parent->depth) break;
        index++;
    }

    return index < tree->len ? index : ui_node_nil;
}

static ui_node ui_tree_row(struct ui_tree *tree, size_t row)
{
    ui_node index = 0;
    for (size_t i = 1; i <= row; ++i)
        index = ui_tree_next(tree, index);
    return index;
}


// -----------------------------------------------------------------------------
// event
// -----------------------------------------------------------------------------

enum ui_ret ui_tree_event(struct ui_tree *tree, const SDL_Event *ev)
{
    enum ui_ret ret = ui_scroll_event(&tree->scroll, ev);
    if (ret) return ret;

    struct SDL_Rect rect = ui_widget_rect(&tree->w);

    switch (ev->type) {

    case SDL_MOUSEMOTION: {
        SDL_Point point = render.cursor.point;
        if (!SDL_PointInRect(&point, &rect)) tree->hover = 0;
        else {
            size_t row = (point.y - rect.y) / tree->s.idle.font->glyph_h;
            row += ui_scroll_first(&tree->scroll);

            ui_node index = ui_tree_row(tree, row);
            if (index == ui_node_nil) tree->hover = 0;
            else {
                struct ui_node *node = ui_tree_node(tree, index);
                tree->hover = node ? node->user : 0;
            }
        }
        return ui_nil;
    }

    case SDL_MOUSEBUTTONUP: {
        SDL_Point point = render.cursor.point;
        if (!SDL_PointInRect(&point, &rect)) return ui_nil;

        size_t row = (point.y - rect.y) / tree->s.idle.font->glyph_h;
        row += ui_scroll_first(&tree->scroll);

        ui_node index = ui_tree_row(tree, row);
        if (index == ui_node_nil) return ui_consume;

        struct ui_node *node = tree->nodes + index;

        size_t col = (point.x - rect.x) / tree->s.idle.font->glyph_w;
        if (col < node->depth) return ui_consume;

        if (!node->leaf && (!node->open || col == node->depth)) {
            node->open = !node->open;
            if (!node->open) hset_del(tree->open, node->user);
            else tree->open = hset_put(tree->open, node->user);
        }

        if (node->user && col > node->depth) {
            tree->selected = node->user;
            hset_clear(tree->path);
            return ui_action;
        }

        return ui_consume;
    }

    default: { return ui_nil; }
    }
}


// -----------------------------------------------------------------------------
// render
// -----------------------------------------------------------------------------

static void ui_tree_update_path(struct ui_tree *tree)
{
    if (!tree->selected) return;
    if (tree->path && tree->path->len) return;

    for (ui_node index = ui_tree_user(tree, tree->selected);
         index != ui_node_nil; index = (tree->nodes + index)->parent)
        tree->path = hset_put(tree->path, index + 1);
}

void ui_tree_render(
        struct ui_tree *tree, struct ui_layout *layout, SDL_Renderer *renderer)
{
    if (!tree->len) ui_scroll_update(&tree->scroll, 0);
    else {
        size_t n = 1;
        ui_node index = 0;
        while ((index = ui_tree_next(tree, index)) < tree->len) n++;
        ui_scroll_update(&tree->scroll, n);
    }

    struct ui_layout inner = ui_scroll_render(&tree->scroll, layout, renderer);
    if (ui_layout_is_nil(&inner)) return;
    tree->w = tree->scroll.w;

    struct pos pos = inner.base.pos;
    size_t last = ui_scroll_last(&tree->scroll);
    size_t first = ui_scroll_first(&tree->scroll);
    ui_node index = ui_tree_row(tree, first);
    ui_tree_update_path(tree);

    for (size_t i = first; i < last; ++i, index = ui_tree_next(tree, index)) {
        struct ui_node *node = tree->nodes + index;

        const struct font *font = NULL;
        struct rgba fg = {0}, bg = {0};
        bool selected = hset_test(tree->path, index + 1);

        if (node->user == tree->hover) {
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

        rgba_render(bg, renderer);
        sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                            .x = pos.x, .y = pos.y,
                            .w = inner.base.dim.w,
                            .h = font->glyph_h
                        }));

        pos.x += (node->depth * font->glyph_w);

        if (!node->leaf) {
            const char *sigil = node->open ? "-" : "+";
            font_render(font, renderer, pos_as_point(pos), fg, sigil, 1);
        }
        pos.x += font->glyph_w;

        font_render(
                font, renderer, pos_as_point(pos), fg,
                node->str.str, node->str.len);

        pos.x = inner.base.pos.x;
        pos.y += font->glyph_h;
    }
}
