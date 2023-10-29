/* ui_focus.h
   Rémi Attab (remi.attab@gmail.com), 22 Sep 2023
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

struct ui_panel;

// -----------------------------------------------------------------------------
// focus
// -----------------------------------------------------------------------------

struct ui_panel *ui_focus_panel(void);
void *ui_focus_element(void);

void ui_focus_acquire(struct ui_panel *panel, void *element);
void ui_focus_release(struct ui_panel *panel, void *element);

void ui_focus_panel_acquire(struct ui_panel *panel);
void ui_focus_panel_release(struct ui_panel *panel);
