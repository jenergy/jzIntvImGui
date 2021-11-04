/*
 * ============================================================================
 *  Title:    AY-8910 family Programmable Sound Generator
 *  Author:   J. Zbiciak
 * ============================================================================
 *  This module implements the AY-8910 sound chip.
 * ============================================================================
 *
 *    0   Channel A Period          (Low 8 bits of 12)
 *    1   Channel B Period          (Low 8 bits of 12)
 *    2   Channel C Period          (Low 8 bits of 12)
 *    3   Envelope Period           (Low 8 bits of 16)
 *    4   Channel A Period          (High 4 bits of 12)
 *    5   Channel B Period          (High 4 bits of 12)
 *    6   Channel C Period          (High 4 bits of 12)
 *    7   Envelope Period           (High 8 bits of 16)
 *    8   Enable Noise/Tone         (bits 3-5 Noise : 0-2 Tone)
 *    9   Noise Period              (5 bits)
 *   10   Envelope characteristics  (4 bits)
 *   11   Channel A Volume          (6 bits)
 *   12   Channel B Volume          (6 bits)
 *   13   Channel C Volume          (6 bits)
 *   14   Controller input
 *   15   Controller input
 *
 *  Or, slightly differently:
 *
 *     Register pairs:
 *
 *       7   6   5   4   3   2   1   0 | 7   6   5   4   3   2   1   0
 *     +---------------+---------------|-------------------------------+
 *  R4 |    unused     |                Channel A Period               | R0
 *     +---------------+---------------|-------------------------------+
 *  R5 |    unused     |                Channel B Period               | R1
 *     +---------------+---------------|-------------------------------+
 *  R6 |    unused     |                Channel C Period               | R2
 *     +---------------+---------------|-------------------------------+
 *  R7 |                        Envelope Period                        | R3
 *     +-------------------------------|-------+-----------------------+
 *
 *     Single registers:
 *
 *         7       6       5       4       3       2       1       0
 *     +---------------+-----------------------+-----------------------+
 *     | I/O Port Dir  |    Noise Enables      |     Tone Enables      |
 *  R8 |   0   |   0   |   C   |   B   |   A   |   C   |   B   |   A   |
 *     +-------+-------+-------+-------+-------+-------+-------+-------+
 *  R9 |            unused     |              Noise Period             |
 *     +-----------------------+-------+-------+-------+-------+-------+
 *     |                               |   Envelope Characteristics    |
 * R10 |            unused             | CONT  |ATTACK | ALTER | HOLD  |
 *     +---------------+---------------+-------+-------+-------+-------+
 * R11 |    unused     | A Envl Select |    Channel A Volume Level     |
 *     +---------------+---------------+-------------------------------+
 * R12 |    unused     | B Envl Select |    Channel B Volume Level     |
 *     +---------------+---------------+-------------------------------+
 * R13 |    unused     | C Envl Select |    Channel C Volume Level     |
 *     +---------------+---------------+-------------------------------+
 *
 * ============================================================================
 *
 *  For accuracy, the AY8910's state is evaluated at its native rate,
 *  3579545 / 32 Hz. (4MHz on PAL)  This corresponds to jzIntv's tick rate
 *  divided by 8.  Output samples are generated using a sliding-window
 *  average at the requested sample rate.
 *
 *  Sound samples are built up in buffers of length "snd_buf".
 *  Whole buffers are handed off to the SND driver for playback.
 *
 * ============================================================================
 *
 *  Note, some aspects of this model were built with information from
 *  "psg.c" that's in MAME/MESS.  Notably, certain peculiarities of the
 *  PSG, such as its random-number generator and counter-rollover behavior
 *  were gleaned from that source.  The "psg.c" model is credited as follows:
 *
 *    "Based on various code snippets by Ville Hallik, Michael Cuddy,
 *     Tatsuyuki Satoh, Fabrice Frances, Nicola Salmoria."
 *
 *  This module uses no code directly from "psg.c", merely information.
 *
 * ============================================================================
 */



#include "config.h"
#include "periph/periph.h"
#include "snd/snd.h"
#include "serializer/serializer.h"
#include "ay8910.h"

LOCAL uint32_t ay8910_calc_sound(ay8910_t *ay8910, uint64_t until);

/*
 * ============================================================================
 *  AY8910_VOL   -- Sound volume levels.
 *
 *                  It appears, from the spec sheet, that the amplitude
 *                  halves for every two steps, implying a 1/sqrt(2) ratio
 *                  between steps.  This table contains these steps.
 *
 *                  The volume steps have been scaled to provide the largest
 *                  dynamic range when mixing all 3 channels together.
 * ============================================================================
 */
