/* ui_man.c
   Rémi Attab (remi.attab@gmail.com), 18 Mar 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "ui/ui.h"
#include "game/man.h"

#include <stdarg.h>

static void ui_man_free(void *);
static void ui_man_update(void *);
static void ui_man_event(void *);
static void ui_man_render(void *, struct ui_layout *);


// -----------------------------------------------------------------------------
// man
// -----------------------------------------------------------------------------

enum { ui_man_cols = 89, ui_man_depth = 4 };

struct ui_man
{
    struct ui_panel *panel;
    struct ui_tree toc;
    struct ui_doc doc;

    struct link page;
    struct tape_set known;
};


static void ui_man_toc(
        struct ui_man *ui, const struct toc *toc, ui_tree_node parent)
{
    for (size_t i = 0; i < toc->len; ++i) {
        const struct toc *child = toc->nodes + i;

        if (child->item && !tech_known(proxy_tech(), child->item))
            continue;

        uint64_t user = link_to_u64(child->link);
        if (!user) failf("missing man page for '%s'", child->name);

        ui_tree_node node = ui_tree_index(&ui->toc);
        ui_str_setv(ui_tree_add(&ui->toc, parent, user), child->name, sizeof(child->name));
        ui_man_toc(ui, child, node);
    }
}

void ui_man_alloc(struct ui_view_state *state)
{
    struct dim cell = engine_cell();
    size_t tree_width = (man_toc_max + 2*ui_man_depth + 1) * cell.w;
    size_t doc_width = (ui_man_cols + 1) * cell.w;
    size_t width = tree_width + doc_width + cell.w;

    struct ui_man *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_man) {
        .panel = ui_panel_title(make_dim(width, ui_layout_inf), ui_str_c("man")),
        .toc = ui_tree_new(make_dim(tree_width, ui_layout_inf), man_toc_max),
        .doc = ui_doc_new(make_dim(doc_width, ui_layout_inf)),
        .page = link_home(),
    };

    ui_man_toc(ui, man_toc(), ui_tree_node_nil);
    ui_doc_open(&ui->doc, link_home(), proxy_lisp());

    *state = (struct ui_view_state) {
        .state = ui,
        .view = ui_view_man,
        .slots = ui_slot_right | ui_slot_left,
        .panel = ui->panel,
        .fn = {
            .free = ui_man_free,
            .update_frame = ui_man_update,
            .event = ui_man_event,
            .render = ui_man_render,
        },
    };
}

static void ui_man_free(void *state)
{
    struct ui_man *ui = state;
    ui_panel_free(ui->panel);
    ui_tree_free(&ui->toc);
    ui_doc_free(&ui->doc);
}

void ui_man_show_slot(enum ui_slot slot, struct link link)
{
    struct ui_man *ui = ui_state(ui_view_man);
    if (link_is_nil(link)) { ui_hide(ui_view_man); return; }

    ui_man_update(ui);

    if (slot == ui_slot_nil) ui_show(ui_view_man);
    else ui_show_slot(ui_view_man, slot);

    enum item item = man_item(link.page);
    if (!item || tech_known(proxy_tech(), item))
        ui_tree_select(&ui->toc, link_to_u64(link));
    else {
        ui_tree_clear(&ui->toc);
        link = man_sys_locked();
    }

    ui_doc_open(&ui->doc, link, proxy_lisp());
}

void ui_man_show(struct link link)
{
    ui_man_show_slot(ui_slot_nil, link);
}

static struct link ui_man_path_link(const char *fmt, va_list args)
{
    char path[man_path_max] = {0};
    ssize_t len = vsnprintf(path, sizeof(path), fmt, args);
    assert(len >= 0);

    struct link link = man_link(path, len);
    if (link_is_nil(link))
        ui_log(st_error, "unable to open man page '%s'", path);

    return link;
}

void ui_man_show_path(const char *path, ...)
{
    va_list args = {0};
    va_start(args, path);
    struct link link = ui_man_path_link(path, args);
    va_end(args);

    if (!link_is_nil(link)) ui_man_show(link);
}

void ui_man_show_slot_path(enum ui_slot slot, const char *path, ...)
{
    va_list args = {0};
    va_start(args, path);
    struct link link = ui_man_path_link(path, args);
    va_end(args);

    if (!link_is_nil(link)) ui_man_show_slot(slot, link);
}

static void ui_man_update(void *state)
{
    struct ui_man *ui = state;

    const struct tech *tech = proxy_tech();
    if (tape_set_eq(&tech->known, &ui->known)) return;
    ui->known = tech->known;

    ui_tree_reset(&ui->toc);
    ui_man_toc(ui, man_toc(), ui_tree_node_nil);
}

static void ui_man_event(void *state)
{
    struct ui_man *ui = state;

    ui_doc_event(&ui->doc);
    if (ui_tree_event(&ui->toc))
        ui_doc_open(&ui->doc, link_from_u64(ui->toc.selected), proxy_lisp());
}

static void ui_man_render(void *state, struct ui_layout *layout)
{
    struct ui_man *ui = state;
    ui_tree_render(&ui->toc, layout);
    ui_doc_render(&ui->doc, layout);
}
