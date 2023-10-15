/* ui.h
   Rémi Attab (remi.attab@gmail.com), 10 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "engine/engine.h"

#include "utils/time.h"
#include "utils/str.h"

// -----------------------------------------------------------------------------
// types
// -----------------------------------------------------------------------------


struct ui_style;
struct ui_panel;
struct ui_layout;

enum ui_align : uint8_t
{
    ui_align_left = 0,
    ui_align_center,
    ui_align_right,
};

typedef struct rect ui_widget;
inline ui_widget make_ui_widget(struct dim d) { return make_rect(0, 0, d.w, d.h); }


// -----------------------------------------------------------------------------
// implementation
// -----------------------------------------------------------------------------

#include "clipboard.h"
#include "layout.h"
#include "scroll.h"
#include "str.h"

#include "label.h"
#include "link.h"
#include "button.h"
#include "tooltip.h"

#include "input.h"
#include "asm.h"
#include "code.h"
#include "doc.h"

#include "tabs.h"
#include "list.h"
#include "tree.h"
#include "histo.h"

#include "game.h"
#include "panel.h"
#include "focus.h"
#include "style.h"


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
    } while (false)                             \

