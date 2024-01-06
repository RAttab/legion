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

typedef uint64_t sys_ts;

constexpr sys_ts sys_nsec = 1;
constexpr sys_ts sys_usec = 1000 * sys_nsec;
constexpr sys_ts sys_msec = 1000 * sys_usec;
constexpr sys_ts sys_sec  = 1000 * sys_msec;

// get around compiler warnings about static variables in inlined headers.
#define sys_sec_s ((sys_ts) 1000*1000*1000)

inline sys_ts sys_now(void)
{
    struct timespec ts = {0};
    (void) clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * sys_sec_s + ts.tv_nsec;
}

inline sys_ts sys_sleep_until(sys_ts until)
{
    sys_ts now = sys_now();
    if (unlikely(now >= until)) return now;
    sys_ts delta = until - now;

    struct timespec ts = { .tv_sec = 0, .tv_nsec = delta };
    if (unlikely(delta > sys_sec_s)) {
        ts.tv_sec = delta / sys_sec_s;
        ts.tv_nsec = delta % sys_sec_s;
    }

    while (nanosleep(&ts, &ts));
    return sys_now();
}

#undef sys_sec_s
