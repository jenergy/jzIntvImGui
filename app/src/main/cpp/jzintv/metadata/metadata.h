#ifndef METADATA_H_
#define METADATA_H_

#include "config.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include "misc/types.h"

/* ------------------------------------------------------------------------ */
/*  COMPAT_LEVEL_T  Specifies compatibility with various hardware.          */
/*                                                                          */
/*      Incompatible   Will not function correctly if hardware is present.  */
/*      Tolerates      Works, but is not enhanced by hardware's presence.   */
/*      Enhanced       Enhanced by hardware, but doesn't require it.        */
/*      Requires       Will not function correctly if hardware is absent.   */
/* ------------------------------------------------------------------------ */
typedef enum compat_level_t
{
    CMP_UNSPECIFIED     = -1,
    CMP_INCOMPATIBLE    = 0x00,
    CMP_TOLERATES       = 0x01,
    CMP_ENHANCED        = 0x02,
    CMP_REQUIRES        = 0x03
} compat_level_t;

/* ------------------------------------------------------------------------ */
/*  JLP_ACCEL_T     Specifies JLP Acceleration ability                      */
/*                                                                          */
/*      Disabled        No JLP functionality                                */
/*      Accel On        Accel enabled at reset; no flash.                   */
/*      Accel Off       Accel disabled at reset; jlp_flash controls flash   */
/*      Accel Flash On  Accel enabled at reset; jlp_flash controls flash    */
/*                                                                          */
/*  In all modes but "disabled", programs can switch accelerators + RAM     */
/*  on and off by writing special values to $8033/$8034.                    */
/*                                                                          */
/*  The "Accel On" setting is redundant with respect to "Accel Flash On"    */
/*  when combined with jlp_flash setting.                                   */
/* ------------------------------------------------------------------------ */
typedef enum jlp_accel_t
{
    JLP_UNSPECIFIED     = -1,
    JLP_DISABLED        = 0x00, /* No JLP functionality, period             */
    JLP_ACCEL_ON        = 0x01, /* Accel+RAM on; no flash                   */
    JLP_ACCEL_OFF       = 0x02, /* Accel+RAM off; jlp_flash controls flash  */
    JLP_ACCEL_FLASH_ON  = 0x03  /* Accel+RAM on;  jlp_flash controls flash  */
} jlp_accel_t;

#define REQ_JLP(j) ((int)(j) >= (int)JLP_ACCEL_ON)

/* ------------------------------------------------------------------------ */
/*  JLP Flash Parameters                                                    */
/* ------------------------------------------------------------------------ */
#define JLP_FLASH_MIN (1)
#define JLP_FLASH_MAX (682)

/* ======================================================================== */
/*  Each of these fields is NULL if absent.                                 */
/*                                                                          */
/*  Pointer-to-pointer fields represent NULL-terminated lists when the      */
/*  list pointer is itself not-NULL.                                        */
/*                                                                          */
/*  Pointer-to-integer fields represent zero-terminated lists when the list */
/*  pointer is itself not-NULL.                                             */
/*                                                                          */
/*  Pointer-to-date fields represent 0/0/0-terminated lists when the list   */
/*  pointer is itself not-NULL.                                             */
/* ======================================================================== */
typedef struct game_metadata_t
{
    const char* name;               /*  Full title of program               */
    const char* short_name;         /*  Abbreviated title of program        */
    const char** authors;           /*  Program authors (programmers)       */
    const char** game_artists;      /*  Game artists (in-game gfx)          */
    const char** composers;         /*  In-game music composers/arrangers   */
    const char** sfx_artists;       /*  Sound effects artists               */
    const char** voice_actors;      /*  Voice actors for samples            */
    const char** doc_writers;       /*  Documentation writers               */
    const char** conceptualizers;   /*  Game concept creators               */
    const char** box_artists;       /*  Box/manual/overlay/etc. artists.    */
    const char** more_infos;        /*  Pointers to more information.       */
    const char** publishers;        /*  Publishers of the title             */
    const game_date_t* release_dates;   /*  List of game release dates      */
    const char** licenses;          /*  License published under.            */
    const char** descriptions;      /*  Description of the game             */
    const char** misc;              /*  key=value pairs for un-handled vars */
    const game_date_t* build_dates; /* List of game build dates.            */
    const char** versions;          /* List of game version strings.        */

    compat_level_t      ecs_compat;
    compat_level_t      voice_compat;
    compat_level_t      intv2_compat;
    compat_level_t      kc_compat;  /* Keyboard component */
    compat_level_t      tv_compat;  /* TutorVision / INTV88 SuperPro */
    int                 lto_mapper;
    jlp_accel_t         jlp_accel;
    int                 jlp_flash;

    /* -------------------------------------------------------------------- */
    /*  is_defaults == 0 means user has explicitly specified at least 1     */
    /*  setting, whether or not the setting is equivalent to the default.   */
    /* -------------------------------------------------------------------- */
    int                 is_defaults; 
} game_metadata_t;

