/* ui_label.h
   Rémi Attab (remi.attab@gmail.com), 23 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once


// -----------------------------------------------------------------------------
// label
// -----------------------------------------------------------------------------

struct ui_label_style
{
    enum render_font font;
    struct rgba fg, bg, zeroes, disabled;
};

void ui_label_style_default(struct ui_style *);


struct ui_label
{
    ui_widget w;
    struct ui_label_style s;
    struct ui_str str;
    bool disabled;
};

struct ui_label ui_label_new(struct ui_str);
struct ui_label ui_label_new_s(const struct ui_label_style *, struct ui_str);
void ui_label_free(struct ui_label *);
void ui_label_render(struct ui_label *, struct ui_layout *);


// -----------------------------------------------------------------------------
// values
// -----------------------------------------------------------------------------

struct ui_value
{
    uint64_t user;
    const char *str;
    struct rgba fg;
};

struct ui_values
{
    size_t len;
    struct ui_value *list;
};

struct ui_values ui_values_new(const struct ui_value *, size_t len);
void ui_values_free(struct ui_values *);
void ui_values_set(struct ui_values *, struct ui_label *, uint64_t user);