const int32_t ay8910_vol[16] =
{
    0x0000, 0x0055, 0x0079, 0x00AB, 0x00F1, 0x0155, 0x01E3, 0x02AA,
    0x03C5, 0x0555, 0x078B, 0x0AAB, 0x0F16, 0x1555, 0x1E2B, 0x2AAA,
    /* 0x3C57, 0x5555, */
};


/*
 * ============================================================================
 *  AY8910_READ      -- Read from device.
 * ============================================================================
 */
uint32_t ay8910_read
(
    periph_t        *bus,       /*  Peripheral bus being read.          */
    periph_t        *req,       /*  Peripheral requesting read.         */
    uint32_t        addr,       /*  Address being read.                 */
    uint32_t        data        /*  Current state of data being read.   */
)
{
    ay8910_t *const ay8910 = PERIPH_AS(ay8910_t, bus);

    UNUSED(req);
    UNUSED(data);

    addr &= 15;

    return addr < 14 ? ay8910->reg[addr] & 0xFF : 0xFFFF;
}

/*
 * ============================================================================
 *  AY8910_TRACE     -- Write to device, storing trace data out to tracefile
 * ============================================================================
 */
LOCAL void ay8910_trace
(
    periph_t        *bus,       /*  Peripheral bus being written.       */
    periph_t        *req,       /*  Peripheral requesting write.        */
    uint32_t        addr,       /*  Address being written.              */
    uint32_t        data        /*  Data being written.                 */
)
{
    ay8910_t *const ay8910 = PERIPH_AS(ay8910_t, bus);
    uint64_t write_time = req && req->req ? req->req->now + 4 : 0;

    fprintf(ay8910->trace, "%.8X%.8X: %.1X %.2X\n",
            (uint32_t)(0xFFFFFFFFu & (write_time >> 32)),
            (uint32_t)(0xFFFFFFFFu & (write_time)),
            (uint32_t)addr & 0xF, (uint32_t)data & 0xFF);

    ay8910_write(bus, req, addr, data);
}


/*
 * ============================================================================
 *  AY8910_WRITE     -- Write to device.
 * ============================================================================
 */
