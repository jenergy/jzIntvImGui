/*
 * ============================================================================
 *  Title:    Event Handling Subsystem
 *  Author:   J. Zbiciak
 * ============================================================================
 *  The Event subsystem receives input from the keyboard, joystick, and
 *  (experimentally) mouse.
 *
 *  This is the common event core.  Frameworks such as SDL, SDL2, etc. should
 *  adapt their notion of events to this.
 *
 *  EVENT notifies other subsystems of events by setting flags.  When
 *  an event comes in, the event is looked up in a table and bits in a
 *  uint32_t are set/cleared according to masks assigned to the event.
 *  The masks are specified as four uint32_t's:  "AND" and "OR" masks
 *  for a "down" event, and "AND" and "OR" masks for an "up" event.
 *
 *  Event->mask mappings are registered with EVENT via calls to "event_map".
 *  Each event can only be mapped to ONE set of masks, and so the order
 *  in which events are mapped determines the final set of mappings.  (This
 *  allows a specialized config file to override a standard config.)  Mapping
 *  an event to a NULL pointer or to an empty set/clear mask disables the
 *  event.
 * ============================================================================
 *  The following event classes are handled by the EVENT subsystem:
 *
 *   -- Quit events       (eg. somebody hitting the little 'X' in the corner)
 *   -- Keyboard events   (key down / key up)
 *   -- Joystick events   (not yet implemented)
 *   -- Mouse events      (not yet implemented, may never be implemented)
 *   -- Activation events (hide / unhide window)
 *
 *  Event symbol names are assigned in 'event.h', and are stored as strings.
 *  This should simplify dynamic configuration from CFG files.  For
 *  simplicity's sake, I will convert joystick and quit events to
 *  keyboard-like events.
 * ============================================================================
 *  EVENT_INIT       -- Initializes the Event Subsystem
 *  EVENT_TICK       -- Processes currently pending events in the event queue
 *  EVENT_MAP        -- Map an event to a set/clear mask and a word pointer.
 * ============================================================================
 */

#include "config.h"
#include "periph/periph.h"
#include "cp1600/cp1600.h"
#include "cp1600/emu_link.h"
#include "event/event.h"
#include "event/event_tbl.h"
#include "event/event_plat.h"

#define KBD_STACK_EMPTY (-1)

/* ======================================================================== */
/*  EVENT_MASK_T     -- Structure containing AND/OR masks for an event.     */
/* ======================================================================== */
typedef struct event_mask_t
{
    uint32_t    *word;           /* Word to mask, or NULL if none.   */
    uint32_t    and_mask[2];    /* AND masks (up/down)              */
    uint32_t    or_mask [2];    /* OR masks (up/down)               */
} event_mask_t;

#define EV_Q_LEN (256)
#define COMBO_BMAP_SIZE ((EVENT_COMBO0 + 31) >> 5)

/* ======================================================================== */
/*  COMBO_STATE_T    -- State associated with COMBOs.                       */
/* ======================================================================== */
typedef struct
{
    uint64_t half_active;           /* combos that might be forming         */
    uint64_t full_active;           /* combos actively asserted             */
    uint64_t allocated;             /* combos that are currently defined    */

    uint64_t event_bits[64];        /* event bits associated w/ combos      */

    int         event_cnt;          /* total # of unique events in combos   */
    event_num_t event_num[64];      /* event #s assoc'd w/ any combos       */
    event_num_t pairs[64][2];       /* event #s assoc'd with each combo     */

    uint32_t in_combo[COMBO_BMAP_SIZE]; /* Bmap: event is in a combo.       */
    uint32_t key_down[COMBO_BMAP_SIZE]; /* Bmap: key is pressed.            */
} combo_state_t;

/* ======================================================================== */
/*  EVT_PVT_T        -- Private event machine state.                        */
/* ======================================================================== */
struct evt_pvt_t
{
    void    *plat_pvt;              /* Private for platform-specific code.  */

