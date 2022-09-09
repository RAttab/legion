/* time.h
   Rémi Attab (remi.attab@gmail.com), 10 Dec 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"

#include <time.h>


// -----------------------------------------------------------------------------
// time
// -----------------------------------------------------------------------------

typedef uint64_t ts_nano;

static const ts_nano ts_nsec = 1;
static const ts_nano ts_usec = 1000 * ts_nsec;
static const ts_nano ts_msec = 1000 * ts_usec;
static const ts_nano ts_sec  = 1000 * ts_msec;

// get around compiler warnings about static variables in inlined headers.
#define ts_sec_s ((ts_nano) 1000*1000*1000)

inline ts_nano ts_now(void)
{
    struct timespec ts = {0};
    (void) clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * ts_sec_s + ts.tv_nsec;
}

inline ts_nano ts_sleep_until(ts_nano until)
{
    ts_nano now = ts_now();
    if (unlikely(now >= until)) return now;
    ts_nano delta = until - now;

    struct timespec ts = { .tv_sec = 0, .tv_nsec = delta };
    if (unlikely(delta > ts_sec_s)) {
        ts.tv_sec = delta / ts_sec_s;
        ts.tv_nsec = delta % ts_sec_s;
    }

    while (nanosleep(&ts, &ts));
    return ts_now();
}

#undef ts_sec_s
