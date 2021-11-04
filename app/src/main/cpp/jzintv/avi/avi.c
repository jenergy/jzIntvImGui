//
//  AVI.c
//  CoolCV
//
//  Created by Oscar Toledo on 02/11/15.
//  Copyright (c) 2015 Oscar Toledo. All rights reserved.
//
//  With portions of my private video converter to AVI
//
//  Modified by J. Zbiciak for jzIntv:
//   -- Variable audio rates
//   -- 8bpp palette-based encoding mode (was 32bpp)
//   -- Border color and border region
//   -- Selectable compression
//   -- Slight code and data restructuring

//  ZMBV specs from http://wiki.multimedia.cx/?title=DosBox_Capture_Codec

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "config.h"
#include "plat/plat_lib.h"
#include "zlib/zlib.h"
#include "gfx/palette.h"
#include "avi/avi.h"

#define APP_TITLE       "jzIntv"

#define AVI_AUDIO_BUF   (65536)
#define AVI_VIDEO_BUF_BYTES     (1 << 20)   /* 1MB should be waaay plenty   */
#define AVI_VIDEO_BUF_FRAMES    (4096)      /* waaaaay plenty               */

#define INPUT_DIM_X     (320)
#define INPUT_DIM_Y     (192)

#define AVI_BORD_X_SZ   (16)
#define AVI_BORD_Y_SZ   (16)
#define AVI_DIM_X       (INPUT_DIM_X + 2*AVI_BORD_X_SZ)
#define AVI_DIM_Y       (INPUT_DIM_Y + 2*AVI_BORD_Y_SZ)
#define AVI_BPP         (8)
#define AVI_FRAME_PIX   (AVI_DIM_X * AVI_DIM_Y)
#define AVI_FRAME_BYTES (AVI_DIM_X * AVI_DIM_Y * AVI_BPP / 8)

#define AVI_VIDEO_ENCODE_BYTES (AVI_FRAME_BYTES + 4096)
#define AVI_VIDEO_WRAP_THRESH (AVI_VIDEO_BUF_BYTES - AVI_VIDEO_ENCODE_BYTES)

#define BLOCK_WIDTH     (16)
#define BLOCK_HEIGHT    (16)

#define NUM_X_BLOCK     (AVI_DIM_X / BLOCK_WIDTH)
#define NUM_Y_BLOCK     (AVI_DIM_Y / BLOCK_HEIGHT)

#if AVI_BPP != 8
#error  "Code needs more work to support BPP != 8"
#endif

#if AVI_DIM_X % BLOCK_WIDTH || AVI_DIM_Y % BLOCK_HEIGHT
#error  "Image dimensions must be a multiple of block size"
#endif

#define STRUCT_ALIGN (64)

static double avi_time_scale         = 1.0;
static double audio_time_scale       = 1.0;
static double audio_time_scale_ratio = 1.0;


/* Locations to be patched */
typedef struct avi_container_t
{
    long    pos[6];
    long    base_movie_size;
} avi_container_t;

/* RIFF state */
typedef struct avi_riff_t
{
    FILE   *file;
    int    *index;
    int    *base_index;
    int     index_size;
    int     nesting_level;
    long    nesting[512];
} avi_riff_t;

/* Audio buffering */
typedef struct avi_audio_t
{
    uint16_t    buffer[AVI_AUDIO_BUF];
    uint16_t   *buffer_read;
    uint16_t   *buffer_write;
    int         buffer_total_read;
    int         buffer_total_write;
    double      time_remainder;
} avi_audio_t ALIGN(STRUCT_ALIGN);

typedef struct
{
    uint8_t f[AVI_DIM_Y][AVI_DIM_X];
} avi_frame_t ALIGN(STRUCT_ALIGN);

/* Video buffering */
typedef struct avi_video_t
{
    avi_frame_t frame[2];
    uint8_t     toggle;
    uint8_t     palette[256][3];
    uint8_t     encode_buf[AVI_VIDEO_BUF_BYTES];
    uint32_t    encode_buf_wr, encode_buf_rd;
    uint32_t    encode_len[AVI_VIDEO_BUF_FRAMES];
    uint32_t    encode_len_wr, encode_len_rd;
    uint32_t    advance_frames_remaining;
    double      time_remainder;
} avi_video_t ALIGN(STRUCT_ALIGN);

typedef struct avi_pvt_t
{
    avi_container_t container;  /* Locations to be patched */
    avi_info_t      info;       /* Information and statistics */
    avi_riff_t      riff;       /* RIFF state */
    avi_audio_t     audio;      /* Audio buffering */
    avi_video_t     video;      /* Video buffering */
    z_stream        zstream;    /* ZLib state */
} avi_pvt_t ALIGN(STRUCT_ALIGN);


/*
 ** Write a 32 bits code
 */
LOCAL INLINE void riff_write_code(avi_riff_t *const riff, char *code)
{
    fwrite(code, 1, 4, riff->file);
}

/*
 ** Write a 16 bits integer
 */
LOCAL INLINE void riff_write_int_16(avi_riff_t *const riff, int value)
{
    char buf[2];

    buf[0] = value;
    buf[1] = value >> 8;
    fwrite(buf, 1, 2, riff->file);
}

/*
 ** Write a 32 bits integer
 */