    event_mask_t
        mask_tbl[4][EVENT_COUNT];   /* Event mask tables.                   */

    int           current_map;      /* Currently active event mapping.      */
    int           stacked_map;      /* "Pushed" event mapping, or none.     */

    /* Combo-related processing. */
    double        soon;             /* When to next process events.         */
    double        coalesce_time;    /* Time-window to recognize combos.     */
    combo_state_t combo;            /* State associated with COMBOs.        */

    /* Event history queue, used for event processing, combos, and emu-link */
    event_updn_t  ev_updn_q[EV_Q_LEN];
    event_num_t   ev_num_q [EV_Q_LEN];
    uint32_t      ev_q_wr, ev_q_el_rd, ev_q_ev_rd;
};

/* ======================================================================== */
/*  Bitmap helper functions for combo event bitmap.                         */
/*  GET_EN_BIT   -- Gets the current value of the bit for event_num.        */
/*  SET_EN_BIT   -- Sets the bit to 1 for event_num.                        */
/*  CLR_EN_BIT   -- Clears the bit to 0 for event_num.                      */
/* ======================================================================== */
LOCAL bool get_en_bit(const uint32_t *const bitmap, event_num_t event_num)
{
    return (bitmap[event_num >> 5] >> (event_num & 31)) & 1;
}

LOCAL void set_en_bit(uint32_t *const bitmap, event_num_t event_num)
{
    bitmap[event_num >> 5] |= 1ul << (event_num & 31);
}

LOCAL void clr_en_bit(uint32_t *const bitmap, event_num_t event_num)
{
    bitmap[event_num >> 5] &= ~(1ul << (event_num & 31));
}


LOCAL int event_emu_link(cp1600_t *, int *, void *);
LOCAL uint32_t event_tick(periph_t *const p, uint32_t len);
LOCAL void event_dtor(periph_t *const p);

/* ======================================================================== */
/*  EVENT_INIT       -- Initializes the Event subsystem.                    */
/* ======================================================================== */
int event_init
(
    event_t *const event,           // The event structure.
    const bool enable_mouse,        // Are we enabling the mouse?
    const int initial_event_map     // Initial event map at startup.
)
{
    /* -------------------------------------------------------------------- */
    /*  The event 'peripheral' is ticked every so often in order to         */
    /*  drain input events from the event queue and post inputs to the      */
    /*  emulator.                                                           */
    /* -------------------------------------------------------------------- */
    static const periph_t event_periph = {
        PERIPH_NO_RDWR,
        .min_tick = PERIPH_HZ(1000), .max_tick = PERIPH_HZ(200),
        .tick = event_tick, .dtor = event_dtor
    };
    event->periph = event_periph;

    /* -------------------------------------------------------------------- */
    /*  Set up our "private" structure.                                     */
    /* -------------------------------------------------------------------- */
    evt_pvt_t *pvt = CALLOC(evt_pvt_t, 1);
    event->pvt = pvt;

    if (!pvt)
    {
        fprintf(stderr, "event_init: Unable to allocate private state.\n");
        return -1;
    }

    /* -------------------------------------------------------------------- */
    /*  Initialize our active event mapping and event map stack.            */
    /* -------------------------------------------------------------------- */
    pvt->current_map  = initial_event_map;
    pvt->stacked_map  = KBD_STACK_EMPTY;

    /* -------------------------------------------------------------------- */
    /*  Set up our event-coalescing timer.                                  */
    /* -------------------------------------------------------------------- */
    pvt->soon          = 0.;
    pvt->coalesce_time = 0.0001;   /* 1ms */

    /* -------------------------------------------------------------------- */
    /*  Perform platform-specific initialization.                           */
    /* -------------------------------------------------------------------- */
    if (event_plat_init(enable_mouse, pvt, &pvt->plat_pvt))
    {
        fprintf(stderr,
            "event_init: Could not initalize platform specific event code.\n");
        free(pvt);
        return -1;
    }

    /* -------------------------------------------------------------------- */
    /*  Register us on Emu-Link as major API #9.                            */
    /* -------------------------------------------------------------------- */
    emu_link_register(event_emu_link, 9, pvt);

    /* -------------------------------------------------------------------- */
    /*  Done!                                                               */
    /* -------------------------------------------------------------------- */
    return 0;
}

