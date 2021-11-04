/* ======================================================================== */
/*  STIC.C -- New, complete, hopefully fast STIC implementation.            */
/* ======================================================================== */

#include "config.h"
#include "periph/periph.h"
#include "mem/mem.h"
#include "cp1600/cp1600.h"
#include "demo/demo.h"
#include "gfx/gfx.h"
#include "stic.h"
#include "stic_timings.h"
#include "speed/speed.h"
#include "lzoe/lzoe.h"
#include "debug/debug_if.h"
#include "gif/gif_enc.h"
#include "file/file.h"

LOCAL void stic_draw_fgbg(stic_t *stic);
LOCAL void stic_draw_cstk(stic_t *stic);
LOCAL void stic_simulate_until(stic_t *const RESTRICT stic,
                               const uint64_t when);
LOCAL void stic_generate_reqs(stic_t *const RESTRICT stic);
LOCAL void stic_do_req_drop
(
    struct req_q_t *const RESTRICT req_q,
    const uint64_t                 cycle
);
LOCAL void stic_do_req_ack
(
    struct req_q_t *const RESTRICT req_q,
    const uint64_t                 cycle
);

/* ======================================================================== */
/*  STIC Register Masks                                                     */
/*  Only certain bits in each STIC register are writeable.  The bits that   */
/*  are not implemented return a fixed pattern of 0s and 1s.  The table     */
/*  below encodes this information in the form of an "AND / OR" mask pair.  */
/*  The data is first ANDed with the AND mask.  This mask effectively       */
/*  indicates the implemented bits.  The data is then ORed with the OR      */
/*  mask.  This second mask effectively indicates which of the bits always  */
/*  read as 1.                                                              */
/* ======================================================================== */
struct stic_reg_mask_t
{
    uint32_t and_mask;
    uint32_t or_mask;
};

LOCAL const struct stic_reg_mask_t stic_reg_mask[0x40] ALIGN(8) =
{
    /* MOB X Registers                                  0x00 - 0x07 */
    {0x07FF,0x3800}, {0x07FF,0x3800}, {0x07FF,0x3800}, {0x07FF,0x3800},
    {0x07FF,0x3800}, {0x07FF,0x3800}, {0x07FF,0x3800}, {0x07FF,0x3800},

    /* MOB Y Registers                                  0x08 - 0x0F */
    {0x0FFF,0x3000}, {0x0FFF,0x3000}, {0x0FFF,0x3000}, {0x0FFF,0x3000},
    {0x0FFF,0x3000}, {0x0FFF,0x3000}, {0x0FFF,0x3000}, {0x0FFF,0x3000},

    /* MOB A Registers                                  0x10 - 0x17 */
    {0x3FFF,0x0000}, {0x3FFF,0x0000}, {0x3FFF,0x0000}, {0x3FFF,0x0000},
    {0x3FFF,0x0000}, {0x3FFF,0x0000}, {0x3FFF,0x0000}, {0x3FFF,0x0000},

    /* MOB C Registers                                  0x18 - 0x1F */
    {0x03FE,0x3C00}, {0x03FD,0x3C00}, {0x03FB,0x3C00}, {0x03F7,0x3C00},
    {0x03EF,0x3C00}, {0x03DF,0x3C00}, {0x03BF,0x3C00}, {0x037F,0x3C00},

    /* Display enable, Mode select                      0x20 - 0x21 */
    {0x0000,0x3FFF}, {0x0000,0x3FFF},

    /* Unimplemented registers                          0x22 - 0x27 */
    {0x0000,0x3FFF}, {0x0000,0x3FFF}, {0x0000,0x3FFF}, {0x0000,0x3FFF},
    {0x0000,0x3FFF}, {0x0000,0x3FFF},

    /* Color stack, border color                        0x28 - 0x2C */
    {0x000F,0x3FF0}, {0x000F,0x3FF0}, {0x000F,0x3FF0}, {0x000F,0x3FF0},
    {0x000F,0x3FF0},

    /* Unimplemented registers                          0x2D - 0x2F */
    {0x0000,0x3FFF}, {0x0000,0x3FFF}, {0x0000,0x3FFF},

    /* Horiz delay, vertical delay, border extension    0x30 - 0x32 */
    {0x0007,0x3FF8}, {0x0007,0x3FF8}, {0x0003,0x3FFC},

    /* Unimplemented registers                          0x33 - 0x3F */
    {0x0000,0x3FFF},
    {0x0000,0x3FFF}, {0x0000,0x3FFF}, {0x0000,0x3FFF}, {0x0000,0x3FFF},
    {0x0000,0x3FFF}, {0x0000,0x3FFF}, {0x0000,0x3FFF}, {0x0000,0x3FFF},
    {0x0000,0x3FFF}, {0x0000,0x3FFF}, {0x0000,0x3FFF}, {0x0000,0x3FFF}
};

/* ======================================================================== */
/*  MOB height secret decoder ring.                                         */
/* ======================================================================== */
LOCAL const int stic_mob_hgt[8] = { 8, 16, 16, 32, 32, 64, 64, 128 };

/* ======================================================================== */
/*  STIC Color Nibble Masks -- For generating packed-nibble pixels.         */
/* ======================================================================== */
LOCAL const uint32_t stic_color_mask[16] ALIGN(64) =
{
    0x00000000, 0x11111111, 0x22222222, 0x33333333,
    0x44444444, 0x55555555, 0x66666666, 0x77777777,
    0x88888888, 0x99999999, 0xAAAAAAAA, 0xBBBBBBBB,
    0xCCCCCCCC, 0xDDDDDDDD, 0xEEEEEEEE, 0xFFFFFFFF
};

/* ======================================================================== */
/*  STIC Color Mask and Bit Manipulation Lookup Tables                      */
/*   -- b2n expands 8 bits to 8 nibbles.                                    */
/*   -- b2n_d expands 8 bits to 4 nibbles, pixel-doubling as it goes.       */
/*   -- b2n_r expands 8 bits to 4 nibbles, reversing bit order as it goes.  */
/*   -- b2n_rd expands 8 bits to 4 nibbles, reversing and pixel-doubling.   */
/*   -- n2b expands 2 nibbles to 2 bytes.                                   */
/*   -- bit_r reverses the bit order in an 8-bit byte.                      */
/*   -- bit_d doubles the bits in an 8-bit byte to a 16-bit int.            */
/*   -- bit_rd doubles the bits and reverses them.                          */
/*  These are computed at runtime for now.                                  */
/* ======================================================================== */
LOCAL uint16_t stic_n2b  [256] ALIGN(64);
LOCAL uint32_t stic_b2n_ [256] ALIGN(64), stic_b2n_r [256] ALIGN(64);
LOCAL uint32_t stic_b2n_d[16]  ALIGN(64), stic_b2n_rd[16]  ALIGN(64);
LOCAL uint16_t stic_bit  [256] ALIGN(64), stic_bit_r [256] ALIGN(64);
LOCAL uint16_t stic_bit_d[256] ALIGN(64), stic_bit_rd[256] ALIGN(64);

/* ======================================================================== */
/*  STIC_B2N                 -- inline function in lieu of array            */
/* ======================================================================== */
LOCAL uint32_t stic_b2n(const uint8_t b)
{
    /* This table lookup may be a bottleneck; however, we haven't found a
       suitably fast replacement. */
    return stic_b2n_[b];

#if 0
    /* First replicate to all four bytes */
    const uint32_t b0 = b * 0x01010101u;

    /* Partition the nibbles */
    /* 76543210765432107654321076543210 => 76547654765476543210321032103210 */
    const uint32_t b1h = (b0 & 0xF0F00000u);
    const uint32_t b1l = (b0 & 0x00000F0Fu);
    const uint32_t b1  = (b1h | (b1h >> 4)) | (b1l | (b1l << 4));

    /* Partition the pairs */
    /* 76547654765476543210321032103210 => 76767676545454543232323210101010 */
    const uint32_t b2a = (b1 & 0xCC33CC33u);
    const uint32_t b2b = (b1 & 0x33003300u) >> 6;
    const uint32_t b2c = (b1 & 0x00CC00CCu) << 6;
    const uint32_t b2  = b2a | b2b | b2c;

    /* Partition the bits */
    /* 76767676545454543232323210101010 => 77776666555544443333222211110000 */
    const uint32_t b3a = (b2 & 0xA5A5A5A5);
    const uint32_t b3b = (b2 & 0x50505050) >> 3;
    const uint32_t b3c = (b2 & 0x0A0A0A0A) << 3;
    const uint32_t b3  = b3a | b3b | b3c;

    return b3;
#endif
}

/* ======================================================================== */
/*  STIC_DROPPING_THIS_FRAME -- Return true if we're dropping this frame    */
/* ======================================================================== */
LOCAL int stic_dropping_this_frame(const stic_t *const stic)
{
    return (stic->drop_frame > 0 || stic->is_hidden) && !stic->movie_active;
}

/* ======================================================================== */
/*  STIC_IS_STIC_ACCESSIBLE  -- Return true if the STIC is accessible at    */
/*                              the time of the access.                     */
/* ======================================================================== */
LOCAL int stic_is_stic_accessible(const uint64_t now, const stic_t *const stic)
{
    return now <= stic->stic_accessible;
}

/* ======================================================================== */
/*  STIC_IS_GMEM_ACCESSIBLE  -- Return true if the GMEM is accessible at    */
/*                              the time of the access.                     */
/* ======================================================================== */
LOCAL int stic_is_gmem_accessible(const uint64_t now, const stic_t *const stic)
{
    return now <= stic->gmem_accessible;
}

/* ======================================================================== */
/*  STIC_CTRL_RD -- Read from a STIC control register (addr <= 0x3F)        */
/* ======================================================================== */
LOCAL uint32_t stic_ctrl_rd(periph_t *const per, periph_t *const req,
                            const uint32_t addr, const uint32_t data)
{
    stic_t *const stic = PERIPH_PARENT_AS(stic_t, per);
    const uint64_t access_time = req && req->req ? req->req->now : 0;

    UNUSED(data);

    /* -------------------------------------------------------------------- */
    /*  Bring simulation up to 'now'.                                       */
    /* -------------------------------------------------------------------- */
    stic_simulate_until(stic, access_time);

    /* -------------------------------------------------------------------- */
    /*  Is this access after the Bus-Copy -> Bus-Isolation transition       */
    /*  or before the Bus-Isolation -> Bus-Copy transition?                 */
    /* -------------------------------------------------------------------- */
    if (!stic_is_stic_accessible(access_time, stic))
    {
        /* ---------------------------------------------------------------- */
        /*  Yes:  Return garbage.                                           */
        /* ---------------------------------------------------------------- */
        if (stic->debug_flags & STIC_SHOW_RD_DROP)
        {
            const cp1600_t *const cpu = req && req->req ? (cp1600_t *)req->req
                                                        : NULL;
            const int pc = cpu ? cpu->r[7] : -1;

            jzp_printf("STIC CTRL RD drop: addr = $%.4X @%llu   ",
                       addr + per->addr_base, access_time);

            if (pc >= 0 && pc <= 0xFFFF)
                jzp_printf("PC = $%.4X\n", pc);
            else
                jzp_printf("PC unknown\n");

            if ((stic->debug_flags & STIC_DBG_CTRL_ACCESS_WINDOW) == 0)
            {
/*
                jzp_printf("Prev STIC CTRL window ended @%llu",
                            stic->stic_accessible);

                if (stic->req_bus->next_intrq != ~0ULL &&
                    stic->req_bus->next_intrq != 0)
                    jzp_printf("; next window approx @%llu\n",
                                stic->req_bus->next_intrq);
                else
                    jzp_printf("\n");
*/
                stic->debug_flags |= STIC_DBG_CTRL_ACCESS_WINDOW;
            }
        }

        return addr < 0x80 ? 0x000E & addr : 0xFFFF;
    }

    /* -------------------------------------------------------------------- */
    /*  If reading location 0x21, put the display into Color-Stack mode.    */
    /* -------------------------------------------------------------------- */
    if ((addr & 0x7F) == 0x0021)
    {
        if (stic->mode != 0) stic->bt_dirty |= 3;
        stic->p_mode = 0;
/*jzp_printf("COLORSTACK\n");*/
        stic->upd  = stic_draw_cstk;
    }

    /* -------------------------------------------------------------------- */
    /*  If we're accessing a STIC CR alias, just return 0xFFFF.             */
    /* -------------------------------------------------------------------- */
    if (addr >= 0x4000)
        return 0xFFFF;

    /* -------------------------------------------------------------------- */
    /*  If we're reading 0x40-0x7F, just sample GRAM and return.            */
    /* -------------------------------------------------------------------- */
    if (addr >= 0x0040 && addr <= 0x007F)
        return stic->gmem[addr + 0x800] & 0xFF;

    /* -------------------------------------------------------------------- */
    /*  STIC1A has a weird bit at $22.                                      */
    /* -------------------------------------------------------------------- */
    if ((addr & 0x7F) == 0x0022 && stic->type == STIC_STIC1A )
    {
        return 0x3FF7;
    }

    /* -------------------------------------------------------------------- */
    /*  Now just return the raw value from our internal register file,      */
    /*  appropriately conditioned by the read/write masks.                  */
    /* -------------------------------------------------------------------- */
    return (stic->raw[addr] & stic_reg_mask[addr].and_mask) |
            stic_reg_mask[addr].or_mask;
}

/* ======================================================================== */
/*  STIC_CTRL_PEEK -- Like read, except w/out side effects or restrictions  */
/* ======================================================================== */
LOCAL uint32_t stic_ctrl_peek(periph_t *const per, periph_t *const req,
                              const uint32_t addr, const uint32_t data)
{
    const stic_t *const stic = PERIPH_PARENT_AS(const stic_t, per);

    UNUSED(req);
    UNUSED(data);

    /* -------------------------------------------------------------------- */
    /*  If we're reading above 0x7F, return dead air.                       */
    /* -------------------------------------------------------------------- */
    if (addr > 0x007F)
        return 0xFFFF;

    /* -------------------------------------------------------------------- */
    /*  If we're reading 0x40-0x7F, just sample GRAM and return.            */
    /* -------------------------------------------------------------------- */
    if (addr >= 0x0040 && addr <= 0x007F)
        return stic->gmem[addr + 0x800] & 0xFF;

    /* -------------------------------------------------------------------- */
    /*  Just return the raw value from our internal register file,          */
    /*  appropriately conditioned by the read/write masks.                  */
    /* -------------------------------------------------------------------- */
    return (stic->raw[addr] & stic_reg_mask[addr].and_mask) |
            stic_reg_mask[addr].or_mask;
}

