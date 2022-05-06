/*
 * ============================================================================
 *  Title:    MAIN
 *  Author:   J. Zbiciak
 * ============================================================================
 *  Main Simulator Driver File
 * ============================================================================
 *  This doesn't do much yet.  :-)
 * ============================================================================
 */

#include "config.h"

#include <signal.h>
#include "plat/plat.h"
#include "lzoe/lzoe.h"
#include "file/file.h"
#include "periph/periph.h"
#include "cp1600/cp1600.h"
#include "mem/mem.h"
#include "ecs/ecs.h"
#include "icart/icart.h"
#include "bincfg/bincfg.h"
#include "bincfg/legacy.h"
#include "pads/pads.h"
#include "pads/pads_cgc.h"
#include "pads/pads_intv2pc.h"
#include "avi/avi.h"
#include "gfx/gfx.h"
#include "gfx/palette.h"
#include "snd/snd.h"
#include "ay8910/ay8910.h"
#include "demo/demo.h"
#include "stic/stic.h"
#include "speed/speed.h"
#include "debug/debug_.h"
#include "debug/debug_if.h"
#include "event/event.h"
#include "ivoice/ivoice.h"
#include "jlp/jlp.h"
#include "locutus/locutus_adapt.h"
#include "cheat/cheat.h"
#include "cfg/mapping.h"
#include "cfg/cfg.h"

cfg_t intv;

double elapsed(const bool);
void save_state(int);
void load_dump(void);
static void fake_osd(uint8_t*, uint32_t);

/*
 * ============================================================================
 *  RELEASE      -- Program name / release
 * ============================================================================
 */
static char * release(void)
{
    static char buf[30];

    snprintf(buf, 30, "%s", "jzintv-20200712\0");

    return buf;
}

#if 1
/*
 * ============================================================================
 *  CART_NAME    -- Look for a game name in a cartridge image.
 * ============================================================================
 */
static const char * cart_name(void)
{
    static char *name_buf = NULL;
    static size_t name_buf_alloc = 0;
    const size_t max_header_name_len = 64;
    uint16_t title_addr, lo, hi, ch;
    int year = 0;
    int ps;
    size_t i = 0;
    const char *base_name;
    char *s1, *s2;

    if (name_buf)
        name_buf[0] = 0;

    if ((base_name = intv.cart_name) != NULL)
    {
        i = 0;
        year = intv.cart_year;
        goto got_name;
    }

    if ((base_name = strrchr(intv.fn_game, '/')) == NULL &&
        (base_name = strrchr(intv.fn_game, '\\')) == NULL)
        base_name = intv.fn_game;
    else
        base_name++;

    periph_t *const intv_periph = AS_PERIPH(intv.intv);

    lo = periph_peek(intv_periph, intv_periph, 0x500A, ~0U);
    hi = periph_peek(intv_periph, intv_periph, 0x500B, ~0U);

    if ((lo | hi) & 0xFF00)
        goto got_name;

    title_addr = ((hi << 8) | lo);

    year = 1900 + (int)periph_peek(intv_periph, intv_periph, title_addr, ~0U);

    if (year < 1977 || year > 2050)
        goto got_name;

    for (i = 0; i != max_header_name_len; i++)
    {
        ch = periph_peek(intv_periph, intv_periph,
                         title_addr + (uint32_t)i + 1, ~0U);


        /* Add 9 for room for date and NUL terminator */
        if (i + 9 >= name_buf_alloc)
        {
            name_buf_alloc = 2*name_buf_alloc > 256 ? 2*name_buf_alloc : 256;
            name_buf = REALLOC(name_buf, char, name_buf_alloc);
            if (!name_buf)
                return NULL;
        }

        name_buf[i] = ch;

        if (ch == 0)
            break;

        if (ch < 32 || ch > 126)
        {
            name_buf[i] = 0;
            goto got_name;
        }
    }
    name_buf[i] = 0;

    ps = 1;
    i  = 0;
    for (s1 = s2 = name_buf; *s1; s1++)
        if (!isspace(*s1) || !ps)
        {
            *s2++ = *s1;
            ps = isspace(*s1);
            if (!ps)
                i = s2 - name_buf;
        }
    name_buf[i] = 0;

got_name:
    if (i == 0)
    {
        const size_t base_len = strlen(base_name);

        if (base_len + 9 >= name_buf_alloc)
        {
            name_buf_alloc =
                2*name_buf_alloc > base_len + 9u ? 2*name_buf_alloc 
                                                 : base_len + 9u;

            name_buf = REALLOC(name_buf, char, name_buf_alloc);
            if (!name_buf)
                return NULL;
        }

        strcpy(name_buf, base_name);
    }

    strip_bad_utf8(name_buf, name_buf);
    i = strlen(name_buf);

    if (year >= 1977 && year <= 2050)
        snprintf(name_buf + i, 8, " (%4d)", year);

    return name_buf;
}
#endif

/*
 * ============================================================================
 *  GRACEFUL_DEATH -- Die gracefully.
 * ============================================================================
 */
static void graceful_death(int x)
{
    if (intv.debugging && intv.debug.step_count)
    {
        fprintf(stderr, "Requesting debugger halt.\n");
        intv.debug.step_count = 0;
        return;
    } else if (intv.do_exit < 2)
    {
        if (intv.do_exit) fprintf(stderr, "\nOUCH!\n");
        fprintf(stderr,
                "\nRequesting exit:  Received signal %d.\n"
                "(You may need to press enter if you're at a prompt.)\n", x);
    } else
    {
        fprintf(stderr, "\nReceived 3 signals:  Aborting on signal %d.\n", x);
        exit(1);
    }
    intv.do_exit++;
}

/*
 * ============================================================================
 *  ELAPSED      -- Returns amount of time that's elapsed since the program
 *                  started, in CP1610 clock cycles (895kHz)
 * ============================================================================
 */
double elapsed(const bool restart)
{
    static double start;
    static int init = 0;
    double now;

    if (!init || restart)
    {
        start = get_time();
        init = 1;
    }

    now = get_time();

    return (now - start) * (intv.pal_mode ? 1000000. : 894886.25);
}

/*
 * ============================================================================
 *  DO_GUI_MODE  -- Implement the simple GUI mode remote controls on stdin.
 * ============================================================================
 */

static void do_gui_mode(void)
{
    char cmd;

    while (read(STDIN_FILENO, &cmd, 1) == 1)
    {
        switch (cmd)
        {
            case '\r' : case '\n' : goto na;        /* ignore CR, LF */
            case 'p'  : intv.do_pause = PAUSE_TOG;          break;
            case 'q'  : intv.do_exit  = 1;                  break;
            case 'r'  : intv.do_reset = 2;                  break;
            default   : putchar('!');  goto bad;
        }

        putchar('.');
bad:
        fflush(stdout);
na:     ;
    }
}


/*
 * ============================================================================
 *  In the beginning, there was a main....
 * ============================================================================
 */
