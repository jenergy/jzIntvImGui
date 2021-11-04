/*
 * ============================================================================
 *  Title:    Sound Interface Abstraction
 *  Author:   J. Zbiciak
 * ============================================================================
 *
 * ============================================================================
 *  SND_TICK     -- Update state of the sound universe.  Drains audio data
 *                  from the PSGs 
 *  SND_REGISTER -- Registers a PSG with the SND module.
 *  SND_INIT     -- Initialize a SND_T
 * ============================================================================
 */

#include "config.h"
#include "periph/periph.h"
#include "snd.h"
#include "avi/avi.h"

LOCAL int32_t *mixbuf = NULL;
LOCAL uint32_t snd_tick(periph_t *const periph, uint32_t len);

/* ======================================================================== */
/*  SND Private structure                                                   */
/* ======================================================================== */
typedef struct snd_pvt_t
{
    avi_writer_t    *avi;
} snd_pvt_t;

/* ======================================================================== */
/*  WAV header.                                                             */
/* ======================================================================== */
LOCAL const uint8_t snd_wav_hdr[44] =
{
    0x52, 0x49, 0x46, 0x46, /*  0  "RIFF"                                   */
    0x00, 0x00, 0x00, 0x00, /*  4  Total file length - 8                    */
    0x57, 0x41, 0x56, 0x45, /*  8  "WAVE"                                   */
    0x66, 0x6D, 0x74, 0x20, /* 12  "fmt "                                   */
    0x10, 0x00, 0x00, 0x00, /* 16  0x10 == PCM                              */
    0x01, 0x00,             /* 20  0x01 == PCM                              */
    0x01, 0x00,             /* 22  1 channel                                */
    0x00, 0x00, 0x00, 0x00, /* 24  Sample rate                              */
    0x00, 0x00, 0x00, 0x00, /* 28  Byte rate:  sample rate * block align    */
    0x02, 0x00,             /* 30  Block align:  channels * bits/sample / 8 */
    0x10, 0x00,             /* 32  Bits/sample = 16.                        */
    0x64, 0x61, 0x74, 0x61, /* 36  "data"                                   */
    0x00, 0x00, 0x00, 0x00  /* 40  Total sample data length                 */
};

/* ======================================================================== */
/*  SND_UPDATE_WAV_HDR -- Keep the WAV header up to date.  Stupid format.   */
/* ======================================================================== */
LOCAL void snd_update_wav_hdr(FILE *f, int rate)
{
    long     filepos;
    uint32_t tot_smp_bytes, tot_file_bytes;
    uint8_t  upd_wav_hdr[44];

    /* -------------------------------------------------------------------- */
    /*  Poke the size information into the header both places it appears.   */
    /*  Do this to a private copy of the header.                            */
    /* -------------------------------------------------------------------- */
    memcpy(upd_wav_hdr, snd_wav_hdr, 44);

    filepos = ftell(f);
    if (filepos < 0)      /* do nothing if ftell() failed. */
        return;

    tot_smp_bytes  = filepos > 44 ? filepos - 44 : 0;
    tot_file_bytes = filepos >  8 ? filepos -  8 : 0;

    /* Total file size, minus first 8 bytes */
    upd_wav_hdr[ 4] = (tot_file_bytes >>  0) & 0xFF;
    upd_wav_hdr[ 5] = (tot_file_bytes >>  8) & 0xFF;
    upd_wav_hdr[ 6] = (tot_file_bytes >> 16) & 0xFF;
    upd_wav_hdr[ 7] = (tot_file_bytes >> 24) & 0xFF;

    /* Total size of sample-data payload */
    upd_wav_hdr[40] = (tot_smp_bytes  >>  0) & 0xFF;
    upd_wav_hdr[41] = (tot_smp_bytes  >>  8) & 0xFF;
    upd_wav_hdr[42] = (tot_smp_bytes  >> 16) & 0xFF;
    upd_wav_hdr[43] = (tot_smp_bytes  >> 24) & 0xFF;

    /* Poke in the sample rate / byte rate too */
    upd_wav_hdr[24] = (rate           >>  0) & 0xFF;
    upd_wav_hdr[25] = (rate           >>  8) & 0xFF;
    upd_wav_hdr[26] = (rate           >> 16) & 0xFF;
    upd_wav_hdr[27] = (rate           >> 24) & 0xFF;
    upd_wav_hdr[28] = (rate * 2       >>  0) & 0xFF;
    upd_wav_hdr[29] = (rate * 2       >>  8) & 0xFF;
    upd_wav_hdr[30] = (rate * 2       >> 16) & 0xFF;
    upd_wav_hdr[31] = (rate * 2       >> 24) & 0xFF;

    /* -------------------------------------------------------------------- */
    /*  Rewind to the beginning of the file to write the header.  Skip      */
    /*  it if the seek fails for some reason.                               */
    /* -------------------------------------------------------------------- */
    if (fseek(f, 0, SEEK_SET) == 0)
    {
        fwrite(upd_wav_hdr, 44, 1, f);
        fseek(f,  0, SEEK_END);
    }
}