/* ======================================================================== */
/*  STIC_CTRL_WR_INTERNAL   -- Does actual control register write.          */
/* ======================================================================== */
LOCAL void stic_ctrl_wr_internal(stic_t *const stic,
                                 const uint32_t addr, const uint32_t data)
{
    /* -------------------------------------------------------------------- */
    /*  If writing location 0x20, enable the display.                       */
    /* -------------------------------------------------------------------- */
    if (addr == 0x0020)
    {
/*jzp_printf("got ve post, stic->phase = %d\n", stic->phase);*/
        stic->vid_enable = VID_ENABLED;
    }

    /* -------------------------------------------------------------------- */
    /*  If writing location 0x21, put the display into FGBG mode.           */
    /* -------------------------------------------------------------------- */
    if (addr == 0x0021)
    {
        if (stic->mode != 1) stic->bt_dirty |= 3;

        stic->p_mode = 1;
/*jzp_printf("FOREGROUND/BACKGROUND\n");*/
        stic->upd  = stic_draw_fgbg;
    }

    /* -------------------------------------------------------------------- */
    /*  Now capture the write and store it in its raw, encoded form (after  */
    /*  adjusting for the and/or masks).                                    */
    /*                                                                      */
    /*  If the old != new, then:                                            */
    /*   -- If we're updating the color stack or scroll registers, mark     */
    /*      the entire frame as 'dirty'.                                    */
    /*   -- If we're updating MOB X/Y/A or border registers, just mark the  */
    /*      BACKTAB as 'dirty'.                                             */
    /*                                                                      */
    /*  The upshot is that a CSTK/border/scroll update will force a full    */
    /*  screen refresh, whereas MOB X/Y/A will still let GFX's dirty        */
    /*  rectangle update do its job.                                        */
    /* -------------------------------------------------------------------- */
    const uint32_t old     = stic->raw[addr];
    const uint32_t data_z  = data   & stic_reg_mask[addr].and_mask;
    const uint32_t data_zo = data_z | stic_reg_mask[addr].or_mask;
    stic->raw[addr] = data_zo;

    if (old != data_zo)
    {
        if      (addr >= 0x28 && addr <= 0x2B)  stic->bt_dirty |= 3;
        else if (addr >= 0x30 && addr <= 0x32)  stic->bt_dirty |= 3;
        else if (addr <= 0x17)                  stic->bt_dirty |= 1;

        if (addr == 0x2C)
            gfx_set_bord(stic->gfx, data_zo & 0xF); /* handles border color */
    }
}

/* ======================================================================== */
/*  STIC_CTRL_WR -- Write to a STIC control register (addr <= 0x3F)         */
/* ======================================================================== */
LOCAL void stic_ctrl_wr(periph_t *const per, periph_t *const req,
                        const uint32_t addr, const uint32_t data)
{
    stic_t *const stic = PERIPH_PARENT_AS(stic_t, per);
    const uint64_t access_time = req && req->req ? req->req->now + 4 : 0;

    /* -------------------------------------------------------------------- */
    /*  Bring simulation up to 'now'.                                       */
    /* -------------------------------------------------------------------- */
    stic_simulate_until(stic, access_time);

    /* -------------------------------------------------------------------- */
    /*  Ignore writes to the strange GROM visibility window at $40 and up.  */
    /* -------------------------------------------------------------------- */
    const uint32_t m_addr = addr & 0x7F;

    if (m_addr >= 0x40)
        return;

    /* -------------------------------------------------------------------- */
    /*  Is this access after the Bus-Copy -> Bus-Isolation transition       */
    /*  or before the Bus-Isolation -> Bus-Copy transition?                 */
    /* -------------------------------------------------------------------- */
    if (!stic_is_stic_accessible(access_time, stic))
    {
        /* ---------------------------------------------------------------- */
        /*  Yes:  Drop the write.                                           */
        /* ---------------------------------------------------------------- */
        if (stic->debug_flags & STIC_SHOW_WR_DROP)
        {
            const cp1600_t *const cpu = req && req->req ? (cp1600_t *)req->req
                                                        : NULL;
            const int pc = cpu ? cpu->r[7] : -1;

            jzp_printf("STIC CTRL WR drop: addr = $%.4X @%llu",
                        m_addr + per->addr_base, access_time);
            jzp_printf(" (data $%.4X) ", data);

            if (pc >= 0 && pc <= 0xFFFF)
                jzp_printf("PC = $%.4X\n", pc);
            else
                jzp_printf("PC unknown\n");

            if ((stic->debug_flags & STIC_DBG_CTRL_ACCESS_WINDOW) == 0)
            {
/*
                jzp_printf("Prev STIC CTRL window ended @%llu",
                            stic->stic_accessible);

                if (stic->req_bus->next_intrq != ~0ULL &&
                    stic->req_bus->next_intrq != 0)
                    jzp_printf("; next window approx @%llu\n",
                                stic->req_bus->next_intrq);
                else
                    jzp_printf("\n");
*/
                stic->debug_flags |= STIC_DBG_CTRL_ACCESS_WINDOW;
            }
        }
        return;
    }

    stic_ctrl_wr_internal(stic, m_addr, data);
}

/* ======================================================================== */
/*  STIC_CTRL_POKE -- Write to a STIC control register (addr <= 0x3F)       */
/* ======================================================================== */
LOCAL void stic_ctrl_poke(periph_t *const per, periph_t *const req,
                          const uint32_t addr, const uint32_t data)
{
    stic_t *const stic = PERIPH_PARENT_AS(stic_t, per);
    const uint32_t m_addr = addr & 0x7F;
    UNUSED(req);

    /* -------------------------------------------------------------------- */
    /*  Ignore writes to the strange GROM visibility window at $40 and up.  */
    /* -------------------------------------------------------------------- */
    if (m_addr >= 0x40)
        return;

    stic_ctrl_wr_internal(stic, addr, data);
}


/* ======================================================================== */
/*  STIC_REG_INIT   Initialize the registers at reset or power-up.          */
/* ======================================================================== */
LOCAL void stic_reg_init(stic_t *const stic)
{
    /* -------------------------------------------------------------------- */
    /*  Fill all the STIC registers either with 1 or random values.         */
    /* -------------------------------------------------------------------- */
    memset(stic->raw, 0, sizeof(stic->raw));  /* first, zero it */
    /* then fill it with 1s or random via ctrl writes */
    for (int a = 0x00; a < 0x40; a++)
        if (a != 0x20)
            stic_ctrl_wr_internal(stic, a,
                                  stic->rand_mem ? rand_jz() : 0xFFFF);
}

/* ======================================================================== */
/*  STIC_RESET   -- Reset state internal to the STIC                        */
/* ======================================================================== */
LOCAL void stic_reset(periph_t *const per)
{
    stic_t *const RESTRICT stic = PERIPH_PARENT_AS(stic_t, per);

    /* -------------------------------------------------------------------- */
    /*  Resync the internal state machine.                                  */
    /* -------------------------------------------------------------------- */
    stic->prev_vid_enable      = VID_DISABLED;
    stic->vid_enable           = VID_UNKNOWN;
    stic->bt_dirty             = 3;
    stic->last_frame_intrq     = stic->eff_cycle
                               + STIC_INITIAL_OFFSET;
    stic->next_frame_intrq     = stic->last_frame_intrq;
    stic->stic_accessible      = stic->last_frame_intrq
                               + STIC_STIC_ACCESSIBLE;
    stic->gmem_accessible      = stic->last_frame_intrq
                               + STIC_GMEM_ACCESSIBLE;
    /* the next_frame_render number seems... like it could need tuning */
    stic->next_frame_render    = stic->gmem_accessible
                               + 194 * STIC_SCANLINE;
    stic->vid_enable_cutoff    = stic->stic_accessible;
    /* Adjust the "vid_enable_cutoff" to the actual point where we need to
       generate new requests--the later of "STIC accessible" vs. "INTRQ hold".
       This is usually a few cycles later.  The STIC control reg write code
       will already filter out writes before this, regardless of this cutoff
       value.  This tweak fixes an assert() that sometimes fired if the 
       CPU acknowledged an interrupt after the cutoff but before the prev.
       INTRQ hold completed--we'd end up with two INTRQs pending in the
       queue. */
    if (STIC_STIC_ACCESSIBLE < STIC_INTRQ_HOLD)
        stic->vid_enable_cutoff += STIC_INTRQ_HOLD - STIC_STIC_ACCESSIBLE;
    stic->fifo_rd_ptr          = 0;
    stic->fifo_wr_ptr          = 0;
    stic->busrq_count          = 0;
    if (stic->req_q)
    {
        req_q_clear(stic->req_q);

        /* ---------------------------------------------------------------- */
        /*  Generate an INTRQ for the first frame.                          */
        /* ---------------------------------------------------------------- */
        {
            const req_t req =
            {
                stic->last_frame_intrq,
                stic->last_frame_intrq + STIC_INTRQ_HOLD,
                0,
                REQ_INT,
                REQ_PENDING
            };

            REQ_Q_PUSH_BACK(stic->req_q, req);
        }

        stic->req_q->horizon = stic->vid_enable_cutoff + 1;
    }

    if (stic->gfx)
        gfx_vid_enable(stic->gfx, 0);

    stic_reg_init(stic);
}

/* ======================================================================== */
/*  STIC_BTAB_WR -- Capture writes to the background cards.                 */
/* ======================================================================== */
LOCAL void stic_btab_wr(periph_t *const per, periph_t *const req,
                        const uint32_t addr, const uint32_t data)
{
    stic_t *const RESTRICT stic = PERIPH_PARENT_AS(stic_t, per);
    const uint64_t access_time = req && req->req ? req->req->now + 4 : 0;
    const uint32_t m_data = data & 0x3FFF;

    /* -------------------------------------------------------------------- */
    /*  Bring simulation up to 'now'.                                       */
    /* -------------------------------------------------------------------- */
    stic_simulate_until(stic, access_time);

    if (addr < 0xF0)
        stic->btab_sr[addr] = m_data;
}

/* ======================================================================== */
/*  STIC_GMEM_WR -- Capture writes to the Graphics RAM.                     */
/* ======================================================================== */
LOCAL void stic_gmem_wr(periph_t *const per, periph_t *const req,
                        const uint32_t addr, const uint32_t data)
{
    stic_t *const RESTRICT stic = PERIPH_PARENT_AS(stic_t, per);
    const uint64_t access_time = req && req->req ? req->req->now + 4 : 0;

    /* -------------------------------------------------------------------- */
    /*  Bring simulation up to 'now'.                                       */
    /* -------------------------------------------------------------------- */
    stic_simulate_until(stic, access_time);

    /* -------------------------------------------------------------------- */
    /*  Drop the write if in Bus Isolation mode.                            */
    /* -------------------------------------------------------------------- */
    if (!stic_is_gmem_accessible(access_time, stic))
    {
        if (stic->debug_flags & STIC_SHOW_WR_DROP)
        {
            const cp1600_t *const cpu = req && req->req ? (cp1600_t *)req->req
                                                        : NULL;
            const int pc = cpu ? cpu->r[7] : -1;

            jzp_printf("STIC GMEM WR drop: addr = $%.4X @%llu",
                        addr + per->addr_base, access_time);

            jzp_printf(" (data $%.4X) ", data);

            if (pc >= 0 && pc <= 0xFFFF)
                jzp_printf("PC = $%.4X\n", pc);
            else
                jzp_printf("PC unknown\n");

            if ((stic->debug_flags & STIC_DBG_GMEM_ACCESS_WINDOW) == 0)
            {
/*
                jzp_printf("Prev STIC GMEM window ended @%llu",
                            stic->gmem_accessible);

                if (stic->req_bus->next_intrq != ~0ULL &&
                    stic->req_bus->next_intrq != 0)
                    jzp_printf("; next window approx @%llu\n",
                                stic->req_bus->next_intrq);
                else
                    jzp_printf("\n");
*/
                stic->debug_flags |= STIC_DBG_GMEM_ACCESS_WINDOW;
            }
        }
        return;
    }

    /* -------------------------------------------------------------------- */
    /*  We're mapped into the entire 4K address space for GRAM/GROM.  Drop  */
    /*  all writes for GROM addresses.                                      */
    /* -------------------------------------------------------------------- */
    if ((addr & 0x0FFF) < 0x0800)
        return;

    /* -------------------------------------------------------------------- */
    /*  Mask according to what the GRAM will actually see address and data  */
    /*  wise.  As a result, this should even correctly work for the many    */
    /*  GRAM write-aliases.                                                 */
    /* -------------------------------------------------------------------- */
    {
        const uint32_t m_addr = addr & (stic->gram_mask | 7);
        /* Only the lower 8 bits of a GRAM write matter. */
        const uint32_t m_data = data & 0x00FF;

        if (m_data != stic->gmem[m_addr])
            stic->gr_dirty |= 1;

        stic->gmem[m_addr] = m_data;
    }
}

/* ======================================================================== */
/*  STIC_GMEM_POKE -- Same as GMEM_WR, except ignores bus isolation.        */
/* ======================================================================== */
LOCAL void stic_gmem_poke(periph_t *const per, periph_t *const req,
                          const uint32_t addr, const uint32_t data)
{
    stic_t *const stic = PERIPH_PARENT_AS(stic_t, per);

    UNUSED(req);

    /* -------------------------------------------------------------------- */
    /*  Don't allow pokes to GROM.                                          */
    /* -------------------------------------------------------------------- */
    if ((addr & 0x0FFF) < 0x0800)
        return;

    /* -------------------------------------------------------------------- */
    /*  Mask according to what the GRAM will actually see address and data  */
    /*  wise.  As a result, this should even correctly work for the many    */
    /*  GRAM write-aliases.                                                 */
    /* -------------------------------------------------------------------- */
    const uint32_t m_addr = addr & (stic->gram_mask | 7);
    const uint32_t m_data = data & 0x00FF; /* Only the lower 8 bits of a    */
                                           /* GRAM write matter.            */

    if (m_data != stic->gmem[m_addr])
        stic->gr_dirty |= 1;

    stic->gmem[m_addr] = m_data;
}

/* ======================================================================== */
/*  STIC_GMEM_RD -- Read values out of GRAM, GROM, taking into account      */
/*                  when GRAM/GROM are visible.                             */
/* ======================================================================== */
LOCAL uint32_t stic_gmem_rd(periph_t *const per, periph_t *const req,
                            const uint32_t addr, const uint32_t data)
{
    stic_t *const stic = PERIPH_PARENT_AS(stic_t, per);
    const uint64_t access_time = req && req->req ? req->req->now + 4 : 0;

    UNUSED(data);

    /* -------------------------------------------------------------------- */
    /*  Bring simulation up to 'now'.                                       */
    /* -------------------------------------------------------------------- */
    stic_simulate_until(stic, access_time);

    /* -------------------------------------------------------------------- */
    /*  Disallow access to graphics memory if in Bus Isolation.  System     */
    /*  Memory will return semi-predictable garbage from the graphics bus   */
    /*  during these reads.                                                 */
    /* -------------------------------------------------------------------- */
    if (!stic_is_gmem_accessible(access_time, stic))
    {
        if (stic->debug_flags & STIC_SHOW_RD_DROP)
        {
            const cp1600_t *const cpu = req && req->req ? (cp1600_t *)req->req
                                                        : NULL;
            const int pc = cpu ? cpu->r[7] : -1;

            jzp_printf("STIC GMEM RD drop: addr = $%.4X @%llu ",
                       addr + per->addr_base, access_time);

            if (pc >= 0 && pc <= 0xFFFF)
                jzp_printf("PC = $%.4X\n", pc);
            else
                jzp_printf("PC unknown\n");

            if ((stic->debug_flags & STIC_DBG_GMEM_ACCESS_WINDOW) == 0)
            {
/*
                jzp_printf("Prev STIC GMEM window ended @%llu",
                            stic->gmem_accessible);

                if (stic->req_bus->next_intrq != ~0ULL &&
                    stic->req_bus->next_intrq != 0)
                    jzp_printf("; next window approx @%llu\n",
                                stic->req_bus->next_intrq);
                else
                    jzp_printf("\n");
*/
                stic->debug_flags |= STIC_DBG_GMEM_ACCESS_WINDOW;
            }
        }

        return 0x3FFF & addr;
    }

    /* -------------------------------------------------------------------- */
    /*  If this is a GRAM address, adjust it for aliases.                   */
    /* -------------------------------------------------------------------- */
    const uint32_t grom_addr = addr;
    const uint32_t gram_addr = addr & (stic->gram_mask | 7);
    const uint32_t gmem_addr = addr & 0x800 ? gram_addr : grom_addr;

    /* -------------------------------------------------------------------- */
    /*  Return the data.                                                    */
    /* -------------------------------------------------------------------- */
    return stic->gmem[gmem_addr] & 0xFF;
}

