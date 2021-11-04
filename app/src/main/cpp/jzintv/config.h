/*
 * ============================================================================
 *  Title:    Project-Wide Config
 *  Author:   J. Zbiciak
 * ============================================================================
 *  _BIG_ENDIAN         -- Host machine is big endian
 *  _LITTLE_ENDIAN      -- Host machine is little endian
 * ============================================================================
 *
 * ============================================================================
 */

#ifndef CONFIG_H_
#define CONFIG_H_


/*
 * ============================================================================
 *  If you get an error here, define BYTE_BE or BYTE_LE as is required for
 *  your host machine!  You can do that in your Makefile by adding it to
 *  CFLAGS, or by uncommenting the appropriate #define below.
 * ============================================================================
 */

/* #define BYTE_BE */  /* Uncomment for big endian    */
/* #define BYTE_LE */  /* Uncomment for little endian */

#if !defined(BYTE_BE) && !defined(BYTE_LE)

#  if defined(__BIG_ENDIAN__)
#    define BYTE_BE
#  endif

#  if defined(__LITTLE_ENDIAN__)
#    define BYTE_LE
#  endif

#  if !(defined(BYTE_BE) || defined(BYTE_LE)) && \
       (defined(sparc)   || defined(__sparc)    || defined(__sparc__)   || \
        defined(sparc64) || defined(__sparc64)  || defined(__sparc64__) || \
        defined(ppc)     || defined(__ppc)      || defined(__ppc__)     || \
        defined(ppc64)   || defined(__ppc64)    || defined(__ppc64__)   || \
        defined(POWERPC) || defined(__POWERPC)  || defined(__POWERPC__))
#    define BYTE_BE
#  endif

#  if !(defined(BYTE_BE) || defined(BYTE_LE)) && \
       (defined(i386)    || defined(__i386)     || defined(__i386__)    || \
        defined(x86_64)  || defined(__x86_64)   || defined(__x86_64__)  || \
        defined(amd64)   || defined(__amd64)    || defined(__amd64__)   || \
        defined(ia64)    || defined(__ia64)     || defined(__ia64__)    || \
        defined(alpha)   || defined(__alpha)    || defined(__alpha__)   || \
        defined(_M_IX86) || defined(_M_X64))
#    define BYTE_LE
#  endif

#  if !defined(BYTE_BE) && !defined(BYTE_LE)
#    include <endian.h>
#    ifndef __BYTE_ORDER
#      error Please manually set your machine endian in 'config.h'
#    endif
#    if __BYTE_ORDER==4321
#      define BYTE_BE
#    endif
#    if __BYTE_ORDER==1234
#      define BYTE_LE
#    endif
#    if !defined(BYTE_BE) && !defined(BYTE_LE)
#      error Cannot determine target endian.  See 'config.h' for details.
#    endif
#  endif

#endif

#if defined(BYTE_BE) && defined(BYTE_LE)
#  error Both BYTE_BE and BYTE_LE defined.  Pick only 1!
#endif

#if !defined(BYTE_BE) && !defined(BYTE_LE)
#  error One of BYTE_BE or BYTE_LE must be defined.
#endif

/* ======================================================================== */
/*  BFE         -- Builds a `B'it`F'ield structure in the correct order as  */
/*                 required for the host machine's `E'ndian.                */
/* ======================================================================== */
#ifdef BYTE_BE
#  define BFE(x,y) y; x
#else /* BYTE_LE */
#  define BFE(x,y) x; y
#endif

