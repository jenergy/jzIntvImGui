/* ======================================================================== */
/*  ECS emulation, except for the second PSG.                               */
/* ======================================================================== */
#include "config.h"
#include "periph/periph.h"
#include "cp1600/cp1600.h"
#include "mem/mem.h"
#include "ecs/ecs.h"
#include "misc/jzprint.h"

/* ======================================================================== */
/*  ESC UART                                                                */
/*                                                                          */
/*      0xE0    Read:   Status      ? ? ? ? ? ? w r                         */
/*                                  w = 1: clear to send                    */
/*                                  r = 1: receive data is ready            */
/*                                  Bit 3 tested in printer code.  Why?     */
/*                                  Bit 3 expected to be 0.                 */
/*              Write:  Control 0                                           */
/*                      Written w/ 0x03 on entering menu.                   */
/*                      Written w/ 0x11 when executing an immediate stmt.   */
/*                      Written w/ 0x1D for 2nd "GO" phase of CSAV, CLOD.   */
/*                                                                          */
/*      0xE1    Read:   Receive data                                        */
/*              Write:  Transmit data                                       */
/*                                                                          */
/*      0xE2    Read:   Control 1 readback (?)  (ECS never reads it.)       */
/*              Write:  Control 1                                           */
/*                      Written w/ 0x1D for "SET" phase of CSAV, CLOD.      */
/*                      Written w/ 0x00 for 1st "GO" phase of CSAV, CLOD.   */
/*                      Written w/ 0x39 for 2nd "GO" phase of CSAV.         */
/*                      Written w/ 0x1D for 2nd "GO" phase of CLOD.         */
/*                      Written w/ 0x23 when executing an immediate stmt,   */
/*                                      incl. printing.                     */
/*                      Bit 4 of 0xE2 controls cassette relay (1 = on).     */
/*                                                                          */
/* ======================================================================== */

/* ======================================================================== */
/*  ECS_UART_CLOSE_FILES                                                    */
/* ======================================================================== */
LOCAL void ecs_uart_close_files( ecs_uart_t *const uart )
{
    if (uart->f_tape_i)
    {
        fclose(uart->f_tape_i);
        uart->f_tape_i = NULL;
        jzp_printf("\nECS: Closed tape input file '%s'.\n",
                    uart->fn_tape_actual);
        CONDFREE(uart->fn_tape_actual);
    }

    if (uart->f_tape_o)
    {
        fclose(uart->f_tape_o);
        uart->f_tape_o = NULL;
        jzp_printf("\nECS: Closed tape output file '%s'.\n",
                    uart->fn_tape_actual);
        CONDFREE(uart->fn_tape_actual);
    }

    if (uart->f_printer_o)
    {
        fclose(uart->f_printer_o);
        uart->f_printer_o = NULL;
        jzp_printf("\nECS: Closed printer output file '%s'.\n",
                    uart->fn_printer);
    }
}

/* ======================================================================== */
/*  ECS_UART_GENERATE_FNAME  -- Generate a CSAV/CLOD filename.              */
/* ======================================================================== */
enum csav_clod { CSAV, CLOD };
LOCAL const char *ecs_uart_generate_fname( const char *templ, mem_t *const ram,
                                           const enum csav_clod csav_clod )
{
    const char *const s = strchr(templ, '#');
    char prog_name[5] = { 0, 0, 0, 0, 0 };
    char *expanded = NULL;
    int alloc_len, prefix_len, prog_name_len, i;
    int tail_index;

    /* -------------------------------------------------------------------- */
    /*  If there's no # in the template, just dupe the string and move on.  */
    /* -------------------------------------------------------------------- */
    if (!s)
        return strdup(templ);

    /* -------------------------------------------------------------------- */
    /*  If there's a # in the template, look for the game name in ECS RAM.  */
    /*  This is up to 4 ASCII characters, stored at $4080 (CLOD) or         */
    /*  $40FA (CSAV).  Treat 0x20 as "end of name".                         */
    /* -------------------------------------------------------------------- */
    const uint32_t addr = csav_clod == CLOD ? 0x80 : 0xFA;
    for (i = 0; i < 4; i++)
    {
        const uint16_t v = ram->periph.peek(AS_PERIPH(ram), AS_PERIPH(ram),
                                            addr + i, 0xFFFF);
        const unsigned char c = v & 0xFF;

        if (v <= 0x20 || v > 0x7F)
            break;

        prog_name[i] = isalnum(c) ? c : '_';
    }
    prog_name[i] = 0;
    prog_name_len = strlen(prog_name);

    /* -------------------------------------------------------------------- */
    /*  Expand the template with whatever name we found (if any), with a    */
    /*  leading underscore.                                                 */
    /*                                                                      */
    /*  Keep in mind the # in the template accounts for the NUL in the      */
    /*  result, length-wise.                                                */
    /* -------------------------------------------------------------------- */
    alloc_len = strlen(templ) + prog_name_len + 1;
    expanded = (char *)malloc(alloc_len);

    if (!expanded)
        return NULL;

    /* -------------------------------------------------------------------- */
    /*  Copy over the prefix.                                               */
    /* -------------------------------------------------------------------- */
    prefix_len = s - templ;
    memcpy(expanded, templ, prefix_len);

    /* -------------------------------------------------------------------- */
    /*  Copy over the discovered program name, if any.                      */
    /* -------------------------------------------------------------------- */
    if (prog_name_len)
    {
        expanded[prefix_len] = '_';
        memcpy(expanded + prefix_len + 1, prog_name, prog_name_len);
        tail_index = prefix_len + prog_name_len + 1;
    } else
    {
        tail_index = prefix_len;
    }

    /* -------------------------------------------------------------------- */
    /*  Copy the remainder.                                                 */
    /* -------------------------------------------------------------------- */
    strcpy(expanded + tail_index, templ + prefix_len + 1);

    return expanded;
}