/* ------------------------------------------------------------------------ */
/*  GAME_METADATA_SET_COMPAT_TO_UNSPEC       -- Set all compat to unspec    */
/* ------------------------------------------------------------------------ */
void game_metadata_set_compat_to_unspec( game_metadata_t *const metadata );

/* ------------------------------------------------------------------------ */
/*  GAME_METADATA_SET_UNSPEC_COMPAT_TO_DEFAULTS                             */
/*  Returns 1 if all of the compat flags were unspecified, 0 otherwise.     */
/* ------------------------------------------------------------------------ */
int game_metadata_set_unspec_compat_to_defaults
(
    game_metadata_t *const metadata
);

/* ------------------------------------------------------------------------ */
/*  DEFAULT_METADATA                                                        */
/* ------------------------------------------------------------------------ */
game_metadata_t *default_game_metadata( void );

/* ------------------------------------------------------------------------ */
/*  METADATA_FREE                                                           */
/*  Free the metadata structure                                             */
/* ------------------------------------------------------------------------ */
void free_game_metadata( game_metadata_t *const metadata );

/* ------------------------------------------------------------------------ */
/*  MERGE_GAME_METADATA                                                     */
/*                                                                          */
/*  Given two game_metadata structures, merge them into one that has data   */
/*  from them both.                                                         */
/*                                                                          */
/*  For "name" and "short_name", first argument takes precedence when both  */
/*  are set.                                                                */
/*                                                                          */
/*  For string-arrays and date arrays, all dupes are eliminated.            */
/*                                                                          */
/*  For "compat" entries, we us the following hierarchy:                    */
/*                                                                          */
/*   -- If both have is_defaults = 1, first argument wins.                  */
/*   -- If exactly one has is_defaults = 0, its settings win.               */
/*   -- If both have is_defaults = 0, then things get fun.                  */
/*                                                                          */
/*                    |   UNS  INC  TOL  ENH  REQ  second                   */
/*            --------+-----------------------------------                  */
/*               UNS  |   UNS  INC  TOL  ENH  REQ                           */
/*               INC  |   INC  INC  INC  INC  uns                           */
/*               TOL  |   TOL  INC  TOL  ENH  REQ                           */
/*               ENH  |   ENH  INC  ENH  ENH  REQ                           */
/*               REQ  |   REQ  uns  REQ  REQ  REQ                           */
/*             first  |                                                     */
/*                                                                          */
/*   -- For jlp_accel:  The greater value takes precedence.                 */
/*   -- For jlp_flash:  The greater value takes precedence.                 */
/*   -- For lto_mapper:  The greater value takes precedence.                */
/*                                                                          */
/*  A new game_metadata structure gets created; neither of the source args  */
/*  is modified.                                                            */
/* ------------------------------------------------------------------------ */
game_metadata_t *merge_game_metadata
(
    const game_metadata_t *const src1,
    const game_metadata_t *const src2
);
    
/* ------------------------------------------------------------------------ */
/*  PRINT_METADATA   -- Print game metadata.                                */
/* ------------------------------------------------------------------------ */
void print_metadata
(
    const game_metadata_t *const meta
);

#ifdef __cplusplus
}
#endif

#endif