extern int jlp_accel_on;
extern int lto_isa_enabled;
int jzintv_entry_point(int argc, char *argv[])
{
    jlp_accel_on=0;
    lto_isa_enabled=0;
    int iter = 0, arg;
    double cycles = 0, rate, irate, then, now, icyc;
    double disp_time = get_time(), reset_time = disp_time, curr_time = disp_time;
    double pause_until = disp_time;
    bool pause_key = false, was_paused = false;
    bool first = true;
    uint32_t s_cnt = 0;
    char title[128];

    /* -------------------------------------------------------------------- */
    /*  On Windows, let's try to get back our stdio.                        */
    /* -------------------------------------------------------------------- */
#if 0
#ifdef WIN32
    {
        FILE *newfp;
        newfp   = fopen("CON:", "w");
        if (newfp) *stdout = *newfp;
        newfp   = fopen("CON:", "w");
        if (newfp) *stderr = *newfp;
        newfp   = fopen("CON:", "r");
        if (newfp) *stdin  = *newfp;
    }
#endif
#endif

#ifndef macintosh
    /* -------------------------------------------------------------------- */
    /*  Sneak real quick and see if the user included -h, --help, -?, or    */
    /*  no flags whatsoever.  In those cases, print a message and leave.    */
    /* -------------------------------------------------------------------- */
    if (argc < 2)
    {
        license();
    }

    for (iter = 1; iter < argc; iter++)
    {
        if (!strcmp(argv[iter], "--help") ||
            !strcmp(argv[iter], "-h")     ||
            !strcmp(argv[iter], "-?"))
        {
            usage();
            return -10;
        }
    }
#endif

    /* -------------------------------------------------------------------- */
    /*  Platform-specific initialization.                                   */
    /* -------------------------------------------------------------------- */
    if (plat_init())
    {
        fprintf(stderr, "Error initializing.\n");
        return -10;;
    }

    /* -------------------------------------------------------------------- */
    /*  Temporary hack so ^C, kill work.                                    */
    /* -------------------------------------------------------------------- */
    signal( 2, graceful_death);
    signal(15, graceful_death);

    /* -------------------------------------------------------------------- */
    /*  Parse our arguments and go get an Intellivision!                    */
    /* -------------------------------------------------------------------- */
#ifdef macintosh
    argc = ccommand( &argv );
#endif
#ifdef USE_AS_BACKEND
	optind=0;
#endif
reload:
    if (-10 == cfg_init(&intv, argc, argv)) {
        return -10;
    }
    init_disp_width(0);
    jzp_flush();

    /* -------------------------------------------------------------------- */
    /*  On Windows, open \\.\UserPort\ if the program has requested         */
    /*  INTV2PC access.  This causes the UserPort driver to yield port      */
    /*  access, if UserPort is active.                                      */
    /* -------------------------------------------------------------------- */
#ifdef WIN32
    if (intv.i2pc0_port || intv.i2pc1_port)
    {
        FILE *UserPortFP;

        UserPortFP = fopen("\\\\.\\UserPort\\", "r");

        if (!UserPortFP)
        {
#if 0
            perror("fopen()");
            fprintf
            (
            stderr,
            "Warning: Unable to open \\\\.\\UserPort\\.\n"
            "         Under WinNT/2K/XP, jzIntv may be unable to use INTV2PC.\n"
            "         Win95/98/ME users can ignore this warning.\n"
            );
#endif
        }
    }
#endif


    /* -------------------------------------------------------------------- */
    /*  Set the window title.  If we recognize a standard Intellivision     */
    /*  ROM header at 0x5000, then also include the cartridge name.         */
    /* -------------------------------------------------------------------- */
    #if 1
    {
        const char *const r = release();
        const char *const n = cart_name();
        /*
        fprintf(stderr, "r = \'%s\', len = %d\n", r, (int)strlen(r));
        fprintf(stderr, "n = \'%s\', len = %d\n", n, (int)strlen(n));
        */
        snprintf(title, 128, "jzintv mod from official %.32s : %.64s", r, n);
        title[127] = 0;
        /*
        fprintf(stderr, "t = \'%s\', len = %d\n", title, (int)strlen(title));
        */
        gfx_set_title(&intv.gfx,title);
    }
    #endif

    /* -------------------------------------------------------------------- */
    /*  Run the simulator.                                                  */
    /* -------------------------------------------------------------------- */
    jzp_printf("Starting jzIntv...\n");
    jzp_flush();

restart:

    now = elapsed(/* restart = */ true);
    while (now == elapsed(/* restart = */ false))    /* Burn some time. */
        ;

    icyc   = 0;
    s_cnt  = 0;
    cycles = 0;
    if (intv.rate_ctl > 0.0)
        speed_resync(&(intv.speed));

    if (!intv.debugging)
        intv.debug.step_count = -1;

    pause_key   = false;
    was_paused  = false;
    curr_time   = get_time();
    disp_time   = curr_time;
    pause_until = curr_time;

    if (first && intv.start_dly > 0)
        pause_until += intv.start_dly / 1000.;

    first = false;

    while (intv.do_exit == 0 && intv.do_reload == 0)
    {
        uint32_t max_step;
        uint32_t do_reset = intv.do_reset;

        if (intv.gui_mode)
            do_gui_mode();

        if (intv.do_dump)
        {
			jzp_printf("\nDump requested.\n");
            intv.do_dump = 0;
			save_state(0);
		}

        if (intv.do_load)
        {
			jzp_printf("\nLoad requested.\n");
            intv.do_load = 0;
			load_dump();
		}

        if (do_reset)
        {
            if (intv.do_reset == 2)
                intv.do_reset = 0;
            max_step = 1000; /* arbitrary */
            intv.gfx.scrshot |= GFX_RESET;
            gfx_vid_enable(&(intv.gfx), 0);
            if (s_cnt == 40)
                gfx_set_bord(&(intv.gfx), 0);
        } else
        {
            if (s_cnt > 140)
            {
                break;
            } else if (s_cnt)
            {
                s_cnt = 0;
                periph_reset(intv.intv);
            }
            /* This is incredibly hackish, and is an outgrowth of my
             * decoupled tick architecture.  */
            if (intv.cp1600.req_q.horizon > intv.cp1600.periph.now)
                max_step = 
                    (uint32_t)
                        (intv.cp1600.req_q.horizon - intv.cp1600.periph.now);
            else
                max_step = 5;
            if (max_step > 20000) max_step = 20000;
            if (max_step < 5) max_step = 5;
        }

#if 0
jzp_printf("cpu.now = %-8d  stic.now = %-8d diff = %-8d step = %-8d\n", (int)intv.cp1600.periph.now, (int)intv.stic.stic_cr.now, (int)intv.cp1600.periph.now-(int)intv.stic.stic_cr.now, (int)max_step);
#endif
        if (intv.gfx.req_pause && intv.do_pause == PAUSE_NOP)
        {
            intv.do_pause = PAUSE_2SEC;
            intv.gfx.req_pause = false;
        }

        if (intv.do_pause != PAUSE_NOP)
        {
            if (intv.do_pause == PAUSE_2SEC)
            {
                pause_until = curr_time + 2.0;
            } else
            {
                pause_key = intv.do_pause == PAUSE_TOG ? !pause_key
                          : intv.do_pause == PAUSE_OFF ? false : true;
            }

            intv.do_pause = PAUSE_NOP;
        }

        bool paused = pause_key || pause_until > curr_time;

        if (was_paused && !paused)
            speed_resync(&(intv.speed));

        was_paused = paused;

        if (intv.chg_evt_map)
        {
            event_change_active_map(&intv.event,
                                    (ev_map_change_req)intv.chg_evt_map);
            pad_reset_inputs(&intv.pad0);
            pad_reset_inputs(&intv.pad1);
            intv.chg_evt_map = 0;
        }

        if (paused || do_reset)
        {
            intv.gfx.dirty = 1;
            gfx_refresh(&intv.gfx);
            snd_play_silence(&(intv.snd));
            intv.event.periph.tick(AS_PERIPH(&intv.event), 0);
            plat_delay(1000/240);
        } else
        {
            intv.gfx.scrshot &= ~GFX_RESET;
            cycles += periph_tick(AS_PERIPH(intv.intv), max_step);
        }

        if (!intv.debugging && intv.debug.step_count == 0)
            intv.debug.step_count = -1;

        curr_time = get_time();

        if (!intv.debugging && !do_reset && (curr_time > disp_time + 1.0))
        {
            disp_time = curr_time;
            then  = now;
            now   = elapsed(0);
            rate  = (cycles / now);
            if (now - then > 0.01)
            {
                irate = (cycles - icyc) / (now - then);
                icyc  = cycles;

                jzp_printf("Rate: [%6.2f%% %6.2f%%]  Drop Gfx:[%6.2f%% %6d] "
                       "Snd:[%6.2f%% %2d %6.3f]\r",
                        rate * 100., irate * 100.,
                        100. * intv.gfx.tot_dropped_frames / intv.gfx.tot_frames,
                        (int)intv.gfx.tot_dropped_frames,
                        100. * intv.snd.mixbuf.tot_drop / intv.snd.tot_frame,
                        (int)intv.snd.mixbuf.tot_drop,
                        (double)intv.snd.tot_dirty / intv.snd.tot_frame);

#if 0
                jzp_printf("speed: min=%-8d max=%-8d thresh=%-8.1f frame=%-8d\n",
                        intv.speed.periph.min_tick,
                        intv.speed.periph.max_tick,
                        intv.speed.threshold * 1e6,
                        intv.gfx.tot_frames);
#endif
                jzp_flush();
            }

            if (curr_time > (reset_time + 60.0))
            {
                reset_time = curr_time;
                then = elapsed(1);
                cycles = icyc = 0;
            }
        }

        if (do_reset)
        {
            intv.cp1600.r[7] = 0x1000;
            gfx_vid_enable(&(intv.gfx), 0);
            s_cnt++;
            debug_fault_detected = DEBUG_NO_FAULT;
        }

        if (debug_fault_detected && !intv.debugging && plat_is_batch_mode())
            intv.do_exit = -1;
    }

    uint64_t seed = 0x2A3A4A5A;

    arg = 0;
    gfx_set_bord  (&(intv.gfx), 0);
    gfx_vid_enable(&(intv.gfx), 1);
    intv.gfx.scrshot |= GFX_RESET;
    iter = 0;

    while (intv.do_exit == 0 && intv.do_reload == 0)
    {
        int i, j;
        uint8_t p;

        if (intv.gui_mode)
            do_gui_mode();

        if (intv.do_reset) arg = 1;
        if ((intv.do_reset == 0 && arg) || intv.do_reset == 2)
        {
            intv.do_reset = 0;

            p = intv.stic.raw[0x2C] & 15;
            for (i = 0; i < 160 * 200; i++)
                intv.stic.disp[i] = p;

            gfx_set_bord(&(intv.gfx), p);
            intv.gfx.scrshot &= ~GFX_RESET;
            periph_reset(intv.intv);
            goto restart;
        }

        int fade = iter < 60 ? (60 - iter) / 4 : 0;
        for (i = 0; i < 160 * 200; i++)
        {
            for (j = 0; j < 4; j++)
                seed = (seed >> 1) ^ (seed & 1 ? 0xD800000000000000u : 0);

            p = (seed & 0xF) + 17 + fade;
            if (p > 31) p = 31;

            intv.stic.disp[i] = p;
        }
        if (iter < 100)
        {
            snd_play_static(&intv.snd);
            iter++;
        } else
        {
            snd_play_silence(&(intv.snd));
            fake_osd(intv.stic.disp, (uint32_t)seed);
        }

        intv.gfx.dirty = 1;
        gfx_refresh(&intv.gfx);
        intv.event.periph.tick(AS_PERIPH(&intv.event), 0);
        plat_delay(1000/60);
    }

    if (intv.do_exit)
        jzp_printf(
            intv.do_exit > 0 ? "\nExited on user request.\n"
                             : "\nExited because game crashed.\n");

    if (intv.do_reload)
    {
        intv.do_reload = 0;
        jzp_printf("\nAttempting reload.\n");
        cfg_dtor(&intv);
        goto reload;
    }

    cfg_dtor(&intv);
    return intv.do_exit > 0 ? 0 : 1;
}