/* ======================================================================== */
/*  ECS_UART_START_CLOD      -- Start loading a file.                       */
/* ======================================================================== */
LOCAL void ecs_uart_start_clod( ecs_uart_t *const uart, mem_t *const ram )
{
    const char *fname = NULL;

    uart->state = ECS_UART_CLOD;
    if (!uart->fn_tape) return;

    fname = ecs_uart_generate_fname( uart->fn_tape, ram, CLOD );
    if (!fname) return;

    uart->fn_tape_actual = fname;
    uart->f_tape_i = fopen(fname, "rb");

    if (!uart->f_tape_i)
    {
        perror("fopen()");
        jzp_printf("\nECS: Could not open '%s' for reading for CLOD.\n",
                    fname);
        CONDFREE(uart->fn_tape_actual);
        return;
    }

    jzp_printf("ECS: CLOD reading from '%s'.\n", fname);
    return;
}

/* ======================================================================== */
/*  ECS_UART_START_CSAV      -- Start saving a file.                        */
/* ======================================================================== */
LOCAL void ecs_uart_start_csav( ecs_uart_t *const uart, mem_t *const ram )
{
    const char *fname = NULL;

    uart->state = ECS_UART_CSAV;
    if (!uart->fn_tape) return;

    fname = ecs_uart_generate_fname( uart->fn_tape, ram, CSAV );
    if (!fname) return;

    uart->fn_tape_actual = fname;
    uart->f_tape_o = fopen(fname, "wb");

    if (!uart->f_tape_o)
    {
        perror("fopen()");
        jzp_printf("ECS: Could not open '%s' for writing for CSAV.\n",
                    fname);
        CONDFREE(uart->fn_tape_actual);
        return;
    }

    jzp_printf("ECS: CSAV writing to '%s'.\n", fname);
    return;
}

/* ======================================================================== */
/*  ECS_UART_START_PRINTING  -- Start printing to the printer.              */
/* ======================================================================== */
LOCAL void ecs_uart_start_printing( ecs_uart_t *const uart )
{
    uart->state = ECS_UART_PRINT;

    if (!uart->f_printer_o && uart->fn_printer)
    {
        uart->f_printer_o = fopen(uart->fn_printer, "a");

        if (uart->f_printer_o)
        {
            jzp_printf("ECS: Appending printer output to '%s'.\n",
                        uart->fn_printer);
        } else
        {
            perror("fopen()");
            jzp_printf("ECS: Unable to open '%s' for append.\n",
                        uart->fn_printer);
        }
    }
    return;
}

/* ======================================================================== */
/*  ECS_UART_RESET   -- Reset the ECS UART.                                 */
/* ======================================================================== */
LOCAL void ecs_uart_reset(periph_t *const per )
{
    ecs_uart_t *const uart = PERIPH_AS(ecs_uart_t, per);

    ecs_uart_close_files(uart);

    uart->cfg[0] = 0x00;
    uart->cfg[1] = 0x00;
    uart->state = ECS_UART_IDLE;
}

