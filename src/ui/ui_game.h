/* ui_game.h
   Rémi Attab (remi.attab@gmail.com), 23 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

struct tech;


// -----------------------------------------------------------------------------
// waiting
// -----------------------------------------------------------------------------

struct ui_label ui_waiting_new(void);
void ui_waiting_idle(struct ui_label *);
void ui_waiting_set(struct ui_label *, bool waiting);

// -----------------------------------------------------------------------------
// loops
// -----------------------------------------------------------------------------

struct ui_label ui_loops_new(void);
void ui_loops_set(struct ui_label *, im_loops);


// -----------------------------------------------------------------------------
// ui
// -----------------------------------------------------------------------------

struct ui_lab_bits_style
{
    struct dim margin;
    struct rgba fg, border;
};

void ui_lab_bits_style_default(struct ui_style *);

struct ui_lab_bits
{
    struct ui_lab_bits_style s;

    uint8_t bits;
    uint64_t known;
};

struct ui_lab_bits ui_lab_bits_new(void);
void ui_lab_bits_update(struct ui_lab_bits *, const struct tech *, enum item);
void ui_lab_bits_render(struct ui_lab_bits *, struct ui_layout *);