/* ======================================================================== */
/*  EVENT_DTOR   -- Tear down the event engine.                             */
/* ======================================================================== */
LOCAL void event_dtor(periph_t *const p)
{
    event_t *const event = PERIPH_AS(event_t, p);
    evt_pvt_t *const pvt = event->pvt;

    event_plat_dtor(pvt->plat_pvt);

    CONDFREE(event->pvt);
}

/* ======================================================================== */
/*  EVENT_QUEUE_HAS_ROOM -- Returns true if there's room for N events.      */
/* ======================================================================== */
int event_queue_has_room(evt_pvt_t *const pvt, const int count)
{
    return pvt->ev_q_wr - pvt->ev_q_ev_rd + count < EV_Q_LEN;
}

/* ======================================================================== */
/*  EVENT_ENQUEUE    -- Internal event queue containing expanded events.    */
/* ======================================================================== */
void event_enqueue
(
    evt_pvt_t *const   pvt,
    const event_updn_t event_updn,
    const event_num_t  event_num
)
{
    /* -------------------------------------------------------------------- */
    /*  Ignore events that are out of range or marked "ignore"              */
    /* -------------------------------------------------------------------- */
    if (!EVENT_VALID(event_num))
        return;

    /* -------------------------------------------------------------------- */
    /*  Drop oldest EMULINK event on overflow                               */
    /* -------------------------------------------------------------------- */
    if (pvt->ev_q_wr - pvt->ev_q_el_rd == EV_Q_LEN)
        pvt->ev_q_el_rd++;

    /* -------------------------------------------------------------------- */
    /*  We should never drop events internally.                             */
    /* -------------------------------------------------------------------- */
    assert(pvt->ev_q_wr - pvt->ev_q_ev_rd < EV_Q_LEN);

    /* -------------------------------------------------------------------- */
    /*  Remember the current event.                                         */
    /* -------------------------------------------------------------------- */
    pvt->ev_num_q [pvt->ev_q_wr % EV_Q_LEN] = event_num;
    pvt->ev_updn_q[pvt->ev_q_wr % EV_Q_LEN] = event_updn;

    pvt->ev_q_wr++;
}

/* ======================================================================== */
/*  EVENT_DEQUEUE   Dequeue events queued by event_queue.  Does not         */
/*                  disturb the EMULINK queue pointer.                      */
/* ======================================================================== */
LOCAL int event_dequeue
(
    evt_pvt_t    *const pvt,
    event_updn_t *const event_updn,
    event_num_t  *const event_num
)
{
    if (pvt->ev_q_ev_rd == pvt->ev_q_wr)
        return 0;

    while (pvt->ev_q_el_rd >= EV_Q_LEN && pvt->ev_q_ev_rd >= EV_Q_LEN)
    {
        pvt->ev_q_el_rd -= EV_Q_LEN;
        pvt->ev_q_ev_rd -= EV_Q_LEN;
        pvt->ev_q_wr    -= EV_Q_LEN;
    }

    const int q_rd = pvt->ev_q_ev_rd++ % EV_Q_LEN;

    *event_updn = pvt->ev_updn_q[q_rd];
    *event_num  = pvt->ev_num_q [q_rd];

    return 1;
}