void ay8910_write
(
    periph_t        *bus,       /*  Peripheral bus being written.       */
    periph_t        *req,       /*  Peripheral requesting write.        */
    uint32_t        addr,       /*  Address being written.              */
    uint32_t        data        /*  Data being written.                 */
)
{
    ay8910_t *const ay8910 = PERIPH_AS(ay8910_t, bus);
    int old_max, new_max, max_chg;
    uint64_t write_time = req && req->req ? req->req->now + 4
                                         : ay8910->sound_current;
    int chan = addr & 3;

    addr &= 15;
    if (addr >= 14) return;

    /* -------------------------------------------------------------------- */
    /*  Before processing the write, calculate the sound output up to       */
    /*  just before the write occurred.                                     */
    /* -------------------------------------------------------------------- */
    if (write_time > ay8910->sound_current + ay8910->accutick)
    {
        ay8910->unaccounted += ay8910_calc_sound(ay8910, write_time);

        if (write_time > (ay8910->sound_current + 4))
        {
            /* ------------------------------------------------------------ */
            /*  Request that a frame be dropped.                            */
            /* ------------------------------------------------------------ */
            ay8910->snd_buf.drop++;

#if 0
            jzp_printf("short sim: %.8X vs %.8X\n",
                  (uint32_t)write_time, (uint32_t)ay8910->sound_current);
#endif
        }

    } else if (write_time < ay8910->sound_current)
        jzp_printf("sound ahead: %.8X vs %.8X\n",
                   (uint32_t)write_time, (uint32_t)ay8910->sound_current);

    if      (addr >= 4  && addr <= 6 ) data &= 0x0F;
    else if (addr == 9               ) data &= 0x1F;
    else if (addr == 10              ) data &= 0x0F;
    else if (addr >= 11 && addr <= 13) data &= 0x3F;
    else                               data &= 0xFF;

    ay8910->reg[addr] = data;

    /* -------------------------------------------------------------------- */
    /*  Perform higher-order actions required of specific register writes.  */
    /* -------------------------------------------------------------------- */

    /* -------------------------------------------------------------------- */
    /*  Regs R0..R3 hold the 8 LSBs of the channel/envelope period.         */
    /* -------------------------------------------------------------------- */
    if (addr < 3)
    {
        old_max = ay8910->max[chan];
        new_max = (old_max & 0x0F00) | data;
        if (new_max == 0x000) new_max = 0x1000;
        max_chg = new_max - old_max;
        ay8910->max[chan]  = new_max;
/*      ay8910->cnt[chan] += max_chg;*/

        /*jzp_printf("LCH%d: %.4X %.4X\n", addr&3, new_max, ay8910->cnt[addr&3]);*/
    } else
    if (addr == 3)
    {
        old_max = ay8910->max[3];
        new_max = (old_max & 0x1FE00) | (data + data);
        if (new_max == 0x0000) new_max = 0x20000;
        max_chg = new_max - old_max;
        ay8910->max[3]  = new_max;
/*      ay8910->cnt[3] += max_chg;*/
    } else
    /* -------------------------------------------------------------------- */
    /*  Regs R4..R6 hold the 4 MSBs of the channel period.                  */
    /* -------------------------------------------------------------------- */
    if (addr > 3 && addr < 7)
    {
        old_max = ay8910->max[chan];
        new_max = (old_max & 0x00FF) | ((data << 8) & 0x0F00);
        if (new_max == 0x000) new_max = 0x1000;
        max_chg = new_max - old_max;
        ay8910->max[chan]  = new_max;
/*      ay8910->cnt[chan] += max_chg;*/

#ifdef PSG_DEBUG
        jzp_printf("HCH%d: %.4X %.4X\n", addr&3,
               new_max, 0xFFFF&ay8910->cnt[addr&3]);
#endif
    } else
    /* -------------------------------------------------------------------- */
    /*  Reg R7 holds the 8 MSBs of the envelope period.                     */
    /* -------------------------------------------------------------------- */
    if (addr == 7)
    {
        old_max = ay8910->max[3];
        new_max = (old_max & 0x01FE) | (data << 9);
        if (new_max == 0x0000) new_max = 0x20000;
        max_chg = new_max - old_max;
        ay8910->max[3]  = new_max;
/*      ay8910->cnt[3] += max_chg;*/

#ifdef PSG_DEBUG
        jzp_printf("ECNT: %.4X %.4X\n", new_max, ay8910->cnt[3]);
#endif
    } else
    /* -------------------------------------------------------------------- */
    /*  Reg R9 holds the 5-bit noise period.                                */
    /* -------------------------------------------------------------------- */
    if (addr == 9)
    {
        old_max = ay8910->max[4];
        new_max = (data & 31) << 1;
        if (new_max == 0) new_max = 64;
        max_chg = new_max - old_max;
        ay8910->max[4]  = new_max;
    /*  ay8910->cnt[4] += max_chg;*/

    } else
    /* -------------------------------------------------------------------- */
    /*  Reg R10 holds the envelope type and triggers an envelope count      */
    /*  reset.                                                              */
    /* -------------------------------------------------------------------- */
    if (addr == 10)
    {
#ifdef PSG_DEBUG
        int old_cnt = ay8910->cnt[5];
#endif
        ay8910->demo_env_hit = 1;
        ay8910->cnt[5] = 0;
        ay8910->cnt[3] = ay8910->max[3];

        ay8910->env_hold = (data >> 0) & 1;
        ay8910->env_altr = (data >> 1) & 1;
        ay8910->env_atak = (data >> 2) & 1;
        ay8910->env_cont = (data >> 3) & 1;

        ay8910->env_vol  = ay8910->env_atak ? ay8910_vol[0] : ay8910_vol[15];

#ifdef PSG_DEBUG
        jzp_printf("ENVT: %d%d%d%d %.2X %.4X\n",
                !!(data&8), !!(data&4), !!(data&2), data&1, data, old_cnt);
#endif
    } else
    /* -------------------------------------------------------------------- */
    /*  Error check envelope-select bits in volume registers.               */
    /* -------------------------------------------------------------------- */
    if (addr >= 11 && addr <= 13)
    {
#if 0
        if (1 & ((data >> 4) ^ (data >> 5)))
        {
            extern int debug_fault_detected;
            int per, chan = (addr & 0xF) - 11;
            per = ay8910->reg[chan] | (ay8910->reg[chan+4]<<8);
            fprintf(stderr, "Warning: %.4X written to AY8910[%.1X], per=%.4X %c%c\n",
                    data & 0xFFFF, addr & 0xF, per,
                    ay8910->reg[8] & (chan    ) ? '-' : 'T',
                    ay8910->reg[8] & (chan + 3) ? '-' : 'N');
            debug_fault_detected = DEBUG_CRASHING;
        }
#endif
#ifdef PSG_DEBUG
        jzp_printf("VOL%d: %.4X \n", addr-11, data);
#endif
    }

    /* -------------------------------------------------------------------- */
    /*  Mark max_chg unused for now until we rationalize how to emulate     */
    /*  all the various PSG variants out there.                             */
    /* -------------------------------------------------------------------- */
    UNUSED(max_chg);
}

