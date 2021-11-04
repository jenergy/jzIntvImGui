/* ======================================================================== */
/*  ECS emulation, except for the second PSG.                               */
/* ======================================================================== */
#ifndef ECS_H_
#define ECS_H_

/* ======================================================================== */
/*  ECS UART state machine states.                                          */
/* ======================================================================== */
typedef enum ecs_uart_state_t
{
    ECS_UART_IDLE,                  /* UART is doing nothing currently.     */
    ECS_UART_CLOD,                  /* Reading from cassette.               */
    ECS_UART_CSAV,                  /* Saving to cassette.                  */
    ECS_UART_PRINT                  /* Printing to the printer.             */
} ecs_uart_state_t;

/* ======================================================================== */
/*  ECS_UART_T       -- ECS UART state tracking structure.                  */
/* ======================================================================== */
typedef struct ecs_uart_t
{
    periph_t    periph;             /* UART for tape/printer.               */
    uint8_t     cfg[2];             /* UART configuration registers.        */
    ecs_uart_state_t    state;      /* Current UART state.                  */
    const char *fn_tape;            /* File name template for CSAV/CLOD.    */
    const char *fn_tape_actual;     /* Expanded filename for CSAV/CLOD.     */
    const char *fn_printer;         /* File name template for print outs.   */
    FILE       *f_tape_i;           /* File for incoming tape data.         */
    FILE       *f_tape_o;           /* File for outgoing tape data.         */
    FILE       *f_printer_o;        /* File for outgoing printer data.      */
} ecs_uart_t;

/* ======================================================================== */
/*  ECS_T            -- The ECS peripheral, including UART (but not PSG).   */
/* ======================================================================== */
typedef struct ecs_t
{
    mem_t       ram;                /* 8-bit Scratchpad RAM at $4000-$47FF  */
    mem_t       rom[3];             /* ECS ROMs ($2000:1, $7000:0, $E000:1  */
    int         rom_present;        /* Flag: ECS ROM is present.            */
    ecs_uart_t  uart;               /* ECS UART.                            */
} ecs_t;

/* ======================================================================== */
/*  ECS_INIT         -- Initialize the ECS structure.                       */
/* ======================================================================== */
int ecs_init
(
    ecs_t        *const ecs,        /* ECS structure.                       */
    uint16_t     *const ecs_img,    /* ECS ROM image (if any).              */
    cp1600_t     *const cpu,        /* CPU pointer for cache management.    */
    int           const rand_mem,   /* Flag: Randomize memory.              */
    const char   *const fn_tape,    /* File name template for tape.         */
    const char   *const fn_printer  /* File name template for printer.      */
);

/* ======================================================================== */
/*  ECS_REGISTER     -- Register the ECS on the peripheral bus.             */
/* ======================================================================== */
void ecs_register
(
    ecs_t        *const ecs,        /* ECS structure.                       */
    periph_bus_t *const bus         /* Peripheral bus to register on.       */
);

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
/*                   Copyright (c) 2018, Joseph Zbiciak                     */
/* ======================================================================== */