/*
 * ============================================================================
 *  SND_TICK     -- Update state of the sound universe.  Drains audio data
 *                  from the PSGs
 * ============================================================================
 */
LOCAL uint32_t snd_tick(periph_t *const periph, uint32_t len)
{
    snd_t *const snd = PERIPH_AS(snd_t, periph);
    int min_num_dirty;
    int i, j, k, mix;
    int try_drop = 0, did_drop = 0, dly_drop = 0;
    int16_t *clean;
    uint64_t new_now;
    int not_silent = snd->raw_start;
    const int avi_active = avi_is_active(snd->pvt->avi);

    /* -------------------------------------------------------------------- */
    /*  Check for volume up/down requests.                                  */
    /* -------------------------------------------------------------------- */
    if (snd->change_vol == 1) snd->atten -= (snd->atten > 0);
    if (snd->change_vol == 2) snd->atten += (snd->atten < 32);
    snd->change_vol = 0;

    /* -------------------------------------------------------------------- */
    /*  Trival case:  No sound devices == no work for us.                   */
    /* -------------------------------------------------------------------- */
    if (snd->src_cnt == 0)
        return len;

    /* -------------------------------------------------------------------- */
    /*  If all of our buffers are dirty, we can't do anything, so return    */
    /*  the fact that we've made no progress.                               */
    /* -------------------------------------------------------------------- */
    if (snd->mixbuf.num_clean == 0)
        return 0;

    /* -------------------------------------------------------------------- */
    /*  Calculate the minimum number of dirty buffers to process.  We try   */
    /*  to pull as many dirty buffers as we can while keeping all sources   */
    /*  synchronized.                                                       */
    /* -------------------------------------------------------------------- */
    min_num_dirty = snd->src[0]->num_dirty;
    for (i = 1; i < snd->src_cnt; i++)
    {
        if (min_num_dirty > snd->src[i]->num_dirty)
            min_num_dirty = snd->src[i]->num_dirty;
    }

    /* -------------------------------------------------------------------- */
    /*  Try to drop everything coming to us.  However, if we're dumping     */
    /*  sound to a raw audio file or AVI, don't actually drop anything      */
    /*  until after we've written the audio out to file.                    */
    /* -------------------------------------------------------------------- */
    try_drop = min_num_dirty;
    if (snd->raw_file || avi_active)
    {
        dly_drop = try_drop;
        try_drop = 0;
    }

    /* -------------------------------------------------------------------- */
    /*  Update the drop count by the number of buffers that we can drop     */
    /*  during this update pass.                                            */
    /* -------------------------------------------------------------------- */
    did_drop = try_drop;
    //snd->mixbuf.tot_drop += did_drop;  /* doesn't make sense to report it */

    /* -------------------------------------------------------------------- */
    /*  Merge the dirty buffers together into mix buffers, and place the    */
    /*  dirty buffers back on the clean list.  This will allows the sound   */
    /*  devices to continue generating sound while we wait for the mixed    */
    /*  data to play.                                                       */
    /* -------------------------------------------------------------------- */
    assert(try_drop == 0 || dly_drop == 0);
    for (i = try_drop; i < min_num_dirty; i++)
    {
        int clean_idx;
        /* ---------------------------------------------------------------- */
        /*  Remove a buffer from the clean list for mixing.                 */
        /* ---------------------------------------------------------------- */
        clean_idx = --snd->mixbuf.num_clean;
        clean = snd->mixbuf.clean[clean_idx];
        snd->mixbuf.clean[clean_idx] = NULL;    /*  spot bugs!              */

        /* ---------------------------------------------------------------- */
        /*  Simple case:  One source -- no mixing required.                 */
        /* ---------------------------------------------------------------- */
        if (snd->src_cnt == 1)
        {
            int16_t *tmp;

            /*memcpy(clean, snd->src[0]->dirty[i], snd->buf_size * 2);*/

            /* ------------------------------------------------------------ */
            /*  XXX: THIS IS AN EVIL HACK.  I SHOULD BE DOING THE MEMCPY!   */
            /* ------------------------------------------------------------ */
            tmp = snd->src[0]->dirty[i];
            snd->src[0]->dirty[i] = clean;
            clean = tmp;

            /* ------------------------------------------------------------ */
            /*  Handle writing sound to raw files.                          */
            /* ------------------------------------------------------------ */
            if (snd->raw_file || !snd->raw_start)
                for (j = 0; j < snd->buf_size && !not_silent; j++)
                    not_silent = clean[j];

            goto one_source;
        }

        /* ---------------------------------------------------------------- */
        /*  Accumulate all of the source buffers at 32-bit precision.       */
        /* ---------------------------------------------------------------- */
        memset(mixbuf, 0, snd->buf_size * sizeof(int));
        for (j = 0; j < snd->src_cnt; j++)
        {
            for (k = 0; k < snd->buf_size; k++)
                mixbuf[k] += snd->src[j]->dirty[i][k];
        }

        /* ---------------------------------------------------------------- */
        /*  Saturate the mix results to 16-bit precision and place them     */
        /*  in the formerly-clean mix buffer.                               */
        /* ---------------------------------------------------------------- */
        for (j = 0; j < snd->buf_size; j++)
        {
            mix = mixbuf[j];
            if (mix >  0x7FFF) mix =  0x7FFF;
            if (mix < -0x8000) mix = -0x8000;
            clean[j] = mix;
            not_silent |= mix;
        }

        /* ---------------------------------------------------------------- */
        /*  "Atomically" place this in the dirty buffer list so that the    */
        /*  snd_fill() routine can get to it.  Handle delayed-drops due to  */
        /*  file writing here.                                              */
        /* ---------------------------------------------------------------- */
one_source:

        /* ---------------------------------------------------------------- */
        /*  If an AVI is open, send the audio to the AVI file.              */
        /* ---------------------------------------------------------------- */
        if (avi_active && avi_start_time(snd->pvt->avi) < snd->periph.now)
            avi_record_audio(snd->pvt->avi, clean, snd->buf_size,
                             !not_silent);

        /* ---------------------------------------------------------------- */
        /*  If we're also writing this out to an audio file, do that last.  */
        /* ---------------------------------------------------------------- */
        if (snd->raw_file && not_silent)
        {
            fwrite(clean, sizeof(int16_t), snd->buf_size,
                    snd->raw_file);
            snd->raw_start = 1;
        }

        /* ---------------------------------------------------------------- */
        /*  If this frame wasn't actually put on the mixer's dirty list     */
        /*  because we're dropping it, then put it back on the clean list.  */
        /* ---------------------------------------------------------------- */
        assert(dly_drop > 0);
        if (dly_drop > 0)
        {
            snd->mixbuf.clean[snd->mixbuf.num_clean++] = clean;
            dly_drop--;
            did_drop++;
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Update our sources with how much we actually _did_ drop.            */
    /* -------------------------------------------------------------------- */
    for (i = 0; i < snd->src_cnt; i++)
    {
        snd->src[i]->tot_drop += did_drop;
        if (snd->src[i]->drop >= did_drop)
            snd->src[i]->drop -= did_drop;
        else
            snd->src[i]->drop = 0;
    }

    /* -------------------------------------------------------------------- */
    /*  Now fix up our sources' clean and dirty lists.                      */
    /* -------------------------------------------------------------------- */
    for (i = 0; i < snd->src_cnt; i++)
    {
        /* ---------------------------------------------------------------- */
        /*  Place the formerly dirty buffers on the clean list.             */
        /* ---------------------------------------------------------------- */
        for (j = 0; j < min_num_dirty; j++)
            snd->src[i]->clean[snd->src[i]->num_clean++] =
                snd->src[i]->dirty[j];

        /* ---------------------------------------------------------------- */
        /*  Remove the formerly dirty buffers from the dirty list.          */
        /* ---------------------------------------------------------------- */
        snd->src[i]->num_dirty -= min_num_dirty;

        if (min_num_dirty > 0)
        {
            for (j = 0; j < snd->src[i]->num_dirty; j++)
            {
                snd->src[i]->dirty[j] = snd->src[i]->dirty[min_num_dirty + j];
                snd->src[i]->dirty[min_num_dirty + j] = NULL;
            }
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Finally, figure out how many system ticks this accounted for.       */
    /* -------------------------------------------------------------------- */
    snd->samples += min_num_dirty * snd->buf_size;
    if (snd->time_scale > 0)
    {
        new_now = (double)snd->samples * snd->cyc_per_sec * snd->time_scale
                                                                  / snd->rate;
        if (new_now >= snd->periph.now)  /* Uhoh... are we slipping away? */
            len = new_now - snd->periph.now;
    }

    /* -------------------------------------------------------------------- */
    /*  If we're writing a sound sample file, update the file length.       */
    /* -------------------------------------------------------------------- */
    if (snd->raw_file && not_silent)
        snd_update_wav_hdr(snd->raw_file, snd->rate);

    return len;
}

/*
 * ============================================================================
 *  SND_REGISTER -- Registers a sound input buffer with the sound object
 * ============================================================================
 */
int snd_register
(
    periph_t    *const per,     /* Sound object.                            */
    snd_buf_t   *const src      /* Sound input buffer.                      */
)
{
    int i;
    snd_t *const snd = PERIPH_AS(snd_t, per);

    /* -------------------------------------------------------------------- */
    /*  Initialize the sound buffer to all 0's and make ourselves parent    */
    /* -------------------------------------------------------------------- */
    memset(src, 0, sizeof(snd_buf_t));
    src->snd       = snd;

    /* -------------------------------------------------------------------- */
    /*  Set up its buffers as 'clean'.                                      */
    /* -------------------------------------------------------------------- */
    src->num_clean = snd->buf_cnt;
    src->tot_buf   = snd->buf_cnt;
    src->num_dirty = 0;

    src->buf   = CALLOC(int16_t,   snd->buf_size * src->num_clean);
    src->clean = CALLOC(int16_t *, src->num_clean);
    src->dirty = CALLOC(int16_t *, src->num_clean);

    if (!src->buf || !src->clean || !src->dirty)
    {
        fprintf(stderr, "snd_register: Out of memory allocating sndbuf.\n");
        return -1;
    }

    for (i = 0; i < src->num_clean; i++)
    {
        src->clean[i] = src->buf + i * snd->buf_size;
        src->dirty[i] = NULL;
    }

    /* -------------------------------------------------------------------- */
    /*  Add this sound source to our list of sound sources.                 */
    /* -------------------------------------------------------------------- */
    snd->src_cnt++;
    snd->src = (snd_buf_t**) realloc(snd->src,
                                     snd->src_cnt * sizeof(snd_buf_t*));
    if (!snd->src)
    {
        fprintf(stderr, "Error:  Out of memory in snd_register()\n");
        return -1;
    }
    snd->src[snd->src_cnt - 1] = src;

    return 0;
}

LOCAL void snd_dtor(periph_t *const p);

/*
 * ============================================================================
 *  SND_INIT     -- Initialize a SND_T
 * ============================================================================
 */
int snd_init(snd_t *snd, int rate, char *raw_file,
             int user_snd_buf_size, int user_snd_buf_cnt,
             struct avi_writer_t *const avi, int pal_mode, double time_scale)
{
    int i;

    /* -------------------------------------------------------------------- */
    /*  Prepare to fill up SND structure.                                   */
    /* -------------------------------------------------------------------- */
    memset(snd, 0, sizeof(snd_t));
    if (!(snd->pvt = CALLOC(snd_pvt_t, 1)))
    {
        fprintf(stderr, "snd_init: Out of memory allocating snd_pvt_t.\n");
        goto fail;
    }

    snd->buf_size = user_snd_buf_size > 0 ? user_snd_buf_size
                  :                         SND_BUF_SIZE_DEFAULT;

    snd->buf_cnt  = user_snd_buf_cnt  > 0 ? user_snd_buf_cnt
                  :                         SND_BUF_CNT_DEFAULT;


    /* -------------------------------------------------------------------- */
    /*  Hook in AVI writer.                                                 */
    /* -------------------------------------------------------------------- */
    snd->pvt->avi         = avi;

    /* -------------------------------------------------------------------- */
    /*  Set up SND's internal varables.                                     */
    /* -------------------------------------------------------------------- */
    snd->rate             = rate;
    snd->atten            = 0;
    snd->cyc_per_sec      = pal_mode ? 1000000 : 894886;
    snd->time_scale       = time_scale;

    /* -------------------------------------------------------------------- */
    /*  Set up SND as a peripheral.                                         */
    /* -------------------------------------------------------------------- */
    snd->periph.read      = NULL;
    snd->periph.write     = NULL;
    snd->periph.peek      = NULL;
    snd->periph.poke      = NULL;
    snd->periph.tick      = snd_tick;
    snd->periph.min_tick  = snd->buf_size * snd->cyc_per_sec / (2*rate);
    snd->periph.max_tick  = snd->periph.min_tick * 3;
    snd->periph.addr_base = ~0U;
    snd->periph.addr_mask = ~0U;
    snd->periph.dtor      = snd_dtor;

    /* -------------------------------------------------------------------- */
    /*  Set up our mix buffers as 'clean'.                                  */
    /* -------------------------------------------------------------------- */
    snd->mixbuf.tot_buf   = snd->buf_cnt;
    snd->mixbuf.num_clean = snd->buf_cnt;
    snd->mixbuf.num_dirty = 0;
    snd->mixbuf.buf   = CALLOC(int16_t, snd->buf_size * snd->mixbuf.num_clean);
    snd->mixbuf.clean = CALLOC(int16_t *, snd->mixbuf.num_clean);
    snd->mixbuf.dirty = CALLOC(int16_t *, snd->mixbuf.num_clean);

    if (mixbuf) free(mixbuf);
    mixbuf            = CALLOC(int32_t,   snd->buf_size);

    if (!snd->mixbuf.buf || !snd->mixbuf.clean || !snd->mixbuf.dirty || !mixbuf)
    {
        fprintf(stderr, "snd_init: Out of memory allocating mixbuf.\n");
        goto fail;
    }

    for (i = 0; i < snd->mixbuf.num_clean; i++)
        snd->mixbuf.clean[i] = snd->mixbuf.buf + i * snd->buf_size;

    /* -------------------------------------------------------------------- */
    /*  If the user is dumping audio to a raw-audio file, open 'er up.      */
    /* -------------------------------------------------------------------- */
    if (raw_file)
    {
        snd->raw_file = fopen(raw_file, "wb");
        if (!snd->raw_file)
        {
            fprintf(stderr,"snd:  Error opening '%s' for writing.\n",raw_file);
            perror("fopen");
            goto fail;
        }
        snd_update_wav_hdr(snd->raw_file, snd->rate);
        return 0;
    }

    return 0;


fail:
    CONDFREE(snd->pvt);
    CONDFREE(snd->mixbuf.buf);
    CONDFREE(snd->mixbuf.clean);
    CONDFREE(snd->mixbuf.dirty);
    CONDFREE(mixbuf);

    return -1;
}

/* ======================================================================== */
/*  SND_DTOR     -- Tear down the sound device.                             */
/* ======================================================================== */
LOCAL void snd_dtor(periph_t *const p)
{
    snd_t *const snd = PERIPH_AS(snd_t, p);
    int i;

    CONDFREE(mixbuf);
    CONDFREE(snd->mixbuf.buf);
    CONDFREE(snd->mixbuf.clean);
    CONDFREE(snd->mixbuf.dirty);

    CONDFREE(snd->pvt);

    for (i = 0; i < snd->src_cnt; i++)
        if (snd->src[i])
        {
            CONDFREE(snd->src[i]->buf);
            CONDFREE(snd->src[i]->clean);
            CONDFREE(snd->src[i]->dirty);
        }

    CONDFREE(snd->src);
}

/*
 * ============================================================================
 *  SND_PLAY_SILENCE -- Pump silent audio frame. (Used during reset.) 
 *  SND_PLAY_STATIC  -- A silly bit of fun.
 * ============================================================================
 */
void snd_play_silence(snd_t *const snd) { UNUSED(snd); }
void snd_play_static (snd_t *const snd) { UNUSED(snd); }

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
/*                 Copyright (c) 1998-1999, Joseph Zbiciak                  */
/* ======================================================================== */
