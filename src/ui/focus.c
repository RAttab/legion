/* focus.c
   Rémi Attab (remi.attab@gmail.com), 22 Sep 2023
   FreeBSD-style copyright and disclaimer apply
*/

#include "focus.h"
#include "ui/panel.h"
#include "utils/htable.h"

// -----------------------------------------------------------------------------
// focus
// -----------------------------------------------------------------------------

struct
{
    struct ui_panel *panel;
    void *element;

    struct htable panels;
} ui_focus = {0};


struct ui_panel *ui_focus_panel(void)
{
    return ui_focus.panel;
}

void *ui_focus_element(void)
{
    return ui_focus.element;
}

void ui_focus_acquire(struct ui_panel *panel, void *element)
{
    assert(panel);
    assert(element);
    if (element == ui_focus.element) return;

    ui_focus.panel = panel;
    ui_focus.element = element;

    struct htable_ret ret =
        htable_xchg(&ui_focus.panels, (uintptr_t) panel, (uintptr_t) element);
    if (!ret.ok)
        ret = htable_put(&ui_focus.panels, (uintptr_t) panel, (uintptr_t) element);
    assert(ret.ok);
}

void ui_focus_release(struct ui_panel *panel, void *element)
{
    assert(panel);
    assert(element);
    (void) htable_xchg(&ui_focus.panels, (uintptr_t) panel, 0);

    if (ui_focus.panel == panel && ui_focus.element == element)
        ui_focus.element = nullptr;
}

void ui_focus_panel_acquire(struct ui_panel *panel)
{
    assert(panel);
    if (panel == ui_focus.panel) return;
    ui_focus.panel = panel;

    struct htable_ret ret = htable_get(&ui_focus.panels, (uintptr_t) panel);
    ui_focus.element = ret.ok ? (void *) ret.value : nullptr;
}

void ui_focus_panel_release(struct ui_panel *panel)
{
    assert(panel);
    if (ui_focus.panel != panel) return;

    ui_focus.panel = nullptr;
    ui_focus.element = nullptr;
}
