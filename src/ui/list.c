/* list.c
   Rémi Attab (remi.attab@gmail.com), 15 May 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "list.h"

// -----------------------------------------------------------------------------
// style
// -----------------------------------------------------------------------------

void ui_list_style_default(struct ui_style *s)
{
    s->list = (struct ui_list_style) {
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
// list
// -----------------------------------------------------------------------------

struct ui_list ui_list_new(struct dim dim, size_t chars)
{
    const struct ui_list_style *s = &ui_st.list;

    return (struct ui_list) {
        .w = ui_widget_new(dim.w, dim.h),
        .s = *s,

        .scroll = ui_scroll_new(dim, s->idle.font->glyph_h),
        .str = ui_str_v(chars),

        .len = 0,
        .cap = 0,
        .entries = NULL,

        .hover = 0,
        .selected = 0,
    };
}

void ui_list_free(struct ui_list *list)
{
    ui_scroll_free(&list->scroll);

    ui_str_free(&list->str);
    for (size_t i = 0; i < list->cap; ++i)
        ui_str_free(&list->entries[i].str);
    free(list->entries);
}


void ui_list_clear(struct ui_list *list)
{
    list->selected = 0;
}

bool ui_list_select(struct ui_list *list, uint64_t user)
{
    for (size_t i = 0; i < list->len; ++i) {
        if (list->entries[i].user != user) continue;
        list->selected = user;
        return true;
    }

    list->selected = 0;
    return false;
}

void ui_list_reset(struct ui_list *list)
{
    list->len = 0;
}

struct ui_str *ui_list_add(struct ui_list *list, uint64_t user)
{
    assert(user);

    if (list->len == list->cap) {
        size_t old = list->cap;
        list->cap = list->cap ? list->cap * 2 : 8;
        list->entries = realloc_zero(list->entries, old, list->cap, sizeof(*list->entries));
    }

    struct ui_entry *entry = list->entries + list->len;
    list->len++;

    entry->user = user;

    if (!entry->str.cap)
        entry->str = ui_str_clone(&list->str);

    return &entry->str;
}


enum ui_ret ui_list_event(struct ui_list *list, const SDL_Event *ev)
{
    enum ui_ret ret = ui_scroll_event(&list->scroll, ev);
    if (ret) return ret;

    struct SDL_Rect rect = ui_widget_rect(&list->w);

    switch (ev->type) {

    case SDL_MOUSEMOTION: {
        list->hover = 0;

        SDL_Point point = render.cursor.point;
        if (!sdl_rect_contains(&rect, &point)) return ui_nil;

        size_t row = (point.y - rect.y) / list->s.idle.font->glyph_h;
        row += ui_scroll_first(&list->scroll);
        if (row >= list->len) return ui_nil;

        list->hover = list->entries[row].user;
        return ui_nil;
    }

    case SDL_MOUSEBUTTONUP: {
        SDL_Point point = render.cursor.point;
        if (!sdl_rect_contains(&rect, &point)) return ui_nil;

        size_t row = (point.y - rect.y) / list->s.idle.font->glyph_h;
        row += ui_scroll_first(&list->scroll);
        if (row >= list->len) return ui_consume;

        list->selected = list->entries[row].user;
        return ui_action;
    }

    default: { return ui_nil; }
    }
}

void ui_list_render(
        struct ui_list *list, struct ui_layout *layout, SDL_Renderer *renderer)
{
    ui_scroll_update(&list->scroll, list->len);

    struct ui_layout inner = ui_scroll_render(&list->scroll, layout, renderer);
    if (ui_layout_is_nil(&inner)) return;
    list->w = list->scroll.w;

    struct pos pos = inner.base.pos;
    size_t last = ui_scroll_last(&list->scroll);
    size_t first = ui_scroll_first(&list->scroll);

    for (size_t i = first; i < last; ++i) {
        const struct ui_entry *entry = list->entries + i;

        const struct font *font = NULL;
        struct rgba fg = {0}, bg = {0};
        bool selected = entry->user == list->selected;

        if (entry->user == list->hover) {
            font = selected ? list->s.selected.font : list->s.hover.font;
            fg = list->s.hover.fg;
            bg = list->s.hover.bg;
        }
        else if (selected) {
            font = list->s.selected.font;
            fg = list->s.selected.fg;
            bg = list->s.selected.bg;
        }
        else {
            font = list->s.idle.font;
            fg = list->s.idle.fg;
            bg = list->s.idle.bg;
        }

        rgba_render(bg, renderer);
        sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                            .x = pos.x, .y = pos.y,
                            .w = inner.base.dim.w,
                            .h = font->glyph_h
                        }));

        font_render(
                font, renderer, pos_as_point(pos), fg,
                entry->str.str, entry->str.len);
        pos.y += font->glyph_h;
    }
}