#include "cp1600/op_decode.h"
#include "cp1600/op_exec.h"

/*
 * ============================================================================
 *  DUMP_STATE   -- Called by op_exec:fn_HLT.  Dumps state of universe.
 * ============================================================================
 */

void dump_state(void)
{
    FILE *f;
    uint32_t i, j, addr, data;

    f = fopen("dump.mem","wb");
    if (!f)
    {
        perror("fopen(\"dump.mem\", \"w\")");
        jzp_printf("couldn't open dump.mem, not dumping memory.\n");
    }
    else
    {
        intv.debug.show_rd = 0;
        intv.debug.show_wr = 0;
        for (addr = 0; addr <= 0xFFFF; addr++)
        {
            data = periph_peek(AS_PERIPH(intv.intv), AS_PERIPH(intv.intv),
                               addr, 0);
            fputc((data >> 8) & 0xFF, f);
            fputc((data     ) & 0xFF, f);
        }

        fclose(f);
    }

    f = fopen("dump.cpu", "wb");
    if (!f)
    {
        perror("fopen(\"dump.cpu\", \"w\")");
        jzp_printf("couldn't open dump.cpu, not dumping cpu info.\n"); return;
    }

    fprintf(f, "CP-1600 State Dump\n");
    fprintf(f, "Tot Cycles:   %" U64_FMT "\n", intv.cp1600.tot_cycle);
    fprintf(f, "Tot Instrs:   %" U64_FMT "\n", intv.cp1600.tot_instr);
    fprintf(f, "Tot Cache:    %" U64_FMT "\n", intv.cp1600.tot_cache);
    fprintf(f, "Tot NonCache: %" U64_FMT "\n", intv.cp1600.tot_noncache);
    fprintf(f, "Registers:    %.4x %.4x %.4x %.4x %.4x %.4x %.4x %.4x\n",
            intv.cp1600.r[0], intv.cp1600.r[1],
            intv.cp1600.r[2], intv.cp1600.r[3],
            intv.cp1600.r[4], intv.cp1600.r[5],
            intv.cp1600.r[6], intv.cp1600.r[7]);
    fprintf(f, "Flags:        S:%d C:%d O:%d Z:%d I:%d D:%d intr:%d irq:%d\n",
            intv.cp1600.S, intv.cp1600.C, intv.cp1600.O, intv.cp1600.Z,
            intv.cp1600.I, intv.cp1600.D,
            intv.cp1600.intr, intv.cp1600.req_ack_state);

    fprintf(f, "Cacheability Map:\n");

    for (i = 0; i < 1 << (CP1600_MEMSIZE-CP1600_DECODE_PAGE - 5); i++)
    {
        addr = (i << (CP1600_DECODE_PAGE + 5));

        fprintf(f, "   %.4x-%.4x:", addr, addr+(32<<CP1600_DECODE_PAGE)-1);
        for (j = 0; j < 32; j++)
        {
            fprintf(f, " %d", 1 & (intv.cp1600.cacheable[i] >> j));
        }
        fprintf(f, "\n");
    }


    fprintf(f,"Decoded Instruction Map:\n");

    for (i = 0; i < 1 << (CP1600_MEMSIZE - 6); i++)
    {
        addr = i << 6;

        fprintf(f, "   %.4x-%.4x:", addr, addr + 63);
        for (j = 0; j < 64; j++)
        {
            fprintf(f, "%c",
                    intv.cp1600.execute[addr + j] == fn_decode_1st ? '-' :
                    intv.cp1600.execute[addr + j] == fn_decode     ? 'N' :
                    intv.cp1600.execute[addr + j] == fn_invalid    ? '!' :
                                                                     'C');
        }
        fprintf(f, "\n");
    }

    fclose(f);
}

/*
 * ============================================================================
 *  SAVE_STATE
 * ============================================================================
 */

