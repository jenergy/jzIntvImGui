/*
 * ============================================================================
 *  Title:    Platform Portability "Library"
 *  Author:   J. Zbiciak, T. Lindner
 * ============================================================================
 *  This module fills in missing features on various platforms.
 * ============================================================================
 *  GETTIMEOFDAY     -- Return current time in seconds/microseconds.
 *  STRDUP           -- Copy a string into freshly malloc'd storage.
 *  STRICMP          -- Case-insensitive string compare.
 *  SNPRINTF         -- Like sprintf(), only with bounds checking.
 *  PLAT_DELAY       -- Sleep w/ millisecond precision.
 *  GET_EXE_DIR      -- Get the directory containing this executable
 * ============================================================================
 */

#include "config.h"
#include "plat/plat_lib_config.h"

#ifdef USE_TERMIO
# include <termio.h>
#endif

#ifdef USE_SYS_IOCTL
# include <sys/ioctl.h>
#endif

/* ======================================================================== */
/*  GENERIC PORTABLE VERSIONS...                                            */
/* ======================================================================== */

/* ------------------------------------------------------------------------ */
/*  GET_TIME -- Return current time in seconds as a double.                 */
/* ------------------------------------------------------------------------ */
#if GET_TIME_STRATEGY == GTS_CLOCK_GETTIME
double get_time(void)
{
    struct timespec now;
    double seconds;

    clock_gettime(CLOCK_MONOTONIC, &now);

    seconds = (double)now.tv_sec + (double)now.tv_nsec * 1e-9;
    return seconds;
}
#endif

#if GET_TIME_STRATEGY == GTS_GETTIMEOFDAY
double get_time(void)
{
    struct timeval now;
    double seconds;

    gettimeofday(&now, NULL);

    seconds = (double)now.tv_sec + (double)now.tv_usec * 1e-6;

    return seconds;
}
#endif

#if GET_TIME_STRATEGY == GTS_WIN_PERF_COUNTERS
#  ifndef WIN_HEADERS_INCLUDED
#     define WIN_HEADERS_INCLUDED
#     define WIN32_LEAN_AND_MEAN
#     include <windows.h>
#     include <math.h>
#  endif

LOCAL double perf_rate = -1;

extern double win32_get_time_fallback(void);

double get_time(void)
{
    LARGE_INTEGER li;

    if (perf_rate < 0)
    {
        if (QueryPerformanceFrequency(&li))
        {
            perf_rate = 1.0 / (double)li.QuadPart;
        } else
        {
            perf_rate = 0;
        }
    }

    if (perf_rate > 0)
    {
        double seconds;
        QueryPerformanceCounter(&li);
        seconds = (double)li.QuadPart * perf_rate;
        return seconds;
    }

    return win32_get_time_fallback();
}
#endif

#if GET_TIME_STRATEGY == GTS_WII
double get_time(void)
{
    extern double wii_get_time();

    return wii_get_time();
}
#endif

/* ------------------------------------------------------------------------ */
/*  PLAT_DELAY        -- Sleep w/ millisecond precision.                    */
/*  PLAT_DELAY_NO_SDL -- Sleep w/ millisecond precision.                    */
/* ------------------------------------------------------------------------ */
#if PLAT_DELAY_STRATEGY_NO_SDL == PDS_WIN_WAIT_TIMER
#  ifndef WIN_HEADERS_INCLUDED
#     define WIN_HEADERS_INCLUDED
#     define WIN32_LEAN_AND_MEAN
#     include <windows.h>
#     include <math.h>
#  endif
void plat_delay_no_sdl(unsigned msec)
{
    HANDLE timer; 
    LARGE_INTEGER ft; 

    /* Windows wants 100ns intervals */
    /* Also, -ve to specify _relative_ time */
    ft.QuadPart = -(10000ll*msec); 

    timer = CreateWaitableTimer(NULL, TRUE, NULL); 
    SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0); 
    WaitForSingleObject(timer, INFINITE); 
    CloseHandle(timer); 
}
#endif

