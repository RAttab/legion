/* ui.c
   Rémi Attab (remi.attab@gmail.com), 14 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "ui/ui.h"

#include "ux/ux.h"
#include "vm/asm.h"
#include "vm/code.h"
#include "vm/atoms.h"
#include "game/proxy.h"
#include "db/specs.h"
#include "utils/str.h"
#include "utils/htable.h"

#include <stdarg.h>

#include "ui/ui_str.c"
#include "ui/ui_layout.c"
#include "ui/ui_clipboard.c"
#include "ui/ui_label.c"
#include "ui/ui_link.c"
#include "ui/ui_tooltip.c"
#include "ui/ui_button.c"
#include "ui/ui_scroll.c"
#include "ui/ui_input.c"
#include "ui/ui_code.c"
#include "ui/ui_asm.c"
#include "ui/ui_doc.c"
#include "ui/ui_tabs.c"
#include "ui/ui_list.c"
#include "ui/ui_tree.c"
#include "ui/ui_histo.c"
#include "ui/ui_game.c"
#include "ui/ui_panel.c"
#include "ui/ui_focus.c"
#include "ui/ui_style.c"