void save_state(int altname)
{
#if 1
    UNUSED(altname);
    jzp_printf("Loading and saving dumps currently disabled.\n");
#else
    FILE *f;
    int addr, i, k;
	unsigned short data;

	if (altname) {
		f = fopen("dump_check.sav","wb");
	} else {
		f = fopen("dump.sav","wb");
	}
    if (!f)
    {
        perror("fopen(\"dump.sav\", \"w\")");
        jzp_printf("couldn't open dump.sav.\n");

		return;
    }

    char phase[7];
    memcpy(phase, "phase_1", sizeof(phase));
	fwrite(phase, sizeof(phase), 1, f);

    fwrite(&intv.cp1600.periph.addr_base, sizeof(intv.cp1600.periph.addr_base), 1, f);
    fwrite(&intv.cp1600.periph.addr_mask, sizeof(intv.cp1600.periph.addr_mask), 1, f);
    fwrite(&intv.cp1600.periph.now, sizeof(intv.cp1600.periph.now), 1, f);
    fwrite(&intv.cp1600.periph.min_tick, sizeof(intv.cp1600.periph.min_tick), 1, f);
    fwrite(&intv.cp1600.periph.max_tick, sizeof(intv.cp1600.periph.max_tick), 1, f);
    fwrite(&intv.cp1600.periph.next_tick, sizeof(intv.cp1600.periph.next_tick), 1, f);

    fwrite(&intv.cp1600.snoop.addr_base, sizeof(intv.cp1600.snoop.addr_base), 1, f);
    fwrite(&intv.cp1600.snoop.addr_mask, sizeof(intv.cp1600.snoop.addr_mask), 1, f);
    fwrite(&intv.cp1600.snoop.now, sizeof(intv.cp1600.snoop.now), 1, f);
    fwrite(&intv.cp1600.snoop.min_tick, sizeof(intv.cp1600.snoop.min_tick), 1, f);
    fwrite(&intv.cp1600.snoop.max_tick, sizeof(intv.cp1600.snoop.max_tick), 1, f);
    fwrite(&intv.cp1600.snoop.next_tick, sizeof(intv.cp1600.snoop.next_tick), 1, f);

    fwrite(&intv.cp1600.tot_cycle, sizeof(intv.cp1600.tot_cycle), 1, f);
    fwrite(&intv.cp1600.tot_instr, sizeof(intv.cp1600.tot_instr), 1, f);
    fwrite(&intv.cp1600.tot_cache, sizeof(intv.cp1600.tot_cache), 1, f);
    fwrite(&intv.cp1600.tot_noncache, sizeof(intv.cp1600.tot_noncache), 1, f);

    for (i = 0; i <= 7; i++)
    {
        fwrite(&intv.cp1600.r[i], sizeof(intv.cp1600.r[i]), 1, f);
    }

    fwrite(&intv.cp1600.S, sizeof(intv.cp1600.S), 1, f);
    fwrite(&intv.cp1600.C, sizeof(intv.cp1600.C), 1, f);
    fwrite(&intv.cp1600.O, sizeof(intv.cp1600.O), 1, f);
    fwrite(&intv.cp1600.Z, sizeof(intv.cp1600.Z), 1, f);
    fwrite(&intv.cp1600.I, sizeof(intv.cp1600.I), 1, f);
    fwrite(&intv.cp1600.D, sizeof(intv.cp1600.D), 1, f);

    fwrite(&intv.cp1600.intr, sizeof(intv.cp1600.intr), 1, f);

//  fwrite(&intv.cp1600.req_bus.intrq, sizeof(intv.cp1600.req_bus.intrq), 1, f);

    memcpy(phase, "phase_2", sizeof(phase));
	fwrite(phase, sizeof(phase), 1, f);

//  fwrite(&intv.cp1600.req_bus.intrq_until, sizeof(intv.cp1600.req_bus.intrq_until), 1, f);
//  fwrite(&intv.cp1600.req_bus.busrq_until, sizeof(intv.cp1600.req_bus.busrq_until), 1, f);
//  fwrite(&intv.cp1600.req_bus.next_intrq, sizeof(intv.cp1600.req_bus.next_intrq), 1, f);
//  fwrite(&intv.cp1600.req_bus.next_busrq, sizeof(intv.cp1600.req_bus.next_busrq), 1, f);
//  fwrite(&intv.cp1600.req_bus.intak, sizeof(intv.cp1600.req_bus.intak), 1, f);
//  fwrite(&intv.cp1600.req_bus.busak, sizeof(intv.cp1600.req_bus.busak), 1, f);

    memcpy(phase, "phase_3", sizeof(phase));
	fwrite(phase, sizeof(phase), 1, f);

    fwrite(&intv.cp1600.oldpc, sizeof(intv.cp1600.oldpc), 1, f);
    fwrite(&intv.cp1600.ext, sizeof(intv.cp1600.ext), 1, f);
    fwrite(&intv.cp1600.int_vec, sizeof(intv.cp1600.int_vec), 1, f);

    memcpy(phase, "phase_4", sizeof(phase));
	fwrite(phase, sizeof(phase), 1, f);

    for (i = 0; i < 1 << (CP1600_MEMSIZE-CP1600_DECODE_PAGE - 5); i++)
    {
        fwrite(&intv.cp1600.cacheable[i], sizeof(intv.cp1600.cacheable[i]), 1, f);
    }

    memcpy(phase, "phase_5", sizeof(phase));
	fwrite(phase, sizeof(phase), 1, f);

//  fwrite(&intv.cp1600.instr_tick_per, sizeof(intv.cp1600.instr_tick_per), 1, f);

	for (addr = 0; addr <= 0xFFFF; addr++)
	{
		data = periph_peek(AS_PERIPH(intv.intv), AS_PERIPH(intv.intv),
						   addr, 0);
        fwrite(&data, sizeof(data), 1, f);
	}

    memcpy(phase, "phase_6", sizeof(phase));
	fwrite(phase, sizeof(phase), 1, f);

    fwrite(&intv.stic.stic_cr.addr_base, sizeof(intv.stic.stic_cr.addr_base), 1, f);
    fwrite(&intv.stic.stic_cr.addr_mask, sizeof(intv.stic.stic_cr.addr_mask), 1, f);
    fwrite(&intv.stic.stic_cr.now, sizeof(intv.stic.stic_cr.now), 1, f);
    fwrite(&intv.stic.stic_cr.min_tick, sizeof(intv.stic.stic_cr.min_tick), 1, f);
    fwrite(&intv.stic.stic_cr.max_tick, sizeof(intv.stic.stic_cr.max_tick), 1, f);
    fwrite(&intv.stic.stic_cr.next_tick, sizeof(intv.stic.stic_cr.next_tick), 1, f);

    fwrite(&intv.stic.snoop_btab.addr_base, sizeof(intv.stic.snoop_btab.addr_base), 1, f);
    fwrite(&intv.stic.snoop_btab.addr_mask, sizeof(intv.stic.snoop_btab.addr_mask), 1, f);
    fwrite(&intv.stic.snoop_btab.now, sizeof(intv.stic.snoop_btab.now), 1, f);
    fwrite(&intv.stic.snoop_btab.min_tick, sizeof(intv.stic.snoop_btab.min_tick), 1, f);
    fwrite(&intv.stic.snoop_btab.max_tick, sizeof(intv.stic.snoop_btab.max_tick), 1, f);
    fwrite(&intv.stic.snoop_btab.next_tick, sizeof(intv.stic.snoop_btab.next_tick), 1, f);

    fwrite(&intv.stic.snoop_gram.addr_base, sizeof(intv.stic.snoop_gram.addr_base), 1, f);
    fwrite(&intv.stic.snoop_gram.addr_mask, sizeof(intv.stic.snoop_gram.addr_mask), 1, f);
    fwrite(&intv.stic.snoop_gram.now, sizeof(intv.stic.snoop_gram.now), 1, f);
    fwrite(&intv.stic.snoop_gram.min_tick, sizeof(intv.stic.snoop_gram.min_tick), 1, f);
    fwrite(&intv.stic.snoop_gram.max_tick, sizeof(intv.stic.snoop_gram.max_tick), 1, f);
    fwrite(&intv.stic.snoop_gram.next_tick, sizeof(intv.stic.snoop_gram.next_tick), 1, f);

//  fwrite(&intv.stic.req_bus->intrq, sizeof(intv.stic.req_bus->intrq), 1, f);
//  fwrite(&intv.stic.req_bus->intrq_until, sizeof(intv.stic.req_bus->intrq_until), 1, f);
//  fwrite(&intv.stic.req_bus->busrq_until, sizeof(intv.stic.req_bus->busrq_until), 1, f);
//  fwrite(&intv.stic.req_bus->next_intrq, sizeof(intv.stic.req_bus->next_intrq), 1, f);
//  fwrite(&intv.stic.req_bus->next_busrq, sizeof(intv.stic.req_bus->next_busrq), 1, f);
//  fwrite(&intv.stic.req_bus->intak, sizeof(intv.stic.req_bus->intak), 1, f);
//  fwrite(&intv.stic.req_bus->busak, sizeof(intv.stic.req_bus->busak), 1, f);

//  fwrite(&intv.stic.cp_row, sizeof(intv.stic.cp_row), 1, f);
    fwrite(&intv.stic.prev_vid_enable, sizeof(intv.stic.prev_vid_enable), 1, f);
    fwrite(&intv.stic.vid_enable, sizeof(intv.stic.vid_enable), 1, f);
    fwrite(&intv.stic.mode, sizeof(intv.stic.mode), 1, f);
    fwrite(&intv.stic.p_mode, sizeof(intv.stic.p_mode), 1, f);
    fwrite(&intv.stic.bt_dirty, sizeof(intv.stic.bt_dirty), 1, f);
    fwrite(&intv.stic.gr_dirty, sizeof(intv.stic.gr_dirty), 1, f);
    fwrite(&intv.stic.ob_dirty, sizeof(intv.stic.ob_dirty), 1, f);

    fwrite(&intv.stic.rand_regs, sizeof(intv.stic.rand_regs), 1, f);
    fwrite(&intv.stic.pal, sizeof(intv.stic.pal), 1, f);
    fwrite(&intv.stic.drop_frame, sizeof(intv.stic.drop_frame), 1, f);
    fwrite(&intv.stic.gmem_accessible, sizeof(intv.stic.gmem_accessible), 1, f);
    fwrite(&intv.stic.stic_accessible, sizeof(intv.stic.stic_accessible), 1, f);
//  fwrite(&intv.stic.next_irq, sizeof(intv.stic.next_irq), 1, f);

    memcpy(phase, "phase_7", sizeof(phase));
	fwrite(phase, sizeof(phase), 1, f);

//  fwrite(&intv.stic.phase, sizeof(intv.stic.phase), 1, f);
//  fwrite(&intv.stic.next_phase, sizeof(intv.stic.next_phase), 1, f);

    memcpy(phase, "phase_8", sizeof(phase));
	fwrite(phase, sizeof(phase), 1, f);

    for (i=0; i<0x40; i++) {
        fwrite(&intv.stic.raw[i], sizeof(intv.stic.raw[i]), 1, f);
    }

    for (i=0; i<0x140 * 8; i++) {
        fwrite(&intv.stic.gmem[i], sizeof(intv.stic.gmem[i]), 1, f);
    }

//  fwrite(&intv.stic.fifo_ptr, sizeof(intv.stic.fifo_ptr), 1, f);

    memcpy(phase, "phase8a", sizeof(phase));
	fwrite(phase, sizeof(phase), 1, f);

    for (i=0; i<240; i++) {
        fwrite(&intv.stic.btab_sr[i], sizeof(intv.stic.btab_sr[i]), 1, f);
    }
    for (i=0; i<240; i++) {
        fwrite(&intv.stic.btab[i], sizeof(intv.stic.btab[i]), 1, f);
    }
    for (i=0; i<12; i++) {
        fwrite(&intv.stic.last_bg[i], sizeof(intv.stic.last_bg[i]), 1, f);
    }
    memcpy(phase, "phase8b", sizeof(phase));
	fwrite(phase, sizeof(phase), 1, f);

    fwrite(&intv.stic.time, sizeof(intv.stic.time), 1, f);

    memcpy(phase, "phase8c", sizeof(phase));
	fwrite(phase, sizeof(phase), 1, f);

    for (i=0; i<240*8; i++) {
        fwrite(&intv.stic.bt_bmp[i], sizeof(intv.stic.bt_bmp[i]), 1, f);
    }
    for (i=0; i<16*16  / 8; i++) {
        fwrite(&intv.stic.mob_img[i], sizeof(intv.stic.mob_img[i]), 1, f);
    }
    for (i=0; i<8; i++) {
        for (k=0; k<16; k++) {
            fwrite(&intv.stic.mob_bmp[i][k], sizeof(intv.stic.mob_bmp[i][k]), 1, f);
        }
    }
    for (i=0; i<192*224 / 8; i++) {
        fwrite(&intv.stic.mpl_img[i], sizeof(intv.stic.mpl_img[i]), 1, f);
    }
    for (i=0; i<192*224 /32; i++) {
        fwrite(&intv.stic.mpl_vsb[i], sizeof(intv.stic.mpl_vsb[i]), 1, f);
    }
    for (i=0; i<192*224 /32; i++) {
        fwrite(&intv.stic.mpl_pri[i], sizeof(intv.stic.mpl_pri[i]), 1, f);
    }
    for (i=0; i<192*224 / 8; i++) {
        fwrite(&intv.stic.xbt_img[i], sizeof(intv.stic.xbt_img[i]), 1, f);
    }
    for (i=0; i<192*224 /32; i++) {
        fwrite(&intv.stic.xbt_bmp[i], sizeof(intv.stic.xbt_bmp[i]), 1, f);
    }
    for (i=0; i<192*224 / 8; i++) {
        fwrite(&intv.stic.image[i], sizeof(intv.stic.image[i]), 1, f);
    }

    memcpy(phase, "phase8d", sizeof(phase));
	fwrite(phase, sizeof(phase), 1, f);

    for (i=0; i<8; i++) {
        for (k=0; k<4; k++) {
            fwrite(&intv.stic.gfx->bbox[i][k], sizeof(intv.stic.gfx->bbox[i][k]), 1, f);
        }
    }

    fwrite(&intv.stic.gfx->dirty, sizeof(intv.stic.gfx->dirty), 1, f);
    fwrite(&intv.stic.gfx->drop_frame, sizeof(intv.stic.gfx->drop_frame), 1, f);
    fwrite(&intv.stic.gfx->dropped_frames, sizeof(intv.stic.gfx->dropped_frames), 1, f);
    fwrite(&intv.stic.gfx->tot_frames, sizeof(intv.stic.gfx->tot_frames), 1, f);
    fwrite(&intv.stic.gfx->tot_dropped_frames, sizeof(intv.stic.gfx->tot_dropped_frames), 1, f);

    fwrite(&intv.stic.gfx->b_color, sizeof(intv.stic.gfx->b_color), 1, f);
    fwrite(&intv.stic.gfx->b_dirty, sizeof(intv.stic.gfx->b_dirty), 1, f);
    fwrite(&intv.stic.gfx->x_blank, sizeof(intv.stic.gfx->x_blank), 1, f);
    fwrite(&intv.stic.gfx->y_blank, sizeof(intv.stic.gfx->y_blank), 1, f);
    fwrite(&intv.stic.gfx->x_delay, sizeof(intv.stic.gfx->x_delay), 1, f);
    fwrite(&intv.stic.gfx->y_delay, sizeof(intv.stic.gfx->y_delay), 1, f);
    fwrite(&intv.stic.gfx->debug_blank, sizeof(intv.stic.gfx->debug_blank), 1, f);

    memcpy(phase, "phase8e", sizeof(phase));
	fwrite(phase, sizeof(phase), 1, f);

    fwrite(&intv.stic.gfx->periph.addr_base, sizeof(intv.stic.gfx->periph.addr_base), 1, f);
    fwrite(&intv.stic.gfx->periph.addr_mask, sizeof(intv.stic.gfx->periph.addr_mask), 1, f);
    fwrite(&intv.stic.gfx->periph.now, sizeof(intv.stic.gfx->periph.now), 1, f);
    fwrite(&intv.stic.gfx->periph.min_tick, sizeof(intv.stic.gfx->periph.min_tick), 1, f);
    fwrite(&intv.stic.gfx->periph.max_tick, sizeof(intv.stic.gfx->periph.max_tick), 1, f);
    fwrite(&intv.stic.gfx->periph.next_tick, sizeof(intv.stic.gfx->periph.next_tick), 1, f);

    fwrite(&intv.stic.gfx->periph.next_tick, sizeof(intv.stic.gfx->periph.next_tick), 1, f);

    memcpy(phase, "phase_9", sizeof(phase));
	fwrite(phase, sizeof(phase), 1, f);

#if 0
	for (k=0; k<32; k++) {
        fwrite(&intv.exec.banksw[k], sizeof(intv.exec.banksw[k]), 1, f);
	}
	for (k=0; k<32; k++) {
        fwrite(&intv.exec2.banksw[k], sizeof(intv.exec2.banksw[k]), 1, f);
	}
	for (k=0; k<32; k++) {
        fwrite(&intv.sys_ram.banksw[k], sizeof(intv.sys_ram.banksw[k]), 1, f);
	}
	for (k=0; k<32; k++) {
        fwrite(&intv.sys_ram2.banksw[k], sizeof(intv.sys_ram2.banksw[k]), 1, f);
	}
	for (k=0; k<32; k++) {
        fwrite(&intv.scr_ram.banksw[k], sizeof(intv.scr_ram.banksw[k]), 1, f);
	}
	for (k=0; k<32; k++) {
        fwrite(&intv.glt_ram.banksw[k], sizeof(intv.glt_ram.banksw[k]), 1, f);
	}
	for (k=0; k<32; k++) {
        fwrite(&intv.game0.banksw[k], sizeof(intv.game0.banksw[k]), 1, f);
	}
	for (k=0; k<32; k++) {
        fwrite(&intv.game1.banksw[k], sizeof(intv.game1.banksw[k]), 1, f);
	}
	for (k=0; k<32; k++) {
        fwrite(&intv.game2.banksw[k], sizeof(intv.game2.banksw[k]), 1, f);
	}
	for (k=0; k<32; k++) {
        fwrite(&intv.ecs0.banksw[k], sizeof(intv.ecs0.banksw[k]), 1, f);
	}
	for (k=0; k<32; k++) {
        fwrite(&intv.ecs1.banksw[k], sizeof(intv.ecs1.banksw[k]), 1, f);
	}
	for (k=0; k<32; k++) {
        fwrite(&intv.ecs2.banksw[k], sizeof(intv.ecs2.banksw[k]), 1, f);
	}
	for (k=0; k<32; k++) {
        fwrite(&intv.ecs_ram.banksw[k], sizeof(intv.ecs_ram.banksw[k]), 1, f);
	}
#endif

    memcpy(phase, "phase10", sizeof(phase));
	fwrite(phase, sizeof(phase), 1, f);

	for (k=0; k<4096 + 256; k++) {
        fwrite(&intv.exec_img[k], sizeof(intv.exec_img[k]), 1, f);
	}
	for (k=0; k<2048; k++) {
        fwrite(&intv.grom_img[k], sizeof(intv.grom_img[k]), 1, f);
	}
	for (k=0; k<4096 * 3; k++) {
        fwrite(&intv.ecs_img[k], sizeof(intv.ecs_img[k]), 1, f);
	}

    memcpy(phase, "phase11", sizeof(phase));
	fwrite(phase, sizeof(phase), 1, f);

    // Write periph's notion of 'now'
    fwrite(&intv.intv->periph.now, sizeof(intv.intv->periph.now), 1, f);

    fclose(f);
#endif
}