/* ======================================================================== */
/*  ECS_UART_RD      -- Read from the ECS UART.                             */
/* ======================================================================== */
LOCAL uint32_t ecs_uart_rd( periph_t *const per, periph_t *const ign,
                            const uint32_t addr, const uint32_t data )
{
    ecs_uart_t *const uart = PERIPH_AS(ecs_uart_t, per);
    UNUSED(ign);
    UNUSED(data);

    /* -------------------------------------------------------------------- */
    /*  0xE0: Status byte (read-only).                                      */
    /* -------------------------------------------------------------------- */
    if (addr == 0)
    {
        /* ---------------------------------------------------------------- */
        /*  Don't attempt to emulate the real timing of the serial port     */
        /*  for now, and just always return 'ready' for TX and RX.          */
        /* ---------------------------------------------------------------- */
        return 3;
    }

    /* -------------------------------------------------------------------- */
    /*  0xE1: Receive byte.                                                 */
    /* -------------------------------------------------------------------- */
    if (addr == 1)
    {
        if (uart->state == ECS_UART_CLOD && uart->f_tape_i)
        {
            int c = fgetc(uart->f_tape_i);
            if (c == EOF)
            {
                ecs_uart_close_files(uart);
                c = 0;
            }
            return c & 0xFF;
        }
        return 0;
    }

    /* -------------------------------------------------------------------- */
    /*  0xE2: Config byte 1 (read/write).                                   */
    /* -------------------------------------------------------------------- */
    if (addr == 2) return uart->cfg[1];

    return 0xFFFF;
}