/* ======================================================================== */
/*  BSWAP_16         -- Byte-swap a 16-bit value.                           */
/*  BSWAP_32         -- Byte-swap a 32-bit value.                           */
/*  BSWAP_64         -- Byte-swap a 64-bit value.                           */
/*  HOST_TO_LE_16    -- Converts host byte order to little endian           */
/*  HOST_TO_LE_32    -- Converts host byte order to little endian           */
/*  HOST_TO_LE_64    -- Converts host byte order to little endian           */
/*  HOST_TO_BE_16    -- Converts host byte order to big endian              */
/*  HOST_TO_BE_32    -- Converts host byte order to big endian              */
/*  HOST_TO_BE_64    -- Converts host byte order to big endian              */
/*  LE_TO_HOST_16    -- Converts little endian to host byte order           */
/*  LE_TO_HOST_32    -- Converts little endian to host byte order           */
/*  LE_TO_HOST_64    -- Converts little endian to host byte order           */
/*  BE_TO_HOST_16    -- Converts big endian to host byte order              */
/*  BE_TO_HOST_32    -- Converts big endian to host byte order              */
/*  BE_TO_HOST_64    -- Converts big endian to host byte order              */
/* ======================================================================== */

# define BSWAP_16(x) ((((x) << 8) & 0xFF00) | (((x) >> 8) & 0x00FF))
# define BSWAP_32(x) ((((x) << 24) & 0xFF000000u) |                         \
                      (((x) <<  8) & 0x00FF0000u) |                         \
                      (((x) >>  8) & 0x0000FF00u) |                         \
                      (((x) >> 24) & 0x000000FFu))
# define BSWAP_64(x) ((((uint64_t)(x) & 0x000000FFu) << 56) |               \
                      (((uint64_t)(x) & 0x0000FF00u) << 40) |               \
                      (((uint64_t)(x) & 0x00FF0000u) << 24) |               \
                      (((uint64_t)(x) & 0xFF000000u) <<  8) |               \
                      (((x) >>  8) & 0xFF000000u) |                         \
                      (((x) >> 24) & 0x00FF0000u) |                         \
                      (((x) >> 40) & 0x0000FF00u) |                         \
                      (((x) >> 56) & 0x000000FFu))

#ifdef BYTE_LE
# define HOST_TO_LE_16(x) (x)
# define HOST_TO_LE_32(x) (x)
# define HOST_TO_LE_64(x) (x)
# define HOST_TO_BE_16(x) BSWAP_16(x)
# define HOST_TO_BE_32(x) BSWAP_32(x)
# define HOST_TO_BE_64(x) BSWAP_64(x)
# define LE_TO_HOST_16(x) (x)
# define LE_TO_HOST_32(x) (x)
# define LE_TO_HOST_64(x) (x)
# define BE_TO_HOST_16(x) BSWAP_16(x)
# define BE_TO_HOST_32(x) BSWAP_32(x)
# define BE_TO_HOST_64(x) BSWAP_64(x)
#else
# define HOST_TO_BE_16(x) (x)
# define HOST_TO_BE_32(x) (x)
# define HOST_TO_BE_64(x) (x)
# define HOST_TO_LE_16(x) BSWAP_16(x)
# define HOST_TO_LE_32(x) BSWAP_32(x)
# define HOST_TO_LE_64(x) BSWAP_64(x)
# define BE_TO_HOST_16(x) (x)
# define BE_TO_HOST_32(x) (x)
# define BE_TO_HOST_64(x) (x)
# define LE_TO_HOST_16(x) BSWAP_16(x)
# define LE_TO_HOST_32(x) BSWAP_32(x)
# define LE_TO_HOST_64(x) BSWAP_64(x)
#endif

/* ======================================================================== */
/*  Use void casts to indicate when variables or values are unused.         */
/* ======================================================================== */
#define UNUSED(x) ((void)(x))

/* ======================================================================== */
/*  If our compiler supports 'inline', enable it here.                      */
/* ======================================================================== */
#if (defined(__GNUC__) || defined(_TMS320C6X)) && !defined(NO_INLINE)
# define INLINE inline
#else
# define INLINE
#endif

/* ======================================================================== */
/*  If our compiler supports aligning, enable it here.                      */
/* ======================================================================== */
#ifdef __GNUC__
#define ALIGN(x) __attribute__((aligned(x)))
#else
#define ALIGN(x)
#endif

/* ======================================================================== */
/*  As of C99, we have target-independent types and bool.                   */
/* ======================================================================== */
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