LOCAL INLINE void riff_write_int_32(avi_riff_t *const riff, int value)
{
    char buf[4];

    buf[0] = value;
    buf[1] = value >> 8;
    buf[2] = value >> 16;
    buf[3] = value >> 24;
    fwrite(buf, 1, 4, riff->file);
}

#if 0
/*
 ** Writes a text string
 */
LOCAL INLINE void riff_write_string(avi_riff_t *const riff, char *text)
{
    fwrite(text, 1, strlen(text) + 1, riff->file);
}
#endif

/*
 ** Starts a RIFF element
 */
LOCAL INLINE void riff_start_element(avi_riff_t *const riff, char *element)
{
    riff_write_code(riff, element);
    riff_write_int_32(riff, 0);
    riff->nesting[riff->nesting_level++] = ftell(riff->file);
}

/*
 ** Closes a RIFF element
 */
LOCAL INLINE int riff_close_element(avi_riff_t *const riff)
{
    long c;
    long a;

    c = riff->nesting[--riff->nesting_level];
    a = ftell(riff->file);
    if (a & 1)
        fwrite("\0", 1, 1, riff->file);
    fseek(riff->file, c - 4, SEEK_SET);
    riff_write_int_32(riff, (int) (a - c));
    if (a & 1)
        a++;
    fseek(riff->file, a, SEEK_SET);
    return (int) (a - c);
}

/*
 ** Starts a RIFF structure
 */
LOCAL INLINE void riff_start_structure(avi_riff_t *const riff,
                                       char *list, char *title)
{
    riff_start_element(riff, list);
    riff_write_code(riff, title);
}

/*
 ** Terminate a RIFF structure
 */
LOCAL INLINE int riff_close_structure(avi_riff_t *const riff)
{
    return riff_close_element(riff);
}

// Check in following order:
//  -- Zero offset
//  -- 1 STIC tile in four cardinal directions (N/S/E/W)
//  -- Clockwise spiral from center
// Vectors constrained to even pixels due to pixel-doubled nature of Inty.
static const int movement[][2] =
{
    {   0,   0 }, { -16,   0 }, {  16,   0 }, {   0,  16 }, {   0, -16 },
    {   2,   0 }, {   2,   2 }, {   0,   2 }, {  -2,   2 }, {  -2,   0 },
    {  -2,  -2 }, {   0,  -2 }, {   2,  -2 }, {   4,  -2 }, {   4,   0 },
    {   4,   2 }, {   4,   4 }, {   2,   4 }, {   0,   4 }, {  -2,   4 },
    {  -4,   4 }, {  -4,   2 }, {  -4,   0 }, {  -4,  -2 }, {  -4,  -4 },
    {  -2,  -4 }, {   0,  -4 }, {   2,  -4 }, {   4,  -4 }, {   6,  -4 },
    {   6,  -2 }, {   6,   0 }, {   6,   2 }, {   6,   4 }, {   6,   6 },
    {   4,   6 }, {   2,   6 }, {   0,   6 }, {  -2,   6 }, {  -4,   6 },
    {  -6,   6 }, {  -6,   4 }, {  -6,   2 }, {  -6,   0 }, {  -6,  -2 },
    {  -6,  -4 }, {  -6,  -6 }, {  -4,  -6 }, {  -2,  -6 }, {   0,  -6 },
    {   2,  -6 }, {   4,  -6 }, {   6,  -6 }, {   8,  -6 }, {   8,  -4 },
    {   8,  -2 }, {   8,   0 }, {   8,   2 }, {   8,   4 }, {   8,   6 },
    {   8,   8 }, {   6,   8 }, {   4,   8 }, {   2,   8 }, {   0,   8 },
    {  -2,   8 }, {  -4,   8 }, {  -6,   8 }, {  -8,   8 }, {  -8,   6 },
    {  -8,   4 }, {  -8,   2 }, {  -8,   0 }, {  -8,  -2 }, {  -8,  -4 },
    {  -8,  -6 }, {  -8,  -8 }, {  -6,  -8 }, {  -4,  -8 }, {  -2,  -8 },
    {   0,  -8 }, {   2,  -8 }, {   4,  -8 }, {   6,  -8 }, {   8,  -8 },
};

LOCAL void *avi_zalloc( void *opaque, uInt items, uInt size)
{
    UNUSED(opaque);
    return (void *)malloc(items * size);
}

LOCAL void avi_zfree( void *opaque, void *to_free )
{
    UNUSED(opaque);
    free(to_free);
}

#define NUM_MOVEMENT ((int)(sizeof(movement)/sizeof(movement[0])))

LOCAL INLINE avi_pvt_t *avi_pvt_align(void *pvt_alloc)
{
    intptr_t pvt_i = (intptr_t)pvt_alloc;
    pvt_i += (STRUCT_ALIGN - pvt_i) & (STRUCT_ALIGN - 1);
    return (avi_pvt_t *)pvt_i;
}

/*
 ** Start video recording
 */
