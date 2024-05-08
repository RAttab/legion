/* render.h
   Remi Attab (remi.attab@gmail.com), 05 Oct 2023
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once


// -----------------------------------------------------------------------------
// render
// -----------------------------------------------------------------------------

enum render_font : unsigned
{
    font_nil = 0,
    font_base,
    font_bold,
    font_italic,
    font_len,
};

void render_clear(void);
void render_draw(void);

typedef uint16_t render_layer;
render_layer render_layer_push(render_layer n);
render_layer render_layer_push_max(void);
void render_layer_pop(void);


struct line { struct pos a, b; };
void render_line(
        render_layer, struct rgba, struct line);
void render_line_a(
        render_layer, struct rgba, struct line, struct rect);


void render_line_gradient(
        render_layer,
        struct rgba, struct pos,
        struct rgba, struct pos);
void render_line_gradient_a(
        render_layer,
        struct rgba, struct pos,
        struct rgba, struct pos,
        struct rect);

struct triangle { struct pos a, b, c; };
void render_triangle(
        render_layer, struct rgba, struct triangle);
void render_triangle_a(
        render_layer, struct rgba, struct triangle, struct rect);

void render_rect_line(
        render_layer, struct rgba, struct rect);
void render_rect_line_a(
        render_layer, struct rgba, struct rect, struct rect);

void render_rect_fill(
        render_layer, struct rgba, struct rect);
void render_rect_fill_a(
        render_layer, struct rgba, struct rect, struct rect);

void render_font(
        render_layer, enum render_font,
        struct rgba fg,
        struct pos,
        const char *str, size_t len);
void render_font_a(
        render_layer, enum render_font,
        struct rgba fg,
        struct pos, struct rect,
        const char *str, size_t len);

void render_font_bg(
        render_layer, enum render_font,
        struct rgba fg, struct rgba bg,
        struct pos,
        const char *str, size_t len);
void render_font_bg_a(
        render_layer, enum render_font,
        struct rgba fg, struct rgba bg,
        struct pos, struct rect,
        const char *str, size_t len);

void render_star(render_layer, struct rgba, struct rect, struct rect);
unit render_cursor_size(void);
