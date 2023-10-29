/* ui_input.h
   Rémi Attab (remi.attab@gmail.com), 23 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once


// -----------------------------------------------------------------------------
// input
// -----------------------------------------------------------------------------

enum : size_t { ui_input_cap = 256 };


struct ui_input_style
{
    struct dim pad;
    enum render_font font;
    struct rgba fg, bg, border;
    struct { struct rgba fg; time_sys blink; } carret;
};

void ui_input_style_default(struct ui_style *);


struct ui_input
{
    ui_widget w;
    struct ui_input_style s;
    struct ui_panel *p;

    struct { uint8_t col, len; } view;
    struct { char *c; uint8_t len; } buf;
    uint8_t carret;
};

struct ui_input ui_input_new(size_t len);
struct ui_input ui_input_new_s(const struct ui_input_style *, size_t len);
void ui_input_free(struct ui_input *);

void ui_input_focus(struct ui_input *);

void ui_input_clear(struct ui_input *);
void ui_input_set(struct ui_input *, const char *str);

size_t ui_input_get_str(struct ui_input *, const char **str);
bool ui_input_get_u64(struct ui_input *, uint64_t *ret);
bool ui_input_get_hex(struct ui_input *, uint64_t *ret);
bool ui_input_get_symbol(struct ui_input *, struct symbol *ret);
bool ui_input_eval(struct ui_input *, vm_word *ret);

bool ui_input_event(struct ui_input *);
void ui_input_render(struct ui_input *, struct ui_layout *);