/* ======================================================================== */
/*  STIC_GMEM_PEEK -- Like gmem_rd, except always works.                    */
/* ======================================================================== */
LOCAL uint32_t stic_gmem_peek(periph_t *const per, periph_t *const req,
                              const uint32_t addr, const uint32_t data)
{
    stic_t *const stic = PERIPH_PARENT_AS(stic_t, per);
    UNUSED(req);
    UNUSED(data);

    /* -------------------------------------------------------------------- */
    /*  If this is a GRAM address, adjust it for aliases.                   */
    /* -------------------------------------------------------------------- */
    const uint32_t grom_addr = addr;
    const uint32_t gram_addr = addr & (stic->gram_mask | 7);
    const uint32_t gmem_addr = addr & 0x800 ? gram_addr : grom_addr;

    /* -------------------------------------------------------------------- */
    /*  Return the data.                                                    */
    /* -------------------------------------------------------------------- */
    return stic->gmem[gmem_addr] & 0xFF;
}

/* ======================================================================== */
/*  STIC_DTOR    -- Tear down the STIC_T                                    */
/* ======================================================================== */
LOCAL void stic_dtor(periph_t *const p)
{
    stic_t *const stic = PERIPH_AS(stic_t, p);

    if (stic->demo)
        demo_dtor(stic->demo);
}

/* ======================================================================== */
/*  STIC_INIT    -- Initialize this ugly ass peripheral.  Booyah!           */
/* ======================================================================== */
int stic_init
(
    stic_t              *const stic,
    uint16_t            *const grom_img,
    req_q_t             *const req_q,
    gfx_t               *const gfx,
    demo_t              *const demo,
    const int            rand_mem,
    const int            pal_mode,
    const int            gram_size,
    const enum stic_type stic_type
)
{
    /* -------------------------------------------------------------------- */
    /*  First, zero out the STIC structure to get rid of anything that      */
    /*  might be dangling.                                                  */
    /* -------------------------------------------------------------------- */
    memset((void*)stic, 0, sizeof(stic_t));

    /* -------------------------------------------------------------------- */
    /*  PAL or NTSC?  8900 or STIC1A?                                       */
    /* -------------------------------------------------------------------- */
    stic->pal  = pal_mode;
    stic->type = stic_type;

    /* -------------------------------------------------------------------- */
    /*  Set our graphics subsystem pointers.                                */
    /* -------------------------------------------------------------------- */
    stic->gfx  = gfx;
    stic->disp = gfx->vid;

    /* -------------------------------------------------------------------- */
    /*  Register the demo recorder, if there is one.                        */
    /* -------------------------------------------------------------------- */
    stic->demo = demo;

    /* -------------------------------------------------------------------- */
    /*  Set the total GRAM size:                                            */
    /*      64 cards standard, 256 cards for INTV88 / TutorVision.          */
    /*      Or 128 cards if you're just having some fun.                    */
    /* -------------------------------------------------------------------- */
    stic->gram_size = gram_size > 2 ? 2
                    : gram_size < 0 ? 0
                    :                 gram_size;
    stic->gram_mask = stic->gram_size == 2 ? 0x0FF8
                    : stic->gram_size == 1 ? 0x0BF8
                    :                        0x09F8;

    /* -------------------------------------------------------------------- */
    /*  Initialize the bit/nibble expansion tables.                         */
    /* -------------------------------------------------------------------- */

    /*  Calculate bit-to-nibble masks b2n, b2n_r */
    for (int i = 0; i < 256; i++)
    {
        uint32_t b2n, b2n_r;
        b2n = b2n_r = 0;

        for (int j = 0; j < 8; j++)
            if ((i >> j) & 1)
            {
                b2n   |= 0xFu << (j * 4);
                b2n_r |= 0xFu << (28 - j*4);
            }

        stic_b2n_ [i] = b2n;
        stic_b2n_r[i] = b2n_r;
    }

    /* Calculate bit-to-byte masks b2n_d, b2n_rd */
    for (int i = 0; i < 16; i++)
    {
        uint32_t b2n_d, b2n_rd;
        b2n_d = b2n_rd = 0;

        for (int j = 0; j < 4; j++)
            if ((i >> j) & 1)
            {
                b2n_d  |= 0xFFu << (j * 8);
                b2n_rd |= 0xFFu << (24 - j*8);
            }

        stic_b2n_d [i] = b2n_d;
        stic_b2n_rd[i] = b2n_rd;
    }

    /* Calculate n2b */
    for (int i = 0; i < 16; i++)
        for (int j = 0; j < 16; j++)
            stic_n2b[16*j + i] = le_to_host_16((i << 8) | j);

    /* Calculate bit_r 8-bit bit-reverse table */
    for (int i = 0; i < 256; i++)
    {
        const uint32_t bit_r0 = i;
        const uint32_t bit_r1 = ((bit_r0 & 0xAA) >> 1) | ((bit_r0 & 0x55) << 1);
        const uint32_t bit_r2 = ((bit_r1 & 0xCC) >> 2) | ((bit_r1 & 0x33) << 2);
        const uint32_t bit_r3 = ((bit_r2 & 0xF0) >> 4) | ((bit_r2 & 0x0F) << 4);

        stic_bit  [i] = i      << 8;
        stic_bit_r[i] = bit_r3 << 8;
    }

    /* Calculate bit-doubling tables bit_d, bit_rd */
    for (int i = 0; i < 256; i++)
    {
        uint32_t bit_d = i, bit_rd = stic_bit_r[i] >> 8;

        for (int j = 7; j > 0; j--)
        {
            bit_d  += bit_d  & (~0U << j);
            bit_rd += bit_rd & (~0U << j);
        }

        stic_bit_d [i] = 3 * bit_d;
        stic_bit_rd[i] = 3 * bit_rd;
    }

    /* -------------------------------------------------------------------- */
    /*  Initialize graphics memory.                                         */
    /* -------------------------------------------------------------------- */
    for (int i = 0; i < 2048; i++)
        stic->gmem[i] = grom_img[i];

    if (rand_mem)
        for (int i = 2048; i < 4096; i++)
            stic->gmem[i] = (rand_jz() >> 24) & 0xFF;

    /* -------------------------------------------------------------------- */
    /*  Initialize registers.  This requires the stic_cr peripheral.        */
    /* -------------------------------------------------------------------- */
    stic->rand_mem = rand_mem;
    stic_reg_init(stic);

    /* -------------------------------------------------------------------- */
    /*  Set up our internal flags.                                          */
    /* -------------------------------------------------------------------- */
    stic->mode   = 0;
    stic->p_mode = 0;
    stic->upd    = stic_draw_cstk;

    /* -------------------------------------------------------------------- */
    /*  Record our INTRQ/BUSRQ request bus pointer.  Usually points us to   */
    /*  cp1600->req_q.                                                      */
    /* -------------------------------------------------------------------- */
    stic->req_q         = req_q;
    stic->req_q->ack    = stic_do_req_ack;
    stic->req_q->drop   = stic_do_req_drop;
    stic->req_q->opaque = (void *)stic;

    /* -------------------------------------------------------------------- */
    /*  Now, set up our peripheral functions for the main STIC peripheral.  */
    /* -------------------------------------------------------------------- */
    stic->stic_cr.read      = stic_ctrl_rd;
    stic->stic_cr.write     = stic_ctrl_wr;
    stic->stic_cr.peek      = stic_ctrl_peek;
    stic->stic_cr.poke      = stic_ctrl_poke;
    stic->stic_cr.tick      = stic_tick;
    stic->stic_cr.reset     = stic_reset;
    stic->stic_cr.dtor      = stic_dtor;
    stic->stic_cr.min_tick  = 1;
    stic->stic_cr.max_tick  = STIC_FRAMCLKS;
    stic->stic_cr.addr_base = 0x00000000;
    stic->stic_cr.addr_mask = 0x0000FFFF;
    stic->stic_cr.parent    = (void*) stic;

    stic_reset(&(stic->stic_cr));
    if (stic->debug_flags & STIC_DBG_REQS)
        jzp_printf("horizon = %" U64_FMT "\n", stic->req_q->horizon);
    /* -------------------------------------------------------------------- */
    /*  Lastly, set up the 'snooping' STIC peripherals.                     */
    /* -------------------------------------------------------------------- */
    stic->snoop_btab.read       = NULL;
    stic->snoop_btab.write      = stic_btab_wr;
    stic->snoop_btab.peek       = NULL;
    stic->snoop_btab.poke       = stic_btab_wr;
    stic->snoop_btab.tick       = NULL;
    stic->snoop_btab.min_tick   = ~0U;
    stic->snoop_btab.max_tick   = ~0U;
    stic->snoop_btab.addr_base  = 0x00000200;
    stic->snoop_btab.addr_mask  = 0x000000FF;
    stic->snoop_btab.parent     = (void*) stic;
    stic->snoop_btab.dtor       = NULL;

    stic->snoop_gram.read       = stic_gmem_rd;
    stic->snoop_gram.write      = stic_gmem_wr;
    stic->snoop_gram.peek       = stic_gmem_peek;
    stic->snoop_gram.poke       = stic_gmem_poke;
    stic->snoop_gram.tick       = NULL;
    stic->snoop_gram.min_tick   = ~0U;
    stic->snoop_gram.max_tick   = ~0U;
    stic->snoop_gram.addr_base  = 0x00003000;
    stic->snoop_gram.addr_mask  = 0x00003FFF;
    stic->snoop_gram.parent     = (void*) stic;
    stic->snoop_gram.dtor       = NULL;

    stic->alias_gram.read       = NULL;
    stic->alias_gram.write      = stic_gmem_wr;
    stic->alias_gram.peek       = NULL;
    stic->alias_gram.poke       = stic_gmem_poke;
    stic->alias_gram.tick       = NULL;
    stic->alias_gram.min_tick   = ~0U;
    stic->alias_gram.max_tick   = ~0U;
    stic->alias_gram.addr_base  = 0x00003000;
    stic->alias_gram.addr_mask  = 0x00003FFF;
    stic->alias_gram.parent     = (void*) stic;
    stic->alias_gram.dtor       = NULL;

    return 0;
}


/* ======================================================================== */
/*  STIC BACKTAB display list architecture:                                 */
/*                                                                          */
/*  There are two main BACKTAB renderers for the STIC:  DRAW_CSTK and       */
/*  DRAW_FGBG.  These correspond to the two primary STIC modes.  Each of    */
/*  these renderers produce a pair of display lists that feed into the      */
/*  rest of the STIC display computation.                                   */
/*                                                                          */
/*  The two lists correspond to the colors of the displayed pixels for      */
/*  each card, and the bitmap of "foreground vs. background" pixels for     */
/*  each card.  What's important to note about these lists is that they     */
/*  are in card order, and are not rasterized onto the 160x96 background    */
/*  yet.                                                                    */
/*                                                                          */
/*  The display color list stores a list of 32-bit ints, each containing    */
/*  8 4-bit pixels packed as nibbles within each word.  By packing pixels   */
/*  in this manner, pixel color computation becomes exceedingly efficient.  */
/*  Indeed, it's just a couple ANDs and an OR to merge foreground and       */
/*  background colors for an entire 8-pixel row of a card.                  */
/*                                                                          */
/*  The foreground bitmap list stores a list of bytes, each containing      */
/*  bits indicating which pixels are foreground and which pixels are        */
/*  background.  A '1' bit in this bitmap indicates a foreground pixel.     */
/*  This secondary bitmap will be used to compute MOB collisions later,     */
/*  using nice simple bitwise ANDs to detect coincidence.                   */
/*                                                                          */
/*  The two lists are stored as lists of cards to limit the amount of       */
/*  addressing work that the display computation loops must do.  By         */
/*  tightly limiting the focus of these loops and by constructing a nice    */
/*  linear output pattern, this code should be fairly efficient.            */
/*                                                                          */
/*  In addition to the two display lists, the BACKTAB renderers produce     */
/*  a third, short list containing the "last background color" associated   */
/*  with each row of cards.  This information will be used to render the    */
/*  pixels to the left and above the BACKTAB image later in the engine.     */
/*                                                                          */
/*  These lists will feed into a unified render engine which will merge     */
/*  the MOB images and the BACKTAB image into the final frame buffer.       */
/* ======================================================================== */


/* ======================================================================== */
/*  STIC_DO_MOB -- Render a given MOB.                                      */
/* ======================================================================== */
LOCAL void stic_do_mob(stic_t *const stic, const int mob)
{
    uint32_t *const RESTRICT mob_img = stic->mob_img;
    uint16_t *const RESTRICT mob_bmp = stic->mob_bmp[mob];

    /* -------------------------------------------------------------------- */
    /*  Grab the MOB's information.                                         */
    /* -------------------------------------------------------------------- */
    const uint32_t x_reg = stic->raw[mob + 0x00];
    const uint32_t y_reg = stic->raw[mob + 0x08];
    const uint32_t a_reg = stic->raw[mob + 0x10];

    /* -------------------------------------------------------------------- */
    /*  Decode the various control bits from the MOB's registers.           */
    /* -------------------------------------------------------------------- */
    const uint32_t fg_clr = ((a_reg >> 9) & 0x08) | (a_reg & 0x07);
    const uint32_t fg_msk = stic_color_mask[fg_clr];

    const int x_size = x_reg & 0x0400;
    const int x_flip = y_reg & 0x0400;
    const uint16_t *const RESTRICT bit_remap =    /* --- double width --- */
                                                  /* x-flip */  /* normal */
                                x_size ? (x_flip ? stic_bit_rd : stic_bit_d)
                                                  /* --- single width --- */
                                                  /* x-flip */  /* normal */
                                       : (x_flip ? stic_bit_r  : stic_bit  );

    const int y_res  = y_reg & 0x80 ? 16 : 8;
    const int y_flip = y_reg & 0x0800 ? y_res - 1 : 0; /* y-flip vs. normal */

    /* -------------------------------------------------------------------- */
    /*  Decode the GROM/GRAM index.  Bits 9 and 10 are ignored if the card  */
    /*  is from GRAM, or if the display is in Foreground/Background mode.   */
    /* -------------------------------------------------------------------- */

    /* FG/BG 64 card limit, except on STIC1A. */
    const uint32_t mask_fgbg =
        stic->mode == 1 && stic->type != STIC_STIC1A ? 0x9F8 : ~0U;

    /* GRAM limited to 64/128/256 cards. */
    const uint32_t mask_gram = a_reg & 0x800 ? stic->gram_mask : ~0U;

    /* Double-height MOBs always start on an even card number. */
    const uint32_t mask_dhgt = y_res == 16 ? 0xFF0 : ~0U;

    /* Extract address applying all masks */
    const uint32_t gr_idx = a_reg & 0xFF8 & mask_fgbg & mask_gram & mask_dhgt;

    /* -------------------------------------------------------------------- */
    /*  Generate the MOB's bitmap from its color and GRAM/GROM image.       */
    /*  Each MOB is generated to a 16x16 bitmap, regardless of its actual   */
    /*  size.  We handle x-flip, y-flip and x-size here.  We handle y-size  */
    /*  later when compositing the MOBs into a single bitmap.               */
    /* -------------------------------------------------------------------- */
    for (int y = 0; y < y_res; y++)
    {
        const uint32_t row = stic->gmem[gr_idx + y];
        const uint16_t bit = bit_remap[row];
        const uint32_t lpix = stic_b2n(bit >> 8  ) & fg_msk;
        const uint32_t rpix = stic_b2n(bit & 0xFF) & fg_msk;

        const int yy = y ^ y_flip;

        mob_img[2*yy + 0] = lpix;
        mob_img[2*yy + 1] = rpix;
        mob_bmp[yy]       = bit;
    }

    for (int y = y_res; y < 16; y++)
    {
        mob_img[2*y + 0] = 0;
        mob_img[2*y + 1] = 0;
        mob_bmp[y]       = 0;
    }
}