/* ======================================================================== */
/*  Endian conversion macros                                                */
/* ======================================================================== */
#ifdef __cplusplus
#define as_uint8_t(x)  static_cast<uint8_t >(x)
#define as_uint16_t(x) static_cast<uint16_t>(x)
#define as_uint32_t(x) static_cast<uint32_t>(x)
#else
#define as_uint8_t(x)  ((uint8_t )(x))
#define as_uint16_t(x) ((uint16_t)(x))
#define as_uint32_t(x) ((uint32_t)(x))
#endif

#ifdef BYTE_BE
#define be_to_host_16(x) (x)
#define be_to_host_32(x) (x)
#define le_to_host_16(x) ((0x00FF & (as_uint16_t(x) >> 8)) | \
                          (0xFF00 & (as_uint16_t(x) << 8)))
#define le_to_host_32(x) ((0x000000FF & (as_uint32_t(x) >> 24)) | \
                          (0x0000FF00 & (as_uint32_t(x) >>  8)) | \
                          (0x00FF0000 & (as_uint32_t(x) <<  8)) | \
                          (0xFF000000 & (as_uint32_t(x) << 24)))
#endif

#ifdef BYTE_LE
#define le_to_host_16(x) (x)
#define le_to_host_32(x) (x)
#define be_to_host_16(x) ((0x00FF & (as_uint16_t(x) >> 8)) | \
                          (0xFF00 & (as_uint16_t(x) << 8)))
#define be_to_host_32(x) ((0x000000FF & (as_uint32_t(x) >> 24)) | \
                          (0x0000FF00 & (as_uint32_t(x) >>  8)) | \
                          (0x00FF0000 & (as_uint32_t(x) <<  8)) | \
                          (0xFF000000 & (as_uint32_t(x) << 24)))

#endif

#define host_to_le_16(x) le_to_host_16(x)
#define host_to_be_16(x) be_to_host_16(x)
#define host_to_le_32(x) le_to_host_32(x)
#define host_to_be_32(x) be_to_host_32(x)


/* ======================================================================== */
/*  Target-specific library compatibility issues                            */
/* ======================================================================== */

#if defined(__APPLE__) && defined(__MACH__)
# define PLAT_MACOS
#endif

#if defined(__linux__) && !defined(PLAT_LINUX)
# define PLAT_LINUX
#endif

#ifdef PLAT_LINUX
# define USE_STRCASECMP
# define DEFAULT_ROM_PATH ".:=../rom:/usr/local/share/jzintv/rom"
# define HAS_LINK
# define HAS_READLINK
# define HAS_LSTAT
# define PROC_SELF_EXE "/proc/self/exe"
# define DEFAULT_AUDIO_HZ     (48000)
# define SND_BUF_SIZE_DEFAULT (2048)
# define SND_BUF_CNT_DEFAULT  (3)
# define CAN_TIOCGWINSZ
# define CAN_SIGWINCH
# define USE_TERMIO
#endif

#ifdef __TERMUX__
# define USE_STRCASECMP
/* TODO: Fix DEFAULT_ROM_PATH */
# define DEFAULT_ROM_PATH ".:=../rom:/usr/local/share/jzintv/rom"
# define HAS_LINK
# define DEFAULT_AUDIO_HZ     (48000)
# define SND_BUF_SIZE_DEFAULT (2048)
# define SND_BUF_CNT_DEFAULT  (3)
# define NO_SETUID
#endif

#ifdef SOLARIS
/*# define NO_SNPRINTF*/
# define NO_GETOPT_LONG
# define NO_INOUT
# define USE_STRCASECMP
# define DEFAULT_ROM_PATH ".:=../rom:/usr/local/share/jzintv/rom"
# define HAS_LINK
# define HAS_READLINK
# define HAS_LSTAT
# define CAN_TIOCGWINSZ
# define CAN_SIGWINCH
#endif

