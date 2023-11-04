/* ui.h
   Rémi Attab (remi.attab@gmail.com), 10 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "engine/engine.h"

#include "db/db.h"
#include "vm/types.h"
#include "game/types.h"
#include "utils/str.h"
#include "utils/time.h"
#include "utils/hset.h"
#include "utils/symbol.h"


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

#include "ui_clipboard.h"
#include "ui_layout.h"
#include "ui_scroll.h"
#include "ui_str.h"

#include "ui_label.h"
#include "ui_link.h"
#include "ui_button.h"
#include "ui_input.h"
#include "ui_tooltip.h"

#include "ui_tabs.h"
#include "ui_list.h"
#include "ui_tree.h"
#include "ui_histo.h"

#include "ui_asm.h"
#include "ui_code.h"
#include "ui_doc.h"

#include "ui_game.h"
#include "ui_panel.h"
#include "ui_focus.h"
#include "ui_style.h"


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