/*
 * ============================================================================
 *  LOAD_DUMP
 * ============================================================================
 */

void load_dump()
{
#if 1
    jzp_printf("Loading and saving dumps currently disabled.\n");
#else
    FILE *f;
    int addr, i, k;
	unsigned short data;
	char phase[8];
    memset(phase, 0, sizeof(phase));

    f = fopen("dump.sav","rb");
    if (!f)
    {
        perror("fopen(\"dump.sav\", \"r\")");
        jzp_printf("couldn't open dump.sav.\n");

		return;
    }

	fread(phase, sizeof(phase)-1, 1, f);
	jzp_printf("phase read: %s\n", phase);//1

    fread(&intv.cp1600.periph.addr_base, sizeof(intv.cp1600.periph.addr_base), 1, f);
    fread(&intv.cp1600.periph.addr_mask, sizeof(intv.cp1600.periph.addr_mask), 1, f);
    fread(&intv.cp1600.periph.now, sizeof(intv.cp1600.periph.now), 1, f);
    fread(&intv.cp1600.periph.min_tick, sizeof(intv.cp1600.periph.min_tick), 1, f);
    fread(&intv.cp1600.periph.max_tick, sizeof(intv.cp1600.periph.max_tick), 1, f);
    fread(&intv.cp1600.periph.next_tick, sizeof(intv.cp1600.periph.next_tick), 1, f);

    fread(&intv.cp1600.snoop.addr_base, sizeof(intv.cp1600.snoop.addr_base), 1, f);
    fread(&intv.cp1600.snoop.addr_mask, sizeof(intv.cp1600.snoop.addr_mask), 1, f);
    fread(&intv.cp1600.snoop.now, sizeof(intv.cp1600.snoop.now), 1, f);
    fread(&intv.cp1600.snoop.min_tick, sizeof(intv.cp1600.snoop.min_tick), 1, f);
    fread(&intv.cp1600.snoop.max_tick, sizeof(intv.cp1600.snoop.max_tick), 1, f);
    fread(&intv.cp1600.snoop.next_tick, sizeof(intv.cp1600.snoop.next_tick), 1, f);

    fread(&intv.cp1600.tot_cycle, sizeof(intv.cp1600.tot_cycle), 1, f);
    fread(&intv.cp1600.tot_instr, sizeof(intv.cp1600.tot_instr), 1, f);
    fread(&intv.cp1600.tot_cache, sizeof(intv.cp1600.tot_cache), 1, f);
    fread(&intv.cp1600.tot_noncache, sizeof(intv.cp1600.tot_noncache), 1, f);

    for (i = 0; i <= 7; i++)
    {
        fread(&intv.cp1600.r[i], sizeof(intv.cp1600.r[i]), 1, f);
    }

    fread(&intv.cp1600.S, sizeof(intv.cp1600.S), 1, f);
    fread(&intv.cp1600.C, sizeof(intv.cp1600.C), 1, f);
    fread(&intv.cp1600.O, sizeof(intv.cp1600.O), 1, f);
    fread(&intv.cp1600.Z, sizeof(intv.cp1600.Z), 1, f);
    fread(&intv.cp1600.I, sizeof(intv.cp1600.I), 1, f);
    fread(&intv.cp1600.D, sizeof(intv.cp1600.D), 1, f);

    fread(&intv.cp1600.intr, sizeof(intv.cp1600.intr), 1, f);


	fread(phase, sizeof(phase)-1, 1, f);
	jzp_printf("phase read: %s\n", phase); //2

//  fread(&intv.cp1600.req_bus.intrq, sizeof(intv.cp1600.req_bus.intrq), 1, f);
//  fread(&intv.cp1600.req_bus.intrq_until, sizeof(intv.cp1600.req_bus.intrq_until), 1, f);
//  fread(&intv.cp1600.req_bus.busrq_until, sizeof(intv.cp1600.req_bus.busrq_until), 1, f);
//  fread(&intv.cp1600.req_bus.next_intrq, sizeof(intv.cp1600.req_bus.next_intrq), 1, f);
//  fread(&intv.cp1600.req_bus.next_busrq, sizeof(intv.cp1600.req_bus.next_busrq), 1, f);
//  fread(&intv.cp1600.req_bus.intak, sizeof(intv.cp1600.req_bus.intak), 1, f);
//  fread(&intv.cp1600.req_bus.busak, sizeof(intv.cp1600.req_bus.busak), 1, f);

	fread(phase, sizeof(phase)-1, 1, f);
	jzp_printf("phase read: %s\n", phase); //3

    fread(&intv.cp1600.oldpc, sizeof(intv.cp1600.oldpc), 1, f);
    fread(&intv.cp1600.ext, sizeof(intv.cp1600.ext), 1, f);
    fread(&intv.cp1600.int_vec, sizeof(intv.cp1600.int_vec), 1, f);

	fread(phase, sizeof(phase)-1, 1, f);
	jzp_printf("phase read: %s\n", phase); //4

    for (i = 0; i < 1 << (CP1600_MEMSIZE-CP1600_DECODE_PAGE - 5); i++)
    {
        fread(&intv.cp1600.cacheable[i], sizeof(intv.cp1600.cacheable[i]), 1, f);
    }

	fread(phase, sizeof(phase)-1, 1, f);
	jzp_printf("phase read: %s\n", phase); //5

//  fread(&intv.cp1600.instr_tick_per, sizeof(intv.cp1600.instr_tick_per), 1, f);

	for (addr = 0; addr <= 0xFFFF; addr++)
	{
        fread(&data, sizeof(data), 1, f);
        if (addr > 0x40)
        {
            periph_write(AS_PERIPH(intv.intv), AS_PERIPH(intv.intv),
                               addr, data);
        }
	}

	fread(phase, sizeof(phase)-1, 1, f);
	jzp_printf("phase read: %s\n", phase); //6

    fread(&intv.stic.stic_cr.addr_base, sizeof(intv.stic.stic_cr.addr_base), 1, f);
    fread(&intv.stic.stic_cr.addr_mask, sizeof(intv.stic.stic_cr.addr_mask), 1, f);
    fread(&intv.stic.stic_cr.now, sizeof(intv.stic.stic_cr.now), 1, f);
    fread(&intv.stic.stic_cr.min_tick, sizeof(intv.stic.stic_cr.min_tick), 1, f);
    fread(&intv.stic.stic_cr.max_tick, sizeof(intv.stic.stic_cr.max_tick), 1, f);
    fread(&intv.stic.stic_cr.next_tick, sizeof(intv.stic.stic_cr.next_tick), 1, f);

    fread(&intv.stic.snoop_btab.addr_base, sizeof(intv.stic.snoop_btab.addr_base), 1, f);
    fread(&intv.stic.snoop_btab.addr_mask, sizeof(intv.stic.snoop_btab.addr_mask), 1, f);
    fread(&intv.stic.snoop_btab.now, sizeof(intv.stic.snoop_btab.now), 1, f);
    fread(&intv.stic.snoop_btab.min_tick, sizeof(intv.stic.snoop_btab.min_tick), 1, f);
    fread(&intv.stic.snoop_btab.max_tick, sizeof(intv.stic.snoop_btab.max_tick), 1, f);
    fread(&intv.stic.snoop_btab.next_tick, sizeof(intv.stic.snoop_btab.next_tick), 1, f);

    fread(&intv.stic.snoop_gram.addr_base, sizeof(intv.stic.snoop_gram.addr_base), 1, f);
    fread(&intv.stic.snoop_gram.addr_mask, sizeof(intv.stic.snoop_gram.addr_mask), 1, f);
    fread(&intv.stic.snoop_gram.now, sizeof(intv.stic.snoop_gram.now), 1, f);
    fread(&intv.stic.snoop_gram.min_tick, sizeof(intv.stic.snoop_gram.min_tick), 1, f);
    fread(&intv.stic.snoop_gram.max_tick, sizeof(intv.stic.snoop_gram.max_tick), 1, f);
    fread(&intv.stic.snoop_gram.next_tick, sizeof(intv.stic.snoop_gram.next_tick), 1, f);

//  fread(&intv.stic.req_bus->intrq, sizeof(intv.stic.req_bus->intrq), 1, f);
//  fread(&intv.stic.req_bus->intrq_until, sizeof(intv.stic.req_bus->intrq_until), 1, f);
//  fread(&intv.stic.req_bus->busrq_until, sizeof(intv.stic.req_bus->busrq_until), 1, f);
//  fread(&intv.stic.req_bus->next_intrq, sizeof(intv.stic.req_bus->next_intrq), 1, f);
//  fread(&intv.stic.req_bus->next_busrq, sizeof(intv.stic.req_bus->next_busrq), 1, f);
//  fread(&intv.stic.req_bus->intak, sizeof(intv.stic.req_bus->intak), 1, f);
//  fread(&intv.stic.req_bus->busak, sizeof(intv.stic.req_bus->busak), 1, f);

//  fread(&intv.stic.cp_row, sizeof(intv.stic.cp_row), 1, f);
    fread(&intv.stic.prev_vid_enable, sizeof(intv.stic.prev_vid_enable), 1, f);
    fread(&intv.stic.vid_enable, sizeof(intv.stic.vid_enable), 1, f);
    fread(&intv.stic.mode, sizeof(intv.stic.mode), 1, f);

    fread(&intv.stic.p_mode, sizeof(intv.stic.p_mode), 1, f);
    fread(&intv.stic.bt_dirty, sizeof(intv.stic.bt_dirty), 1, f);
    fread(&intv.stic.gr_dirty, sizeof(intv.stic.gr_dirty), 1, f);
    fread(&intv.stic.ob_dirty, sizeof(intv.stic.ob_dirty), 1, f);
    fread(&intv.stic.rand_regs, sizeof(intv.stic.rand_regs), 1, f);

    fread(&intv.stic.pal, sizeof(intv.stic.pal), 1, f);
    fread(&intv.stic.drop_frame, sizeof(intv.stic.drop_frame), 1, f);
    fread(&intv.stic.gmem_accessible, sizeof(intv.stic.gmem_accessible), 1, f);
    fread(&intv.stic.stic_accessible, sizeof(intv.stic.stic_accessible), 1, f);
//  fread(&intv.stic.next_irq, sizeof(intv.stic.next_irq), 1, f);

	fread(phase, sizeof(phase)-1, 1, f);
	jzp_printf("phase read: %s\n", phase); //7

//  fread(&intv.stic.phase, sizeof(intv.stic.phase), 1, f);
//  fread(&intv.stic.next_phase, sizeof(intv.stic.next_phase), 1, f);

	fread(phase, sizeof(phase)-1, 1, f);
	jzp_printf("phase read: %s\n", phase); //8

    for (i=0; i<0x40; i++) {
        fread(&intv.stic.raw[i], sizeof(intv.stic.raw[i]), 1, f);
    }

    for (i=0; i<0x140 * 8; i++) {
        fread(&intv.stic.gmem[i], sizeof(intv.stic.gmem[i]), 1, f);
    }

//  fread(&intv.stic.fifo_ptr, sizeof(intv.stic.fifo_ptr), 1, f);

	fread(phase, sizeof(phase)-1, 1, f);
	jzp_printf("phase read: %s\n", phase); //8a

    for (i=0; i<240; i++) {
        fread(&intv.stic.btab_sr[i], sizeof(intv.stic.btab_sr[i]), 1, f);
    }
    for (i=0; i<240; i++) {
        fread(&intv.stic.btab[i], sizeof(intv.stic.btab[i]), 1, f);
    }
    for (i=0; i<12; i++) {
        fread(&intv.stic.last_bg[i], sizeof(intv.stic.last_bg[i]), 1, f);
    }

	fread(phase, sizeof(phase)-1, 1, f);
	jzp_printf("phase read: %s\n", phase); //8b

    fread(&intv.stic.time, sizeof(intv.stic.time), 1, f);

	fread(phase, sizeof(phase)-1, 1, f);
	jzp_printf("phase read: %s\n", phase); //8c

    for (i=0; i<240*8; i++) {
        fread(&intv.stic.bt_bmp[i], sizeof(intv.stic.bt_bmp[i]), 1, f);
    }
    for (i=0; i<16*16  / 8; i++) {
        fread(&intv.stic.mob_img[i], sizeof(intv.stic.mob_img[i]), 1, f);
    }
    for (i=0; i<8; i++) {
        for (k=0; k<16; k++) {
            fread(&intv.stic.mob_bmp[i][k], sizeof(intv.stic.mob_bmp[i][k]), 1, f);
        }
    }
    for (i=0; i<192*224 / 8; i++) {
        fread(&intv.stic.mpl_img[i], sizeof(intv.stic.mpl_img[i]), 1, f);
    }
    for (i=0; i<192*224 /32; i++) {
        fread(&intv.stic.mpl_vsb[i], sizeof(intv.stic.mpl_vsb[i]), 1, f);
    }
    for (i=0; i<192*224 /32; i++) {
        fread(&intv.stic.mpl_pri[i], sizeof(intv.stic.mpl_pri[i]), 1, f);
    }
    for (i=0; i<192*224 / 8; i++) {
        fread(&intv.stic.xbt_img[i], sizeof(intv.stic.xbt_img[i]), 1, f);
    }
    for (i=0; i<192*224 /32; i++) {
        fread(&intv.stic.xbt_bmp[i], sizeof(intv.stic.xbt_bmp[i]), 1, f);
    }
    for (i=0; i<192*224 / 8; i++) {
        fread(&intv.stic.image[i], sizeof(intv.stic.image[i]), 1, f);
    }

	fread(phase, sizeof(phase)-1, 1, f);
	jzp_printf("phase read: %s\n", phase); //8d

    for (i=0; i<8; i++) {
        for (k=0; k<4; k++) {
            fread(&intv.stic.gfx->bbox[i][k], sizeof(intv.stic.gfx->bbox[i][k]), 1, f);
        }
    }

    fread(&intv.stic.gfx->dirty, sizeof(intv.stic.gfx->dirty), 1, f);
    fread(&intv.stic.gfx->drop_frame, sizeof(intv.stic.gfx->drop_frame), 1, f);
    fread(&intv.stic.gfx->dropped_frames, sizeof(intv.stic.gfx->dropped_frames), 1, f);
    fread(&intv.stic.gfx->tot_frames, sizeof(intv.stic.gfx->tot_frames), 1, f);
    fread(&intv.stic.gfx->tot_dropped_frames, sizeof(intv.stic.gfx->tot_dropped_frames), 1, f);

    fread(&intv.stic.gfx->b_color, sizeof(intv.stic.gfx->b_color), 1, f);
    fread(&intv.stic.gfx->b_dirty, sizeof(intv.stic.gfx->b_dirty), 1, f);
    fread(&intv.stic.gfx->x_blank, sizeof(intv.stic.gfx->x_blank), 1, f);
    fread(&intv.stic.gfx->y_blank, sizeof(intv.stic.gfx->y_blank), 1, f);
    fread(&intv.stic.gfx->x_delay, sizeof(intv.stic.gfx->x_delay), 1, f);
    fread(&intv.stic.gfx->y_delay, sizeof(intv.stic.gfx->y_delay), 1, f);
    fread(&intv.stic.gfx->debug_blank, sizeof(intv.stic.gfx->debug_blank), 1, f);

	fread(phase, sizeof(phase)-1, 1, f);
	jzp_printf("phase read: %s\n", phase); //9

    fread(&intv.stic.gfx->periph.addr_base, sizeof(intv.stic.gfx->periph.addr_base), 1, f);
    fread(&intv.stic.gfx->periph.addr_mask, sizeof(intv.stic.gfx->periph.addr_mask), 1, f);
    fread(&intv.stic.gfx->periph.now, sizeof(intv.stic.gfx->periph.now), 1, f);
    fread(&intv.stic.gfx->periph.min_tick, sizeof(intv.stic.gfx->periph.min_tick), 1, f);
    fread(&intv.stic.gfx->periph.max_tick, sizeof(intv.stic.gfx->periph.max_tick), 1, f);
    fread(&intv.stic.gfx->periph.next_tick, sizeof(intv.stic.gfx->periph.next_tick), 1, f);

    fread(&intv.stic.gfx->periph.next_tick, sizeof(intv.stic.gfx->periph.next_tick), 1, f);

	fread(phase, sizeof(phase)-1, 1, f);
	jzp_printf("phase read: %s\n", phase); //9

#if 0
	for (k=0; k<32; k++) {
        fread(&intv.exec.banksw[k], sizeof(intv.exec.banksw[k]), 1, f);
	}
	for (k=0; k<32; k++) {
        fread(&intv.exec2.banksw[k], sizeof(intv.exec2.banksw[k]), 1, f);
	}
	for (k=0; k<32; k++) {
        fread(&intv.sys_ram.banksw[k], sizeof(intv.sys_ram.banksw[k]), 1, f);
	}
	for (k=0; k<32; k++) {
        fread(&intv.sys_ram2.banksw[k], sizeof(intv.sys_ram2.banksw[k]), 1, f);
	}
	for (k=0; k<32; k++) {
        fread(&intv.scr_ram.banksw[k], sizeof(intv.scr_ram.banksw[k]), 1, f);
	}
	for (k=0; k<32; k++) {
        fread(&intv.glt_ram.banksw[k], sizeof(intv.glt_ram.banksw[k]), 1, f);
	}
	for (k=0; k<32; k++) {
        fread(&intv.game0.banksw[k], sizeof(intv.game0.banksw[k]), 1, f);
	}
	for (k=0; k<32; k++) {
        fread(&intv.game1.banksw[k], sizeof(intv.game1.banksw[k]), 1, f);
	}
	for (k=0; k<32; k++) {
        fread(&intv.game2.banksw[k], sizeof(intv.game2.banksw[k]), 1, f);
	}
	for (k=0; k<32; k++) {
        fread(&intv.ecs0.banksw[k], sizeof(intv.ecs0.banksw[k]), 1, f);
	}
	for (k=0; k<32; k++) {
        fread(&intv.ecs1.banksw[k], sizeof(intv.ecs1.banksw[k]), 1, f);
	}
	for (k=0; k<32; k++) {
        fread(&intv.ecs2.banksw[k], sizeof(intv.ecs2.banksw[k]), 1, f);
	}
	for (k=0; k<32; k++) {
        fread(&intv.ecs_ram.banksw[k], sizeof(intv.ecs_ram.banksw[k]), 1, f);
	}
#endif

	fread(phase, sizeof(phase)-1, 1, f);
	jzp_printf("phase read: %s\n", phase); //10

	for (k=0; k<4096 + 256; k++) {
        fread(&intv.exec_img[k], sizeof(intv.exec_img[k]), 1, f);
	}
	for (k=0; k<2048; k++) {
        fread(&intv.grom_img[k], sizeof(intv.grom_img[k]), 1, f);
	}
	for (k=0; k<4096 * 3; k++) {
        fread(&intv.ecs_img[k], sizeof(intv.ecs_img[k]), 1, f);
	}

	fread(phase, sizeof(phase)-1, 1, f);
	jzp_printf("phase read: %s\n", phase); //10


    // Get periph's notion of 'now'
    fread(&intv.intv->periph.now, sizeof(intv.intv->periph.now), 1, f);

    fclose(f);

	save_state(1);

    // Force display refreshes everywhere
    stic_resync(&(intv.stic));
    gfx_resync(&(intv.gfx));

    // Force time to resync
    speed_resync(&(intv.speed));

	jzp_printf("load ended\n");
#endif
}