#if PLAT_DELAY_STRATEGY_NO_SDL == PDS_NANOSLEEP
void plat_delay_no_sdl(unsigned delay)
{
    struct timespec ts;
    ts.tv_sec = delay / 1000;
    ts.tv_nsec = (delay % 1000) * 1000000;
    nanosleep(&ts, NULL);
}
#endif

#if PLAT_DELAY_STRATEGY_NO_SDL == PDS_USLEEP
void plat_delay_no_sdl(unsigned delay)
{
    usleep(delay * 1000);
}
#endif

#if PLAT_DELAY_STRATEGY_NO_SDL == PDS_BUSY_LOOP
void plat_delay_no_sdl(unsigned delay)
{
    double soon = get_time() + ((double)delay / 1000.0);

    /* -------------------------------------------------------------------- */
    /*  BAD BAD BAD: Sit in a busy loop until time expires.                 */
    /* -------------------------------------------------------------------- */
    while (get_time() < soon)
        ;

    return;
}
#endif

#if PLAT_DELAY_STRATEGY != PDS_MACINTOSH \
   && PLAT_DELAY_STRATEGY != PDS_SDL_DELAY
void plat_delay(unsigned delay)
{
    plat_delay_no_sdl(delay);
}
#endif


/* ======================================================================== */
/*  Window size functions                                                   */
/*                                                                          */
/*  GET_DISP_WIDTH   -- Get the width of the text display.                  */
/*  SET_DISP_WIDTH   -- Try to set the width of the text display.           */
/*  INIT_DISP_WIDTH  -- Initialize the display width.                       */
/*                                                                          */
/*  If the display width is unknown, this should return '80', the default   */
/*  width jzIntv historically assumed.  Because 'set_disp_width' can fail,  */
/*  it returns the new width, which may be unchanged.                       */
/*                                                                          */
/*  On platforms which cannot detect display width, "INIT_DISP_WIDTH" can   */
/*  set the display width.  Passing in 0 here just tells it to detect or    */
/*  use the default width.                                                  */
/* ======================================================================== */

/* ------------------------------------------------------------------------ */
/*  Outsourced implementation: Ask GNU Readline!                            */
/* ------------------------------------------------------------------------ */
#ifdef USE_GNU_READLINE
# define HAVE_WIDTH_IMPL
# include <readline/readline.h>

int get_disp_width(void)
{
    int rows, cols;
    rl_get_screen_size(&rows, &cols);
    return cols;
}

int set_disp_width(int new_width)
{
    UNUSED(new_width);
    return get_disp_width();
}

void init_disp_width(int width)
{
    UNUSED(width);
}
#endif

/* ------------------------------------------------------------------------ */
/*  Weakest implementation:  Just return 80 unless we're told to return     */
/*  something else.                                                         */
/* ------------------------------------------------------------------------ */
#if !defined(HAVE_WIDTH_IMPL) && !defined(WIN32) && !defined(CAN_TIOCGWINSZ)
# define HAVE_WIDTH_IMPL
LOCAL int disp_width = -1;

int get_disp_width(void)
{
    return disp_width;
}

int set_disp_width(int new_width)
{
    disp_width = new_width > 0 ? new_width : disp_width;
    return disp_width;
}

void init_disp_width(int width)
{
    disp_width = width > 0 ? width : 80;
}
#endif

/* ------------------------------------------------------------------------ */
/*  WIN32 Implementation:  Eventually use "mode con:" to change size.       */
/* ------------------------------------------------------------------------ */
#if !defined(HAVE_WIDTH_IMPL) && defined(WIN32)
# define HAVE_WIDTH_IMPL
LOCAL int disp_width = -1;