/*
 * ============================================================================
 *  AY8910_TICK      -- Tick the device.
 * ============================================================================
 */
uint32_t ay8910_tick
(
    periph_t        *bus,       /*  Peripheral bus being ticked.        */
    uint32_t        len
)
{
    ay8910_t *const ay8910 = PERIPH_AS(ay8910_t, bus);
    uint64_t elapsed = ay8910->unaccounted;
    uint64_t soon    = bus->now + len;

    if (ay8910->cpu_time && soon > *ay8910->cpu_time)
        soon = *ay8910->cpu_time;

    ay8910->unaccounted = 0;

    if (soon > ay8910->sound_current)
        elapsed += ay8910_calc_sound(ay8910, soon);

    if (ay8910->sound_current >= bus->now)
        return elapsed;

    return 0;
}

LOCAL const int ay8910_eshift[4] = { 31, 2, 1, 0 };

/*
 * ============================================================================
 *  AY8910_CALC_SOUND    -- The device.
 * ============================================================================
 */
LOCAL uint32_t ay8910_calc_sound(ay8910_t *psg, uint64_t until)
{
    /* -------------------------------------------------------------------- */
    /*  This is a rather inefficient implementation that is striving for    */
    /*  sound quality and correctness, not speed.  Speed comes next.        */
    /* -------------------------------------------------------------------- */
    uint32_t    elapsed = 0;
    uint64_t    max_step;
    int         step;
    int         sample;
    int         hit_a, hit_b, hit_c, hit_e, hit_n;
    int         chn_a, chn_b, chn_c, chn_n, env_cnt, env_vol;
    int         bit_a, bit_b, bit_c;
    int         snd_a, snd_b, snd_c, noi_a, noi_b, noi_c;
    int         vol_a, vol_b, vol_c, val_a, val_b, val_c;
    int         esh_a, esh_b, esh_c;
    uint32_t    rng;
    int         cnt0, cnt1, cnt2, cntE, cntN;
    int         max0, max1, max2, maxE, maxN;
    int         zero_vol = ay8910_vol[0];

    if (until <= (psg->sound_current + 3))
        return 0;

    /* -------------------------------------------------------------------- */
    /*  If we don't have a current buffer, see if we can get a clean one.   */
    /*  If not, then we can't process any data, and time doesn't advance.   */
    /* -------------------------------------------------------------------- */
    if (!psg->cur_buf)
    {
        if (psg->snd_buf.num_clean)
        {
            psg->cur_buf = psg->snd_buf.clean[--psg->snd_buf.num_clean];
            psg->cur_len = 0;
        } else
            return 0;
    }

    /* -------------------------------------------------------------------- */
    /*  Load up some PSG state varables into locals for efficiency.         */
    /* -------------------------------------------------------------------- */
    cnt0 = psg->cnt[0];      
    cnt1 = psg->cnt[1];      
    cnt2 = psg->cnt[2];      
    cntE = psg->cnt[3];      
    cntN = psg->cnt[4];      

    max0 = psg->max[0] * psg->time_scale;       if (!max0) max0 = 1;
    max1 = psg->max[1] * psg->time_scale;       if (!max1) max1 = 1;
    max2 = psg->max[2] * psg->time_scale;       if (!max2) max2 = 1;
    maxE = psg->max[3] * psg->time_scale;       if (!maxE) maxE = 1;
    maxN = psg->max[4] * psg->time_scale;       if (!maxN) maxN = 1;

    env_cnt = psg->cnt[5];
    env_vol = psg->env_vol;

    chn_a = psg->chan[0] & 1;
    chn_b = psg->chan[1] & 1;
    chn_c = psg->chan[2] & 1;
    rng   = psg->noise_rng;
    chn_n = rng & 1;


    snd_a = (psg->reg[8] >> 0) & 1; noi_a = (psg->reg[8] >> 3) & 1;
    snd_b = (psg->reg[8] >> 1) & 1; noi_b = (psg->reg[8] >> 4) & 1;
    snd_c = (psg->reg[8] >> 2) & 1; noi_c = (psg->reg[8] >> 5) & 1;

    esh_a = ay8910_eshift[(psg->reg[11] >> 4) & 0x3];
    esh_b = ay8910_eshift[(psg->reg[12] >> 4) & 0x3];
    esh_c = ay8910_eshift[(psg->reg[13] >> 4) & 0x3];
    vol_a = psg->reg[11];       vol_a = vol_a & 0x30 ? -1 : ay8910_vol[vol_a];
    vol_b = psg->reg[12];       vol_b = vol_b & 0x30 ? -1 : ay8910_vol[vol_b];
    vol_c = psg->reg[13];       vol_c = vol_c & 0x30 ? -1 : ay8910_vol[vol_c];


    /* -------------------------------------------------------------------- */
    /*  Tick the time away.                                                 */
    /* -------------------------------------------------------------------- */
    while ((psg->sound_current + 3) < until)
    {
        max_step = (until - psg->sound_current) >> 2;
        max_step = max_step > 256 ? 256 : max_step;
        step = cnt0 < cnt1          ? cnt0 : cnt1;
        step = step < cnt2          ? step : cnt2;
        step = step < cntN          ? step : cntN;
        step = step < cntE          ? step : cntE;
        step = step < 0             ? 0    : step; /* cntX may be -ve! */
        step = step < (int)max_step ? step : (int)max_step;

        elapsed            += 4 * step;
        psg->sound_current += 4 * step;

        /* ---------------------------------------------------------------- */
        /*  Decrement the channel counters.                                 */
        /*  NOTE:  I'm not sure if they toggle channels if max == 0.        */
        /* ---------------------------------------------------------------- */
        hit_a = hit_b = hit_c = hit_e = hit_n = 0;

        if ((cnt0 -= step) <= 0) { hit_a = 1; cnt0 += max0; }
        if ((cnt1 -= step) <= 0) { hit_b = 1; cnt1 += max1; }
        if ((cnt2 -= step) <= 0) { hit_c = 1; cnt2 += max2; }
        if ((cntN -= step) <= 0) { hit_n = 1; cntN += maxN; }
        if ((cntE -= step) <= 0) { hit_e = 1; cntE += maxE; }

        /* ---------------------------------------------------------------- */
        /*  Handle noise generator.                                         */
        /* ---------------------------------------------------------------- */
        if (hit_n)
        {
            rng = (rng >> 1) ^ (chn_n ? 0x10004 : 0);
            chn_n = rng & 1;
        }

        /* ---------------------------------------------------------------- */
        /*  Handle envelope generator.                                      */
        /* ---------------------------------------------------------------- */
        if (hit_e && env_cnt >= 0)
        {
            int env_idx = 0;

            /* ------------------------------------------------------------ */
            /*  Increment the envelope counter.                             */
            /* ------------------------------------------------------------ */
            env_cnt++;
            env_cnt &= 31;

            /* ------------------------------------------------------------ */
            /*  Most common case: count < 16, index == count XOR direction  */
            /* ------------------------------------------------------------ */
            if (env_cnt < 16)
            {
                env_idx = psg->env_atak ? env_cnt : (15 - env_cnt);
            }
            /* ------------------------------------------------------------ */
            /*  Handle halting cases at top of the 16-step ramp.            */
            /*   -- If CONT==0, zero out the volume and stop the envelope.  */
            /*   -- If HOLD==1, set our volume to the appropriate level     */
            /*      and stop the envelope.                                  */
            /* ------------------------------------------------------------ */
            else if (env_cnt == 16 && (!psg->env_cont || psg->env_hold))
            {
                env_idx = psg->env_cont & (psg->env_atak^psg->env_altr) ? 15:0;
                env_cnt = -1;
            }
            /* ------------------------------------------------------------ */
            /*  If count == 16 && waveform doesn't alternate, reset count.  */
            /* ------------------------------------------------------------ */
            else if (env_cnt == 16 && !psg->env_altr)
            {
                env_cnt = 0;
                env_idx = psg->env_atak ? 0 : 15;
            }
            /* ------------------------------------------------------------ */
            /*  Waveform alternates and count is >= 16, so alternate it.    */
            /* ------------------------------------------------------------ */
            else if (env_cnt >= 16)
            {
                env_idx  = psg->env_atak ? (15 - env_cnt) : env_cnt;
            }

            env_vol = ay8910_vol[env_idx & 15];
        }

        /* ---------------------------------------------------------------- */
        /*  Recalculate sample.                                             */
        /* ---------------------------------------------------------------- */
        bit_a = (snd_a | chn_a) & (noi_a | chn_n);
        bit_b = (snd_b | chn_b) & (noi_b | chn_n);
        bit_c = (snd_c | chn_c) & (noi_c | chn_n);

        chn_a ^= hit_a;
        chn_b ^= hit_b;
        chn_c ^= hit_c;

        val_a = bit_a ? (vol_a < 0 ? env_vol >> esh_a : vol_a) : zero_vol;
        val_b = bit_b ? (vol_b < 0 ? env_vol >> esh_b : vol_b) : zero_vol;
        val_c = bit_c ? (vol_c < 0 ? env_vol >> esh_c : vol_c) : zero_vol;

        sample = val_a + val_b + val_c;

        while (step-->0)
        {
            /* ------------------------------------------------------------ */
            /*  Update the sliding window.                                  */
            /* ------------------------------------------------------------ */
            psg->wind_sum -= psg->window[psg->wind_ptr];
            psg->wind_sum += psg->window[psg->wind_ptr] = sample;
            if (++psg->wind_ptr >= psg->wind) psg->wind_ptr = 0;

            /* ------------------------------------------------------------ */
            /*  Update the output buffer every so often according to our    */
            /*  sample rate and the availability of buffer space.           */
            /* ------------------------------------------------------------ */
            psg->sample_frc += (psg->rate << 4) / psg->time_scale;
            if (psg->sample_frc >= psg->sys_clock)
            {
                int s;

                /* -------------------------------------------------------- */
                /*  See if the buffer is full.  If so, put it on the dirty  */
                /*  buffer list.                                            */
                /* -------------------------------------------------------- */
                if (psg->cur_len >= psg->snd_buf.snd->buf_size)
                {
                    /* ---------------------------------------------------- */
                    /*  It is.  Put it on the dirty list.                   */
                    /* ---------------------------------------------------- */
                    psg->snd_buf.dirty[psg->snd_buf.num_dirty] = psg->cur_buf;
                    psg->snd_buf.num_dirty++;

                    /* ---------------------------------------------------- */
                    /*  Try to get a clean buffer.                          */
                    /* ---------------------------------------------------- */
                    if (psg->snd_buf.num_clean == 0)
                    {
                        /* ------------------------------------------------ */
                        /*  No clean buffers:  Abort early.  *sniffle*      */
                        /* ------------------------------------------------ */
                        psg->cur_buf = NULL;
                        goto no_buffer;
                    }

                    /* ---------------------------------------------------- */
                    /*  Pull the clean buffer off the end of the list.      */
                    /* ---------------------------------------------------- */
                    psg->snd_buf.num_clean--;
                    psg->cur_buf = psg->snd_buf.clean[psg->snd_buf.num_clean];
                    psg->cur_len = 0;
                }

#if 1
                psg->sample_frc -= psg->sys_clock;
                /* -------------------------------------------------------- */
                /*  Store out the sliding window average.                   */
                /* -------------------------------------------------------- */
                s = psg->wind_sum / psg->wind;
                if (s > 0x6000) s = 0x6000 + (s - 0x6000)/6;
                psg->cur_buf[psg->cur_len++] = s;
#else
                psg->cur_buf[psg->cur_len++] = 0;
#endif
            }
        }
    }
no_buffer:

    /* -------------------------------------------------------------------- */
    /*  Save the modified PSG state variables, and return elapsed time.     */
    /* -------------------------------------------------------------------- */

    psg->cnt[0] = cnt0;
    psg->cnt[1] = cnt1;
    psg->cnt[2] = cnt2;
    psg->cnt[3] = cntE;
    psg->cnt[4] = cntN;
    psg->cnt[5] = env_cnt;

    psg->chan[0] = chn_a & 1;
    psg->chan[1] = chn_b & 1;
    psg->chan[2] = chn_c & 1;

    psg->env_vol     = env_vol;
    psg->noise_rng   = rng;

    return elapsed;
}

