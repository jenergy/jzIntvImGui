/*
 * ============================================================================
 *  Title:    Joystick Support via SDL
 *  Author:   J. Zbiciak
 * ============================================================================
 */

#ifndef JOY_H_
#define JOY_H_

typedef struct joy_t joy_t;

#define MAX_AXES   (20)
#define MAX_STICKS (10)
#define MAX_HATS   (10)
#define MAX_JOY    (10)

#define DUMMY_AXIS (999)
#define IS_DUMMY_AXIS(x) ((x) == DUMMY_AXIS)

/* ------------------------------------------------------------------------ */
/*  Stick configuration and state on a Joy.                                 */
/* ------------------------------------------------------------------------ */
struct joy_stick_t
{
    int     x_axis, y_axis; /* X, Y axis bindings                           */
    int     push_thresh;    /* not-pushed to pushed threshold               */
    int     rels_thresh;    /* pushed to not-pushed threshold               */
    uint8_t autocenter;     /* Flag:  Autocentering enabled?                */
    int8_t  dir_type;       /* Flag: 1=16-dir, 2=8-dir, 4=4-dir, -4=4-diag  */
    int     disc_dir;       /* Last disc direction reported for stick       */
};

/* ------------------------------------------------------------------------ */
/*  Axis configuration on a Joy.                                            */
/* ------------------------------------------------------------------------ */
struct joy_axis_t
{
    int stick;                          /* Axis to stick binding            */
    int max, min, ctr, pos, inv, prv;   /* Tracking parameters              */
    double last;
};

/* ======================================================================== */
/*  Joystick Information Structure.                                         */
/* ======================================================================== */
struct joy_t
{
    /* -------------------------------------------------------------------- */
    /*  Information from SDL's APIs directly.                               */
    /* -------------------------------------------------------------------- */
    char    *name;          /* Name of the joystick, if available.          */
    int     num_axes;       /* Number of analog axes.                       */
    int     num_balls;      /* Number of trackballs                         */
    int     num_hats;       /* Number of hats                               */
    int     num_buttons;    /* Number of buttons                            */
    void    *ptr;           /* Pointer to SDL joystick structure.           */

    /* -------------------------------------------------------------------- */
    /*  Configure each Stick on a Joy.                                      */
    /* -------------------------------------------------------------------- */
    struct joy_stick_t stick[MAX_STICKS];

    /* -------------------------------------------------------------------- */
    /*  Current X/Y axis state, and ranges                                  */
    /* -------------------------------------------------------------------- */
    struct joy_axis_t axis[MAX_AXES + 1];

    int hat_dir[MAX_HATS];
};


int  joy_init(int, char *cfg[MAX_JOY][MAX_STICKS]);
void joy_dtor(void);

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
/*                 Copyright (c) 2005-2020, Joseph Zbiciak                  */
/* ======================================================================== */
