/* types.h
   Remi Attab (remi.attab@gmail.com), 04 Nov 2023
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

struct ui_style;
struct ui_panel;
struct ui_layout;


// -----------------------------------------------------------------------------
// types
// -----------------------------------------------------------------------------

enum ui_align : uint8_t
{
    ui_align_left = 0,
    ui_align_center,
    ui_align_right,
};

typedef struct rect ui_widget;
inline ui_widget make_ui_widget(struct dim d) { return make_rect(0, 0, d.w, d.h); }


// -----------------------------------------------------------------------------
// macros
// -----------------------------------------------------------------------------

#define ui_set(elem)                            \
    ({                                          \
        typeof(elem) elem_ = (elem);            \
        elem_->disabled = false;                \
        &elem_->str;                            \
    })

#define ui_set_nil(elem)                        \
    do {                                        \
        typeof(elem) elem_ = (elem);            \
        elem_->disabled = true;                 \
        ui_str_set_nil(&elem_->str);            \
    } while (false)