/*
 * ============================================================================
 *  AY8910_RESET     -- Reset the PSG
 * ============================================================================
 */
LOCAL void ay8910_reset
(
    periph_t        *bus        /*  Peripheral bus being reset.         */
)
{
    ay8910_t *const ay8910 = PERIPH_AS(ay8910_t, bus);
    int i;

    for (i = 0; i < 5; i++)
    {
        ay8910->max[i] = 1;
        ay8910->cnt[i] = 0;
    }
    ay8910->cnt[5] = 0;

    for (i = 0; i < 14; i++)
        ay8910->reg[i] = 0;

    ay8910->env_hold = 0;
    ay8910->env_altr = 0;
    ay8910->env_atak = 0;
    ay8910->env_cont = 0;
    ay8910->env_vol  = ay8910_vol[0];

    for (i = 0; i < 14; i++)
        ay8910->periph.write(AS_PERIPH(ay8910), AS_PERIPH(ay8910), i, 0);
}


/*
 * ============================================================================
 *  AY8910_D_WR      -- Dummy write to PSG
 * ============================================================================
 */
LOCAL void ay8910_d_wr
(
    periph_t        *bus,       /*  Peripheral bus being written.       */
    periph_t        *req,       /*  Peripheral requesting write.        */
    uint32_t        addr,       /*  Address being written.              */
    uint32_t        data        /*  Data being written.                 */
)
{
    ay8910_t *const ay8910 = PERIPH_AS(ay8910_t, bus);

    UNUSED(req);

    addr &= 15;

    if      (addr >= 4  && addr <= 6 ) data &= 0x0F;
    else if (addr == 9               ) data &= 0x1F;
    else if (addr == 10              ) data &= 0x0F;
    else if (addr >= 11 && addr <= 13) data &= 0x3F;
    else                               data &= 0xFF;

    if (addr < 14) ay8910->reg[addr] = data;
}



