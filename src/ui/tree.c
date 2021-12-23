/* tree.c
   Rémi Attab (remi.attab@gmail.com), 15 Sep 2021
   FreeBSD-style copyright and disclaimer apply
*/


#include "common.h"
#include "ui/ui.h"
#include "render/font.h"
#include "utils/hset.h"


// -----------------------------------------------------------------------------
// tree
// -----------------------------------------------------------------------------

struct ui_tree ui_tree_new(struct dim dim, struct font *font, struct ui_str str)
{
    return (struct ui_tree) {
        .w = ui_widget_new(dim.w, dim.h),
        .scroll = ui_scroll_new(dim, font->glyph_h),

        .font = font,
        .str = str,

        .len = 0,
        .cap = 0,
        .nodes = NULL,

        .hover = 0,
        .selected = 0,
        .open = NULL,
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


struct ui_node *ui_tree_node(struct ui_tree *tree, ui_node_t index)
{
    assert(index < tree->len);
    return tree->nodes + index;
}

ui_node_t ui_tree_user(struct ui_tree *tree, uint64_t user)
{
    for (ui_node_t i = 0; i < tree->len; ++i) {
        if (tree->nodes[i].user == user) return i;
    }
    return ui_node_nil;
}

void ui_tree_clear(struct ui_tree *tree)
{
    tree->selected = ui_node_nil;
}

void ui_tree_select(struct ui_tree *tree, uint64_t user)
{
    tree->selected = user;
}

void ui_tree_reset(struct ui_tree *tree)
{
    tree->len = 0;
}

ui_node_t ui_tree_index(struct ui_tree *tree)
{
    return tree->len;
}

struct ui_str *ui_tree_add(
        struct ui_tree *tree, ui_node_t parent, uint64_t user)
{
    assert(user);
    assert(parent == ui_node_nil || parent < tree->len);

    if (tree->len == tree->cap) {
        size_t old = tree->cap;
        tree->cap = tree->cap ? tree->cap * 2 : 8;
        tree->nodes = realloc(tree->nodes, tree->cap * sizeof(*tree->nodes));

        for (size_t i = old; i < tree->cap; ++i) {
            tree->nodes[i].str = ui_str_clone(&tree->str);
        }
    }

    struct ui_node *node = tree->nodes + tree->len;
    tree->len++;

    node->user = user;
    node->open = hset_test(tree->open, user);
    node->parent = parent;
    node->depth = parent == ui_node_nil ? 0 : tree->nodes[parent].depth + 1;

    return &node->str;
}

static ui_node_t ui_tree_next(struct ui_tree *tree, ui_node_t index)
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

static ui_node_t ui_tree_row(struct ui_tree *tree, size_t row)
{
    ui_node_t index = 0;
    for (size_t i = 1; i <= row; ++i)
        index = ui_tree_next(tree, index);
    return index;
}

static bool ui_tree_leaf(struct ui_tree *tree, ui_node_t index)
{
    struct ui_node *node = ui_tree_node(tree, index);
    return index == tree->len - 1
        || (node + 1)->depth <= node->depth;
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
        if (!sdl_rect_contains(&rect, &point)) tree->hover = 0;
        else {
            size_t row = (point.y - rect.y) / tree->font->glyph_h;
            row += ui_scroll_first(&tree->scroll);

            ui_node_t index = ui_tree_row(tree, row);
            if (index == ui_node_nil) tree->hover = 0;
            else tree->hover = ui_tree_node(tree, index)->user;
        }
        return ui_nil;
    }

    case SDL_MOUSEBUTTONDOWN: {
        SDL_Point point = render.cursor.point;
        if (!sdl_rect_contains(&rect, &point)) return ui_nil;

        size_t row = (point.y - rect.y) / tree->font->glyph_h;
        row += ui_scroll_first(&tree->scroll);

        ui_node_t index = ui_tree_row(tree, row);
        if (index == ui_node_nil) return ui_consume;

        struct ui_node *node = tree->nodes + index;
        if (ui_tree_leaf(tree, index)) {
            tree->selected = node->user;
            return ui_action;
        }

        node->open = !node->open;
        if (!node->open) hset_del(tree->open, node->user);
        else tree->open = hset_put(tree->open, node->user);
        return ui_consume;
    }

    default: { return ui_nil; }
    }
}


// -----------------------------------------------------------------------------
// render
// -----------------------------------------------------------------------------

void ui_tree_render(
        struct ui_tree *tree, struct ui_layout *layout, SDL_Renderer *renderer)
{
    struct font *font = tree->font;

    if (!tree->len) ui_scroll_update(&tree->scroll, 0);
    else {
        size_t n = 1;
        ui_node_t index = 0;
        while ((index = ui_tree_next(tree, index)) < tree->len) n++;
        ui_scroll_update(&tree->scroll, n);
    }

    struct ui_layout inner = ui_scroll_render(&tree->scroll, layout, renderer);
    if (ui_layout_is_nil(&inner)) return;
    tree->w = tree->scroll.w;

    size_t last = ui_scroll_last(&tree->scroll);
    size_t first = ui_scroll_first(&tree->scroll);
    ui_node_t index = ui_tree_row(tree, first);

    for (size_t i = first; i < last; ++i, index = ui_tree_next(tree, index)) {
        struct ui_node *node = tree->nodes + index;
        struct ui_widget widget =
            ui_widget_new(node->str.len * font->glyph_w, font->glyph_h);

        ui_layout_sep_x(&inner, (2 * node->depth) * font->glyph_w);
        ui_layout_add(&inner, &widget);

        struct rgba bg = rgba_nil();
        if (node->user == tree->selected) bg = rgba_gray(0x22);
        else if (node->user == tree->hover) bg = rgba_gray(0x44);
        else if (!ui_tree_leaf(tree, index)) bg = rgba_gray_a(0x11, 0x88);
        rgba_render(bg, renderer);

        SDL_Rect rect = ui_widget_rect(&widget);
        if (!ui_tree_leaf(tree, index)) {
            rect.x = inner.top.x;
            rect.w = inner.dim.w;
        }
        sdl_err(SDL_RenderFillRect(renderer, &rect));

        font_render(
                font, renderer, pos_as_point(widget.pos), rgba_white(),
                node->str.str, node->str.len);

        ui_layout_next_row(&inner);
    }
}