/* ======================================================================== */
/*  ECS_UART_WR      -- Write to the ECS UART.                              */
/* ======================================================================== */
LOCAL void ecs_uart_wr( periph_t *const per, periph_t *const ign,
                        const uint32_t addr, const uint32_t data )
{
    ecs_uart_t *const uart = PERIPH_AS(ecs_uart_t, per);
    ecs_t *const ecs = PERIPH_PARENT_AS(ecs_t, per);
    mem_t *const ram = &ecs->ram;
    const uint8_t data8 = data & 0xFF;
    UNUSED(ign);

    /* -------------------------------------------------------------------- */
    /*  0xE0: Config byte 0 (write-only).                                   */
    /* -------------------------------------------------------------------- */
    if (addr == 0)
    {
        /* ---------------------------------------------------------------- */
        /*  ECS EXEC writes 0x03 at "start of time."  Presumably a reset?   */
        /*  Close open files and transition to idle.                        */
        /* ---------------------------------------------------------------- */
        if (data8 == 0x03)
        {
            ecs_uart_close_files(uart);
            uart->state = ECS_UART_IDLE;
        }
        /* ---------------------------------------------------------------- */
        /*  ECS EXEC writes 0x11 when executing an immediate instruction.   */
        /*  It seems to be part of the init for printing mode, really.      */
        /* ---------------------------------------------------------------- */
        else if (data8 == 0x11)
        {
            /* ------------------------------------------------------------ */
            /*  If we were saving or loading, close open files and go idle. */
            /* ------------------------------------------------------------ */
            if (uart->state == ECS_UART_CSAV || uart->state == ECS_UART_CLOD)
            {
                ecs_uart_close_files(uart);
                uart->state = ECS_UART_IDLE;
            }

            /* ------------------------------------------------------------ */
            /*  If we're printing, merely flush files.                      */
            /* ------------------------------------------------------------ */
            if (uart->state == ECS_UART_PRINT && uart->f_printer_o)
                fflush(uart->f_printer_o);
        }
        /* ---------------------------------------------------------------- */
        /*  ECS EXEC writes 0x1D when starting a CSAV or CLOD.  It does     */
        /*  this after configuring the operation with a write to 0xE2.      */
        /* ---------------------------------------------------------------- */
        else if (data8 == 0x1D)
        {
            /* ------------------------------------------------------------ */
            /*  cfg[1] tells us whether we're doing CSAV or CLOD.           */
            /*  cfg[1] == 0x1D means CLOD.  cfg[1] == 0x39 means CSAV.      */
            /* ------------------------------------------------------------ */
            if (uart->cfg[1] == 0x1D || uart->cfg[1] == 0x39)
            {
                jzp_printf("\nECS: Starting %s.\n",
                            uart->cfg[1] == 0x1D ? "CLOD" : "CSAV");

                /* -------------------------------------------------------- */
                /*  Close any open files.                                   */
                /* -------------------------------------------------------- */
                ecs_uart_close_files(uart);

                /* -------------------------------------------------------- */
                /*  Warn if we were in some unexpected state.               */
                /* -------------------------------------------------------- */
                if (uart->state != ECS_UART_IDLE &&
                    uart->state == ECS_UART_PRINT)
                    jzp_printf("ECS: Warning, state was %d\n", uart->state);

                if (uart->cfg[1] == 0x1D) ecs_uart_start_clod(uart, ram);
                else                      ecs_uart_start_csav(uart, ram);
            } else
            {
                jzp_printf("\nECS: Unable to determine if CSAV/CLOD. "
                           "cfg[1] = %.2X\n", uart->cfg[1]);
                ecs_uart_close_files(uart);
                uart->state = ECS_UART_IDLE;
            }
        }
        /* ---------------------------------------------------------------- */
        /*  Otherwise, we don't know /what/ is going on.                    */
        /* ---------------------------------------------------------------- */
        else
        {
            jzp_printf("\nECS: Unexpected value %0.4X written to 0xE0\n", data);
            ecs_uart_close_files(uart);
            uart->state = ECS_UART_IDLE;
        }

        uart->cfg[0] = data8;
        return;
    }

    /* -------------------------------------------------------------------- */
    /*  0xE1: Transmit byte.                                                */
    /* -------------------------------------------------------------------- */
    if (addr == 1)
    {
        /* ---------------------------------------------------------------- */
        /*  If we're idle, but cfg[0,1] == { 0x11, 0x23 }, start printing.  */
        /* ---------------------------------------------------------------- */
        if (uart->state == ECS_UART_IDLE &&
            uart->cfg[0] == 0x11 && uart->cfg[1] == 0x23)
        {
            jzp_printf("\nECS: Printing.\n");
            ecs_uart_start_printing(uart);
        }

        /* ---------------------------------------------------------------- */
        /*  If we're printing or CSAV-ing, append character to the file.    */
        /* ---------------------------------------------------------------- */
        if (uart->state == ECS_UART_PRINT && uart->f_printer_o)
        {
            fputc(data8, uart->f_printer_o);
            if (data8 == 13)  /* Carriage return:  Add a line feed. */
                fputc(10, uart->f_printer_o);
        }
        if (uart->state == ECS_UART_CSAV && uart->f_tape_o)
            fputc(data8, uart->f_tape_o);

        return;
    }

    /* -------------------------------------------------------------------- */
    /*  0xE2: Config byte 1 (read/write).                                   */
    /* -------------------------------------------------------------------- */
    if (addr == 2)
    {
        /* ---------------------------------------------------------------- */
        /*  The ECS writes 0x00 before we start a CSAV or CLOD while we     */
        /*  are still idle ("GO" state in BASIC), and another 0x00 after    */
        /*  we're done.                                                     */
        /*                                                                  */
        /*  Whenever ECS BASIC executes an immediate statement, it also     */
        /*  writes 0x23 to this register.                                   */
        /* ---------------------------------------------------------------- */
        if (data8 == 0x00 || data8 == 0x23)
        {
            /* ------------------------------------------------------------ */
            /*  If we were saving or loading, close open files.             */
            /*  If we're printing, merely flush files.                      */
            /* ------------------------------------------------------------ */
            if (uart->state == ECS_UART_CSAV || uart->state == ECS_UART_CLOD)
                ecs_uart_close_files(uart);

            if (uart->state == ECS_UART_PRINT && uart->f_printer_o)
                fflush(uart->f_printer_o);
            return;
        }
        /* ---------------------------------------------------------------- */
        /*  ECS EXEC will write either 0x1D or 0x39 in prep for a CLOD or   */
        /*  CSAV.  Don't actually start CLOD or CSAV yet.  Just silently    */
        /*  consume the write.  If we were printing, end it.                */
        /* ---------------------------------------------------------------- */
        else if (data8 == 0x1D || data8 == 0x39)
        {
            if (uart->state != ECS_UART_IDLE)
                ecs_uart_close_files(uart);

            uart->state = ECS_UART_IDLE;
        }
        /* ---------------------------------------------------------------- */
        /*  Complain if we see a write we don't expect.                     */
        /* ---------------------------------------------------------------- */
        else
        {
            jzp_printf("\nECS: Unexpected value %0.4X written to 0xE2\n", data);
            ecs_uart_close_files(uart);
            uart->state = ECS_UART_IDLE;
        }

        uart->cfg[1] = data8;
        return;
    }
}