/*
 * ============================================================================
 *  AY8910_SER_INIT      -- Registers the PSG w/ the serializer.
 * ============================================================================
 */
LOCAL void ay8910_ser_init(periph_t *p)
{
#ifdef NO_SERIALIZER
    UNUSED(p);
#else
    ay8910_t *const psg = PERIPH_AS(ay8910_t, p);
    ser_hier_t *hier, *phier;

    hier = ser_new_hierarchy(NULL, p->name);
    phier = ser_new_hierarchy(hier, "periph");

    ser_register(hier, "reg",      psg->reg, ser_u16, 14, SER_HEX|SER_MAND);
    ser_register(hier, "max",      psg->max, ser_s32, 5,  SER_HEX|SER_MAND);
    ser_register(hier, "cnt",      psg->cnt, ser_s32, 6,  SER_HEX|SER_MAND);
    ser_register(hier, "env_cont", &psg->env_cont, ser_s32, 1, SER_MAND);
    ser_register(hier, "env_atak", &psg->env_atak, ser_s32, 1, SER_MAND);
    ser_register(hier, "env_altr", &psg->env_altr, ser_s32, 1, SER_MAND);
    ser_register(hier, "env_hold", &psg->env_hold, ser_s32, 1, SER_MAND);
    ser_register(hier, "env_vol",  &psg->env_vol,  ser_s32, 1, SER_MAND);
    ser_register(hier, "env_samp", &psg->env_samp, ser_s32, 1, SER_MAND);
    ser_register(hier, "chan",     psg->chan,      ser_s32, 3, SER_MAND);

    periph_ser_register(p, phier);
#endif
}


