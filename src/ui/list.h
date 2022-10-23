/* list.h
   Rémi Attab (remi.attab@gmail.com), 23 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "types.h"
#include "str.h"


// -----------------------------------------------------------------------------
// list
// -----------------------------------------------------------------------------

struct ui_entry
{
    struct ui_str str;
    uint64_t user;
};

struct ui_list_style
{
    struct {
        const struct font *font;
        struct rgba fg, bg;
    } idle, hover, selected;
};

struct ui_list
{
    struct ui_widget w;
    struct ui_list_style s;

    struct ui_scroll scroll;
    struct ui_str str;

    size_t len, cap;
    struct ui_entry *entries;

    uint64_t hover, selected;
};


struct ui_list ui_list_new(struct dim, size_t chars);
void ui_list_free(struct ui_list *);

void ui_list_clear(struct ui_list *);
bool ui_list_select(struct ui_list *, uint64_t user);

void ui_list_reset(struct ui_list *);
struct ui_str *ui_list_add(struct ui_list *, uint64_t user);

enum ui_ret ui_list_event(struct ui_list *, const SDL_Event *);
void ui_list_render(struct ui_list *, struct ui_layout *, SDL_Renderer *);
