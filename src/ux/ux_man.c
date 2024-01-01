/* ux_man.c
   Rémi Attab (remi.attab@gmail.com), 18 Mar 2022
   FreeBSD-style copyright and disclaimer apply
*/


static void ux_man_free(void *);
static void ux_man_update(void *);
static void ux_man_event(void *);
static void ux_man_render(void *, struct ui_layout *);


// -----------------------------------------------------------------------------
// man
// -----------------------------------------------------------------------------

constexpr size_t ux_man_cols = 89;
constexpr size_t ux_man_depth = 4;

struct ux_man
{
    struct ui_panel *panel;
    struct ui_tree toc;
    struct ui_doc doc;

    struct link page;
    struct tape_set known;
};


static void ux_man_toc(
        struct ux_man *ux, const struct toc *toc, ui_tree_node parent)
{
    for (size_t i = 0; i < toc->len; ++i) {
        const struct toc *child = toc->nodes + i;

        if (child->item && !tech_known(proxy_tech(), child->item))
            continue;

        uint64_t user = link_to_u64(child->link);
        if (!user) failf("missing man page for '%s'", child->name);

        ui_tree_node node = ui_tree_index(&ux->toc);
        ui_str_setv(ui_tree_add(&ux->toc, parent, user), child->name, sizeof(child->name));
        ux_man_toc(ux, child, node);
    }
}

void ux_man_alloc(struct ux_view_state *state)
{
    struct dim cell = engine_cell();
    size_t tree_width = (man_toc_max + 2*ux_man_depth + 1) * cell.w;
    size_t doc_width = (ux_man_cols + 1) * cell.w;
    size_t width = tree_width + doc_width + cell.w;

    struct ux_man *ux = calloc(1, sizeof(*ux));
    *ux = (struct ux_man) {
        .panel = ui_panel_title(make_dim(width, ui_layout_inf), ui_str_c("man")),
        .toc = ui_tree_new(make_dim(tree_width, ui_layout_inf), man_toc_max),
        .doc = ui_doc_new(make_dim(doc_width, ui_layout_inf)),
        .page = link_home(),
    };

    ux_man_toc(ux, man_toc(), ui_tree_node_nil);
    ui_doc_open(&ux->doc, link_home(), proxy_lisp());

    *state = (struct ux_view_state) {
        .state = ux,
        .view = ux_view_man,
        .slots = ux_slot_right | ux_slot_left,
        .panel = ux->panel,
        .fn = {
            .free = ux_man_free,
            .update = ux_man_update,
            .event = ux_man_event,
            .render = ux_man_render,
        },
    };
}

static void ux_man_free(void *state)
{
    struct ux_man *ux = state;
    ui_panel_free(ux->panel);
    ui_tree_free(&ux->toc);
    ui_doc_free(&ux->doc);
}

void ux_man_show_slot(enum ux_slot slot, struct link link)
{
    struct ux_man *ux = ux_state(ux_view_man);
    if (link_is_nil(link)) { ux_hide(ux_view_man); return; }

    ux_man_update(ux);

    if (slot == ux_slot_nil) ux_show(ux_view_man);
    else ux_show_slot(ux_view_man, slot);

    enum item item = man_item(link.page);
    if (!item || tech_known(proxy_tech(), item))
        ui_tree_select(&ux->toc, link_to_u64(link));
    else {
        ui_tree_clear(&ux->toc);
        link = man_sys_locked();
    }

    ui_doc_open(&ux->doc, link, proxy_lisp());
}

void ux_man_show(struct link link)
{
    ux_man_show_slot(ux_slot_nil, link);
}

static struct link ux_man_path_link(const char *fmt, va_list args)
{
    char path[man_path_max] = {0};
    ssize_t len = vsnprintf(path, sizeof(path), fmt, args);
    assert(len >= 0);

    struct link link = man_link(path, len);
    if (link_is_nil(link))
        ux_log(st_error, "unable to open man page '%s'", path);

    return link;
}

void ux_man_show_path(const char *path, ...)
{
    va_list args = {0};
    va_start(args, path);
    struct link link = ux_man_path_link(path, args);
    va_end(args);

    if (!link_is_nil(link)) ux_man_show(link);
}

void ux_man_show_slot_path(enum ux_slot slot, const char *path, ...)
{
    va_list args = {0};
    va_start(args, path);
    struct link link = ux_man_path_link(path, args);
    va_end(args);

    if (!link_is_nil(link)) ux_man_show_slot(slot, link);
}

static void ux_man_update(void *state)
{
    struct ux_man *ux = state;

    const struct tech *tech = proxy_tech();
    if (tape_set_eq(&tech->known, &ux->known)) return;
    ux->known = tech->known;

    ui_tree_reset(&ux->toc);
    ux_man_toc(ux, man_toc(), ui_tree_node_nil);
}

static void ux_man_event(void *state)
{
    struct ux_man *ux = state;

    ui_doc_event(&ux->doc);
    if (ui_tree_event(&ux->toc))
        ui_doc_open(&ux->doc, link_from_u64(ux->toc.selected), proxy_lisp());
}

static void ux_man_render(void *state, struct ui_layout *layout)
{
    struct ux_man *ux = state;
    ui_tree_render(&ux->toc, layout);
    ui_doc_render(&ux->doc, layout);
}