extern void tick_called();
/* ======================================================================== */
/*  EVENT_TICK   -- Processes currently pending events in the event queue   */
/* ======================================================================== */
LOCAL uint32_t event_tick(periph_t *const p, uint32_t len)
{
    tick_called();
    event_t   *const event = PERIPH_AS(event_t, p);
    evt_pvt_t *const pvt   = event->pvt;

    /* -------------------------------------------------------------------- */
    /*  First, pump the event loop and gather some events.                  */
    /* -------------------------------------------------------------------- */
    event_plat_pump(pvt, pvt->plat_pvt);

    /* -------------------------------------------------------------------- */
    /*  If we've corked event queue processing, leave early if we're still  */
    /*  before the deadline.                                                */
    /* -------------------------------------------------------------------- */
    const double now = get_time();
    if (now < pvt->soon)
        return len;

    pvt->soon = 0.;

    /* -------------------------------------------------------------------- */
    /*  Let the platform fill our internal event queue.                     */
    /* -------------------------------------------------------------------- */
    if (event_plat_tick(pvt, pvt->plat_pvt))
        return len;     /* Early exit; likely corking input. */

    /* -------------------------------------------------------------------- */
    /*  Drain the internal event queue and trigger all the event.s          */
    /* -------------------------------------------------------------------- */
    event_num_t  event_num;
    event_updn_t event_updn;
    while (event_dequeue(pvt, &event_updn, &event_num))
    {
        /* ---------------------------------------------------------------- */
        /*  Process the event.  If pvt->mask_tbl[event_num].word == NULL    */
        /*  then we aren't interested in this event.                        */
        /* ---------------------------------------------------------------- */
        const event_mask_t *const event_mask =
            &pvt->mask_tbl[pvt->current_map][event_num];

        if (event_mask->word == NULL)
            continue;

        /* ---------------------------------------------------------------- */
        /*  Apply the appropriate AND and OR masks to the word.             */
        /* ---------------------------------------------------------------- */
        *event_mask->word &= event_mask->and_mask[event_updn];
        *event_mask->word |= event_mask->or_mask [event_updn];
    }

    /* -------------------------------------------------------------------- */
    /*  Allow the platform to perform any late event processing tasks.      */
    /* -------------------------------------------------------------------- */
    event_plat_tick_late(pvt, pvt->plat_pvt);

    /* -------------------------------------------------------------------- */
    /*  Done!  We always "consume" our entire "tick."                       */
    /* -------------------------------------------------------------------- */
    return len;
}

/* ======================================================================== */
/*  EVENT_EMU_LINK -- Allow games to get raw event feed from event queue.   */
/* ======================================================================== */
LOCAL int event_emu_link(cp1600_t *cpu, int *fail, void *opaque)
{
    evt_pvt_t *const pvt = (evt_pvt_t *)opaque;

    /* -------------------------------------------------------------------- */
    /*  The event Emu-Link API is very simple:                              */
    /*                                                                      */
    /*  INPUTS:                                                             */
    /*      R2 == 0x0000:  Just return event number and up/down             */
    /*      R2 != 0x0000:  Try to write ASCII name of event @R1.            */
    /*                     ASCII names are bounded to 18 chars + NUL.       */
    /*                                                                      */
    /*  OUTPUTS:                                                            */
    /*      R0:   0xFFFF = No events, otherwise event #                     */
    /*      R1:   0 = UP, 1 = DOWN                                          */
    /*      R2:   Unmodified.                                               */
    /* -------------------------------------------------------------------- */

    *fail = 0;

    if (pvt->ev_q_wr == pvt->ev_q_el_rd)
    {
        cpu->r[1] = 0;
        return 0xFFFF;
    }

    while (pvt->ev_q_el_rd >= EV_Q_LEN && pvt->ev_q_ev_rd >= EV_Q_LEN)
    {
        pvt->ev_q_el_rd -= EV_Q_LEN;
        pvt->ev_q_ev_rd -= EV_Q_LEN;
        pvt->ev_q_wr    -= EV_Q_LEN;
    }

    const int q_rd = pvt->ev_q_el_rd++ % EV_Q_LEN;

    if (cpu->r[2] != 0)
    {
        const char *s = event_num_to_name(pvt->ev_num_q[q_rd]);
        int i;
        int addr = cpu->r[2];

        /* This should never fire; but I'd rather print this than crash. */
        if (!s)
            s = "INTERNAL ERROR";

        for (i = 0; i < 18 && *s; i++, s++)
            CP1600_WR(cpu, addr++, *s);

        CP1600_WR(cpu, addr, 0);
    }

    cpu->r[1] = pvt->ev_updn_q[q_rd];

    return pvt->ev_num_q[q_rd];
}