int avi_start_video
(
    avi_writer_t *const avi,
    FILE         *const avi_file,
    const int           fps,
    const int           audio_rate,
    const int           compress,
    const uint64_t      start_time
)
{
    // For now, don't try to handle silent AVIs.
    if (!audio_rate)
        return 0;

    int stereo = 0;
    int bits = 16;
    double real_fps = fps == 60 ? 59.92274 : 50.08012; // STIC_FRAMCLKS/MHz

    if (!avi->pvt_alloc)
    {
        /* Allocate a fresh avi_pvt_t for all of our stuff */
        avi->pvt_alloc = (void*)calloc(sizeof(avi_pvt_t) + STRUCT_ALIGN, 1);
        if (!avi->pvt_alloc)
            return -1;

        avi->pvt = avi_pvt_align(avi->pvt_alloc);
    } else
    {
        memset((void*)avi->pvt, 0, sizeof(avi_pvt_t));
    }

    avi_pvt_t       *const pvt       = avi->pvt;
    avi_container_t *const container = &(pvt->container);
    avi_info_t      *const info      = &(pvt->info);
    avi_riff_t      *const riff      = &(pvt->riff);
    avi_audio_t     *const audio     = &(pvt->audio);
    avi_video_t     *const video     = &(pvt->video);
    z_stream        *const zstream   = &(pvt->zstream);

    info->start_time                = start_time;
    info->active                    = 1;
    info->frames_per_sec            = real_fps * 1000000;
    info->usec_per_frame            = 1000000 / real_fps;
    info->audio_rate                = audio_rate;
    info->key_frames                = 32;   // Must be power-of-2
    info->advance_audio_frames      = 0;
    info->total_frames              = 0;
    info->max_size_frame            = 0;
    info->max_size_audio            = 0;
    info->total_audio               = 0;
    info->compress                  = compress;

    audio->buffer_read              = audio->buffer;
    audio->buffer_write             = audio->buffer;
    audio->buffer_total_read        = 0;
    audio->buffer_total_write       = 0;
    audio->time_remainder           = 0;

    video->toggle                   = 0;
    video->encode_buf_rd            = 0;
    video->encode_buf_wr            = 0;
    video->encode_len_rd            = 0;
    video->encode_len_wr            = 0;
    video->advance_frames_remaining = info->advance_audio_frames;
    video->time_remainder           = 0;

    riff->file                      = avi_file;
    riff->nesting_level             = 0;
    riff->index_size                = 1048576;
    riff->index                     = CALLOC(int, riff->index_size * 4);
    riff->base_index                = riff->index;
    if (!riff->index)
        return -1;

    riff_start_structure(riff, "RIFF", "AVI ");
    riff_start_structure(riff, "LIST", "hdrl");
    riff_start_element(riff, "avih");
    riff_write_int_32(riff, info->usec_per_frame);  /* Microsecs per frame */
    riff_write_int_32(riff, 300000);            /* Maximum bytes per second */
    riff_write_int_32(riff, 0);                 /* Reserved */
    riff_write_int_32(riff, 0x0110);            /* Flags */
    /* bit 4 = Includes idx1 */
    /* bit 5 = Use idx1 for everything */
    /* bit 8 = Video and audio intermixed */
    /* bit 16 = Optimized for realtime capture */
    /* bit 17 = Copyrighted data */
    container->pos[4] = ftell(riff->file);
    riff_write_int_32(riff, 0);                 /* Total frames */
    riff_write_int_32(riff, info->advance_audio_frames);
                                                /* Sound frames in advance */
    riff_write_int_32(riff, 2);                 /* Total streams */
    container->pos[0] = ftell(riff->file);
    riff_write_int_32(riff, 0);                 /* Suggested buffer size */
    riff_write_int_32(riff, AVI_DIM_X);         /* X size */
    riff_write_int_32(riff, AVI_DIM_Y);         /* Y size */
    riff_write_int_32(riff, 0);                 /* Scale */
    riff_write_int_32(riff, 0);                 /* Speed */
    riff_write_int_32(riff, 0);                 /* Start */
    riff_write_int_32(riff, 0);                 /* Length */
    riff_close_element(riff);
    /* Video stream */
    riff_start_structure(riff,"LIST","strl");   /* Stream Leader */
    riff_start_element(riff, "strh");           /* Stream Header */
    riff_write_code(riff, "vids");              /* Video */
    riff_write_code(riff, "ZMBV");              /* Codec */
    riff_write_int_32(riff, 0);                 /* Flags */
    riff_write_int_32(riff, 0);                 /* Reserved */
    riff_write_int_32(riff, 0);                 /* Frames in advance */
    riff_write_int_32(riff, 1000000);           /* Scale */
    riff_write_int_32(riff, info->frames_per_sec); /* Speed */
    riff_write_int_32(riff, 0);                 /* Start of clip */
    container->pos[5] = ftell(riff->file);
    riff_write_int_32(riff, 0);                 /* Length of clip in frames */
    container->pos[1] = ftell(riff->file);
    riff_write_int_32(riff, 0);                 /* Suggested buffer site */
    riff_write_int_32(riff, 8000);              /* Quality (by 100) */
    riff_write_int_32(riff, 0);                 /* Sample size */
    riff_write_int_32(riff, 0);                 /* Reserved */
    riff_write_int_32(riff, 0);                 /* Reserved */
    riff_close_element(riff);
    riff_start_element(riff, "strf");           /* Stream Format (idem. BMP) */
    riff_write_int_32(riff, 40);                /* Header size (incl itself) */
    riff_write_int_32(riff, AVI_DIM_X);         /* X size */
    riff_write_int_32(riff, AVI_DIM_Y);         /* Y size */
    riff_write_int_16(riff, 1);                 /* Number of planes */
    riff_write_int_16(riff, AVI_BPP);            /* Bits per pixel */
    riff_write_code(riff, "ZMBV");              /* Codec */
    riff_write_int_32(riff, AVI_FRAME_BYTES);   /* Image size (in bytes) */
    riff_write_int_32(riff, 0);                 /* X pixels per meter */
    riff_write_int_32(riff, 0);                 /* Y pixels per meter */
    riff_write_int_32(riff, 0);                 /* Used colors */
    riff_write_int_32(riff, 0);                 /* Important colors */
    riff_close_element(riff);
    riff_close_structure(riff);
    /* Audio stream */
    riff_start_structure(riff, "LIST", "strl"); /* Stream Leader */
    riff_start_element(riff, "strh");           /* Stream Header */
    riff_write_code(riff, "auds");              /* Audio */
    riff_write_code(riff, "\0\0\0\0");          /* Codec (no compression) */
    riff_write_int_32(riff, 0);                 /* Flags */
    riff_write_int_32(riff, 0);                 /* Reserved */
    riff_write_int_32(riff, info->advance_audio_frames);
                                                /* Frames in advance */
    riff_write_int_32(riff, 1);                 /* Scale */
    riff_write_int_32(riff, audio_rate);        /* Audio sample rate */
    riff_write_int_32(riff, 0);                 /* Start of clip */
    container->pos[2] = ftell(riff->file);
    riff_write_int_32(riff, 0);                 /* Length of clip */
    container->pos[3] = ftell(riff->file);
    riff_write_int_32(riff, 0);                 /* Suggested buffer size */
    riff_write_int_32(riff, 8000);              /* Quality (by 100) */
    riff_write_int_32(riff, (stereo + 1) * (bits / 8)); /* Sample size */
    riff_write_int_32(riff, 0);                 /* Reserved */
    riff_write_int_32(riff, 0);                 /* Reserved */
    riff_close_element(riff);
    riff_start_element(riff, "strf");           /* Stream Format (idem. WAV) */
    riff_write_int_16(riff, 1);                 /* Format = PCM */
    riff_write_int_16(riff, stereo + 1);        /* Number of channels */
    riff_write_int_32(riff, audio_rate);        /* Audio sample rate */
    riff_write_int_32(riff,
        audio_rate * (stereo + 1) * (bits / 8)); /* Frequency x Chans x Bytes */
    riff_write_int_16(riff, (stereo + 1) * (bits / 8)); /* Bytes per sample */
    riff_write_int_16(riff, bits);            /* Bits per individual sample */
    riff_close_element(riff);
    riff_close_structure(riff);

    riff_start_structure(riff, "LIST", "info");

    riff_start_element(riff, "ISFT");
    fwrite(APP_TITLE, 1, sizeof(APP_TITLE) - 1, riff->file);
    riff_close_element(riff);

    riff_close_structure(riff);

    riff_close_structure(riff);
    container->base_movie_size = ftell(riff->file) + 8;
    riff_start_structure(riff, "LIST", "movi");

    if (info->compress)
    {
        int ret;

        // These are needed because jzIntv's embedded zlib is Z_SOLO
        zstream->zalloc = avi_zalloc;
        zstream->zfree  = avi_zfree;
        ret = deflateInit(zstream, 6);
        if (ret != Z_OK) {
            jzp_printf("deflateInit returned %d; disabling compression\n", ret);
            info->compress = 0;
        }
    }
    return 0;
}

