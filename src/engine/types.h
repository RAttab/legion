/* types.h
   Remi Attab (remi.attab@gmail.com), 07 Oct 2023
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once


// -----------------------------------------------------------------------------
// unit
// -----------------------------------------------------------------------------

typedef int64_t unit;


// -----------------------------------------------------------------------------
// point
// -----------------------------------------------------------------------------

struct pos { unit x, y; };

inline struct pos make_pos(unit x, unit y)
{
    return (struct pos) { .x = x, .y = y };
}

inline struct pos pos_nil(void) { return make_pos(0, 0); }
inline bool point_is_nil(struct pos p) { return !p.x && !p.y; }

inline struct pos pos_add(struct pos p, unit x, unit y)
{
    return make_pos(p.x + x, p.y + y);
}


// -----------------------------------------------------------------------------
// dim
// -----------------------------------------------------------------------------

struct dim { unit w, h; };


inline struct dim make_dim(unit w, unit h)
{
    return (struct dim) { .w = w, .h = h };
}

inline struct dim dim_nil(void) { return make_dim(0, 0); }
inline bool dim_is_nil(struct dim d) { return !d.w && !d.h; }


// -----------------------------------------------------------------------------
// rect
// -----------------------------------------------------------------------------

struct rect { unit x, y, w, h; };

inline struct rect make_rect(unit x, unit y, unit w, unit h)
{
    return (struct rect) { .x = x, .y = y, .w = w, .h = h };
}

inline struct rect make_rect_parts(struct pos p, struct dim d)
{
    return (struct rect) { .x = p.x, .y = p.y, .w = d.w, .h = d.h };
}

inline struct rect rect_nil(void) { return make_rect(0, 0, 0, 0); }
inline bool rect_is_nil(struct rect r) { return !r.x && !r.y && !r.w && !r.h; }

inline struct dim rect_dim(struct rect r) { return make_dim(r.w, r.h); }
inline struct pos rect_pos(struct rect r) { return make_pos(r.x, r.y); }

inline struct rect rect_set_pos(struct rect r, struct pos p)
{
    return make_rect(p.x, p.y, r.w, r.h);
}

inline struct rect rect_set_dim(struct rect r, struct dim d)
{
    return make_rect(r.x, r.y, d.w, d.h);
}

inline bool rect_contains(struct rect r, struct pos p)
{
    return p.x >= r.x && p.x <= (r.x + r.w)
        && p.y >= r.y && p.y <= (r.y + r.h);
}

inline bool rect_intersects(struct rect a, struct rect b)
{
    if (legion_max(a.x, b.x) >= legion_min(a.x + a.w, b.x + b.w)) return false;
    if (legion_max(a.y, b.y) >= legion_min(a.y + a.h, b.y + b.h)) return false;
    return true;
}

inline struct pos rect_clamp(struct rect r, struct pos p)
{
    return make_pos(
            legion_bound(p.x, r.x, r.x + r.w),
            legion_bound(p.y, r.y, r.y + r.h));
}