extern void map_event(const char* msg, const char* num, int map);
/* ======================================================================== */
/*  EVENT_MAP        -- Maps an event to a particular AND/OR mask set       */
/* ======================================================================== */
int event_map
(
    event_t    *event,          /* Event_t structure being set up.          */
    const char *event_name,     /* Name of event to map.                    */
    int         map,            /* Keyboard map number to map within.       */
    const char* jzintv_event_name,
    uint32_t   *word,           /* Word modified by event, (NULL to ignore) */
    uint32_t    and_mask[2],    /* AND masks for event up/down.             */
    uint32_t    or_mask[2]      /* OR masks for event up/down.              */
)
{
    evt_pvt_t *const pvt = event->pvt;
    int num, j;

    /* -------------------------------------------------------------------- */
    /*  Get the event number for this name.                                 */
    /* -------------------------------------------------------------------- */
    if ((num = event_name_to_num(event_name)) < 0)
        return -1;

    map_event(jzintv_event_name, event_name, map);

    /* -------------------------------------------------------------------- */
    /*  Register ourselves with this event.                                 */
    /* -------------------------------------------------------------------- */
    pvt->mask_tbl[map][num].word = word;

    for (j = 0; j < 2; j++)
    {
        pvt->mask_tbl[map][num].and_mask[j] = and_mask[j];
        pvt->mask_tbl[map][num].or_mask [j] = or_mask [j];
    }

    /* -------------------------------------------------------------------- */
    /*  Done:  Return success.                                              */
    /* -------------------------------------------------------------------- */
    return 0;
}

/* ======================================================================== */
/*  EVENT_COMBO_EVENT_NUM    -- Return the compressed event number for a    */
/*                              combo, or combo->event_cnt if not found.    */
/* ======================================================================== */
LOCAL int event_combo_event_num
(
    combo_state_t *const combo,
    const event_num_t    event_num
)
{
    for (int combo_event_num = 0; combo_event_num < combo->event_cnt;
         combo_event_num++)
        if (combo->event_num[combo_event_num] == event_num)
            return combo_event_num;

    return combo->event_cnt;
}

/* ======================================================================== */
/*  EVENT_ADD_TO_COMBO   -- Bind a name to half of a combo.                 */
/* ======================================================================== */
LOCAL int event_add_to_combo
(
    combo_state_t *const combo,
    const char    *const event_name,
    const int            combo_num,
    const int            which   /* 0/1: first/second half of combo pair.   */
)
{
    const uint64_t event_bit = 1ull << combo_num;

    const event_num_t event_num = event_name_to_num(event_name);
    if (!EVENT_VALID(event_num))
        return -1;

    if (event_num >= EVENT_COMBO0)
    {
        fprintf(stderr, "event:  Cannot use '%s' in a combo event\n",
                event_name);
        return -1;
    }

    const int combo_event_num = event_combo_event_num(combo, event_num);

    if (combo_event_num == combo->event_cnt)
    {
        combo->event_cnt++;
        combo->event_num[combo_event_num] = event_num;
        set_en_bit(combo->in_combo, event_num);
    }

    if (combo->event_bits[combo_event_num] & event_bit)
    {
        fprintf(stderr, "event:  Must use two distinct events in a combo\n");
        return -1;
    }

    combo->event_bits[combo_event_num] |= event_bit;
    combo->pairs[combo_num][which] = event_num;
    return 0;
}