// Avoid cumulative rounding error...
//
// For now, just be lazy and use multiplies and divides.  This *can* be
// done completely with adds/subs and 32-bit arithmetic using the same
// error-bounding approach Bresenham line drawing uses.  I'm just too lazy
// at the moment.
LOCAL int next_audio_frame_size(const avi_info_t *info)
{
    const uint64_t curr = (int64_t)info->total_frames * info->usec_per_frame;
    const uint64_t next = curr + info->usec_per_frame;
    const uint32_t curr_samples = (curr * info->audio_rate) / 1000000;
    const uint32_t next_samples = (next * info->audio_rate) / 1000000;

    return next_samples - curr_samples;
}

/*
 ** Grow AVI index
 */
LOCAL int grow_base_riff(avi_riff_t *const riff)
{
    int *new_pointer;

    if (riff->base_index == riff->index + riff->index_size * 4) {
        new_pointer = (int *)realloc((void *)riff->index,
                                     riff->index_size * 2 * 4 * sizeof(int));
        if (new_pointer == NULL)
            return 0;
        riff->base_index = (riff->base_index - riff->index) + new_pointer;
        riff->index = new_pointer;
        riff->index_size *= 2;
    }
    return 1;
}

LOCAL void avi_end_video_internal
(
    const avi_writer_t *const avi,
    const int leave_active
);

/*
 ** Write out the interleaved A/V stream.
 */
