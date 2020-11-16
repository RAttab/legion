/* panel_system.c
   Rémi Attab (remi.attab@gmail.com), 15 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/


struct panel_system_state
{
    struct system *system;

    size_t coord_y;
    size_t star_y;
    size_t elem_y;
};

enum { val_str_len = 4 };
static void system_val_str(char *dst, size_t len, size_t val)
{
    assert(len >= val_str_len);

    static const char units[] = " kmgt";

    size_t unit = 0;
    while (val > 1000) {
        val /= 1000;
        unit++;
    }

    dst[3] = units[unit];
    dst[2] = '0' + val % 10;
    dst[1] = '0' + (val / 10) % 10;
    dst[0] = '0' + (val / 100) % 10;
}

static void panel_system_render(void *state_, SDL_Renderer *renderer, SDL_Rect *rect)
{
    struct panel_system_state *state = state_;
    struct font *font = font_mono4;

    {
        char str[coord_str_len+1] = {0};
        coord_str(state->system->coord, str, sizeof(str));
        font_render(font, renderer, str, coord_str_len, (SDL_Point) {
                    .x = rect->x, .y = rect->y + state->coord_y });
    }

    {
        static const char star_str[] = {'s', 't', 'a', 'r', ':'};
        SDL_Point pos = { .x = rect->x, .y = rect->y + state->star_y };
        
        font_render(font, renderer, star_str, sizeof(star_str), pos);
        pos.x += sizeof(star_str) * font->glyph_w;
        
        char val_str[val_str_len];
        system_val_str(val_str, sizeof(val_str), state->system->star);
        font_render(font, renderer, val_str, sizeof(val_str), pos);
    }
    
    {
        SDL_Point pos = { .x = rect->x, .y = rect->y + state->elem_y };
        
        for (size_t elem = 0; elem < num_elements; ++elem) {
            if (elem == num_elements - 1) pos.x += font->glyph_w * 7 * 2;
            
            char elem_str[] = {'A'+elem, ':'};
            font_render(font, renderer, elem_str, sizeof(elem_str), pos);
            pos.x += sizeof(elem_str) * font->glyph_w;
            
            char val_str[val_str_len];
            system_val_str(val_str, sizeof(val_str), state->system->elements[elem]);
            font_render(font, renderer, val_str, sizeof(val_str), pos);
            pos.x += sizeof(val_str) * font->glyph_w;

            if (elem % 5 == 4) {
                pos.x = rect->x;
                pos.y += font->glyph_h;
            }
            else pos.x += font->glyph_w;
        }
    }

}

static bool panel_system_events(void *state_, struct panel *panel, SDL_Event *event)
{
    struct panel_system_state *state = state_;
    if (event->type != core.event) return false;
    
    switch (event->user.code) {

    case EV_SYSTEM_SELECT: {
        struct system *selected = event->user.data1;
        if (state->system && coord_eq(selected->coord, state->system->coord))
            return false;

        state->system = selected;
        panel_show(panel);
        break;
    }

    case EV_SYSTEM_CLEAR: {
        state->system = NULL;
        panel_hide(panel);
        break;
    }

    }

    return false;
}

static void panel_system_free(void *state)
{
    free(state);
};

struct panel *panel_system_new()
{
    enum { spacing = 5 };

    size_t coord_w = 0, coord_h = 0;
    font_text_size(font_mono4, coord_str_len, &coord_w, &coord_h);

    size_t star_w = 0, star_h = 0;
    font_text_size(font_mono4, 4+1+4, &star_w, &star_h);
    
    size_t elem_w = 0, elem_h = 0;
    font_text_size(font_mono4, (1+1+4)*5+4, &elem_w, &elem_h);
    elem_h *= 6;

    size_t inner_w = i64_max(coord_w, i64_max(star_w, elem_w));
    size_t inner_h = coord_h + spacing + star_h + spacing + elem_h;
    
    int outer_w = 0, outer_h = 0;
    panel_add_borders(inner_w, inner_h, &outer_w, &outer_h);

    struct panel_system_state *state = calloc(1, sizeof(*state));
    state->coord_y = 0;
    state->star_y = state->coord_y + coord_h + spacing;
    state->elem_y = state->star_y + star_h + spacing;

    struct panel *panel = panel_new(&(SDL_Rect) {
                .x = core.rect.w - outer_w,
                .y = core.rect.h - outer_h,
                .w = outer_w, .h = outer_h });
    panel->hidden = true;
    panel->state = state;
    panel->render = panel_system_render;
    panel->events = panel_system_events;
    panel->free = panel_system_free;

    return panel;
}
