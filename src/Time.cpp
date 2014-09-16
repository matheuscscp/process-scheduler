/*
 * Time.cpp
 *
 *  Created on: Sep 12, 2014
 *      Author: matheus
 */

#include "Time.hpp"

// =============================================================================
// SDL time functions begin
// =============================================================================

#include <ctime>
#include <cerrno>
#include <sys/time.h>

#define HAVE_CLOCK_GETTIME  0
#define HAVE_NANOSLEEP      1

typedef enum
{
    SDL_FALSE = 0,
    SDL_TRUE = 1
} SDL_bool;
typedef unsigned int Uint32;

static timespec start_ts;
static SDL_bool has_monotonic_time = SDL_FALSE;
static timeval start_tv;
static SDL_bool ticks_started = SDL_FALSE;

static void
SDL_TicksInit(void)
{
    if (ticks_started) {
        return;
    }
    ticks_started = SDL_TRUE;

    /* Set first ticks value */
#if HAVE_CLOCK_GETTIME
    if (clock_gettime(CLOCK_MONOTONIC, &start_ts) == 0) {
        has_monotonic_time = SDL_TRUE;
    } else
#elif defined(__APPLE__)
    kern_return_t ret = mach_timebase_info(&mach_base_info);
    if (ret == 0) {
        has_monotonic_time = SDL_TRUE;
        start_mach = mach_absolute_time();
    } else
#endif
    {
        gettimeofday(&start_tv, NULL);
    }
}

static Uint32
SDL_GetTicks(void)
{
    Uint32 ticks;
    if (!ticks_started) {
        SDL_TicksInit();
    }

    if (has_monotonic_time) {
#if HAVE_CLOCK_GETTIME
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        ticks = (now.tv_sec - start_ts.tv_sec) * 1000 + (now.tv_nsec -
                                                 start_ts.tv_nsec) / 1000000;
#elif defined(__APPLE__)
        uint64_t now = mach_absolute_time();
        ticks = ((now - start_mach) * mach_base_info.numer);
        ticks = (ticks / mach_base_info.denom) / 1000000;
#endif
    } else {
        struct timeval now;

        gettimeofday(&now, NULL);
        ticks =
            (now.tv_sec - start_tv.tv_sec) * 1000 + (now.tv_usec -
                                                  start_tv.tv_usec) / 1000;
    }
    return (ticks);
}

static void
SDL_Delay(Uint32 ms)
{
    int was_error;

#if HAVE_NANOSLEEP
    struct timespec elapsed, tv;
#else
    struct timeval tv;
    Uint32 then, now, elapsed;
#endif

    /* Set the timeout interval */
#if HAVE_NANOSLEEP
    elapsed.tv_sec = ms / 1000;
    elapsed.tv_nsec = (ms % 1000) * 1000000;
#else
    then = SDL_GetTicks();
#endif
    do {
        errno = 0;

#if HAVE_NANOSLEEP
        tv.tv_sec = elapsed.tv_sec;
        tv.tv_nsec = elapsed.tv_nsec;
        was_error = nanosleep(&tv, &elapsed);
#else
        /* Calculate the time interval left (in case of interrupt) */
        now = SDL_GetTicks();
        elapsed = (now - then);
        then = now;
        if (elapsed >= ms) {
            break;
        }
        ms -= elapsed;
        tv.tv_sec = ms / 1000;
        tv.tv_usec = (ms % 1000) * 1000;

        was_error = select(0, NULL, NULL, NULL, &tv);
#endif /* HAVE_NANOSLEEP */
    } while (was_error && (errno == EINTR));
}

// =============================================================================
// SDL time functions end
// =============================================================================

Time::type Time::get() {
  return SDL_GetTicks();
}

void Time::sleep(type ms) {
  SDL_Delay(ms);
}