LOCAL int avi_write_interleaved_stream
(
    const avi_writer_t *const avi,
    const int                 flush
)
{
    avi_pvt_t       *const pvt       = avi->pvt;
    avi_info_t      *const info      = &(pvt->info);
    avi_riff_t      *const riff      = &(pvt->riff);
    avi_audio_t     *const audio     = &(pvt->audio);
    avi_video_t     *const video     = &(pvt->video);
    avi_container_t *const container = &(pvt->container);

    if (!riff->file)
        return 0;

    long v0;
    long v1;
    long v2;
    const int audio_frame_size = next_audio_frame_size(info);
    const int audio_avail = audio->buffer_total_write -
                            audio->buffer_total_read;
    const int video_avail = video->encode_len_wr - video->encode_len_rd +
                            video->advance_frames_remaining;
    uint32_t encoded_video_size = 0;
    int key_frame = 0;

    if (!video_avail || audio_avail < audio_frame_size)
        return 0;

    v0 = ftell(riff->file);
    riff_start_structure(riff, "LIST", "rec ");
    v1 = ftell(riff->file);
    if (!video->advance_frames_remaining)
    {
        riff_start_element(riff, "00dc");
        uint8_t *const encoded_video = video->encode_buf + video->encode_buf_rd;
        encoded_video_size =
            video->encode_len[video->encode_len_rd++ % AVI_VIDEO_BUF_FRAMES];

        key_frame = (*encoded_video == 0x01);

        fwrite(encoded_video, 1, encoded_video_size, riff->file);
        riff_close_element(riff);
        if (info->max_size_frame < encoded_video_size)
            info->max_size_frame = encoded_video_size;

        video->encode_buf_rd += encoded_video_size;
        if (video->encode_buf_rd >= AVI_VIDEO_WRAP_THRESH)
            video->encode_buf_rd = 0;
    }
    v2 = ftell(riff->file);

    const uint32_t encoded_audio_size = audio_frame_size * sizeof(int16_t);
    if (video->advance_frames_remaining || !flush)
    {
        riff_start_element(riff, "01wb");
        uint32_t audio_remain = audio_frame_size;
        if (audio->buffer_read + audio_remain > audio->buffer + AVI_AUDIO_BUF)
        {
            const uint32_t c = (audio->buffer + AVI_AUDIO_BUF) -
                                audio->buffer_read;
            if (c)
                fwrite(audio->buffer_read, 1, c * sizeof(int16_t), riff->file);
            audio->buffer_read = audio->buffer;
            audio->buffer_total_read += c;
            audio_remain -= c;
        }
        fwrite(audio->buffer_read, sizeof(int16_t), audio_remain, riff->file);
        audio->buffer_read += audio_remain;
        audio->buffer_total_read += audio_remain;
        riff_close_element(riff);
        if (info->max_size_audio < encoded_audio_size)
            info->max_size_audio = encoded_audio_size;
        info->total_audio += audio_frame_size;
    }
    const uint32_t container_size = riff_close_structure(riff);

    if (grow_base_riff(riff)) {
        *riff->base_index++ = 0x20636572;
        *riff->base_index++ = 0x00000001;
        *riff->base_index++ = (int) (v0 - container->base_movie_size);
        *riff->base_index++ = (int) container_size;
    }
    if (!video->advance_frames_remaining) {
        if (grow_base_riff(riff)) {
            *riff->base_index++ = 0x63643030;
            *riff->base_index++ = (key_frame ? 0x10 : 0x00) | (0x02);
            *riff->base_index++ = (int) (v1 - container->base_movie_size);
            *riff->base_index++ = (int) encoded_video_size;
        }
    }
    if (info->total_frames < info->advance_audio_frames || !flush) {
        if (grow_base_riff(riff)) {
            *riff->base_index++ = 0x62773130;
            *riff->base_index++ = 0;
            *riff->base_index++ = (int) (v2 - container->base_movie_size);
            *riff->base_index++ = (int) encoded_audio_size;
        }
    }
    info->total_frames++;
    if (video->advance_frames_remaining)
        video->advance_frames_remaining--;

    /* Safeguard, stop AVI file before reaching 2 GB */
    long size_thresh = (ftell(riff->file)
                     + audio_frame_size * info->advance_audio_frames
                     + (riff->base_index - riff->index) * 4);
    if (size_thresh > 0x7fff0000l)
    {
        jzp_printf("Closing AVI near to 2GB\n");
        avi_end_video_internal(avi,1);
        return 0;
    }
    return 1;
}


/*
 ** Record a frame of audio
 */
static void avi_record_audio_internal
(
    const avi_writer_t *const avi,
    const int16_t      *const audio_data,
    const int                 num_samples
)
{
    // Push out as much interleaved audio/video as we can
    while (avi_write_interleaved_stream(avi, 0))
        ;

    avi_audio_t *const audio = &(avi->pvt->audio);
    size_t wrap = 0;

    if (audio->buffer_write + num_samples > audio->buffer + AVI_AUDIO_BUF) {
        wrap = (audio->buffer + AVI_AUDIO_BUF) - audio->buffer_write;
        if (wrap)
            memcpy(audio->buffer_write, audio_data, wrap * sizeof(int16_t));
        audio->buffer_write = audio->buffer;
    }

    memcpy(audio->buffer_write, audio_data + wrap,
           (num_samples - wrap) * sizeof(int16_t));

    audio->buffer_write += num_samples - wrap;
    audio->buffer_total_write += num_samples;
}