LOCAL int set_width_from_mode_con(void)
{
    FILE *f;
    f = popen("mode con:", "r");
    if (f)
    {
        char buf[80];
        int w = -1;

        while (fgets(buf, sizeof(buf), f) != NULL)
        {
            if (sscanf(buf, " Columns: %d", &w) > 0)
                disp_width = w;
        }

        pclose(f);

        if ( w == -1 ) // couldn't find it!
            return -1;
        else
            return 0;  // success
    }
    return 0;
}

int get_disp_width(void)
{
    return disp_width;
}

int set_disp_width(int new_width)
{
    char buf[80];

    if (new_width == disp_width || new_width < 1)
        return disp_width;

    sprintf(buf, "mode con: cols=%d", new_width);

    if (system(buf) == 0)
        if ( set_width_from_mode_con() < 0 ) // asking Windows didn't work?
            disp_width = new_width;          // hack:  assume it worked.

    return disp_width;
}

void init_disp_width(int width)
{
    disp_width = width > 0 ? width : 80;
}

#endif

/* ------------------------------------------------------------------------ */
/*  TIOCGWINSZ w/out SIGWINCH:  Always call the ioctl when asked the size.  */
/* ------------------------------------------------------------------------ */
#if !defined(HAVE_WIDTH_IMPL) && defined(CAN_TIOCGWINSZ) && !defined(CAN_SIGWINCH)
# define HAVE_WIDTH_IMPL
LOCAL int disp_width = 80;

int get_disp_width(void)
{
    Dims my_win;

    if (ioctl(0, TIOCGWINSZ, &my_win) != 0)
        return disp_width;

    return my_win.ws_col;
}

int set_disp_width(int new_width)
{
    disp_width = new_width > 0 ? new_width : disp_width;
    return get_disp_width();
}

void init_disp_width(int width)
{
    disp_width = width > 0 ? width : 80;
}
#endif

/* ------------------------------------------------------------------------ */
/*  TIOCGWINSZ with SIGWINCH:  Only call the ioctl if a SIGWINCH happened.  */
/* ------------------------------------------------------------------------ */
#if !defined(HAVE_WIDTH_IMPL) && defined(CAN_TIOCGWINSZ) && defined(CAN_SIGWINCH)
# define HAVE_WIDTH_IMPL
# include <signal.h>

LOCAL volatile char need_width_ioctl = 1;
LOCAL int           disp_width       = 80;

LOCAL void sigwinch_handler(int sig)
{
    UNUSED(sig);
    need_width_ioctl = 1;
}

int get_disp_width(void)
{
    struct winsize my_win;

    if (need_width_ioctl)
    {
        need_width_ioctl = 0;
        if (ioctl(0, TIOCGWINSZ, &my_win) == 0)
            disp_width = my_win.ws_col;
    }
    signal(SIGWINCH, sigwinch_handler);

    return disp_width;
}

int set_disp_width(int new_width)
{
    disp_width = new_width > 0 ? new_width : disp_width;
    need_width_ioctl = 1;
    return get_disp_width();
}

void init_disp_width(int new_width)
{
    disp_width = new_width > 0 ? new_width : disp_width;
    need_width_ioctl = 1;

    signal(SIGWINCH, sigwinch_handler);
}
#endif

/* ======================================================================== */
/*  This program is free software; you can redistribute it and/or modify    */
/*  it under the terms of the GNU General Public License as published by    */
/*  the Free Software Foundation; either version 2 of the License, or       */
/*  (at your option) any later version.                                     */
/*                                                                          */
/*  This program is distributed in the hope that it will be useful,         */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       */
/*  General Public License for more details.                                */
/*                                                                          */
/*  You should have received a copy of the GNU General Public License along */
/*  with this program; if not, write to the Free Software Foundation, Inc., */
/*  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.             */
/* ======================================================================== */
/*           Copyright (c) 1998-2020, Joseph Zbiciak, Tim Lindner           */
/* ======================================================================== */