#ifdef WIN32
# define NO_CLOCK_GETTIME
# define NO_GETTIMEOFDAY
/*# define NO_SNPRINTF*/
# define NO_GETOPT_LONG
# define NOGETOPT
# define NO_SETUID
# define NO_NANOSLEEP
# define NO_RAND48
# define NO_FCNTL
# define USE_FCNTL_H
# define USE_MKTEMP
# define DEFAULT_ROM_PATH ".;=..\\rom"
# define PATH_SEP '\\'
# define PATH_COMPONENT_SEP ";"
# define NO_SETUID
# define DEFAULT_AUDIO_HZ (48000)
/* If there is a better way to detect this, someone please tell me, as using */
/* the macro __USE_MINGW_ANSI_STDIO directly is supposedly unsupported.      */
# if !defined(__USE_MINGW_ANSI_STDIO) || !__USE_MINGW_ANSI_STDIO
#  define LL_FMT "I64"
#  define I64_FMT "I64i"
#  define U64_FMT "I64u"
#  define X64_FMT "I64x"
# endif
# ifndef _MSC_VER
#  define USE_STRCASECMP
#  define NEED_INOUT INOUT_GCC
# else
#  define NO_ACCESS
#  define NO_SYS_TIME_H
#  define NO_UNISTD_H
#  define YY_NO_UNISTD_H
#  define NEED_CORECRT_IO_H
#  define NEED_DIRECT_H
#  define popen _popen
#  define pclose _pclose
#  define NEED_INOUT INOUT_MSVC
# endif
#endif

#ifdef PLAT_MACOS
# define NO_RAND48
# define NO_INOUT
# define NO_GETOPT_LONG
# define NO_CLOCK_GETTIME
# define NOGETOPT
# define USE_STRCASECMP /* ? */
# define DEFAULT_ROM_PATH ".:=../rom"
# define HAS_LINK
# define HAS_READLINK
# define HAS_LSTAT
# define DEFAULT_AUDIO_HZ (48000)
# define CAN_TIOCGWINSZ
# define CAN_SIGWINCH
# define USE_SYS_IOCTL
#endif

#ifdef __FreeBSD__
# define NO_INOUT
# define USE_STRCASECMP
# define DEFAULT_ROM_PATH ".:=../rom:/usr/local/share/jzintv/rom"
# define HAS_LINK
# define HAS_READLINK
# define HAS_LSTAT
# define PROC_SELF_EXE "/proc/curproc/exe"
# define CAN_TIOCGWINSZ
# define CAN_SIGWINCH
# define USE_SYS_IOCTL
#endif

#ifdef _TMS320C6X
# define NO_CLOCK_GETTIME
# define NO_GETTIMEOFDAY
# define NO_STRDUP
# define NO_SYS_TIME_H
# define NO_UNISTD_H
# define NO_GETOPT_LONG
# define NO_SETUID
# define NO_NANOSLEEP
# define NO_USLEEP
# define NO_RAND48
# define NO_INOUT
# define NO_STRICMP
# define HAVE_RESTRICT
# define NO_FCNTL
# ifndef CLK_TCK
#  define CLK_TCK 200000000 /* Assume 200MHz C6201 device */
# endif
# define DEFAULT_ROM_PATH "."
# define FULLSC_START_DLY   (0)
# define NO_SETUID
# define NO_GETCWD
#endif

#ifdef GP2X
# define FULLSC_START_DLY   (0)
# define SMALLMEM
#endif

#ifdef __AMIGAOS4__
# define NO_GETOPT_LONG
# define NO_SETUID
# define PATH_SEP '/'
# define PATH_COMPONENT_SEP ";"
# define DEFAULT_ROM_PATH ".:=../rom"
# define NO_GETCWD
#endif

#ifdef WII
# define NO_ACCESS
# define NO_GETOPT_LONG
# define NO_CLOCK_GETTIME
# define NO_GETTIMEOFDAY
# define NO_FCNTL
# define NO_SETUID
# define NO_SDL_DELAY
# define NO_CLOCK
# define PATH_SEP '/'
# define PATH_COMPONENT_SEP ";"
# define DEFAULT_CFG_PATH "sd:/jzintvWii/cfg"
# define DEFAULT_ROM_PATH "sd:/jzintvWii/roms"
# define DEFAULT_AUDIO_HZ (48000)
# define SND_BUF_SIZE_DEFAULT (512)
# define SND_BUF_CNT_DEFAULT  (3)
# define NO_GETCWD
#endif