/*
 * ============================================================================
 *  AY8910_DTOR          -- Deconstructs the PSG
 * ============================================================================
 */
LOCAL void ay8910_dtor(periph_t *p)
{
    ay8910_t *psg = PERIPH_AS(ay8910_t, p);

    /* -------------------------------------------------------------------- */
    /*  Only free what we allocated; Let snd_t free its sound buffers.      */
    /* -------------------------------------------------------------------- */
    CONDFREE(psg->window);
    CONDFREE(psg->trace_filename);

    if (psg->trace)
        fclose(psg->trace);
}

/*
 * ============================================================================
 *  AY8910_INIT          -- Makes a new PSG.
 * ============================================================================
 */
int ay8910_init
(
    ay8910_t       *ay8910,     /*  Structure to initialize.        */
    uint32_t        addr,       /*  Base address of ay8910.         */
    snd_t          *snd,        /*  Sound device to register w/.    */
    int             rate,       /*  Desired sample rate.            */
    int             wind,       /*  Sliding window size.            */
    int             accutick,   /*  Min ticks to simulate           */
    double          time_scale, /*  For --macho                     */
    int             pal_mode,   /*  PAL vs. NTSC                    */
    uint64_t       *cpu_time    /*  HACK: don't get ahead of CPU    */
)
{
    int i;
    char *trace_filename = NULL, *env;
    FILE *trace_file = NULL;
    int sys_clock = pal_mode ? 4000000 : 3579545;

    /* -------------------------------------------------------------------- */
    /*  First, lets zero out the structure to be safe.                      */
    /* -------------------------------------------------------------------- */
    memset(ay8910, 0, sizeof(ay8910_t));

    /* -------------------------------------------------------------------- */
    /*  Sanity checks.                                                      */
    /* -------------------------------------------------------------------- */
    if (wind < 1 && wind != -1)
    {
        fprintf(stderr, "ay8910:  Window size of %d is invalid.  Must be at "
                        "least 1.\n", wind);
        return -1;
    }

    if ((rate < 4000 || rate > 96000) && rate != 0)
    {
        fprintf(stderr, "ay8910:  Sampling rate of %d is invalid.  Must be "
                        "between 4000 and 96000.\n", rate);
        return -1;
    }


    /* -------------------------------------------------------------------- */
    /*  Set up the peripheral.                                              */
    /* -------------------------------------------------------------------- */
    ay8910->periph.read      = ay8910_read;
    ay8910->periph.write     = rate ? ay8910_write : ay8910_d_wr;
    ay8910->periph.peek      = ay8910_read;
    ay8910->periph.poke      = rate ? ay8910_write : ay8910_d_wr;
    ay8910->periph.tick      = rate ? ay8910_tick  : NULL;
    ay8910->periph.reset     = ay8910_reset;
    ay8910->periph.min_tick  = snd->buf_size;
    ay8910->periph.max_tick  = snd->buf_size * 3 / 2;
    ay8910->periph.addr_base = addr;
    ay8910->periph.addr_mask = 15;
    ay8910->periph.ser_init  = ay8910_ser_init;
    ay8910->periph.dtor      = ay8910_dtor;

    if (!rate) return 0;  /* return if no actual sound. */

    /* -------------------------------------------------------------------- */
    /*  If wind == -1, calculate a window size based on the ratio of our    */
    /*  sample rate to the device's native rate.                            */
    /* -------------------------------------------------------------------- */
    if (wind == -1)
    {
        wind = (sys_clock / rate) >> 3;
        if (wind < 1)
            wind = 1;

        jzp_printf("ay8910:  Automatic sliding-window setting: %d\n", wind);
    }


    /* -------------------------------------------------------------------- */
    /*  Configure our internal variables.                                   */
    /* -------------------------------------------------------------------- */
    ay8910->cpu_time        = cpu_time;
    ay8910->sys_clock       = sys_clock;
    ay8910->time_scale      = time_scale;
    ay8910->accutick        = accutick;
    ay8910->rate            = rate;
    ay8910->wind            = wind;
    ay8910->noise_rng       = 1;
    ay8910->window          = CALLOC(int, wind);
    ay8910->wind_sum        = 0;
    if (!ay8910->window)
    {
        fprintf(stderr, "ay8910:  Out of memory allocating sliding window.\n");
        return -1;
    }

    /* -------------------------------------------------------------------- */
    /*  Clear out the PSG on powerup.                                       */
    /* -------------------------------------------------------------------- */
    for (i = 0; i < 14; i++)
        ay8910->periph.write(AS_PERIPH(ay8910), AS_PERIPH(ay8910), i, 0);

    /* -------------------------------------------------------------------- */
    /*  If the JZINTV_PSG_TRACE environment variable is set, change our     */
    /*  setup just slightly so that we can write a register trace out to    */
    /*  the desired filename.  Since multiple PSGs may be in the system,    */
    /*  we append our hex address to the name just before the last period.  */
    /* -------------------------------------------------------------------- */
    if ((env = getenv("JZINTV_PSG_TRACE")) != NULL)
    {
        char *s1, *s2;
        int len;

        len = strlen(env);

        trace_filename  = CALLOC(char, len + 6);
        if (!trace_filename)
        {
            perror("ay8910:  Out of memory opening trace file\n");
            return -1;
        }

        strcpy(trace_filename, env);
        if ((s1 = strrchr(env, '.')) != NULL)
        {
            s2 = trace_filename + (s1 - env);
            snprintf(s2, 5, "%.4X", addr);
            s2 += 4;
            strcpy(s2, s1);
        } else
        {
            s2 = trace_filename + len;
            snprintf(s2, 5, "%.4X", addr);
        }

        trace_file = fopen(trace_filename, "w");
        if (!trace_file)
        {
            perror("ay8910: fopen()");
            fprintf(stderr, "ay8910:  Could not open trace file '%s' "
                    "for writing\n", trace_filename);
            return -1;
        } else
        {
            jzp_printf("ay8910:  Writing trace file to '%s'.\n", trace_filename);
        }

        ay8910->trace_filename  = trace_filename;
        ay8910->trace           = trace_file;
        ay8910->periph.write    = ay8910_trace;
        ay8910->periph.poke     = ay8910_trace;
    }

    /* -------------------------------------------------------------------- */
    /*  Register this as a sound peripheral with the SND driver.            */
    /* -------------------------------------------------------------------- */
    if (snd_register(AS_PERIPH(snd), &ay8910->snd_buf))
    {
        return -1;
    }

    /* -------------------------------------------------------------------- */
    /*  Set up our initial working buffer.                                  */
    /* -------------------------------------------------------------------- */
    ay8910->cur_buf = ay8910->snd_buf.clean[--ay8910->snd_buf.num_clean];
    ay8910->cur_len = 0;

    return 0;
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
/*                 Copyright (c) 1998-2000, Joseph Zbiciak                  */
/* ======================================================================== */