/* ======================================================================== */
/*  EVENT_COMBINE    -- Register a combo event as COMBOxx                   */
/* ======================================================================== */
int event_combine
(
    event_t     *const event,
    const char  *const event_name1,
    const char  *const event_name2,
    const int          combo_num
)
{
    combo_state_t *const combo = &event->pvt->combo;
    const uint32_t event_bit = 1u << combo_num;

    if (combo->allocated & event_bit)
    {
        fprintf(stderr, "event:  Error: COMBO%d already in use\n", combo_num);
        return -1;
    }

    combo->allocated |= event_bit;

    if (event_add_to_combo(combo, event_name1, combo_num, 0)) return -1;
    if (event_add_to_combo(combo, event_name2, combo_num, 1)) return -1;

    return 0;
}

/* ======================================================================== */
/*  GET_TRAILING_1      Helper function for event_check_combo.              */
/*  Source:  http://graphics.stanford.edu/~seander/bithacks.html            */
/*  Uses a deBruijn sequence to do the bit position calc.  Special thanks   */
/*  to Arnauld Chevallier for introducing me to deBruijn sequences!         */
/* ======================================================================== */
static const int mult_DeBruijn_bit_pos[32] =
{
    0,   1, 28,  2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17,  4, 8,
    31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18,  6, 11,  5, 10, 9
};

LOCAL uint32_t get_trailing_1(uint64_t v)
{
    const uint32_t lo = (uint32_t)v;
    const uint32_t hi = (uint32_t)(v >> 32);
    const uint32_t lbit = lo & -lo;
    const uint32_t hbit = hi & -hi;
    return
        lo ? mult_DeBruijn_bit_pos[((uint32_t)(lbit * 0x077CB531U)) >> 27]
           : mult_DeBruijn_bit_pos[((uint32_t)(hbit * 0x077CB531U)) >> 27] + 32;
}

/* ======================================================================== */
/*  EVENT_CHECK_COMBO_PRESS                                                 */
/*                                                                          */
/*  Look for compound keypresses, and convert them into special compound    */
/*  events.  Returns "true" if we need to cork further inputs.  Callers     */
/*  should stop draining their event queue at this time and wait until      */
/*  the next "tick."                                                        */
/* ======================================================================== */
LOCAL bool event_check_combo_press
(
    evt_pvt_t     *const pvt,
    const event_num_t    event_num
)
{
    combo_state_t *const combo = &pvt->combo;
    bool cork = false;

    /* -------------------------------------------------------------------- */
    /*  If this event isn't part of a combo, or is already down, pass thru  */
    /* -------------------------------------------------------------------- */
    if (event_num >= EVENT_COMBO0 || !get_en_bit(combo->in_combo, event_num) ||
        get_en_bit(combo->key_down, event_num))
    {
        event_enqueue(pvt, EV_DOWN, event_num);
        return cork;
    }

    /* -------------------------------------------------------------------- */
    /*  Mark this key as "pressed".                                         */
    /* -------------------------------------------------------------------- */
    set_en_bit(combo->key_down, event_num);

    /* -------------------------------------------------------------------- */
    /*  New "full" combos will have a bit already set in "half_combo" that  */
    /*  is also set in this event's combo list, and isn't already in the    */
    /*  full-combos list.                                                   */
    /* -------------------------------------------------------------------- */
    const int combo_event_num = event_combo_event_num(combo, event_num);
    assert(combo_event_num != combo->event_cnt);

    const uint64_t prev_half_active = combo->half_active;
    const uint64_t prev_full_active = combo->full_active;
    const uint64_t this_combos = combo->event_bits[combo_event_num];
    const uint64_t now_active = this_combos & combo->half_active;

    combo->full_active |= now_active;
    combo->half_active |= this_combos;

    /* -------------------------------------------------------------------- */
    /*  If we made any new "half" combos, cork further event processing     */
    /*  for "coalesce_time".                                                */
    /* -------------------------------------------------------------------- */
    if (prev_half_active != combo->half_active)
    {
        pvt->soon = get_time() + pvt->coalesce_time;
        cork = true;
    }

    /* -------------------------------------------------------------------- */
    /*  If this didn't make any new combos, go ahead and send a EV_DOWN.    */
    /* -------------------------------------------------------------------- */
    uint64_t new_combos = combo->full_active & ~prev_full_active;
    if (!new_combos)
    {
        event_enqueue(pvt, EV_DOWN, event_num);
        return cork;
    }

    /* -------------------------------------------------------------------- */
    /*  If we made any new "full" combos, go send the appropriate up/dn     */
    /* -------------------------------------------------------------------- */
    while (new_combos)
    {
        const int combo_num = get_trailing_1(new_combos);
        new_combos &= new_combos - 1;

        /* ---------------------------------------------------------------- */
        /*  Send a key-up for the first guy in the combo.                   */
        /* ---------------------------------------------------------------- */
        const int other = combo->pairs[combo_num][0] == event_num ? 1 : 0;
        const event_num_t other_event_num = combo->pairs[combo_num][other];

        if (event_num != other_event_num)
            event_enqueue(pvt, EV_UP, other_event_num);

        /* ---------------------------------------------------------------- */
        /*  Send a key-down for the newly created combo.                    */
        /* ---------------------------------------------------------------- */
        event_enqueue(pvt, EV_DOWN, (event_num_t)(EVENT_COMBO0 + combo_num));
    }

    return cork;
}

