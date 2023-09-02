/* ui.c
   Rémi Attab (remi.attab@gmail.com), 14 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "render/render.h"
#include "render/font.h"

#include "ui/str.c"
#include "ui/layout.c"
#include "ui/clipboard.c"
#include "ui/label.c"
#include "ui/link.c"
#include "ui/tooltip.c"
#include "ui/button.c"
#include "ui/scroll.c"
#include "ui/input.c"
#include "ui/code.c"
#include "ui/asm.c"
#include "ui/doc.c"
#include "ui/tabs.c"
#include "ui/list.c"
#include "ui/tree.c"
#include "ui/histo.c"
#include "ui/game.c"
#include "ui/panel.c"
#include "ui/style.c"

// The compiler sometimes doesn't feel like inlining it and we can't shove this
// in utils as not all objects link against sdl... This is kinda aweful but
// whatever...
extern inline void rgba_render(struct rgba, SDL_Renderer *);