/* ======================================================================== */
/*  STIC_DRAW_MOBS -- Draw all 8 MOBs onto the 256x96 bitplane.             */
/* ======================================================================== */
LOCAL void stic_draw_mobs(stic_t *stic)
{
    uint32_t *const RESTRICT mpl_img = stic->mpl_img;
    uint32_t *const RESTRICT mpl_pri = stic->mpl_pri;
    uint32_t *const RESTRICT mpl_vsb = stic->mpl_vsb;
    uint32_t *const RESTRICT mob_img = stic->mob_img;

    /* -------------------------------------------------------------------- */
    /*  First, clear the MOB plane.  We only need to clear the visibility   */
    /*  and priority bits, not the color plane.  This is because we ignore  */
    /*  the contents of the color plane wherever the visibility bit is 0.   */
    /* -------------------------------------------------------------------- */
    memset(mpl_pri, 0, 192 * 224 / 8);
    memset(mpl_vsb, 0, 192 * 224 / 8);

    /* -------------------------------------------------------------------- */
    /*  Generate the bitmaps for the 8 MOBs if they're active, and put      */
    /*  together the MOB plane.                                             */
    /* -------------------------------------------------------------------- */
    for (int i = 7; i >= 0; i--)
    {
        const uint32_t x_reg = stic->raw[i + 0x00];
        const uint32_t y_reg = stic->raw[i + 0x08];
        const uint32_t x_pos =  x_reg & 0xFF;
        const uint32_t y_pos = (y_reg & 0x7F) * 2;
        const uint32_t prio  = (stic->raw[i + 0x10] & 0x2000) ? ~0U : 0;
        const uint32_t visb  = x_reg & 0x200;
        const uint32_t y_shf = (y_reg >> 8) & 3;
        const int      y_stp = 1u << y_shf;
        const int      y_hgt = stic_mob_hgt[(y_reg >> 7) & 7];
        const uint32_t x_rad = (x_pos & 7) * 4;
        const uint32_t x_lad = 32 - x_rad;
        const uint32_t x_ofs = (x_pos >> 3);
        const uint32_t x_ofb = (x_pos & 31);
        const uint16_t *const RESTRICT mob_bmp = stic->mob_bmp[i];

        /* ---------------------------------------------------------------- */
        /*  Compute bounding box for MOB and tell gfx about it.  We can     */
        /*  use this information to draw debug boxes around MOBs and other  */
        /*  nice things.  Bounding box is inclusive on all four edges.      */
        /* ---------------------------------------------------------------- */
        stic->gfx->bbox[i][0] = x_pos;
        stic->gfx->bbox[i][1] = y_pos;
        stic->gfx->bbox[i][2] = x_pos + (x_reg & 0x400 ? 15 : 7);
        stic->gfx->bbox[i][3] = y_pos + y_hgt - 1;

        /* ---------------------------------------------------------------- */
        /*  Skip this MOB if it is off-screen or is both not-visible and    */
        /*  non-interacting.                                                */
        /* ---------------------------------------------------------------- */
        if (x_pos == 0 || x_pos >= 167 || (x_reg & 0x300) == 0 ||
            y_pos >= 208)
            continue;

        /* ---------------------------------------------------------------- */
        /*  Generate the bitmap information for this MOB.                   */
        /* ---------------------------------------------------------------- */
        stic_do_mob(stic, i);

        /* ---------------------------------------------------------------- */
        /*  If this MOB is visible, put it into the color display image.    */
        /* ---------------------------------------------------------------- */
        if (!visb || stic_dropping_this_frame(stic))
            continue;

        int y_res = y_reg & 0x80 ? 16 : 8;

        /* Clip to bottom of the screen. */
        if (y_pos + (y_res << y_shf) > 208)
            y_res = (207 - y_pos + (1u << y_shf)) >> y_shf;

        for (int y = 0; y < y_res; y++)
        {
            uint32_t l_pix, m_pix, r_pix;   /* colors for 16-pixel MOB      */
            uint32_t l_msk, m_msk, r_msk;   /* visibility masks             */
            uint32_t l_bmp, r_bmp;          /* 1-bpp bitmaps of MOB         */

            /* ------------------------------------------------------------ */
            /*  Get the 4-bpp images of the MOB into l_pix, m_pix.          */
            /* ------------------------------------------------------------ */
            l_pix = mob_img[2*y + 0];
            m_pix = mob_img[2*y + 1];
            l_msk = stic_b2n(mob_bmp[y] >> 8  );
            m_msk = stic_b2n(mob_bmp[y] & 0xFF);

            /* ------------------------------------------------------------ */
            /*  Shift these right according to the X position of MOB.       */
            /*  A 16-pixel wide MOB will straddle up to 3 32-bit words      */
            /*  at 4-bpp.  (x_rad == "x position right adjust")             */
            /* ------------------------------------------------------------ */
            if (x_rad)
            {
                r_pix = (m_pix << x_lad);
                m_pix = (l_pix << x_lad) | (m_pix >> x_rad);
                l_pix =                    (l_pix >> x_rad);
                r_msk = (m_msk << x_lad);
                m_msk = (l_msk << x_lad) | (m_msk >> x_rad);
                l_msk =                    (l_msk >> x_rad);
            } else
            {
                r_pix = 0;
                r_msk = 0;
            }

            /* ------------------------------------------------------------ */
            /*  Similarly, shift the 1-bpp masks right according to the X   */
            /*  position of the MOB.                                        */
            /* ------------------------------------------------------------ */
            if (x_ofb <= 16)
            {
                l_bmp = (uint32_t)mob_bmp[y] << (16 - x_ofb);
                r_bmp = 0;
            } else if (x_ofb <= 32)
            {
                l_bmp = (uint32_t)mob_bmp[y] >> (x_ofb - 16);
                r_bmp = (uint32_t)mob_bmp[y] << (48 - x_ofb);
            } else
            {
                l_bmp = 0;
                r_bmp = (uint32_t)mob_bmp[y] << (48 - x_ofb);
            }

            /* ------------------------------------------------------------ */
            /*  Take the computed values and merge them into the image.     */
            /*  This is where we replicate pixels to account for y-size.    */
            /*  b_idx == index into BMP; v_idx == index into VISB.          */
            /* ------------------------------------------------------------ */
            int b_idx = (y_pos + (y << y_shf)) * 24 + x_ofs;
            int v_idx = b_idx >> 2;
            for (int j = 0; j < y_stp; j++, b_idx += 24, v_idx += 6)
            {
                /* -------------------------------------------------------- */
                /*  Now, merge the colors into the MOB color plane.         */
                /* -------------------------------------------------------- */

                /* Previous pixel colors from the display. */
                const uint32_t l_old = mpl_img[b_idx + 0];
                const uint32_t m_old = mpl_img[b_idx + 1];
                const uint32_t r_old = mpl_img[b_idx + 2];

                /* Display with MOB pixels merged in. */
                const uint32_t l_new = (l_old & ~l_msk) | (l_pix & l_msk);
                const uint32_t m_new = (m_old & ~m_msk) | (m_pix & m_msk);
                const uint32_t r_new = (r_old & ~r_msk) | (r_pix & r_msk);

                mpl_img[b_idx + 0] = l_new;
                mpl_img[b_idx + 1] = m_new;
                mpl_img[b_idx + 2] = r_new;

                /* -------------------------------------------------------- */
                /*  Next, set the MOB visibility bits and priority bits in  */
                /*  the corresponding 1-bpp bitmaps.                        */
                /* -------------------------------------------------------- */
                mpl_vsb[v_idx + 0] |= l_bmp;
                mpl_vsb[v_idx + 1] |= r_bmp;

                const uint32_t old_l_pri = mpl_pri[v_idx + 0];
                const uint32_t old_r_pri = mpl_pri[v_idx + 1];
                const uint32_t new_l_pri = (old_l_pri & ~l_bmp)|(prio & l_bmp);
                const uint32_t new_r_pri = (old_r_pri & ~r_bmp)|(prio & r_bmp);

                mpl_pri[v_idx + 0] = new_l_pri;
                mpl_pri[v_idx + 1] = new_r_pri;
            }
        }
    }
}

/* ======================================================================== */
/*  STIC_DRAW_CSTK -- Draw the 160x96 backtab image into a display list.    */
/* ======================================================================== */
LOCAL void stic_draw_cstk(stic_t *stic)
{
    int r, c;               /*  Current row, column into backtab.           */
    int bt, bti, btl;       /*  Index into the BACKTAB.                     */
    int cs_idx;             /*  Current color-stack position                */
    int cstk[4];            /*  The color stack.                            */
    uint32_t bg_msk;        /*  Background color mask from color-stack.     */
    const uint16_t *const RESTRICT btab = stic->btab;
    uint32_t *const RESTRICT bt_img = stic->xbt_img;
    uint8_t  *const RESTRICT bt_bmp = stic->bt_bmp;

    /* -------------------------------------------------------------------- */
    /*  Read out the color-stack color values.                              */
    /* -------------------------------------------------------------------- */
    cstk[0] = stic_color_mask[stic->raw[0x28] & 0xF];
    cstk[1] = stic_color_mask[stic->raw[0x29] & 0xF];
    cstk[2] = stic_color_mask[stic->raw[0x2A] & 0xF];
    cstk[3] = stic_color_mask[stic->raw[0x2B] & 0xF];

    cs_idx = 0;
    bg_msk = cstk[0];
    /* -------------------------------------------------------------------- */
    /*  Step by rows and columns filling tiles.                             */
    /* -------------------------------------------------------------------- */
    for (bti = 8*24 + 1, bt = btl = r = c = 0; bt < 240; bt++, bti++, btl += 8)
    {
        /* ---------------------------------------------------------------- */
        /*  For each tile, do the following:                                */
        /*   -- If it's colored-squares, render as colored squares,         */
        /*      and move to the next tile.                                  */
        /*   -- If it's a color-stack advance, advance the color stack.     */
        /*   -- If we haven't rendered the tile yet, render it.             */
        /* ---------------------------------------------------------------- */
        const uint32_t card = btab[bt];

        /* ---------------------------------------------------------------- */
        /*  Handle colored-squares cards.                                   */
        /* ---------------------------------------------------------------- */
        if ((card & 0x1800) == 0x1000)
        {
            /* ------------------------------------------------------------ */
            /*                                                              */
            /*    13  12  11  10   9   8   7   6   5   4   3   2   1   0    */
            /*  +----+---+---+---+---+---+---+---+---+---+---+---+---+---+  */
            /*  |Pix3| 1   0 |Pix. 3 |Pix 2 color|Pix 1 color|Pix 0 color|  */
            /*  |Bit2|       |bit 0-1|   (0-7)   |   (0-7)   |   (0-7)   |  */
            /*  +----+---+---+---+---+---+---+---+---+---+---+---+---+---+  */
            /*                                                              */
            /*  The four pixels are displayed within an 8x8 card like so:   */
            /*                                                              */
            /*                       +-----+-----+                          */
            /*                       |Pixel|Pixel|                          */
            /*                       |  0  |  1  |                          */
            /*                       +-----+-----+                          */
            /*                       |Pixel|Pixel|                          */
            /*                       |  2  |  3  |                          */
            /*                       +-----+-----+                          */
            /*                                                              */
            /*  Notes:                                                      */
            /*                                                              */
            /*   -- Colors 0 through 6 display directly from the Primary    */
            /*      Color Set.                                              */
            /*                                                              */
            /*   -- Color 7 actually displays the current color on the top  */
            /*      of the color-stack.                                     */
            /*                                                              */
            /*   -- Colors 0 through 6 behave as "on" pixels that will      */
            /*      interact with MOBs.  Color 7 behaves as "off" pixels    */
            /*      and does not interact with MOBs.                        */
            /* ------------------------------------------------------------ */
            uint32_t csq0 =  (card >> 0) & 7;
            uint32_t csq1 =  (card >> 3) & 7;
            uint32_t csq2 =  (card >> 6) & 7;
            uint32_t csq3 = ((card >> 9) & 3) | ((card >> 11) & 4);

            uint32_t bmp_top = 0xFF, bmp_bot = 0xFF;

            if (csq0 == 7) { csq0 = bg_msk & 0xF; bmp_top &= 0x0F; }
            if (csq1 == 7) { csq1 = bg_msk & 0xF; bmp_top &= 0xF0; }
            if (csq2 == 7) { csq2 = bg_msk & 0xF; bmp_bot &= 0x0F; }
            if (csq3 == 7) { csq3 = bg_msk & 0xF; bmp_bot &= 0xF0; }

            const uint32_t csq0_color = stic_color_mask[csq0];
            const uint32_t csq1_color = stic_color_mask[csq1];
            const uint32_t csq2_color = stic_color_mask[csq2];
            const uint32_t csq3_color = stic_color_mask[csq3];

            const uint32_t csq_top = (0xFFFF0000u & csq0_color)
                                  | (0x0000FFFFu & csq1_color);
            const uint32_t csq_bot = (0xFFFF0000u & csq2_color)
                                  | (0x0000FFFFu & csq3_color);

            bt_img[bti + 0*24] = csq_top;
            bt_img[bti + 1*24] = csq_top;
            bt_img[bti + 2*24] = csq_top;
            bt_img[bti + 3*24] = csq_top;
            bt_img[bti + 4*24] = csq_bot;
            bt_img[bti + 5*24] = csq_bot;
            bt_img[bti + 6*24] = csq_bot;
            bt_img[bti + 7*24] = csq_bot;

            bt_bmp[btl + 0] = bmp_top;
            bt_bmp[btl + 1] = bmp_top;
            bt_bmp[btl + 2] = bmp_top;
            bt_bmp[btl + 3] = bmp_top;
            bt_bmp[btl + 4] = bmp_bot;
            bt_bmp[btl + 5] = bmp_bot;
            bt_bmp[btl + 6] = bmp_bot;
            bt_bmp[btl + 7] = bmp_bot;

            /* ------------------------------------------------------------ */
            /*  Skip remainder of processing for this block since the       */
            /*  colored square mode is a special case.                      */
            /* ------------------------------------------------------------ */
            if (c++ == 19) { c=0; stic->last_bg[r++] = bg_msk; bti += 8*24-20; }
            continue;
        }

        /* ---------------------------------------------------------------- */
        /*  The color stack advances when bit 13 is one.                    */
        /* ---------------------------------------------------------------- */
        if (card & 0x2000)
        {
            cs_idx = (cs_idx + 1) & 3;
            bg_msk = cstk[cs_idx];
        }

        /* ---------------------------------------------------------------- */
        /*  Extract the GROM/GRAM index from bits 11..3.  If the card is    */
        /*  from GRAM, potentially ignore bits 10..9 (gram_mask).           */
        /* ---------------------------------------------------------------- */
        const uint32_t gr_idx = (card & 0xFF8)
                              & (card & 0x800 ? stic->gram_mask : ~0U);

        /* ---------------------------------------------------------------- */
        /*  The foreground color comes from bits 12 and 2..0.               */
        /* ---------------------------------------------------------------- */
        const int      fg_clr = ((card >> 9) & 0x8) | (card & 7);
        const uint32_t fg_msk = stic_color_mask[fg_clr];

        /* ---------------------------------------------------------------- */
        /*  Now blit the bits into the packed-nibble display list.          */
        /* ---------------------------------------------------------------- */
        for (int yy = 0; yy < 8; yy++)
        {
            const uint32_t px_bmp = stic->gmem[gr_idx + yy];
            const uint32_t px_msk = stic_b2n(px_bmp);

            bt_bmp[btl + yy   ] = px_bmp;
            bt_img[bti + yy*24] = (fg_msk & px_msk) | (bg_msk & ~px_msk);
        }

        /* ---------------------------------------------------------------- */
        /*  Advance row, column counters.                                   */
        /* ---------------------------------------------------------------- */
        if (c++ == 19) { c = 0; stic->last_bg[r++] = bg_msk; bti += 8*24-20; }
    }
}