// Note: Audio comes in pre-time-scaled according to whatever the --ratecontrol
// setting is.  The audio_time_scale_ratio is the ratio between avi_time_scale
// and the ratecontrol setting.
void avi_record_audio
(
    const avi_writer_t *const avi,
    const int16_t      *const audio_data,
    const int                 num_samples,
    const int                 silent
)
{
    if (!avi->pvt || !avi->pvt->riff.file)
        return;

    if (num_samples >= AVI_AUDIO_BUF)
        return;

    if (audio_time_scale_ratio == 1.0)
    {
        avi_record_audio_internal(avi, audio_data, num_samples);
        return;
    }

    avi_audio_t *audio = &(avi->pvt->audio);

    // This could behave oddly if num_samples differs wildly call-to-call.
    // Ordinarily, though, it won't, because the snd subsystem mixes in
    // audio_buf sized quanta.
    audio->time_remainder -= num_samples;

#if 1
    // If we're scaling to faster than real-time, allow non-silent segments
    // to borrow into silent segments by a few extra buffers.
    if (audio_time_scale_ratio > 1.0 && silent &&
        audio->time_remainder >= -5.0 * num_samples)
        return;
#endif

    while (audio->time_remainder < 0.0)
    {
        audio->time_remainder += audio_time_scale_ratio * num_samples;
        avi_record_audio_internal(avi, audio_data, num_samples);
        if (audio_time_scale_ratio > 1.0)
            break;
    }
}


// Compute block difference, decimated by 4x in each direction.
LOCAL INLINE int block_diff_decim4(const avi_frame_t *const RESTRICT prev,
                                   const avi_frame_t *const RESTRICT curr,
                                   const int px, const int py,
                                   const int cx, const int cy)
{
    int x, y, dif = 0;

    assert(px >= 0);    assert(px <= AVI_DIM_X - BLOCK_WIDTH);
    assert(py >= 0);    assert(py <= AVI_DIM_Y - BLOCK_HEIGHT);
    assert(cx >= 0);    assert(cx <= AVI_DIM_X - BLOCK_WIDTH);
    assert(cy >= 0);    assert(cy <= AVI_DIM_Y - BLOCK_HEIGHT);

    for (y = 0; y < BLOCK_HEIGHT; y += 4)
        for (x = 0; x < BLOCK_WIDTH; x += 4)
            dif += prev->f[py + y][px + x] != curr->f[cy + y][cx + x];

    return dif << 4;
}

// Compute block difference across all pixels.
LOCAL INLINE int block_diff(const avi_frame_t *const RESTRICT prev,
                            const avi_frame_t *const RESTRICT curr,
                            const int px, const int py,
                            const int cx, const int cy)
{
    int x, y, dif = 0;

    assert(px >= 0);    assert(px <= AVI_DIM_X - BLOCK_WIDTH);
    assert(py >= 0);    assert(py <= AVI_DIM_Y - BLOCK_HEIGHT);
    assert(cx >= 0);    assert(cx <= AVI_DIM_X - BLOCK_WIDTH);
    assert(cy >= 0);    assert(cy <= AVI_DIM_Y - BLOCK_HEIGHT);

    for (y = 0; y < BLOCK_HEIGHT; y++)
        for (x = 0; x < BLOCK_WIDTH; x++)
            dif += prev->f[py + y][px + x] != curr->f[cy + y][cx + x];

    return dif;
}

// Search for the best match for the current block in the previous image.
LOCAL INLINE int motion_search(const avi_frame_t *const RESTRICT prev,
                               const avi_frame_t *const RESTRICT curr,
                               const int x, const int y,
                               int *const RESTRICT move_x,
                               int *const RESTRICT move_y)
{
    int best_dif = INT_MAX;
    int best_explore = 0;
    int explore, ex, ey, dif;

    for (explore = 0; explore < NUM_MOVEMENT; explore++)
    {
        ex = x + movement[explore][0];
        ey = y + movement[explore][1];

        /* First see if we're out of bounds */
        if (ex < 0 || ex > (AVI_DIM_X - BLOCK_WIDTH )) continue;
        if (ey < 0 || ey > (AVI_DIM_Y - BLOCK_HEIGHT)) continue;

        /* Next see if this is even worth it. */
        dif = block_diff_decim4(prev, curr, ex, ey, x, y);

        /* If it's a possible improvement, do a finer-grain search */
        if (dif < 64)
            dif = block_diff(prev, curr, ex, ey, x, y);

        if (dif < best_dif)
        {
            best_dif = dif;
            best_explore = explore;

            /* If it's a small enough difference, stop now. */
            if (dif < 4)
                break;
        }
    }

    *move_x = movement[best_explore][0];
    *move_y = movement[best_explore][1];
    return best_dif;
}

LOCAL INLINE uint8_t *send_block_delta(const avi_frame_t *const RESTRICT prev,
                                       const avi_frame_t *const RESTRICT curr,
                                       const int px, const int py,
                                       const int cx, const int cy,
                                       uint8_t *const RESTRICT out)
{
    int x, y, cnt = 0;

    for (y = 0; y < BLOCK_HEIGHT; y++)
        for (x = 0; x < BLOCK_WIDTH; x++)
            out[cnt++] = prev->f[py + y][px + x] ^ curr->f[cy + y][cx + x];

    return out + cnt;
}