/* ======================================================================== */
/*  EVENT_CHECK_COMBO_RELEASE                                               */
/*                                                                          */
/*  Identifies when a combo has been released.  Returns "true" if we need   */
/*  cork further inputs.  Callers should stop draining their event queue    */
/*  at this time and wait until the next "tick."                            */
/* ======================================================================== */
LOCAL bool event_check_combo_release
(
    evt_pvt_t  *const pvt,
    const event_num_t event_num
)
{
    combo_state_t *const combo = &pvt->combo;
    bool cork = false;

    /* -------------------------------------------------------------------- */
    /*  If this event isn't part of a combo, or is already up, pass thru    */
    /* -------------------------------------------------------------------- */
    if (event_num >= EVENT_COMBO0 || !get_en_bit(combo->in_combo, event_num) ||
        !get_en_bit(combo->key_down, event_num))
    {
        event_enqueue(pvt, EV_UP, event_num);
        return cork;
    }

    /* -------------------------------------------------------------------- */
    /*  Mark this key as "unpressed".                                       */
    /* -------------------------------------------------------------------- */
    clr_en_bit(combo->key_down, event_num);

    /* -------------------------------------------------------------------- */
    /*  Look for full combos that now got broken by this event, and half    */
    /*  combos that are no longer even half-combos.                         */
    /* -------------------------------------------------------------------- */
    const int combo_event_num = event_combo_event_num(combo, event_num);;
    assert(combo_event_num != combo->event_cnt);

    const uint64_t this_combos  = combo->event_bits[combo_event_num];
    uint64_t dying_combos = this_combos & combo->full_active;

    combo->full_active &= ~dying_combos;
    combo->half_active &= ~(this_combos & ~dying_combos);

    /* -------------------------------------------------------------------- */
    /*  If we have any dying combos, cork further event processing for      */
    /*  "coalesce_time".                                                    */
    /* -------------------------------------------------------------------- */
    if (dying_combos)
    {
        pvt->soon = get_time() + pvt->coalesce_time;
        cork = 1;
    }

    /* -------------------------------------------------------------------- */
    /*  If this didn't break any combos, go ahead and send an EV_UP.        */
    /* -------------------------------------------------------------------- */
    if (!dying_combos)
    {
        event_enqueue(pvt, EV_UP, event_num);
        return cork;
    }

    /* -------------------------------------------------------------------- */
    /*  If we broke any new "full" combos, go send the appropriate up/dn    */
    /* -------------------------------------------------------------------- */
    while (dying_combos)
    {
        const int combo_num = get_trailing_1(dying_combos);
        dying_combos &= dying_combos - 1;

        /* ---------------------------------------------------------------- */
        /*  Send a key-up for the dying combo.                              */
        /* ---------------------------------------------------------------- */
        event_enqueue(pvt, EV_UP, (event_num_t)(EVENT_COMBO0 + combo_num));

        /* ---------------------------------------------------------------- */
        /*  Send a key-down for the remaining guy in the combo, *if* he's   */
        /*  not active in any other combos.                                 */
        /* ---------------------------------------------------------------- */
        const int other = combo->pairs[combo_num][0] == event_num ? 1 : 0;
        const event_num_t other_event_num = combo->pairs[combo_num][other];
        const int other_combo_event_num =
            event_combo_event_num(combo, other_event_num);

        if (other_combo_event_num != combo->event_cnt &&
            !(combo->full_active & combo->event_bits[other_combo_event_num]))
        {
            event_enqueue(pvt, EV_DOWN, other_event_num);
            break;
        }
    }

    return cork;
}