/* ======================================================================== */
/*  STIC_DRAW_FGBG -- Draw the 160x96 backtab image into a display list.    */
/* ======================================================================== */
LOCAL void stic_draw_fgbg(stic_t *stic)
{
    int r, c;               /* current row, column into backtab.            */
    int bt, btl, bti;       /* Index into the BACKTAB.                      */
    const uint16_t *const RESTRICT btab = stic->btab;
    uint32_t *const RESTRICT bt_img = stic->xbt_img;
    uint8_t  *const RESTRICT bt_bmp = stic->bt_bmp;

    /* -------------------------------------------------------------------- */
    /*  Step by rows and columns filling tiles.                             */
    /* -------------------------------------------------------------------- */
    for (bti = 8*24 + 1, bt = btl = r = c = 0; bt < 240; bt++, bti++, btl += 8)
    {
        /* ---------------------------------------------------------------- */
        /*  For each tile, do the following:                                */
        /*   -- Extract foreground, background and card number.             */
        /*   -- Draw it.                                                    */
        /* ---------------------------------------------------------------- */
        const uint32_t card = btab[bt];

        /* ---------------------------------------------------------------- */
        /*  The GRAM/GROM index comes from bit 11 and bits 8..3.            */
        /* ---------------------------------------------------------------- */
        const uint32_t gr_idx = card & 0x9F8;

        /* ---------------------------------------------------------------- */
        /*  The foreground color comes from bits 2..0.                      */
        /*  The background color comes from bit 12, 13, 10 and 9, in that   */
        /*  annoying order.  At least bits 3, 1, and 0 are right.           */
        /* ---------------------------------------------------------------- */
        const int fg_clr = (card & 7);
        const int bg_clr = ((card >> 9) & 0xB) | ((card >> 11) & 0x4);

        /* ---------------------------------------------------------------- */
        /*  Convert colors to color masks.                                  */
        /* ---------------------------------------------------------------- */
        const uint32_t fg_msk = stic_color_mask[fg_clr];
        const uint32_t bg_msk = stic_color_mask[bg_clr];

        /* ---------------------------------------------------------------- */
        /*  Now blit the bits into the packed-nibble display list.          */
        /* ---------------------------------------------------------------- */
        for (int yy = 0; yy < 8; yy++)
        {
            const uint32_t px_bmp = stic->gmem[gr_idx + yy];
            const uint32_t px_msk = stic_b2n(px_bmp);

            bt_bmp[btl + yy   ] = px_bmp;
            bt_img[bti + yy*24] = (fg_msk & px_msk) | (bg_msk & ~px_msk);
        }

        /* ---------------------------------------------------------------- */
        /*  Advance row, column counters.                                   */
        /* ---------------------------------------------------------------- */
        if (c++ == 19) { c = 0; stic->last_bg[r++] = bg_msk; bti += 8*24-20; }
    }
}

/* ======================================================================== */
/*  STIC_FIX_BORD -- Trim the display list and MOB image to 159 columns.    */
/* ======================================================================== */
LOCAL void stic_fix_bord(stic_t *stic)
{
    uint32_t *const RESTRICT mpl_vsb = stic->mpl_vsb;
    const int h_dly = stic->raw[0x30] & 7;

    /* -------------------------------------------------------------------- */
    /*  STIC1A appears to output full 160x192 image.  Full impact not yet   */
    /*  understood.  For now, just skip this routine on STIC1A.             */
    /* -------------------------------------------------------------------- */
    if (stic->type == STIC_STIC1A)
        return;

#if 0
    /* -------------------------------------------------------------------- */
    /*  Make sure column 159 holds nothing interesting, and has no bg bits  */
    /*  beyond column 160 to cause false collisions.                        */
    /* -------------------------------------------------------------------- */
    n_msk = 0xFFFFFFF0 << (h_dly * 4);
    b_msk = 0xFFFFFFFE << (h_dly);
    bord  = stic_color_mask[bord] & ~n_msk;

    for (i = 0; i < 12; i++)
    {
        for (j = 0; j < 8; j++)
        {
            uint8_t  new_bmp =  bt_bmp[19*8 + 20*8*i + j] & b_msk;
            bt_bmp[19*8 + 20*8*i + j] = new_bmp;
        }
    }
#endif

    /* -------------------------------------------------------------------- */
    /*  We do the same for the MOBs, but we do this to column 167, because  */
    /*  the MOB bitmap starts 8 pixels to the left of the backtab bitmap.   */
    /* -------------------------------------------------------------------- */
    const uint32_t b_msk = 0xFE000000 << (h_dly);
    for (int i = 0; i < 224; i++)
        mpl_vsb[i*6 + 5] &= b_msk;

    /* -------------------------------------------------------------------- */
    /*  Trim the MOB collision bitmaps for left, right edges.               */
    /* -------------------------------------------------------------------- */
    for (int i = 0; i < 8; i++)
    {
        const uint32_t x      = (stic->raw[i + 0x00] & 0xFF) + h_dly;
        const int      l_edge = x < 8;
        const int      r_edge = x > 150 && x <= 167;
        const uint32_t le_msk = l_edge ? 0xFFFFFE00 <<  x        : 0;
        const uint32_t re_msk = r_edge ? 0x00007FFF >> (167 - x) : 0;
        const uint32_t msk    = ~(le_msk | re_msk);

        for (int j = 0; j < 16; j++)
            stic->mob_bmp[i][j] &= msk;
    }
}