LOCAL INLINE uint8_t *send_frame
(
    const uint8_t *const palette,
    void *const data, const int data_size,
    uint8_t *const output, const int output_avail,
    const int compress, z_stream *const zstream
)
{
    assert(compress || output != data || !palette);

    if (!compress)
    {
        int offset = 0;
        if (palette)
        {
            memcpy((void *)output, palette, 256 * 3);
            offset = 256 * 3;
        }
        if ((void *)output != data)
            memcpy((void *)(output + offset), data, data_size);
        return output + offset + data_size;
    }

    if (palette)    // XXX: palette != NULL means intraframe
        deflateReset(zstream);
    zstream->total_in = 0;
    zstream->total_out = 0;
    zstream->next_out = output;
    zstream->avail_out = output_avail;

    if (palette)
    {
        zstream->next_in = palette;
        zstream->avail_in = 256 * 3;
        deflate(zstream, Z_SYNC_FLUSH);
        assert(zstream->avail_in == 0);
        assert(zstream->avail_out != 0);
    }

    zstream->next_in = (const z_Bytef*)data;
    zstream->avail_in = data_size;
    deflate(zstream, Z_SYNC_FLUSH);
    assert(zstream->avail_in == 0);
    assert(zstream->avail_out != 0);
    return output + zstream->total_out;
}

/*
 ** Set the palette for 8bpp mode.
 ** Note, code doesn't support dynamic palettes.  Call once when opening video.
 */
void avi_set_palette
(
    const avi_writer_t *const avi,
    const palette_t    *const palette,
    const int           length,
    const int           offset
)
{
    if (avi->pvt)
    {
        memcpy((void*)&avi->pvt->video.palette[offset],
               (const void *)&palette->color[0][0], length * 3);
    }
}

// Only encode video frame into video frame circular buffer
LOCAL void avi_record_video_internal
(
    const avi_writer_t *const avi,
    const uint8_t      *const image,
    const uint8_t             border
)
{
    avi_pvt_t       *const pvt       = avi->pvt;
    avi_info_t      *const info      = &(pvt->info);
    avi_video_t     *const video     = &(pvt->video);
    z_stream        *const zstream   = &(pvt->zstream);

    unsigned char *base_output;
    unsigned char *output;
    unsigned char *motion_vecs;
    unsigned char *block_deltas;
    static unsigned char buffer_codec[AVI_FRAME_BYTES + 4096];
    int x, y;
    int block;
    const int rel_frame = info->total_frames - info->advance_audio_frames;
    const int key_frame = (rel_frame & (info->key_frames - 1)) == 0;

    avi_frame_t       *const RESTRICT curr = &(video->frame[ video->toggle]);
    const avi_frame_t *const RESTRICT prev = &(video->frame[!video->toggle]);

    if (rel_frame < 0)
        return;

    // Push out as much interleaved audio/video as we can
    while (avi_write_interleaved_stream(avi, 0))
        ;

    // Enough room for a full encoded frame?
    if (video->encode_buf_wr < AVI_VIDEO_WRAP_THRESH)
    {
        output = base_output = video->encode_buf + video->encode_buf_wr;
    } else
    {
        video->encode_buf_wr = 0;
        output = base_output = video->encode_buf;
    }

    // Did we overrun circular buffer?
    if (video->encode_buf_wr < video->encode_buf_rd &&
        video->encode_buf_wr + AVI_VIDEO_ENCODE_BYTES > video->encode_buf_rd)
    {
        jzp_printf("Video overrun! Closing AVI.");
        avi_end_video_internal(avi,1);
    }

    video->toggle = !video->toggle;

#if AVI_BORD_X_SZ > 0 || AVI_BORD_Y_SZ > 0
    if (border != curr->f[0][0])
        memset((void*)&(curr->f), border, sizeof(avi_frame_t));
#endif

    // NOTE: This pixel-doubles horizontally for the Intellivision.
    // If we want to be more efficient, we would teach the whole encoder
    // to work in 160x192 and then just double pixels when sending blocks.
    for (y = 0; y < INPUT_DIM_Y; y++)
    {
        const uint8_t *RESTRICT vid_row_i = image + y * (INPUT_DIM_X/2);
        uint8_t *RESTRICT vid_row_o =
            &(curr->f[y + AVI_BORD_Y_SZ][AVI_BORD_X_SZ]);

        for (x = 0; x < INPUT_DIM_X/2; x++)
        {
            uint8_t p = *vid_row_i++;
            *vid_row_o++ = p;
            *vid_row_o++ = p;
        }
    }

    if (key_frame)
    {
        *output++ = 0x01;           /* Key frame */
        *output++ = 0x00;           /* Hi version */
        *output++ = 0x01;           /* Lo version */
        /* Compression (0x00 = Uncompressed, 0x01 = ZLIB) */
        *output++ = info->compress;
        *output++ = 0x04;           /* 4 = 8 bpp */
        *output++ = BLOCK_WIDTH;    /* Block width */
        *output++ = BLOCK_HEIGHT;   /* Block height */

        const uint32_t remain = AVI_VIDEO_ENCODE_BYTES - (output - base_output);
        output = send_frame(AVI_BPP == 8 ? &video->palette[0][0] : NULL,
                            (void *)curr->f, AVI_FRAME_BYTES, output, remain,
                            info->compress, zstream);
    } else
    {
        *output++ = 0x00;   /* Non-key frame (+2 = Delta palette) */
        motion_vecs  = buffer_codec;
        block_deltas = motion_vecs + NUM_X_BLOCK * NUM_Y_BLOCK * 2;
        block = 0;
        for (y = 0; y < AVI_DIM_Y; y += BLOCK_HEIGHT)
        {
            for (x = 0; x < AVI_DIM_X; x += BLOCK_WIDTH)
            {
                int move_x = 0, move_y = 0;
                int best_dif = motion_search(prev, curr, x, y,
                                             &move_x, &move_y);
                if (best_dif == 0)
                {
                    motion_vecs[block++] = move_x * 2;
                    motion_vecs[block++] = move_y * 2;
                } else
                {
                    motion_vecs[block++] = move_x * 2 + 1;
                    motion_vecs[block++] = move_y * 2;
                    block_deltas = send_block_delta(prev, curr,
                                                    x + move_x, y + move_y,
                                                    x, y, block_deltas);
                }
            }
        }
        assert(block == NUM_X_BLOCK * NUM_Y_BLOCK * 2);
        assert(block % 4 == 0);

        const uint32_t remain = AVI_VIDEO_ENCODE_BYTES - (output - base_output);
        output = send_frame(NULL, (void *)buffer_codec,
                            block_deltas - buffer_codec,
                            output, remain, info->compress, zstream);
    }

    const uint32_t length = output - base_output;

    video->encode_len[video->encode_len_wr++ % AVI_VIDEO_BUF_FRAMES] = length;
    video->encode_buf_wr += length;
}

