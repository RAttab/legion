/* ui_list.h
   Rémi Attab (remi.attab@gmail.com), 23 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once


// -----------------------------------------------------------------------------
// list
// -----------------------------------------------------------------------------

struct ui_list_entry
{
    struct ui_str str;
    uint64_t user;
};


struct ui_list_style
{
    struct {
        enum render_font font;
        struct rgba fg, bg;
    } idle, hover, selected;
};

void ui_list_style_default(struct ui_style *);


struct ui_list
{
    ui_widget w;
    struct ui_list_style s;

    struct ui_scroll scroll;
    struct ui_str str;

    size_t len, cap;
    struct ui_list_entry *entries;

    uint64_t selected;
};


struct ui_list ui_list_new(struct dim, size_t chars);
void ui_list_free(struct ui_list *);

void ui_list_clear(struct ui_list *);
bool ui_list_select(struct ui_list *, uint64_t user);

void ui_list_reset(struct ui_list *);
struct ui_str *ui_list_add(struct ui_list *, uint64_t user);

uint64_t ui_list_event(struct ui_list *);
void ui_list_render(struct ui_list *, struct ui_layout *);
