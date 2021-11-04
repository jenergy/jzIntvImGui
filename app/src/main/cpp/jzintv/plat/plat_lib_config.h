#ifndef PLAT_PLAT_LIB_CONFIG_H_
#define PLAT_PLAT_LIB_CONFIG_H_

/* Strategies for getting the current time, in decreasing order of
   precedence: */
#define GTS_WII                 (1)     /* WII native */
#define GTS_WIN_PERF_COUNTERS   (2)     /* Windows high precision counters */
#define GTS_CLOCK_GETTIME       (3)     /* clock_gettime() */
#define GTS_GETTIMEOFDAY        (4)     /* gettimeofday() */
#define GTS_SDL_GETTICKS        (5)     /* SDL_GetTicks() */


/* If we're on the WII, just use its method. */
#if !defined(GET_TIME_STRATEGY) && defined(WII)
#   define GET_TIME_STRATEGY GTS_WII
#endif

/* On WIN32, try to use performance counters */
#if !defined(GET_TIME_STRATEGY) \
    && defined(WIN32) && !defined(NO_QUERY_PERF_COUNTER)
#   define GET_TIME_STRATEGY GTS_WIN_PERF_COUNTERS 
#endif

/* Use clock_gettime if it's available */
#if !defined(GET_TIME_STRATEGY) && !defined(NO_CLOCK_GETTIME)
#   define GET_TIME_STRATEGY GTS_CLOCK_GETTIME
#endif

/* Otherwise, use gettimeofday if /that/ is available */
#if !defined(GET_TIME_STRATEGY) && !defined(NO_GETTIMEOFDAY)
#   define GET_TIME_STRATEGY GTS_GETTIMEOFDAY
#endif

/* If we fell through, just fall back to SDL.  This won't work out so well */
/* if we don't actually link against SDL and end up needing delays. */
/* Not worrying about it for now, since most things don't fall back to SDL. */
#if !defined(GET_TIME_STRATEGY)
#   define GET_TIME_STRATEGY GTS_SDL_GETTICKS
#endif

/* Strategies for burning time, in decreasing order of precedence: */
#define PDS_MACINTOSH           (1) /* old-school MacOS */
#define PDS_WIN_WAIT_TIMER      (2) /* Windows high precision timers */
#define PDS_SDL_DELAY           (3) /* SDL_Delay, preferred for most */
#define PDS_NANOSLEEP           (4) /* nanosleep() */
#define PDS_USLEEP              (5) /* usleep() */
#define PDS_BUSY_LOOP           (6) /* Busy-loop fallback */

/* If SDL is the primary strategy (which it is for most platforms), we */
/* need a fallback for non-SDL builds such as headless. */


#if !defined(PLAT_DELAY_STRATEGY) && defined(macintosh)
#   define PLAT_DELAY_STRATEGY PDS_MACINTOSH
#endif

#if !defined(PLAT_DELAY_STRATEGY) \
    && defined(WIN32) && !defined(NO_HIRES_WAIT_TIMER)
#   define PLAT_DELAY_STRATEGY PDS_WIN_WAIT_TIMER
#endif

#if !defined(PLAT_DELAY_STRATEGY) && !defined(NO_SDL_DELAY)
#   define PLAT_DELAY_STRATEGY PDS_SDL_DELAY
#endif

#if !defined(PLAT_DELAY_STRATEGY) && !defined(NO_NANOSLEEP)
#   define PLAT_DELAY_STRATEGY_NO_SDL PDS2_NANOSLEEP
#endif

#if !defined(PLAT_DELAY_STRATEGY) && !defined(NO_USLEEP)
#   define PLAT_DELAY_STRATEGY_NO_SDL PDS2_USLEEP
#endif

#if !defined(PLAT_DELAY_STRATEGY)
#   define PLAT_DELAY_STRATEGY PDS_BUSY_LOOP
#endif


#if !defined(PLAT_DELAY_STRATEGY_NO_SDL) && defined(macintosh)
#   define PLAT_DELAY_STRATEGY_NO_SDL PDS_MACINTOSH
#endif

#if !defined(PLAT_DELAY_STRATEGY_NO_SDL) \
    && defined(WIN32) && !defined(NO_HIRES_WAIT_TIMER)
#   define PLAT_DELAY_STRATEGY_NO_SDL PDS_WIN_WAIT_TIMER
#endif

#if !defined(PLAT_DELAY_STRATEGY_NO_SDL) && !defined(NO_NANOSLEEP)
#   define PLAT_DELAY_STRATEGY_NO_SDL PDS_NANOSLEEP
#endif

#if !defined(PLAT_DELAY_STRATEGY_NO_SDL) && !defined(NO_USLEEP)
#   define PLAT_DELAY_STRATEGY_NO_SDL PDS_USLEEP
#endif

#if !defined(PLAT_DELAY_STRATEGY_NO_SDL)
#   define PLAT_DELAY_STRATEGY_NO_SDL PDS_BUSY_LOOP
#endif

/* Internal API to allow us to fall back from one to the other */
void plat_delay_no_sdl(unsigned delay);


/* The delay strategies *must* match if PLAT_DELAY_STRATEGY is not SDL.     */
/* The _NO_SDL fallback is there just to provide an alternate impl. when    */
/* building the headless no-SDL build.                                      */
#if PLAT_DELAY_STRATEGY != PDS_SDL_DELAY \
   && PLAT_DELAY_STRATEGY != PLAT_DELAY_STRATEGY_NO_SDL
#error "Incorrect PLAT_DELAY_STRATEGY config. See plat_lib_config.h"
#endif

#endif /* PLAT_PLAT_LIB_CONFIG_H_*/