static const char *const osd_top[] =
{
    "  ##    #    #    ##      ##  ",
    " ####   #    #   ####    #### ",
    "##  ##  #    #  ##  ##  ##  ##",
    "#    #  #    #  #    #  #    #",
    "#       #    #  #   ##       #",
    "#       ######  #  ###     ## ",
    "#       ######  # ## #     ## ",
    "#       #    #  ###  #       #",
    "#       #    #  ##   #       #",
    "#    #  #    #  #    #  #    #",
    "##  ##  #    #  ##  ##  ##  ##",
    " ####   #    #   ####    #### ",
    "  ##    #    #    ##      ##  ",
    0
};

static const char *const osd_bot[] =
{
    "#     #  #    # #######  #####",
    "##   ##  #    # #######  #####",
    "##   ##  #    #    #     #    ",
    "### ###  #    #    #     #    ",
    "# ### #  #    #    #     #    ",
    "# ### #  #    #    #     #### ",
    "#  #  #  #    #    #     #### ",
    "#     #  #    #    #     #    ",
    "#     #  #    #    #     #    ",
    "#     #  #    #    #     #    ",
    "#     #  ##  ##    #     #    ",
    "#     #  ######    #     #####",
    "#     #   ####     #     #####",
    0
};

/* ======================================================================== */
/*  PUT_OSD_BITS -- Just a bit of fun.                                      */
/* ======================================================================== */
static void put_osd_bits(uint8_t *dst, const char * const *src, int color)
{
    for (int i = 0; src[i]; ++i)
        for (int j = 0; src[i][j]; ++j)
            if (src[i][j] == '#')
                dst[i*160 + j] = color;
}

/* ======================================================================== */
/*  FAKE_OSD     -- Just a bit of fun.                                      */
/* ======================================================================== */
static void fake_osd(uint8_t *disp, uint32_t rnd)
{
    int ofs = 0;

    if ((rnd & 0x0F) == 0)
        ofs += (rnd & 0x20) ? -160 : +160;

    if ((rnd & 0xAA8000) == 0)
        ofs += (rnd & 0x2000000) ? -1 : 1;

    put_osd_bits(disp + 16*160 + 120 + ofs, osd_top, 14);  // Yellow-Green
    put_osd_bits(disp + 170*160 + 120 + ofs, osd_bot, 14); 
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
/*                 Copyright (c) 1998-2020, Joseph Zbiciak                  */
/* ======================================================================== */