#ifdef __EMSCRIPTEN__
# define USE_STRCASECMP
# define NO_FCNTL
# define NO_SETUID
# define NO_GETCWD
# define NO_SERIALIZER
# define DEFAULT_ROM_PATH "$LZOE$"
# define DEFAULT_AUDIO_HZ     (48000)
# define SND_BUF_SIZE_DEFAULT (2048)
# define SND_BUF_CNT_DEFAULT  (3)
# define USE_FCNTL_H
# define GET_TIME_STRATEGY GTS_GETTIMEOFDAY
# include <emscripten.h>
#endif

/* ======================================================================== */
/*  If we're compiling with GCC, and it's 4.8 or newer, we can use the      */
/*  target_clones facility to specify multiple optimization points for      */
/*  certain functions.  This is mainly a win in the scaler.                 */
/* ======================================================================== */

#if !defined(PLAT_MACOS) && !defined(__MINGW32__) && \
    ((__GNUC__+0 == 4 && __GNUC_MINOR__+0 >= 8) || (__GNUC__+0 > 4))
/* For now, just do this on x86. */
# if (defined(i386)    || defined(__i386)     || defined(__i386__)    || \
      defined(x86_64)  || defined(__x86_64)   || defined(__x86_64__)  || \
      defined(amd64)   || defined(__amd64)    || defined(__amd64__))
#  define TARGET_CLONES(X) \
   X __attribute__((__target_clones__("default,sse,sse2,sse3,ssse3,sse4.1,sse4.2,avx"))); X
# endif
#endif

#ifndef TARGET_CLONES
# define TARGET_CLONES(X) X
#endif

/* ======================================================================== */
/*  Enable some C language features if we're compiling with a new enough    */
/*  C standard.                                                             */
/* ======================================================================== */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
# define STATIC_ASSERT(expr, msg) _Static_assert((expr), msg)
#else
# define STATIC_ASSERT(expr, msg)
#endif /* __STDC_VERSION__ >= 201112L */

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
# define HAVE_RESTRICT
# undef GNU_RESTRICT
#endif /* __STDC_VERSION__ >= 199901L */
/*
 * ============================================================================
 *  Clean up per-arch configs w/ some defaults.
 * ============================================================================
 */

#ifndef PATH_SEP
# define PATH_SEP '/'
#endif

#ifndef PATH_COMPONENT_SEP
# define PATH_COMPONENT_SEP ":"
#endif

#ifndef DEFAULT_ROM_PATH
# define DEFAULT_ROM_PATH NULL
#endif

#ifndef FULLSC_START_DLY
# define FULLSC_START_DLY (500)
#endif

#ifndef DEFAULT_AUDIO_HZ
# define DEFAULT_AUDIO_HZ (44100)
#endif

#if !defined(SND_BUF_SIZE_DEFAULT)
# define SND_BUF_SIZE_DEFAULT (2048)
#endif

#if !defined(SND_BUF_CNT_DEFAULT)
# define SND_BUF_CNT_DEFAULT  (2)
#endif

#if !defined(LL_FMT)
# define LL_FMT "ll"
#endif

#if !defined(I64_FMT)
# define I64_FMT PRIi64
#endif

#if !defined(U64_FMT)
# define U64_FMT PRIu64
#endif

#if !defined(X64_FMT)
# define X64_FMT PRIx64
#endif

#if !defined(NEED_INOUT)
# define NEED_INOUT INOUT_NONE
#endif

#define INOUT_NONE 0
#define INOUT_GCC  1
#define INOUT_MSVC 2

/*
 * ============================================================================
 *  CGC support configuration
 * ============================================================================
 */

#if defined(WIN32)
# define CGC_SUPPORTED
# define CGC_DLL
#endif

#if defined(PLAT_MACOS) || defined(PLAT_LINUX)
#define CGC_SUPPORTED
#define CGC_THREAD
#endif