void avi_record_video
(
    const avi_writer_t *const avi,
    const uint8_t      *const image,
    const uint8_t             border
)
{
    if (!avi->pvt || !avi->pvt->info.active)
        return;

    if (avi_time_scale == 1.0)
    {
        avi_record_video_internal(avi, image, border);
        return;
    }

    avi_video_t *video = &(avi->pvt->video);

    video->time_remainder -= 1.0;

    while (video->time_remainder < 0.0)
    {
        video->time_remainder += avi_time_scale;
        avi_record_video_internal(avi, image, border);
    }
}

/*
 ** End of video recording
 */
LOCAL void avi_end_video_internal
(
    const avi_writer_t *const avi,
    const int leave_active
)
{
    avi_pvt_t       *const pvt       = avi->pvt;
    avi_info_t      *const info      = &(pvt->info);
    avi_riff_t      *const riff      = &(pvt->riff);
    avi_container_t *const container = &(pvt->container);
    z_stream        *const zstream   = &(pvt->zstream);
    size_t c;
    size_t d;

    if (!riff->file)
        return;

    // Push out as much interleaved audio/video as we can
    while (avi_write_interleaved_stream(avi, 0))
        ;

    fseek(riff->file, container->pos[4], SEEK_SET);
    riff_write_int_32(riff, info->total_frames);
    fseek(riff->file, container->pos[5], SEEK_SET);
    riff_write_int_32(riff, info->total_frames);
    fseek(riff->file, 0, SEEK_END);

    // Flush the rest
    while (avi_write_interleaved_stream(avi, 1))
        ;

    riff_close_structure(riff);
    riff_start_element(riff, "idx1");
    c = riff->base_index - riff->index;
    for (d = 0; d < c; d++)
        riff_write_int_32(riff, riff->index[d]);
    riff_close_element(riff);
    riff_close_structure(riff);
    fseek(riff->file, container->pos[0], SEEK_SET);
    riff_write_int_32(riff,
        (info->max_size_frame + 20 + 20 +
         info->max_size_audio + 2047) & ~0x07ff);
    fseek(riff->file, container->pos[1], SEEK_SET);
    riff_write_int_32(riff, info->max_size_frame);
    fseek(riff->file, container->pos[2], SEEK_SET);
    riff_write_int_32(riff, info->total_audio);
    fseek(riff->file, container->pos[3], SEEK_SET);
    riff_write_int_32(riff, info->max_size_audio);
    free(riff->index);
    fclose(riff->file);
    riff->file = NULL;

    if (info->compress)
    {
        deflateReset(zstream);
        int ret = deflateEnd(zstream);
        if (ret != Z_OK)
            printf("\nWarning: deflateEnd returned %d: %s\n", ret,
                   zstream->msg ? zstream->msg :"");
    }

    if (!leave_active)
        info->active = 0;
}

void avi_end_video( const avi_writer_t *const avi )
{
    if (avi->pvt && avi->pvt->info.active)
        avi_end_video_internal( avi, 0 );
}

int avi_is_active( const avi_writer_t *const avi )
{
    return avi->pvt && avi->pvt->info.active;
}

uint64_t avi_start_time( const avi_writer_t *const avi )
{
    return avi->pvt ? avi->pvt->info.start_time : 0;
}

const avi_info_t *avi_info( const avi_writer_t *const avi )
{
    return avi->pvt ? &avi->pvt->info : (avi_info_t*)NULL;
}

void avi_set_time_scale( const double avi_time_scale_,
                         const double incoming_audio_time_scale )
{
    avi_time_scale         = avi_time_scale_ > 0.01 ? avi_time_scale_ : 1.0;
    audio_time_scale       = incoming_audio_time_scale;
    audio_time_scale_ratio = avi_time_scale / incoming_audio_time_scale;
    printf("AVI: %5.3f %5.3f %5.3f\n", avi_time_scale, audio_time_scale,
           audio_time_scale_ratio);
}