/* ======================================================================== */
/*  EVENT_ENQUEUE_CHECK_COMBO                                               */
/*                                                                          */
/*  Enqueues an event, or combo-related events associated with this event.  */
/*  Returns "true" if we need to cork further inputs.  Callers should stop  */ 
/*  draining event queue at this time and wait until the next "tick."       */
/* ======================================================================== */
bool event_enqueue_check_combo
(
    evt_pvt_t *const   pvt,
    const event_updn_t event_updn,
    const event_num_t  event_num
)
{
    return event_updn == EV_UP ? event_check_combo_release(pvt, event_num)
                               : event_check_combo_press(pvt, event_num);
}

/* ======================================================================== */
/*  EVENT_SET_COMBO_COALESCE                                                */
/*  Adjust the coalesce timer for combo matching.                           */
/* ======================================================================== */
void event_set_combo_coalesce
(
    event_t *const event,
    const double   coalesce_time
)
{
    evt_pvt_t *const pvt = event->pvt;
    pvt->coalesce_time = coalesce_time;
}

int get_current_map(evt_pvt_t *const pvt) {
    return pvt->current_map;
}

/* ======================================================================== */
/*  EVENT_CHANGE_ACTIVE_MAP  -- Change the current input mapping.           */
/* ======================================================================== */
void event_change_active_map(event_t *const event,
                             const ev_map_change_req map_change_req)
{
    evt_pvt_t *const pvt = event->pvt;
    int previous_map = pvt->current_map;

    switch (map_change_req)
    {
        case EV_MAP_SET_0: case EV_MAP_SET_1:
        case EV_MAP_SET_2: case EV_MAP_SET_3:
            pvt->current_map = ((int)map_change_req - EV_MAP_SET_0) & 3;
            break;

        case EV_MAP_NEXT:
            pvt->current_map = (pvt->current_map + 1) & 3;
            break;

        case EV_MAP_PREV:
            pvt->current_map = (pvt->current_map - 1) & 3;
            break;

        case EV_MAP_PSH_0: case EV_MAP_PSH_1:
        case EV_MAP_PSH_2: case EV_MAP_PSH_3:
            pvt->stacked_map = pvt->current_map | 4;
            pvt->current_map = ((int)map_change_req - EV_MAP_PSH_0) & 3;
            break;

        case EV_MAP_POP:
            if (pvt->stacked_map)
            {
                pvt->current_map = pvt->stacked_map & 3;
                pvt->stacked_map = 0;
            }
            break;

        case EV_MAP_NOP:
        default:
            break;
    }

    if (previous_map != pvt->current_map)
    {
        jzp_clear_and_eol(
            jzp_printf("Switching to input map %d", pvt->current_map));
        jzp_flush();
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
/*                 Copyright (c) 1998-2020, Joseph Zbiciak                  */
/* ======================================================================== */