/*
 * ============================================================================
 *  Standard #includes that almost everyone needs
 * ============================================================================
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <assert.h>
#include <limits.h>
#include <errno.h>
#include <math.h>

#ifndef NO_SYS_TIME_H
# include <sys/time.h>
#endif

#ifdef _TMS320C6X           /* This seems to be TI-specific.    */
# include <file.h>
#endif

#ifdef NO_GETOPT_LONG
# include "plat/gnu_getopt.h"
#else
# include <getopt.h>
#endif

#ifndef NO_UNISTD_H
# include <unistd.h>
#endif

#ifdef NEED_CORECRT_IO_H    /* Provides open(), read() etc. on MSVC */
# include <corecrt_io.h>
# ifndef STDIN_FILENO
#  define STDIN_FILENO 0
# endif
#endif

#ifdef NEED_DIRECT_H        /* Provides getcwd on MSVC */
# include <direct.h>
#endif

#ifdef USE_STRCASECMP
# include <strings.h>
# define stricmp strcasecmp
#endif

#if !defined(NO_FCNTL) || defined(USE_FCNTL_H)
# include <fcntl.h>
#endif

#if defined(HAS_LSTAT)
# include <sys/stat.h>
#endif

#ifndef M_PI
# ifdef PI
#  define M_PI PI
# else
#  define M_PI (3.14159265358979323846)
# endif
#endif

/*
 * ============================================================================
 *  If this compiler implements the C99 'restrict' keyword, then enable it.
 * ============================================================================
 */
#ifdef HAVE_RESTRICT
# define RESTRICT restrict
#endif

#ifdef GNU_RESTRICT
# define RESTRICT __restrict__
#endif

#ifndef RESTRICT
# define RESTRICT
#endif

/*
 * ============================================================================
 *  Allow exposing "local" symbols by using LOCAL instead of static
 * ============================================================================
 */
#ifndef LOCAL
# define LOCAL static
#else
# warning "LOCAL already defined before config.h"
#endif

/*
 * ============================================================================
 *  Indicate when we specifically intend to fall-through on a switch-case
 * ============================================================================
 */
#if defined(__GNUC__) && __GNUC__ >= 7
# define FALLTHROUGH_INTENDED __attribute__((fallthrough))
#else
# define FALLTHROUGH_INTENDED /* fallthrough */
#endif

/*
 * ============================================================================
 *  Include the "platform library" to handle missing functions
 * ============================================================================
 */

#include "plat/plat_lib.h"

/* We use this everywhere, so just include it here. */
#include "misc/jzprint.h"

/*
 * ============================================================================
 *  No other good place for this at the moment.  :-(
 * ============================================================================
 */
extern void dump_state(void);
#define CONDFREE_k(x) \
    do {                                                                    \
        if (x)                                                              \
        {                                                                   \
            /* Ugly hack to avoid cast-away-const warnings on free() */     \
            union                                                           \
            {                                                               \
                void *non_const_ptr;                                        \
                const void *const_ptr;                                      \
            } pun;                                                          \
            pun.const_ptr = (const void *)(x);                              \
            free(pun.non_const_ptr);                                        \
        }                                                                   \
    } while (0)

#define CONDFREE(x) \
    do {                                                                    \
        CONDFREE_k(x);                                                      \
        (x) = NULL;                                                         \
    } while (0)

#define NO_SERIALIZER

/*
 * ============================================================================
 *  Version number
 * ============================================================================
 */

#ifndef JZINTV_VERSION_MAJOR
# define JZINTV_VERSION_MAJOR (0)
#endif

#ifndef JZINTV_VERSION_MINOR
# define JZINTV_VERSION_MINOR (0)
#endif

#define JZINTV_VERSION ((JZINTV_VERSION_MAJOR << 8) | JZINTV_VERSION_MINOR)

extern const char *svn_revision;

/*
 * ============================================================================
 *  The jzIntv entry point
 * ============================================================================
 */
extern int jzintv_entry_point(int argv, char *argc[]);

#endif /* CONFIG_H */

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
/*                 Copyright (c) 1998-2020, Joseph Zbiciak                  */
/* ======================================================================== */