/* ======================================================================== */
/*  STIC_MERGE_PLANES -- Merge MOB and BACKTAB planes.                      */
/* ======================================================================== */
LOCAL void stic_merge_planes(stic_t *stic)
{
    uint32_t *const RESTRICT image   = stic->image;
    /*uint32_t *const RESTRICT bt_img  = stic->bt_img;*/
    uint8_t  *const RESTRICT bt_bmp  = stic->bt_bmp;
    uint32_t *const RESTRICT xbt_img = stic->xbt_img;
    uint32_t *const RESTRICT xbt_bmp = stic->xbt_bmp;
    uint32_t *const RESTRICT mpl_img = stic->mpl_img;
    uint32_t *const RESTRICT mpl_vsb = stic->mpl_vsb;
    uint32_t *const RESTRICT mpl_pri = stic->mpl_pri;
    const uint32_t bord = stic_color_mask[stic->raw[0x2C] & 0xF];

#if 0
    /* -------------------------------------------------------------------- */
    /*  First, re-tile the backtab display list.  That should be quick.     */
    /* -------------------------------------------------------------------- */
    for (bt = r = 0, ri = 1 + 8*24; r < 12; r++, ri += 8*24)
    {
        for (c = 0; c < 20; c++, bt++)
        {
            uint32_t img0 = bt_img[bt*8 + 0];
            uint32_t img1 = bt_img[bt*8 + 1];
            uint32_t img2 = bt_img[bt*8 + 2];
            uint32_t img3 = bt_img[bt*8 + 3];
            uint32_t img4 = bt_img[bt*8 + 4];
            uint32_t img5 = bt_img[bt*8 + 5];
            uint32_t img6 = bt_img[bt*8 + 6];
            uint32_t img7 = bt_img[bt*8 + 7];

            xbt_img[ri + c + 0*24] = img0;
            xbt_img[ri + c + 1*24] = img1;
            xbt_img[ri + c + 2*24] = img2;
            xbt_img[ri + c + 3*24] = img3;
            xbt_img[ri + c + 4*24] = img4;
            xbt_img[ri + c + 5*24] = img5;
            xbt_img[ri + c + 6*24] = img6;
            xbt_img[ri + c + 7*24] = img7;
        }
    }
#endif

    /* -------------------------------------------------------------------- */
    /*  Retile the fg/bg bitmap too.                                        */
    /* -------------------------------------------------------------------- */
    for (int bt = 0, r = 0, ri = 8*6; r < 12; r++, ri += 8*6, bt += 20)
    {
        for (int y = 0; y < 8; y++)
        {
            const uint32_t bmp0 = ((uint32_t)bt_bmp[bt*8 +  0*8 + y] << 16) |
                                  ((uint32_t)bt_bmp[bt*8 +  1*8 + y] <<  8) |
                                  ((uint32_t)bt_bmp[bt*8 +  2*8 + y]      );
            const uint32_t bmp1 = ((uint32_t)bt_bmp[bt*8 +  3*8 + y] << 24) |
                                  ((uint32_t)bt_bmp[bt*8 +  4*8 + y] << 16) |
                                  ((uint32_t)bt_bmp[bt*8 +  5*8 + y] <<  8) |
                                  ((uint32_t)bt_bmp[bt*8 +  6*8 + y]      );
            const uint32_t bmp2 = ((uint32_t)bt_bmp[bt*8 +  7*8 + y] << 24) |
                                  ((uint32_t)bt_bmp[bt*8 +  8*8 + y] << 16) |
                                  ((uint32_t)bt_bmp[bt*8 +  9*8 + y] <<  8) |
                                  ((uint32_t)bt_bmp[bt*8 + 10*8 + y]      );
            const uint32_t bmp3 = ((uint32_t)bt_bmp[bt*8 + 11*8 + y] << 24) |
                                  ((uint32_t)bt_bmp[bt*8 + 12*8 + y] << 16) |
                                  ((uint32_t)bt_bmp[bt*8 + 13*8 + y] <<  8) |
                                  ((uint32_t)bt_bmp[bt*8 + 14*8 + y]      );
            const uint32_t bmp4 = ((uint32_t)bt_bmp[bt*8 + 15*8 + y] << 24) |
                                  ((uint32_t)bt_bmp[bt*8 + 16*8 + y] << 16) |
                                  ((uint32_t)bt_bmp[bt*8 + 17*8 + y] <<  8) |
                                  ((uint32_t)bt_bmp[bt*8 + 18*8 + y]      );
            const uint32_t bmp5 = ((uint32_t)bt_bmp[bt*8 + 19*8 + y] << 24);

            xbt_bmp[ri + 0 + y*6] = bmp0;
            xbt_bmp[ri + 1 + y*6] = bmp1;
            xbt_bmp[ri + 2 + y*6] = bmp2;
            xbt_bmp[ri + 3 + y*6] = bmp3;
            xbt_bmp[ri + 4 + y*6] = bmp4;
            xbt_bmp[ri + 5 + y*6] = bmp5;
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Stop here if we're dropping the frame.                              */
    /* -------------------------------------------------------------------- */
    if (stic_dropping_this_frame(stic))
        return;

    /* -------------------------------------------------------------------- */
    /*  Fill in colors based on "last_bg" along the top and left.           */
    /* -------------------------------------------------------------------- */
    const int h_dly   = (stic->raw[0x30] & 7);
    const int v_dly   = (stic->raw[0x31] & 7);
    const int img_ofs = v_dly*2 * 24;

    memset(xbt_img, stic->last_bg[11], (8*192 + 8)/2);
    {
        const uint32_t h_msk = 0xfffffff0 << (h_dly * 4);
        const uint32_t h_fix = (stic->last_bg[11] & h_msk) | (bord & ~h_msk);

        for (int y = 0, ri = 20; y < 8; y++, ri += 24)
            xbt_img[ri] = h_fix;
    }

    for (int r = 0, ri = 9*24; r < 12; r++)
    {
        const uint32_t bg_clr = stic->last_bg[r];

        for (int y = 0; y < 8; y++, ri += 24)
            xbt_img[ri] = bg_clr;
    }


    /* -------------------------------------------------------------------- */
    /*  Now channel between the mob and backtab images into the final       */
    /*  display image.  We also account for h_dly/v_dly here.  The vert     */
    /*  delay is really cheap -- we just draw further down the screen.      */
    /*  the horz delay isn't quite so cheap but is still not terribly       */
    /*  expensive.  We shift the pixels right as a huge extended-precision  */
    /*  right shift.                                                        */
    /* -------------------------------------------------------------------- */

    if (h_dly == 0)
    {
        const int len = 208 - v_dly*2;

        for (int r = 0, img_idx = 0, bmp_idx = 0, bti_idx = 0, btb_idx = 0;
             r < len; r++, img_idx += 24, bmp_idx += 6)
        {
            for (int c = 0, cc = 0; c < 24; c += 4, cc++)
            {
                const uint32_t btab_0 = xbt_img[bti_idx + c + 0];
                const uint32_t btab_1 = xbt_img[bti_idx + c + 1];
                const uint32_t btab_2 = xbt_img[bti_idx + c + 2];
                const uint32_t btab_3 = xbt_img[bti_idx + c + 3];

                const uint32_t mobs_0 = mpl_img[img_idx + c + 0];
                const uint32_t mobs_1 = mpl_img[img_idx + c + 1];
                const uint32_t mobs_2 = mpl_img[img_idx + c + 2];
                const uint32_t mobs_3 = mpl_img[img_idx + c + 3];

                const uint32_t bt_msk = xbt_bmp[btb_idx + cc];
                const uint32_t vs_msk = mpl_vsb[bmp_idx + cc];
                const uint32_t pr_msk = mpl_pri[bmp_idx + cc];
                const uint32_t mb_msk = vs_msk & ~(pr_msk & bt_msk);

                const uint32_t mask_0 = stic_b2n((mb_msk >> 24)       );
                const uint32_t mask_1 = stic_b2n((mb_msk >> 16) & 0xFF);
                const uint32_t mask_2 = stic_b2n((mb_msk >>  8) & 0xFF);
                const uint32_t mask_3 = stic_b2n((mb_msk      ) & 0xFF);

                const uint32_t img_0  = (mobs_0 & mask_0) | (btab_0 & ~mask_0);
                const uint32_t img_1  = (mobs_1 & mask_1) | (btab_1 & ~mask_1);
                const uint32_t img_2  = (mobs_2 & mask_2) | (btab_2 & ~mask_2);
                const uint32_t img_3  = (mobs_3 & mask_3) | (btab_3 & ~mask_3);

                image[img_idx + c + 0 + img_ofs] = img_0;
                image[img_idx + c + 1 + img_ofs] = img_1;
                image[img_idx + c + 2 + img_ofs] = img_2;
                image[img_idx + c + 3 + img_ofs] = img_3;
            }

            if (r & 1) { bti_idx += 24; btb_idx += 6; }
            else       { xbt_img[bti_idx] = xbt_img[bti_idx + 24]; }
        }
    } else
    {
        const int r_shf = h_dly * 4;
        const int l_shf = 32 - r_shf;
        const int len   = 208 - v_dly*2;

        for (int r = 0, img_idx = 0, bmp_idx = 0, bti_idx = 0, btb_idx = 0;
             r < len; r++, img_idx += 24, bmp_idx += 6)
        {
            uint32_t pimg_3 = 0;  /* extending on left w/ 0 is ok */

#if 1
            for (int c = 0, cc = 0; c < 24; c += 4, cc++)
            {
                const uint32_t btab_0 = xbt_img[bti_idx + c + 0];
                const uint32_t btab_1 = xbt_img[bti_idx + c + 1];
                const uint32_t btab_2 = xbt_img[bti_idx + c + 2];
                const uint32_t btab_3 = xbt_img[bti_idx + c + 3];

                const uint32_t mobs_0 = mpl_img[img_idx + c + 0];
                const uint32_t mobs_1 = mpl_img[img_idx + c + 1];
                const uint32_t mobs_2 = mpl_img[img_idx + c + 2];
                const uint32_t mobs_3 = mpl_img[img_idx + c + 3];

                const uint32_t bt_msk = xbt_bmp[btb_idx + cc];
                const uint32_t vs_msk = mpl_vsb[bmp_idx + cc];
                const uint32_t pr_msk = mpl_pri[bmp_idx + cc];
                const uint32_t mb_msk = vs_msk & ~(pr_msk & bt_msk);

                const uint32_t mask_0 = stic_b2n((mb_msk >> 24)       );
                const uint32_t mask_1 = stic_b2n((mb_msk >> 16) & 0xFF);
                const uint32_t mask_2 = stic_b2n((mb_msk >>  8) & 0xFF);
                const uint32_t mask_3 = stic_b2n((mb_msk      ) & 0xFF);

                const uint32_t ximg_0 = (mobs_0 & mask_0) | (btab_0 & ~mask_0);
                const uint32_t ximg_1 = (mobs_1 & mask_1) | (btab_1 & ~mask_1);
                const uint32_t ximg_2 = (mobs_2 & mask_2) | (btab_2 & ~mask_2);
                const uint32_t ximg_3 = (mobs_3 & mask_3) | (btab_3 & ~mask_3);

                const uint32_t img_0  = (pimg_3 << l_shf) | (ximg_0 >> r_shf);
                const uint32_t img_1  = (ximg_0 << l_shf) | (ximg_1 >> r_shf);
                const uint32_t img_2  = (ximg_1 << l_shf) | (ximg_2 >> r_shf);
                const uint32_t img_3  = (ximg_2 << l_shf) | (ximg_3 >> r_shf);

                image[img_idx + c + 0 + img_ofs] = img_0;
                image[img_idx + c + 1 + img_ofs] = img_1;
                image[img_idx + c + 2 + img_ofs] = img_2;
                image[img_idx + c + 3 + img_ofs] = img_3;

                pimg_3 = ximg_3;
            }
#else
            for (int c = 0, cc = 0; c < 24; c += 4, cc++)
            {
                uint32_t bt_msk, btab_0, btab_1, btab_2, btab_3;
                uint32_t vs_msk, mobs_0, mobs_1, mobs_2, mobs_3;
                uint32_t pr_msk, mask_0, mask_1, mask_2, mask_3;
                uint32_t mb_msk, ximg_0, ximg_1, ximg_2, ximg_3;
                uint32_t img_0;
                uint32_t img_1;
                uint32_t img_2;
                uint32_t img_3;

                bt_msk = xbt_bmp[btb_idx + cc];
                vs_msk = mpl_vsb[bmp_idx + cc];
                pr_msk = mpl_pri[bmp_idx + cc];
                mb_msk = vs_msk & ~(pr_msk & bt_msk);

                btab_0 = xbt_img[bti_idx + c + 0];
                mobs_0 = mpl_img[img_idx + c + 0];
                mask_0 = stic_b2n((mb_msk >> 24)       );
                ximg_0 = (mobs_0 & mask_0) | (btab_0 & ~mask_0);
                img_0  = (pimg_3 << l_shf) | (ximg_0 >> r_shf);
                image[img_idx + c + 0 + img_ofs] = img_0;

                btab_1 = xbt_img[bti_idx + c + 1];
                mobs_1 = mpl_img[img_idx + c + 1];
                mask_1 = stic_b2n((mb_msk >> 16) & 0xFF);
                ximg_1 = (mobs_1 & mask_1) | (btab_1 & ~mask_1);
                img_1  = (ximg_0 << l_shf) | (ximg_1 >> r_shf);
                image[img_idx + c + 1 + img_ofs] = img_1;

                btab_2 = xbt_img[bti_idx + c + 2];
                mobs_2 = mpl_img[img_idx + c + 2];
                mask_2 = stic_b2n((mb_msk >>  8) & 0xFF);
                ximg_2 = (mobs_2 & mask_2) | (btab_2 & ~mask_2);
                img_2  = (ximg_1 << l_shf) | (ximg_2 >> r_shf);
                image[img_idx + c + 2 + img_ofs] = img_2;

                btab_3 = xbt_img[bti_idx + c + 3];
                mobs_3 = mpl_img[img_idx + c + 3];
                mask_3 = stic_b2n((mb_msk      ) & 0xFF);
                ximg_3 = (mobs_3 & mask_3) | (btab_3 & ~mask_3);
                img_3  = (ximg_2 << l_shf) | (ximg_3 >> r_shf);
                image[img_idx + c + 3 + img_ofs] = img_3;

                pimg_3 = ximg_3;
            }
#endif

            if (r & 1) { bti_idx += 24; btb_idx += 6; }
            else       { xbt_img[bti_idx] = xbt_img[bti_idx + 24]; }
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Apply top and bottom borders.                                       */
    /* -------------------------------------------------------------------- */
    const int top = (stic->raw[0x32] & 2 ? 32 : 16);  /* 32 or 16 rows. */
    memset(image,          bord, top * 192 / 2);
    memset(image + 208*24, bord,  16 * 192 / 2);

    /* -------------------------------------------------------------------- */
    /*  Apply left and right borders.                                       */
    /* -------------------------------------------------------------------- */
    const int lft = stic->raw[0x32] & 1;
    for (int r = 16, ri = 16*24; r < 208; r++, ri += 24)
    {
        image[ri     ] = bord;
        image[ri + 21] = bord;
        if (lft) image[ri + 1] = bord;
    }

    /* -------------------------------------------------------------------- */
    /*  Fill in colors in column 159, except on STIC1A.                     */
    /* -------------------------------------------------------------------- */
    if (stic->type != STIC_STIC1A)
        for (int r = 16 + v_dly, ri = r*24 + 20; r < 208; r++, ri += 24)
            image[ri] = (0xFFFFFFF0 & image[ri]) | (0xF & bord);
}

/* ======================================================================== */
/*  STIC_PUSH_VID -- Temporary:  Unpack the 4-bpp image to 160x200 8bpp     */
/* ======================================================================== */
LOCAL void stic_push_vid(stic_t *stic)
{
    uint32_t *RESTRICT vid   = (uint32_t*)(void *)stic->disp;
    uint32_t *RESTRICT image = stic->image;

    image += 12*24 + 1;

    for (int y = 12; y < 212; y++)
    {
        for (int x = 1; x <= 20; x++)
        {
#ifdef BYTE_LE
    #if 0
            uint32_t pix   = *image++;
            uint32_t p76   = stic_n2b[pix       & 0xFF];
            uint32_t p54   = stic_n2b[pix >>  8 & 0xFF];
            uint32_t p7654 = (p76 << 16) | p54;
            uint32_t p32   = stic_n2b[pix >> 16 & 0xFF];
            uint32_t p10   = stic_n2b[pix >> 24       ];
            uint32_t p3210 = (p32 << 16) | p10;
            *vid++ = p3210;
            *vid++ = p7654;
    #else
            /* 01234567 => 77665544:33221100 */
            const uint32_t pix   = *image++;
            const uint32_t p7    = (pix << 24) & 0x0F000000;
            const uint32_t p6    = (pix << 12) & 0x000F0000;
            const uint32_t p5    = (pix      ) & 0x00000F00;
            const uint32_t p4    = (pix >> 12) & 0x0000000F;
            const uint32_t p7654 = (p7 | p6) | (p5 | p4);
            const uint32_t p3    = (pix <<  8) & 0x0F000000;
            const uint32_t p2    = (pix >>  4) & 0x000F0000;
            const uint32_t p1    = (pix >> 16) & 0x00000F00;
            const uint32_t p0    = (pix >> 28) & 0x0000000F;
            const uint32_t p3210 = (p3 | p2) | (p1 | p0);

            *vid++ = p3210;
            *vid++ = p7654;
    #endif
#else
            const uint32_t pix   = *image++;
            const uint32_t p01   = stic_n2b[pix >> 24       ];
            const uint32_t p23   = stic_n2b[pix >> 16 & 0xFF];
            const uint32_t p0123 = (p01 << 16) | p23;
            const uint32_t p45   = stic_n2b[pix >>  8 & 0xFF];
            const uint32_t p67   = stic_n2b[pix       & 0xFF];
            const uint32_t p4567 = (p45 << 16) | p67;

            *vid++ = p0123;
            *vid++ = p4567;
#endif
        }
        image += 4;
    }
}

/* ======================================================================== */
/*  STIC_MOB_COLLDET -- Do collision detection on all the MOBs.             */
/*                      XXX: h_dly and v_dly??                              */
/* ======================================================================== */
LOCAL void stic_mob_colldet(stic_t *stic)
{
    const int h_dly =  stic->raw[0x30] & 7;
    const int v_dly = (stic->raw[0x31] & 7) * 2;

    for (int mob0 = 0; mob0 < 8; mob0++)
    {
        const uint32_t x_reg0 = stic->raw[mob0 + 0x00];
        const uint32_t y_reg0 = stic->raw[mob0 + 0x08];
        const uint32_t c_reg0 = stic->raw[mob0 + 0x18];

        /* ---------------------------------------------------------------- */
        /*  Decode the first MOB.  Reject it trivially if off screen or     */
        /*  non-interacting.                                                */
        /* ---------------------------------------------------------------- */
        const int yhgt0 = stic_mob_hgt[(y_reg0 >> 7) & 7];
        const int yshf0 = (y_reg0 >> 8) & 3;

        const int ixl0 = (x_reg0 & 0x1FF);     /* X coord and INTR bit */
        const int yl0  = (y_reg0 & 0x07F)<<1;  /* Y coord              */

        const int ixh0 = ixl0 + (x_reg0 & 0x400 ? 15 : 7);
        const int yh0  = yl0 + yhgt0 - 1;

        if (ixl0 <= 0x100 || ixl0 >= (0x1A8-h_dly) || yl0 > (0xD8-v_dly))
            continue;

        /* ---------------------------------------------------------------- */
        /*  Generate 'edge mask' which discards pixels that are outside     */
        /*  the display area.                                               */
        /* ---------------------------------------------------------------- */

        /* ---------------------------------------------------------------- */
        /*  Do MOB-to-MOB collision detection first.                        */
        /* ---------------------------------------------------------------- */
        for (int mob1 = mob0 + 1; mob1 < 8; mob1++)
        {
            const uint32_t x_reg1 = stic->raw[mob1 + 0x00];
            const uint32_t y_reg1 = stic->raw[mob1 + 0x08];
            const uint32_t c_reg1 = stic->raw[mob1 + 0x18];

            /* ------------------------------------------------------------ */
            /*  Super trivial reject:  If we already have a collision for   */
            /*  this MOB pair, we don't need to compute again -- it'd be    */
            /*  useless since we can only set bits to 1 and it's already 1. */
            /* ------------------------------------------------------------ */
            if (((c_reg0 >> mob1) & 1) &&
                ((c_reg1 >> mob0) & 1))
                continue;

            /* ------------------------------------------------------------ */
            /*  Decode second MOB.  Reject it trivially if off-screen.      */
            /* ------------------------------------------------------------ */
            const int yhgt1 = stic_mob_hgt[(y_reg1 >> 7) & 7];
            const int yshf1 = (y_reg1 >> 8) & 3;

            const int ixl1 = (x_reg1 & 0x1FF); /* X coord + INTR bit */
            const int yl1  = (y_reg1 & 0x07F)<<1;

            const int ixh1 = ixl1 + (x_reg1 & 0x400 ? 15 : 7);
            const int yh1  = yl1 + yhgt1 - 1;

            if (ixl1 <= 0x100 || ixl1 >= (0x1A7-h_dly) || yl1 > (0xD8-v_dly))
                continue;

            /* ------------------------------------------------------------ */
            /*  Only slightly less trivial reject:  Bounding box compare    */
            /*  the two.  Basically, the left edge of one box must be       */
            /*  between left and right of the other.  Ditto for top edge    */
            /*  vs. top/bot for other.                                      */
            /* ------------------------------------------------------------ */
            if ((ixl0 < ixl1 || ixl0 > ixh1) && (ixl1 < ixl0 || ixl1 > ixh0))
                continue;

            if ((yl0 < yl1 || yl0 > yh1) && (yl1 < yl0 || yl1 > yh0))
                continue;

            /* ------------------------------------------------------------ */
            /*  Compute bitwise collision.                                  */
            /* ------------------------------------------------------------ */
            const int ls0 = ixl0 < ixl1 ? ixl1 - ixl0 : 0;
            const int ls1 = ixl0 < ixl1 ? 0           : ixl0 - ixl1;

            const int ylo_tmp = yl0 > yl1 ? yl0 : yl1;
            const int yhi_tmp = yh0 < yh1 ? yh0 : yh1;
            const int ylo = ylo_tmp < 15  - v_dly ? 15  - v_dly : ylo_tmp;
            const int yhi = yhi_tmp > 208 - v_dly ? 208 - v_dly : yhi_tmp;

            for (int yy = ylo; yy <= yhi; yy++)
            {
                const int yy0 = (yy - yl0) >> yshf0;
                const int yy1 = (yy - yl1) >> yshf1;

                const uint32_t mb0 = stic->mob_bmp[mob0][yy0] << ls0;
                const uint32_t mb1 = stic->mob_bmp[mob1][yy1] << ls1;

                if (mb0 & mb1)
                {
                    stic->raw[mob0 + 0x18] |= 1u << mob1;
                    stic->raw[mob1 + 0x18] |= 1u << mob0;
                    break;
                }
            }
        }

        /* ---------------------------------------------------------------- */
        /*  Discard INTR bit from ixl0.                                     */
        /* ---------------------------------------------------------------- */
        const int xl0 = ixl0 & 0xFF;

        /* ---------------------------------------------------------------- */
        /*  Do MOB-to-BACKTAB.  Skip this test if this bit is already 1.    */
        /* ---------------------------------------------------------------- */
        if ((c_reg0 & 0x100) == 0x000)
        {
            const int ymax = yh0 > (206 - v_dly) ? 206 - v_dly : yh0;

            assert(ymax <= 206);

            int bt_idx = yl0 * 3 + (xl0 >> 5);
            const uint32_t bt_ls  = xl0 & 31;
            const uint32_t bt_rs  = 32 - bt_ls;

            /* ------------------------------------------------------------ */
            /*  Sadly, we need the 'if (bt_ls)' because >>32 is undefined.  */
            /* ------------------------------------------------------------ */
            if (bt_ls)
            {
                int yy;
                for (yy = yl0; yy <= ymax; yy++)
                {
                    const int yy0 = (yy - yl0) >> yshf0;
                    const uint32_t bt = (stic->xbt_bmp[bt_idx    ] << bt_ls) |
                                        (stic->xbt_bmp[bt_idx + 1] >> bt_rs);

                    if (bt & ((uint32_t)stic->mob_bmp[mob0][yy0] << 16))
                        break;

                    if (yy & 1) bt_idx += 6;
                }
                if (yy <= ymax)
                    stic->raw[mob0 + 0x18] |= 0x100;
            } else
            {
                int yy;
                for (yy = yl0; yy <= ymax; yy++)
                {
                    const int yy0 = (yy - yl0) >> yshf0;
                    const int bt  = stic->xbt_bmp[bt_idx];

                    if (bt & ((uint32_t)stic->mob_bmp[mob0][yy0] << 16))
                        break;

                    if (yy & 1) bt_idx += 6;
                }
                if (yy <= ymax)
                    stic->raw[mob0 + 0x18] |= 0x100;
            }
        }

        /* ---------------------------------------------------------------- */
        /*  Do MOB-to-Border.   Skip this test if this bit is already 1.    */
        /* ---------------------------------------------------------------- */
        if ((c_reg0 & 0x200) == 0x000)
        {
            const int      mx     = xl0 + h_dly, my = yl0 + v_dly;
            const int      ted    = (stic->raw[0x32] & 2) ? 32    : 16;
            const uint32_t le_gen = (stic->raw[0x32] & 1) ? 0x1FF : 0x100;

            /* ------------------------------------------------------------ */
            /*  Compute left/right collisions by ANDing a bitmask with      */
            /*  each of the rows of the MOB bitmap that are inside the      */
            /*  visible field.  The "le_gen" takes into account edge ext.   */
            /* ------------------------------------------------------------ */
            const uint32_t le_msk = mx < 9   ? le_gen <<  mx        : 0;
            const uint32_t re_msk = mx > 150 ? 0x8000 >> (167 - mx) : 0;
            const uint32_t msk    = 0xFFFF & (le_msk | re_msk);

            /* compute top/bottom rows that l/r edges might interact with */
            const int ymax = 
                (((yh0 + v_dly) < 208 ? (yh0 + v_dly) : 208) - my) >> yshf0;
            const int ymin = my < 16 ? (15 - my) >> yshf0 : 0;
/*if (le_msk) jzp_printf("le_msk = %.8X mx = %-3d\n", le_msk, mx);*/
/*if (re_msk) jzp_printf("re_msk = %.8X mx = %-3d\n", re_msk, mx);*/

            /* left, right edges */
            {
                int yy;
                for (yy = ymin; yy <= ymax; yy++)
                {
                    if (yy < yhgt0 &&
                        stic->mob_bmp[mob0][yy] & msk)
                        break;
                }
                if (yy <= ymax)
                    stic->raw[mob0 + 0x18] |= 0x200;
            }

            /* ------------------------------------------------------------ */
            /*  Compute top/bottom collisions by examining the row(s) that  */
            /*  might intersect with either edge.  We regenerate the left/  */
            /*  right masks ignoring border extension, so that we can use   */
            /*  them to mask away the pixels that aren't included in the    */
            /*  computation.                                                */;
            /* ------------------------------------------------------------ */
            /* extend left/right edge masks */
            const uint32_t le_msk_e = mx < 8 ? 0xFFFFFE00 << mx  : 0;
            const uint32_t re_msk_e = re_msk ? (re_msk << 1) - 1 : 0;
            const uint32_t msk_e    = ~(le_msk_e | re_msk_e);

            /* top edge */
            if (my <= ted)
            {
/*jzp_printf("ted=%-2d my=%-2d:", ted, my);*/
                for (int yy = 15; yy < ted; yy += (1u << yshf0))
                {
                    if (my <= yy)
                    {
                        const int row = (yy - my) >> yshf0;
                        if (row < yhgt0 &&
                            stic->mob_bmp[mob0][row] & msk_e)
                        {
/*jzp_printf(" %2d,%-2d", yy, row);*/
                                stic->raw[mob0 + 0x18] |= 0x200;
                        }
                    }
                }
/*putchar('\n');*/
            }

            /* bottom edge */
            if (yh0 + v_dly > 207 && my <= 208)
            {
                const int ybot = (208 - my) >> yshf0;

                if (stic->mob_bmp[mob0][ybot] & msk_e)
                    stic->raw[mob0 + 0x18] |= 0x200;
            }
        }
    }
}

/* ======================================================================== */
/*  STIC_UPDATE -- wrapper around all the pieces above.                     */
/* ======================================================================== */
#ifdef BENCHMARK_STIC
LOCAL void stic_update(stic_t *stic)
{
    double a, b, c;
    static double ovhd = 1e6;

    if (stic->mode != stic->p_mode)
    {
        stic->bt_dirty = 3;
        stic->mode = stic->p_mode;
    }

    if (!stic->bt_dirty)
    {
        if (memcmp(stic->btab_pr, stic->btab, sizeof(stic->btab_pr)))
        {
            stic->bt_dirty |= 3;
            memcpy(stic->btab_pr, stic->btab, sizeof(stic->btab_pr));
        }
    }

    a = get_time();
    b = get_time();
    if (b - a < ovhd) ovhd = b - a;
    a = b;

    /* draw the backtab */
    if (stic->upd) stic->upd(stic);
    c = get_time(); stic->time.draw_btab    += c - b - ovhd; b = c;

    stic_draw_mobs   (stic);
    c = get_time(); stic->time.draw_mobs    += c - b - ovhd; b = c;
    stic_fix_bord    (stic);
    c = get_time(); stic->time.fix_bord     += c - b - ovhd; b = c;
    stic_merge_planes(stic);
    c = get_time(); stic->time.merge_planes += c - b - ovhd; b = c;

    if (!stic_dropping_this_frame(stic))
    {
        stic_push_vid    (stic);
        c = get_time(); stic->time.push_vid     += c - b - ovhd; b = c;
    }
    if (stic->drop_frame > 0)
        stic->drop_frame--;


    stic_mob_colldet (stic);
    c = get_time(); stic->time.mob_colldet  += c - b - ovhd; b = c;

    c = get_time(); stic->time.gfx_vid_enable += c - b - ovhd;

    stic->time.full_update  += c - a - 7*ovhd;
    stic->time.total_frames++;

    if (stic->time.total_frames >= 100)
    {
        double scale = 1e6 / (double)stic->time.total_frames;

        jzp_printf("stic performance update:\n");
        jzp_printf("  draw_btab     %9.4f usec\n", stic->time.draw_btab    * scale);
        jzp_printf("  draw_mobs     %9.4f usec\n", stic->time.draw_mobs    * scale);
        jzp_printf("  fix_bord      %9.4f usec\n", stic->time.fix_bord     * scale);
        jzp_printf("  merge_planes  %9.4f usec\n", stic->time.merge_planes * scale);
        jzp_printf("  push_vid      %9.4f usec\n", stic->time.push_vid     * scale);
        jzp_printf("  mob_colldet   %9.4f usec\n", stic->time.mob_colldet  * scale);
        jzp_printf("  TOTAL:        %9.4f usec\n", stic->time.full_update  * scale);

        jzp_flush();

        memset((void*)&stic->time, 0, sizeof(stic->time));
    }
}
#else
LOCAL void stic_update(stic_t *stic)
{
    if (stic->mode != stic->p_mode)
    {
        stic->bt_dirty = 3;
        stic->mode = stic->p_mode;
    }

    /* draw the backtab */
    if (stic->upd) stic->upd(stic);

    stic_draw_mobs   (stic);
    stic_fix_bord    (stic);
    stic_merge_planes(stic);

    if (!stic_dropping_this_frame(stic))
    {
        stic_push_vid    (stic);
    }
    if (stic->drop_frame > 0)
        stic->drop_frame--;

    stic_mob_colldet (stic);
}
#endif

/* ======================================================================== */
/*  STIC timing variables                                                   */
/*                                                                          */
/*      gmem_accessible     Last cycle GRAM/GROM is accessible.             */
/*      stic_accessible     Last cycle STIC control regs are accessible.    */
/*      vid_enable_cutoff   Cycle where MAYBE becomes NO for vid_enable.    */
/*      next_frame_render   CPU cycle to trigger stic_update on.            */
/*      last_frame_intrq    CPU cycle of the most recent previous INTRQ     */
/*      next_frame_intrq    CPU cycle of the next INTRQ                     */
/*                                                                          */
/* ======================================================================== */


/* ======================================================================== */
/*  STIC_LATE_BUSRQ      -- Handle a BUSRQ arriving late.                   */
/* ======================================================================== */
LOCAL void stic_late_busrq(stic_t *const RESTRICT stic)
{
    const int busrq_number = stic->busrq_count++;

    /* -------------------------------------------------------------------- */
    /*  Nothing happens on the very first BUSRQ on a standard STIC.         */
    /* -------------------------------------------------------------------- */
    if (busrq_number == 0)
    {
        stic->fifo_rd_ptr = 0;
        stic->fifo_wr_ptr = 0;
        return;
    }

    /* -------------------------------------------------------------------- */
    /*  Nothing happens on the very last BUSRQ on a standard STIC either.   */
    /* -------------------------------------------------------------------- */
    if (busrq_number > 12)
        return;

    /* -------------------------------------------------------------------- */
    /*  Because the BUSRQ arrived late, we replay the previous 20 cards     */
    /*  from the FIFO.  Our copy counter increments (the display beam       */
    /*  keeps moving down the display), but our fetch pointer doesn't.      */
    /* -------------------------------------------------------------------- */
    assert(stic->fifo_wr_ptr <= 220);
    assert(stic->fifo_rd_ptr <= 220);
    if (stic->fifo_wr_ptr == 0)
    {
        /* ---------------------------------------------------------------- */
        /*  I'm not really sure what to do if this happens at the top of    */
        /*  screen, so for now assume it replays the bottom-most row.       */
        /* ---------------------------------------------------------------- */
        memcpy(&stic->btab[0], &stic->btab[220], 20 * sizeof(stic->btab[0]));
    } else
    {
        memcpy(&stic->btab[stic->fifo_wr_ptr],
               &stic->btab[stic->fifo_wr_ptr - 20],
               20 * sizeof(stic->btab[0]));
    }
    stic->fifo_wr_ptr += 20;
}


/* ======================================================================== */
/*  STIC_ONTIME_BUSRQ    -- Handle a BUSRQ arriving on time.                */
/* ======================================================================== */
LOCAL void stic_ontime_busrq(stic_t *const RESTRICT stic)
{
    const int busrq_number = stic->busrq_count++;

    /* -------------------------------------------------------------------- */
    /*  Nothing happens on the very first BUSRQ on a standard STIC.         */
    /* -------------------------------------------------------------------- */
    if (busrq_number == 0)
    {
        stic->fifo_rd_ptr = 0;
        stic->fifo_wr_ptr = 0;
        return;
    }

    /* -------------------------------------------------------------------- */
    /*  Nothing happens on the very last BUSRQ on a standard STIC either.   */
    /* -------------------------------------------------------------------- */
    if (busrq_number > 12)
        return;

    /* -------------------------------------------------------------------- */
    /*  Copy the next row of cards from BACKTAB to the BACKTAB shadow.      */
    /* -------------------------------------------------------------------- */
    assert(stic->fifo_wr_ptr <= 220);
    assert(stic->fifo_rd_ptr <= 220);
    memcpy(&stic->btab[stic->fifo_wr_ptr], &stic->btab_sr[stic->fifo_rd_ptr],
            20 * sizeof(stic->btab[0]));

    stic->fifo_wr_ptr += 20;
    stic->fifo_rd_ptr += 20;
}

/* ======================================================================== */
/*  STIC_DO_REQ_DROP -- A BUSAK or INTAK didn't happen in time.             */
/* ======================================================================== */
LOCAL void stic_do_req_drop
(
    struct req_q_t *const RESTRICT  req_q,
    const uint64_t                  cycle
)
{
    stic_t *const RESTRICT stic = (stic_t *)req_q->opaque;
    const req_t            req  = REQ_Q_FRONT(req_q);
    const uint8_t          type = req.type;

    req_q_default_drop_fn(req_q, cycle);

    /* -------------------------------------------------------------------- */
    /*  If dropping an interrupt or bus request, halt us if the user set    */
    /*  the corresponding halt flag.                                        */
    /* -------------------------------------------------------------------- */
    if (type == REQ_BUS && (stic->debug_flags & STIC_HALT_ON_BUSRQ_DROP))
    {
        debug_fault_detected = DEBUG_ASYNC_HALT;
        debug_halt_reason = "BUSRQ dropped.";
    }

    if (type == REQ_INT && (stic->debug_flags & STIC_HALT_ON_INTRM_DROP))
    {
        debug_fault_detected = DEBUG_ASYNC_HALT;
        debug_halt_reason = "INTRM dropped.";
    }

    /* -------------------------------------------------------------------- */
    /*  If an interrupt goes unacknowledged, then neither STIC not GMEM     */
    /*  will become accessible on this frame.  No need to update anything   */
    /*  however; the previous stic_accessible/gmem_accessible cycles are    */
    /*  fine as they stand.                                                 */
    /*                                                                      */
    /*  So, just wind forward our simulation state and return.              */
    /* -------------------------------------------------------------------- */
    if (type == REQ_INT)
    {
        stic_simulate_until(stic, cycle);
        return;
    }

    /* -------------------------------------------------------------------- */
    /*  If a bus request goes unacknowledged, or gets acknowledged late,    */
    /*  then we get a display artifact.                                     */
    /* -------------------------------------------------------------------- */
    if (type == REQ_BUS)
    {
        stic_late_busrq(stic);
        stic_simulate_until(stic, cycle);
        return;
    }
}

/* ======================================================================== */
/*  STIC_DO_REQ_ACK  -- Receive an INTAK or BUSAK.                          */
/* ======================================================================== */
LOCAL void stic_do_req_ack
(
    struct req_q_t *const RESTRICT  req_q,
    const uint64_t                  cycle
)
{
    stic_t *const RESTRICT stic = (stic_t *)req_q->opaque;
    const req_t            req  = REQ_Q_FRONT(req_q);
    const uint8_t          type = req.type;

    req_q_default_ack_fn(req_q, cycle);

    /* -------------------------------------------------------------------- */
    /*  Acknowledging an interrupt makes the STIC and GMEM both accessible  */
    /*  for this frame.  Update stic_accessible/gmem_accessible to this     */
    /*  frame's values.                                                     */
    /* -------------------------------------------------------------------- */
    if (type == REQ_INT)
    {
        if (req.start != stic->next_frame_intrq)
            fprintf(stderr, "req.start=%" U64_FMT " stic->next_frame_intrq=%"
                    U64_FMT "\n", req.start, stic->next_frame_intrq);
        assert(req.start == stic->next_frame_intrq);
        stic->stic_accessible = req.start + STIC_STIC_ACCESSIBLE;
        if (stic->type != STIC_STIC1A)
            stic->gmem_accessible = req.start + STIC_GMEM_ACCESSIBLE;
        else
            stic->gmem_accessible = req.start + STIC_GMEM_ACCESSIBLE + 20;

        if (stic->debug_flags & STIC_DBG_REQS)
            jzp_printf("INTAK %" U64_FMT " / %" U64_FMT "\n", req.start, cycle);
        stic_simulate_until(stic, cycle);
        return;
    }

    /* -------------------------------------------------------------------- */
    /*  If this is a BUSAK, decide whether it was "in time" or not, and     */
    /*  handle it appropriately.                                            */
    /* -------------------------------------------------------------------- */
    if (type == REQ_BUS)
    {
        uint32_t ack_time = cycle - req.start;
        if (ack_time <= STIC_MUST_BUSAK_TIME)
            stic_ontime_busrq(stic);
        else
            stic_late_busrq(stic);
        stic_simulate_until(stic, cycle);
        return;
    }
}

/* ======================================================================== */
/*  STIC_GENERATE_REQS   -- Generate INTRQ/BUSRQ for the next frame.        */
/* ======================================================================== */
LOCAL void stic_generate_reqs(stic_t *const RESTRICT stic)
{
    const uint64_t frame_ref = stic->next_frame_intrq;
    req_q_t *const RESTRICT req_q = stic->req_q;

    /* -------------------------------------------------------------------- */
    /*  Update the INTRQ cycle.                                             */
    /* -------------------------------------------------------------------- */
    stic->last_frame_intrq  = stic->next_frame_intrq;
    stic->next_frame_intrq += STIC_FRAMCLKS;

    if (stic->debug_flags & STIC_DBG_REQS)
    {
        jzp_printf("last_frame_intrq=%" U64_FMT " "
                   "next_frame_intrq=%" U64_FMT " "
                   "next_frame_render=%" U64_FMT "\n",
                   stic->last_frame_intrq,
                   stic->next_frame_intrq,
                   stic->next_frame_render);
    }

    /* -------------------------------------------------------------------- */
    /*  Generate 12 to 14 bus requests based on STIC type, and v_dly.       */
    /* -------------------------------------------------------------------- */
    if (stic->vid_enable == VID_ENABLED)
    {
        const uint32_t h_dly = stic->raw[0x30] & 7;
        const uint32_t v_dly = stic->raw[0x31] & 7;
        const uint32_t dly_cycles  = 2 * STIC_SCANLINE * v_dly + (h_dly >> 2);
        const uint64_t active_disp = frame_ref + STIC_GMEM_ACCESSIBLE;
        /* FIXME: The 143 on the next line needs to be parameterized. */
        /* Right now, it's 2.5 scanlines for PAL and NTSC, and ?? for STIC1A */
        const uint64_t first_fetch = active_disp + dly_cycles +
                            ( stic->type == STIC_STIC1A ? 149
                            : stic->pal                 ? 160
                            :                             143);

        assert(active_disp == stic->gmem_accessible ||
               stic->type == STIC_STIC1A);

        /* ---------------------------------------------------------------- */
        /*  AY-3-8900 and AY-3-8900-1 issue an initial short BUSRQ to flip  */
        /*  the RA-3-9600 into bus-isolation mode.                          */
        /* ---------------------------------------------------------------- */
        if (stic->type != STIC_STIC1A)
        {
            const req_t req =
            {
                active_disp,
                active_disp + STIC_BUSRQ_HOLD_FIRST,
                0,
                REQ_BUS,
                REQ_PENDING
            };

            REQ_Q_PUSH_BACK(req_q, req);
            stic->busrq_count = 0;
        }
        /* ---------------------------------------------------------------- */
        /*  STIC1A doesn't have that extra BUSRQ, so we skip it.            */
        /* ---------------------------------------------------------------- */
        else
        {
            stic->busrq_count = 1;
        }

        /* ---------------------------------------------------------------- */
        /*  Either way, reset our video FIFO fetch pointers.                */
        /* ---------------------------------------------------------------- */
        stic->fifo_rd_ptr = 0;
        stic->fifo_wr_ptr = 0;

        /* ---------------------------------------------------------------- */
        /*  All display types issue 12 BUSRQs that are 16 scan lines apart  */
        /* ---------------------------------------------------------------- */
        {
            const uint32_t busrq_hold =
                stic->type == STIC_STIC1A ? STIC_BUSRQ_HOLD_NORMAL - 6
                                          : STIC_BUSRQ_HOLD_NORMAL;

            for (int i = 0; i < 12; i++)
            {
                const uint64_t
                     this_fetch = first_fetch + i * 16 * STIC_SCANLINE;
                const req_t req =
                {
                    this_fetch,
                    this_fetch + busrq_hold,
                    0,
                    REQ_BUS,
                    REQ_PENDING
                };

                REQ_Q_PUSH_BACK(req_q, req);
            }
        }

        /* ---------------------------------------------------------------- */
        /*  If we're not STIC1A, and vertical delay == 0, issue 14th BUSRQ  */
        /* ---------------------------------------------------------------- */
        if (stic->type != STIC_STIC1A && v_dly == 0)
        {
            const uint64_t extra_fetch = first_fetch + 12 * 16 * STIC_SCANLINE;
            const req_t req =
            {
                extra_fetch,
                extra_fetch + STIC_BUSRQ_HOLD_EXTRA,
                0,
                REQ_BUS,
                REQ_PENDING
            };

            REQ_Q_PUSH_BACK(req_q, req);
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Generate an INTRQ for the next frame.                               */
    /* -------------------------------------------------------------------- */
    {
        const req_t req =
        {
            stic->next_frame_intrq,
            stic->next_frame_intrq + STIC_INTRQ_HOLD,
            0,
            REQ_INT,
            REQ_PENDING
        };

        REQ_Q_PUSH_BACK(req_q, req);
    }

    /* -------------------------------------------------------------------- */
    /*  Set the next CPU execution horizon to be the next video enable      */
    /*  cutoff, plus 1 cycle, so we can go past the cutoff.                 */
    /* -------------------------------------------------------------------- */
    req_q->horizon = stic->vid_enable_cutoff + 1;
}

/* ======================================================================== */
/*  STIC_SIMULATE_UNTIL  -- Hoo boy, this is gonna be fun.                  */
/* ======================================================================== */
LOCAL void stic_simulate_until(stic_t *const RESTRICT stic,
                               const uint64_t cycle)
{
    if (stic->debug_flags & STIC_DBG_REQS)
    {
        jzp_printf("simulate_until c=%" U64_FMT " "
                   "ec=%" U64_FMT " ve=%d h=%" U64_FMT " "
                   "vec=%" U64_FMT " nfr=%" U64_FMT "\n",
                   cycle,
                   stic->eff_cycle, stic->vid_enable,
                   stic->req_q->horizon,
                   stic->vid_enable_cutoff,
                   stic->next_frame_render);
    }

    /* -------------------------------------------------------------------- */
    /*  Don't let time go backward.  (Shouldn't happen?)                    */
    /* -------------------------------------------------------------------- */
    if (cycle < stic->eff_cycle)
    {
        stic->eff_cycle = cycle;
        return;
    }

    /* -------------------------------------------------------------------- */
    /*  Render the next frame for display if we haven't yet.                */
    /* -------------------------------------------------------------------- */
    if (cycle >= stic->next_frame_render &&
        stic->eff_cycle < stic->next_frame_render)
    {
        stic->is_hidden = gfx_hidden(stic->gfx);

        if (stic->vid_enable == VID_ENABLED)
        {
            stic->movie_active = gfx_movie_is_active(stic->gfx);
            stic_update(stic);
        }

        if (stic->drop_frame > 0)
            stic->drop_frame--;

        stic->gfx->dirty |= stic->bt_dirty
                         |  stic->gr_dirty
                         |  stic->ob_dirty;

        gfx_vid_enable(stic->gfx, stic->vid_enable);
        gfx_stic_tick(stic->gfx);

        /*  We really should consider gfx_tick() here, rather than making   */
        /*  gfx a 'tickable peripheral'.  That would solve one of the       */
        /*  other display artifact problems jzIntv has.                     */

        stic->vid_enable = VID_UNKNOWN;
        stic->next_frame_render += STIC_FRAMCLKS;
    }

    /* -------------------------------------------------------------------- */
    /*  If we're past the STIC-accessible threshold, make a go/no-go        */
    /*  decision for the rest of this frame.                                */
    /* -------------------------------------------------------------------- */
    if (cycle >= stic->vid_enable_cutoff &&
        stic->eff_cycle < stic->vid_enable_cutoff)
    {
        /* ---------------------------------------------------------------- */
        /*  Enable/disable video.                                           */
        /* ---------------------------------------------------------------- */
        if (stic->vid_enable == VID_UNKNOWN)
        {
            stic->vid_enable = VID_DISABLED;
            stic->stic_accessible += STIC_FRAMCLKS;
            stic->gmem_accessible += STIC_FRAMCLKS;
        } else if (stic->type == STIC_STIC1A)
        {
            const int v_dly = stic->raw[0x31] & 7;
            stic->gmem_accessible += v_dly * STIC_SCANLINE * 2;
        }

        if (stic->prev_vid_enable != stic->vid_enable)
            stic->bt_dirty |= 3;

        if (stic->prev_vid_enable == VID_ENABLED &&
            stic->vid_enable == VID_DISABLED &&
            (stic->debug_flags & STIC_HALT_ON_BLANK) != 0)
        {
            debug_fault_detected = DEBUG_ASYNC_HALT;
            debug_halt_reason    = "Display blanked.";
        }

        stic->prev_vid_enable = stic->vid_enable;
        stic->vid_enable_cutoff += STIC_FRAMCLKS;

        /* ---------------------------------------------------------------- */
        /*  Generate bus requests and interrupt request for the next frame. */
        /* ---------------------------------------------------------------- */
        if (stic->debug_flags & STIC_DBG_REQS)
            jzp_printf("generate_reqs %" U64_FMT " ve=%d\n",
                        cycle, stic->vid_enable);

        stic_generate_reqs(stic);

        if (stic->debug_flags & STIC_DBG_REQS)
            req_q_print(stic->req_q, "STIC");
    }

    stic->eff_cycle = cycle;
}

static const uint8_t stic_gram_gif_palette[5][3] =
{
    {   0,   0,   0 },  /* Off pixel              */
    { 255, 128, 128 },  /* On pixel (Even tiles)  */
    { 128, 128, 255 },  /* On pixel (Odd tiles)   */
    { 128,  64, 128 },  /* Grid between pixels    */
    { 128, 255, 255 },  /* Grid between tiles     */
};

/* ======================================================================== */
/*  STIC_GRAM_TO_GIF -- Generate GIF containing the current GRAM contents.  */
/* ======================================================================== */
void stic_gram_to_gif(const stic_t *const stic)
{
    static uint8_t *gram_bitmap = NULL;
    static int gram_size = -1;
    static unique_filename_t gram_shot_tmpl =
    {
        "gram", ".gif", NULL, 0, 4, 0
    };
    FILE *f;
    /*  GRAM Size 0 => 8x8 matrix       */
    /*  GRAM Size 1 => 8x16 matrix      */
    /*  GRAM Size 2 => 16x16 matrix     */
    const int dim_x = stic->gram_size < 2 ? 578 : 1154;
    const int dim_y = stic->gram_size < 1 ? 578 : 1154;
    const int bmp_size = dim_x * dim_y * sizeof(uint8_t);
    int g, len;

    if (stic->gram_size != gram_size)
    {
        gram_size   = stic->gram_size;
        gram_bitmap = (uint8_t *)realloc(gram_bitmap, bmp_size);
    }

    if (!gram_bitmap)
    {
        gram_size = -1;
        jzp_printf("Could not allocate buffer to write GRAM shot\n");
        return;
    }

    memset(gram_bitmap, 0, bmp_size);

    /* Color 0 = off                            */
    /* Color 1 = on                             */
    /* Color 2 = divider between pixels         */
    /* Color 3 = divider between 8x8 blocks     */

    for (g = 0; g < (64 << gram_size); g++)
    {
        const int cc = (gram_size < 2 ? (g & 7) : (g & 15));
        const int rr = (gram_size < 2 ? (g >> 3) : (g >> 4));
        const int c = 8*9*cc;
        const int r = 8*9*rr;
        const int on = (cc ^ rr) & 1 ? 2 : 1;
        int x, y, xx, yy;
        for (y = 0; y < 8; y++)
        {
            uint8_t bits = stic->gmem[0x800 + g*8 + y];
            for (x = 0; x < 8; x++)
            {
                if ((bits << x) & 0x80)
                {
                    for (yy = 0; yy < 8; yy++)
                    {
                        uint8_t *grow = gram_bitmap + (1 + r + y*9 + yy)*dim_x;
                        for (xx = 0; xx < 8; xx++)
                            grow[x*9 + xx + c + 1] = on;
                    }
                }
            }
        }
    }

    /* Horizontal stripes in grid               */
    {
        int r;

        for (r = 0; r < dim_y; r += 9)
            memset(gram_bitmap + r * dim_x, 3, dim_x);

    }

    /* Vertical stripes in grid                 */
    {
        int c;
        for (c = 0; c < dim_x; c += 9)
        {
            int r;
            for (r = 0; r < dim_y; r++)
                gram_bitmap[r * dim_x + c] = 3;
        }
    }

    /* Horizontal stripes in grid               */
    {
        int r;

        for (r = 0; r < dim_y; r += 9*8)
            memset(gram_bitmap + r * dim_x, 4, dim_x * 2);

    }

    /* Vertical stripes in grid                 */
    {
        int c;
        for (c = 0; c < dim_x; c += 9*8)
        {
            int r;
            for (r = 0; r < dim_y; r++)
            {
                gram_bitmap[r * dim_x + c    ] = 4;
                gram_bitmap[r * dim_x + c + 1] = 4;
            }
        }
    }

    f = open_unique_filename(&gram_shot_tmpl);
    if (!f)
    {
        jzp_printf("\nCould not open '%s' for GRAM shot.\n",
                    gram_shot_tmpl.f_name);
        return;
    }

    len = gif_write(f, gram_bitmap, dim_x, dim_y, stic_gram_gif_palette, 5);
    fclose(f);

    if (len < 0)
    {
        jzp_printf("\nError writing GRAM shot to '%s'\n",
                    gram_shot_tmpl.f_name);
    } else
    {
        jzp_printf("\nWrote GRAM shot to '%s' (%d bytes)\n",
                    gram_shot_tmpl.f_name, len);
    }
}

/* ======================================================================== */
/*  STIC_GRAM_TO_TEXT -- Display textual representation of GRAM.            */
/* ======================================================================== */
void stic_gram_to_text(const stic_t *const stic, const int start,
                       const int count, const int disp_width)
{
    if (start < 0 || count < 1)
        return;

    const int w_max_batch = (disp_width - 4) / 9;
    const int max_batch = w_max_batch < 1 ? 1 
                        : w_max_batch > 8 ? 8
                        :                   w_max_batch;

    for (int i = 0; i < count; i += max_batch)
    {
        const int card = i + start;
        const int batch = count - i < max_batch ? count - i : max_batch;

        jzp_printf("   ");
        for (int col = 0; col < batch; col++)
        {
            jzp_printf(" %-3d   %2X", card + col, card + col);
        }
        jzp_printf("\n");

        for (int row = 0; row < 8; row++)
        {
            char buf[80];
            memset(buf, 0, sizeof(buf));

            for (int col = 0; col < batch; col++)
            {
                const uint32_t addr  = ((card + col) * 8 + row) + 0x800;
                const uint32_t maddr = addr & (stic->gram_mask | 7);
                const uint8_t  bits  = stic->gmem[maddr];

                buf[col * 9 + 0] = bits & 0x80 ? '#' : '.';
                buf[col * 9 + 1] = bits & 0x40 ? '#' : '.';
                buf[col * 9 + 2] = bits & 0x20 ? '#' : '.';
                buf[col * 9 + 3] = bits & 0x10 ? '#' : '.';
                buf[col * 9 + 4] = bits & 0x08 ? '#' : '.';
                buf[col * 9 + 5] = bits & 0x04 ? '#' : '.';
                buf[col * 9 + 6] = bits & 0x02 ? '#' : '.';
                buf[col * 9 + 7] = bits & 0x01 ? '#' : '.';
                buf[col * 9 + 8] = ' ';
            }

            buf[batch * 9 - 1] = '\n';
            jzp_printf("%d:  %s", row, buf);
        }

        if (i + batch < count)
            jzp_printf("\n");
    }

    return;
}

/* ======================================================================== */
/*  STIC_TICK -- Ugh, this is where the action happens.  Whee.              */
/* ======================================================================== */
uint32_t stic_tick
(
    periph_t *const per,
    const uint32_t len
)
{
    stic_t *const RESTRICT stic = PERIPH_PARENT_AS(stic_t, per);
    const uint64_t now  = per->now;
    const uint64_t soon = now + len;

    if (stic->debug_flags & STIC_DBG_REQS)
        jzp_printf("stic_tick %" U64_FMT "\n", soon);

    if (soon > stic->eff_cycle)
        stic_simulate_until(stic, soon);

    if (stic->debug_flags & STIC_GRAMSHOT)
    {
        stic->debug_flags &= ~STIC_GRAMSHOT;
        stic_gram_to_gif(stic);
    }

    return len;
}

/* ======================================================================== */
/*  STIC_RESYNC  -- Resynchronize STIC internal state after a load.         */
/* ======================================================================== */
void stic_resync(stic_t *const stic)
{
    stic->upd      = stic->mode ? stic_draw_fgbg : stic_draw_cstk;
    stic->bt_dirty = 3;
    stic->gr_dirty = 1;
    stic->ob_dirty = 1;
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
/*                  Copyright (c) 2017, Joseph Zbiciak                      */
/* ======================================================================== */