/* ======================================================================== */
/*  ECS_UART_DTOR    -- Destruct the ECS UART.                              */
/* ======================================================================== */
LOCAL void ecs_uart_dtor( periph_t *const per )
{
    ecs_uart_t *const uart = PERIPH_AS(ecs_uart_t, per);

    ecs_uart_reset(per);

    CONDFREE(uart->fn_tape);
    CONDFREE(uart->fn_printer);
}

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
)
{

    /* -------------------------------------------------------------------- */
    /*  Initialize the UART peripherpal.                                    */
    /* -------------------------------------------------------------------- */
    ecs->uart.periph.read       = ecs_uart_rd;
    ecs->uart.periph.write      = ecs_uart_wr;
    ecs->uart.periph.peek       = ecs_uart_rd;
    ecs->uart.periph.poke       = ecs_uart_wr;
    ecs->uart.periph.ser_init   = NULL;
    ecs->uart.periph.reset      = ecs_uart_reset;
    ecs->uart.periph.dtor       = ecs_uart_dtor;
    ecs->uart.periph.parent     = AS_PERIPH(ecs);

    ecs->uart.periph.tick       = NULL;
    ecs->uart.periph.min_tick   = ~0U;
    ecs->uart.periph.max_tick   = ~0U;
    ecs->uart.periph.addr_base  = 0xE0;
    ecs->uart.periph.addr_mask  = 3;

    ecs->uart.state       = ECS_UART_IDLE;
    ecs->uart.cfg[0]      = 0x00;
    ecs->uart.cfg[1]      = 0x00;
    ecs->uart.f_tape_i    = NULL;
    ecs->uart.f_tape_o    = NULL;
    ecs->uart.f_printer_o = NULL;

    ecs->uart.fn_tape     = fn_tape    ? strdup(fn_tape)    : NULL;
    ecs->uart.fn_printer  = fn_printer ? strdup(fn_printer) : NULL;

    /* -------------------------------------------------------------------- */
    /*  Create the RAM and ROM.                                             */
    /* -------------------------------------------------------------------- */
    if (mem_make_ram(&ecs->ram, 8, 0x4000, 11, rand_mem))
    {
        fprintf(stderr, "ERROR: Could not initialize ECS RAM.\n");
        return -1;
    }

    if (!ecs_img)
    {
        jzp_printf("\nECS: Instantiating ECS without a ROM.\n\n");
        ecs->rom_present = 0;
        return 0;
    }

    if (mem_make_prom(&ecs->rom[0], 16, 0x2000, 12, 1, ecs_img + 0x0000, cpu) ||
        mem_make_prom(&ecs->rom[1], 16, 0x7000, 12, 0, ecs_img + 0x1000, cpu) ||
        mem_make_prom(&ecs->rom[2], 16, 0xE000, 12, 1, ecs_img + 0x2000, cpu))
    {
        fprintf(stderr, "ERROR: Could not make paged ROM from ECS image.\n");
        return -1;
    }

    ecs->rom_present = 1;
    return 0;
}

/* ======================================================================== */
/*  ECS_REGISTER     -- Register the ECS on the peripheral bus.             */
/* ======================================================================== */
void ecs_register
(
    ecs_t        *const ecs,
    periph_bus_t *const bus
)
{
    periph_t *const ram_p  = AS_PERIPH(&ecs->ram);
    periph_t *const rom0_p = AS_PERIPH(&ecs->rom[0]);
    periph_t *const rom1_p = AS_PERIPH(&ecs->rom[1]);
    periph_t *const rom2_p = AS_PERIPH(&ecs->rom[2]);
    periph_t *const uart   = AS_PERIPH(&ecs->uart);

    periph_register(bus, ram_p,  0x4000, 0x47FF, "ECS RAM");
    if (ecs->rom_present)
    {
        periph_register(bus, rom0_p, 0x2000, 0x2FFF, "ECS ROM (2xxx)");
        periph_register(bus, rom1_p, 0x7000, 0x7FFF, "ECS ROM (7xxx)");
        periph_register(bus, rom2_p, 0xE000, 0xEFFF, "ECS ROM (Exxx)");
        periph_register(bus, uart,   0x00E0, 0x00E3, "ECS UART"      );
    }
}

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
