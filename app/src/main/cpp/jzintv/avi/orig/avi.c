//
//  AVI.c
//  CoolCV
//
//  Created by Oscar Toledo on 02/11/15.
//  Copyright (c) 2015 Oscar Toledo. All rights reserved.
//
//  With portions of my private video converter to AVI
//

//  ZMBV specs from http://wiki.multimedia.cx/?title=DosBox_Capture_Codec

#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <SDL.h>
#include <shlobj.h>
#include <wchar.h>
#else
#include <SDL.h>
#endif
#include "hardware.h"
#include "zlib.h"

void riff_start_structure(char *, char *);
void riff_start_element(char *);
void riff_write_code(char *);
void riff_write_int_16(int);
void riff_write_int_32(int);
void riff_write_string(char *);
int riff_close_element(void);
int riff_close_structure(void);

extern FILE *video_file;

/* Locations to be patched */
long avi_b0;
long avi_b1;
long avi_b2;
long avi_b3;
long avi_b4;
long avi_b5;
long base_movie_size;

int total_audio;
int max_size_frame;
int max_size_audio;
int audio_frames;
int frames_per_sec;
int total_frames;
int key_frames;

int *riff_index;
int *base_riff_index;
int riff_index_size;

unsigned short audio_buffer[44100];
unsigned short *audio_buffer_read;
unsigned short *audio_buffer_write;
int audio_buffer_total_read;
int audio_buffer_total_write;

Uint32 video_buffer[60][256 * 192];
int video_buffer_read;
int video_buffer_write;

/*
 ** Structure to keep elements nesting
 */
int riff_nesting_level;
long riff_nesting[512];

static z_stream zstream;

static int movement[85 * 2];

static char square[9 * 9] = {
    72,73,74,75,76,77,78,79,80,
    71,42,43,44,45,46,47,48,49,
    70,41,20,21,22,23,24,25,50,
    69,40,19, 6, 7, 8, 9,26,51,
    68,39,18, 5, 0, 1,10,27,52,
    67,38,17, 4, 3, 2,11,28,53,
    66,37,16,15,14,13,12,29,54,
    65,36,35,34,33,32,31,30,55,
    64,63,62,61,60,59,58,57,56,
};

void avi_end_video(void);

/*
 ** Start video recording
 */
int avi_start_video(void)
{
    int stereo = 0;
    int frequency = PSG_FREC;
    int bits = 16;
    int c;
    int x;

    /*
     ** Sound goes ahead video by 0.8 seconds
     */
    frames_per_sec = vdp_pal ? 50 : 60;
    key_frames = 32;
    audio_frames = 0; /*(int) (0.8 / (1.0 / frames_per_sec) + 0.5);*/
    total_frames = 0;
    max_size_frame = 0;
    max_size_audio = 0;
    total_audio = 0;
    audio_buffer_read = audio_buffer;
    audio_buffer_write = audio_buffer;
    audio_buffer_total_read = 0;
    audio_buffer_total_write = 0;
    video_buffer_read = 0;
    video_buffer_write = 0;
    riff_nesting_level = 0;

    riff_index_size = 1048576;
    riff_index = (int *) malloc(riff_index_size * 4 * sizeof(int));
    if (riff_index == NULL) {
        return 1;
    }
    base_riff_index = riff_index;

    riff_start_structure("RIFF", "AVI ");
    riff_start_structure("LIST", "hdrl");
    riff_start_element("avih");
    riff_write_int_32(1000000 / frames_per_sec); /* Microsecs per frame */
    riff_write_int_32(300000);          /* Maximum bytes per second */
    riff_write_int_32(0);               /* Reserved */
    riff_write_int_32(0x0110);          /* Flags */
    /* bit 4 = Includes idx1 */
    /* bit 5 = Use idx1 for everything */
    /* bit 8 = Video and audio intermixed */
    /* bit 16 = Optimized for realtime capture */
    /* bit 17 = Copyrighted data */
    avi_b4 = ftell(video_file);
    riff_write_int_32(0);               /* Total frames */
    riff_write_int_32(audio_frames);    /* Sound frames in advance */
    riff_write_int_32(2);               /* Total streams */
    avi_b0 = ftell(video_file);
    riff_write_int_32(0);               /* Suggested buffer size */
    riff_write_int_32(256);             /* X size */
    riff_write_int_32(192);             /* Y size */
    riff_write_int_32(0);               /* Scale */
    riff_write_int_32(0);               /* Speed */
    riff_write_int_32(0);               /* Start */
    riff_write_int_32(0);               /* Length */
    riff_close_element();
    /* Video stream */
    riff_start_structure("LIST", "strl"); /* Stream Leader */
    riff_start_element("strh");         /* Stream Header */
    riff_write_code("vids");            /* Video */
    riff_write_code("ZMBV");            /* Codec */
    riff_write_int_32(0);               /* Flags */
    riff_write_int_32(0);               /* Reserved */
    riff_write_int_32(0);               /* Frames in advance */
    riff_write_int_32(1000000);         /* Scale */
    riff_write_int_32(1000000 * frames_per_sec);                  /* Speed */
    riff_write_int_32(0);               /* Start of clip */
    avi_b5 = ftell(video_file);
    riff_write_int_32(0);               /* Length of clip in frames */
    avi_b1 = ftell(video_file);
    riff_write_int_32(0);               /* Suggested buffer site */
    riff_write_int_32(8000);            /* Quality (by 100) */
    riff_write_int_32(0);               /* Sample size */
    riff_write_int_32(0);               /* Reserved */
    riff_write_int_32(0);               /* Reserved */
    riff_close_element();
    riff_start_element("strf");         /* Stream Format (idem. BMP) */
    riff_write_int_32(40);              /* Header size (including itself) */
    riff_write_int_32(256);             /* X size */
    riff_write_int_32(192);             /* Y size */
    riff_write_int_16(1);               /* Number of planes */
    riff_write_int_16(32);              /* Bits per pixel */
    riff_write_code("ZMBV");            /* Codec */
    riff_write_int_32(256 * 192 * 4);   /* Image size (in bytes) */
    riff_write_int_32(0);               /* X pixels per meter */
    riff_write_int_32(0);               /* Y pixels per meter */
    riff_write_int_32(0);               /* Used colors */
    riff_write_int_32(0);               /* Important colors */
    riff_close_element();
    riff_close_structure();
    /* Audio stream */
    riff_start_structure("LIST", "strl"); /* Stream Leader */
    riff_start_element("strh");         /* Stream Header */
    riff_write_code("auds");            /* Audio */
    riff_write_code("\0\0\0\0");        /* Codec (no compression) */
    riff_write_int_32(0);               /* Flags */
    riff_write_int_32(0);               /* Reserved */
    riff_write_int_32(audio_frames);    /* Frames in advance */
    riff_write_int_32(1);               /* Scale */
    riff_write_int_32(frequency);       /* Speed */
    riff_write_int_32(0);               /* Start of clip */
    avi_b2 = ftell(video_file);
    riff_write_int_32(0);               /* Length of clip */
    avi_b3 = ftell(video_file);
    riff_write_int_32(0);               /* Suggested buffer size */
    riff_write_int_32(8000);            /* Quality (by 100) */
    riff_write_int_32((stereo + 1) * (bits / 8)); /* Sample size */
    riff_write_int_32(0);               /* Reserved */
    riff_write_int_32(0);               /* Reserved */
    riff_close_element();
    riff_start_element("strf");         /* Stream Format (idem. WAV) */
    riff_write_int_16(1);               /* Format = PCM */
    riff_write_int_16(stereo + 1);      /* Number of channels */
    riff_write_int_32(frequency);       /* Frequency */
    riff_write_int_32(frequency * (stereo + 1) * (bits / 8)); /* Frequency x Channels x Bytes */
    riff_write_int_16((stereo + 1) * (bits / 8));   /* Bytes per sample */
    riff_write_int_16(bits);            /* Bits per individual sample */
    riff_close_element();
    riff_close_structure();

    riff_start_structure("LIST", "info");

    riff_start_element("ISFT");
    fwrite(APP_TITLE, 1, sizeof(APP_TITLE) - 1, video_file);
    riff_close_element();

    riff_close_structure();

    riff_close_structure();
    base_movie_size = ftell(video_file) + 8;
    riff_start_structure("LIST", "movi");
    memset(&zstream, 0, sizeof(zstream));
    deflateInit(&zstream, 6);
    for (c = 0; c < 81; c++) {
        for (x = 0; x < 81; x++)
            if (square[x] == c)
                break;
        movement[c++] = x % 9 - 4;  /* Range -16..16 */
        movement[c++] = x / 9 - 4;  /* Range -16..16 */
    }
    movement[c++] = -8; movement[c++] = 0;
    movement[c++] = 8; movement[c++] = 0;
    movement[c++] = 0; movement[c++] = 8;
    movement[c++] = 0; movement[c++] = -8;
    return 0;
}

/*
 ** Record a frame of audio
 */
void avi_record_audio(unsigned short *audio, int size)
{
    size_t c;
    int total_size;

    total_size = sizeof(audio_buffer) / sizeof(unsigned short);
    if (audio_buffer_write + size > audio_buffer + total_size) {
        c = (audio_buffer + total_size) - audio_buffer_write;
        if (c)
          memcpy(audio_buffer_write, audio, c * sizeof(unsigned short));
        audio += c;
        audio_buffer_write = audio_buffer;
        audio_buffer_total_write += c;
        size -= c;
    }

    memcpy(audio_buffer_write, audio, size * sizeof(unsigned short));
    audio_buffer_write += size;
    audio_buffer_total_write += size;
}

/*
 ** Grow AVI index
 */
int grow_base_riff(void)
{
    int *new_pointer;

    if (base_riff_index == riff_index + riff_index_size * 4) {
        new_pointer = (int *) realloc(riff_index, riff_index_size * 2 * 4 * sizeof(int));
        if (new_pointer == NULL)
            return 0;
        base_riff_index = (base_riff_index - riff_index) + new_pointer;
        riff_index = new_pointer;
        riff_index_size *= 2;
    }
    return 1;
}

#define BLOCK_WIDTH   16
#define BLOCK_HEIGHT  16

/*
 ** Record a frame of video
 */
void avi_record_video(Uint32 *image, int dump)
{
    long v0;
    long v1;
    long v2;
    unsigned char *base_output;
    unsigned char *output;
    unsigned char *ap;
    unsigned char *ap1;
    static int compress;
    size_t d;
    size_t e;
    int f;
    int size;
    size_t c;
    int total_size;
    int audio_frame = frames_per_sec == 60 ? 735 : 882;
    static unsigned char buffer_codec[256 * 192 * sizeof(Uint32) + 384];
    static unsigned char buffer_compressed[256 * 192 * sizeof(Uint32) + 384];
    int key_frame;
    int x;
    int y;
    int x1;
    int y1;
    Uint32 *ref1;
    Uint32 *ref2;
    int explore;
    int dif;
    int best_explore;
    int best_dif;
    int block;
    int previous = ((video_buffer_read == 0) ? frames_per_sec : video_buffer_read) - 1;

    if (video_file == NULL)
        return;
    if (!dump) {
#ifdef RASPBERRY
        for (x = 0; x < 256 * 192; x++)
            video_buffer[video_buffer_write][x] = (image[x] & 0xff) << 16 | (image[x] & 0xff00) | (image[x] & 0xff0000) >> 16;
#else
        memcpy(&video_buffer[video_buffer_write], image, 256 * 192 * sizeof(Uint32));
#endif
        if (++video_buffer_write == frames_per_sec)
            video_buffer_write = 0;
        if (audio_buffer_total_write - audio_buffer_total_read < audio_frame)
            return;
    }
    v0 = ftell(video_file);
    riff_start_structure("LIST", "rec ");
    v1 = ftell(video_file);
    d = 0;  /* For lint happiness =P */
    key_frame = 0;  /* For lint happiness =P */
    if (total_frames >= audio_frames) {
        riff_start_element("00dc");
        output = base_output = buffer_compressed;
        if (((total_frames - audio_frames) & (key_frames - 1)) == 0) {
            *output++ = key_frame = 0x01;  /* Key frame (+0x02 = Delta palette) */
            *output++ = 0x00;  /* Hi version */
            *output++ = 0x01;  /* Lo version */
            *output++ = compress = 0x01;  /* Compression (0x00 = Uncompressed, 0x01 = ZLIB) */
            *output++ = 0x08;  /* 8 = 32 bpp */
            *output++ = BLOCK_WIDTH;    /* Block width */
            *output++ = BLOCK_HEIGHT;    /* Block height */
            if (compress) {
                deflateReset(&zstream);
                zstream.next_in = (void *) video_buffer[video_buffer_read];
                zstream.avail_in = 256 * 192 * sizeof(Uint32);
                zstream.total_in = 0;

                zstream.next_out = output;
                zstream.avail_out = (int) (sizeof(buffer_compressed) - (output - base_output));
                zstream.total_out = 0;
                deflate(&zstream, Z_SYNC_FLUSH);
                output += zstream.total_out;
            } else {
                memcpy(output, video_buffer[video_buffer_read], 256 * 192 * sizeof(Uint32));
                output += 256 * 192 * sizeof(Uint32);
            }
        } else {
            *output++ = key_frame = 0x00;  /* Non-key frame */
            if (compress) {
                ap = buffer_codec;
            } else {
                ap = output;
            }
            ap1 = ap + (256 / BLOCK_WIDTH) * (192 / BLOCK_HEIGHT) * 2;
            block = 0;
            for (y = 0; y < 192; y += BLOCK_HEIGHT) {
                for (x = 0; x < 256; x += BLOCK_WIDTH) {
                    best_dif = 10000;
                    best_explore = 0;
                    ref2 = &video_buffer[video_buffer_read][y * 256 + x];
                    for (explore = 0; explore < 85 * 2; explore += 2) {
                        if (x + movement[explore] < 0
                         || y + movement[explore + 1] < 0
                         || x + movement[explore] > 256 - BLOCK_WIDTH
                         || y + movement[explore + 1] > 192 - BLOCK_HEIGHT)
                            continue;
                        ref1 = &video_buffer[previous][(y + movement[explore + 1]) * 256 + (x + movement[explore])];
                        dif = 0;
                        for (y1 = 0; y1 < BLOCK_HEIGHT * 256; y1 += 1024) {
                            for (x1 = 0; x1 < BLOCK_WIDTH; x1 += 4) {
                                if ((ref1[y1 + x1] ^ ref2[y1 + x1]) & 0xffffff)
                                    dif++;
                            }
                        }
                        if (dif < 4) {
                            dif = 0;
                            for (y1 = 0; y1 < BLOCK_HEIGHT * 256; y1 += 256) {
                                for (x1 = 0; x1 < BLOCK_WIDTH; x1++) {
                                    if ((ref1[y1 + x1] ^ ref2[y1 + x1]) & 0xffffff)
                                        dif++;
                                }
                            }
                        }
                        if (dif < best_dif) {
                            best_dif = dif;
                            best_explore = explore;
                            if (dif < 4)
                                break;
                        }
                    }
                    if (best_dif == 0) {
                        ap[block++] = movement[best_explore] << 1;
                        ap[block++] = movement[best_explore + 1] << 1;
                    } else {
                        ap[block++] = movement[best_explore] << 1 | 1;
                        ap[block++] = movement[best_explore + 1] << 1;
                        ref1 = &video_buffer[previous][(y + movement[best_explore + 1]) * 256 + (x + movement[best_explore])];
                        for (y1 = 0; y1 < BLOCK_HEIGHT * 256; y1 += 256) {
                            for (x1 = 0; x1 < BLOCK_WIDTH; x1++) {
                                dif = ref1[y1 + x1] ^ ref2[y1 + x1];
                                *ap1++ = dif;
                                *ap1++ = dif >> 8;
                                *ap1++ = dif >> 16;
                                *ap1++ = 0;
                            }
                        }
                    }
                }
            }
            if (compress) {
                zstream.next_in = (void *) buffer_codec;
                zstream.avail_in = (int) (ap1 - buffer_codec);
                zstream.total_in = 0;

                zstream.next_out = output;
                zstream.avail_out = (int) (sizeof(buffer_compressed) - (output - base_output));
                zstream.total_out = 0;
                deflate(&zstream, Z_SYNC_FLUSH);
                output += zstream.total_out;
            } else {
                output = ap1;
            }
        }
        fwrite(base_output, 1, output - base_output, video_file);
        riff_close_element();
        d = (int) (output - base_output);
        if (d > max_size_frame)
            max_size_frame = (int) d;
        if (++video_buffer_read == frames_per_sec)
            video_buffer_read = 0;
    }
    v2 = ftell(video_file);
    f = audio_frame * sizeof(unsigned short);
    if (total_frames < audio_frames || !dump) {
        riff_start_element("01wb");
        size = audio_frame;
        total_size = sizeof(audio_buffer) / sizeof(unsigned short);
        if (audio_buffer_read + size > audio_buffer + total_size) {
            c = (audio_buffer + total_size) - audio_buffer_read;
            if (c)
                fwrite(audio_buffer_read, 1, c * sizeof(unsigned short), video_file);
            audio_buffer_read = audio_buffer;
            audio_buffer_total_read += c;
            size -= c;
        }
        fwrite(audio_buffer_read, 1, size * sizeof(unsigned short), video_file);
        audio_buffer_read += size;
        audio_buffer_total_read += size;
        riff_close_element();
        if (f > max_size_audio)
            max_size_audio = f;
        total_audio += audio_frame;
    }
    e = riff_close_structure();
    if (grow_base_riff()) {
        *base_riff_index++ = 0x20636572;
        *base_riff_index++ = 0x00000001;
        *base_riff_index++ = (int) (v0 - base_movie_size);
        *base_riff_index++ = (int) e;
    }
    if (total_frames >= audio_frames) {
        if (grow_base_riff()) {
            *base_riff_index++ = 0x63643030;
            *base_riff_index++ = (key_frame ? 0x10 : 0x00) | (0x02);
            *base_riff_index++ = (int) (v1 - base_movie_size);
            *base_riff_index++ = (int) d;
        }
    }
    if (total_frames < audio_frames || !dump) {
        if (grow_base_riff()) {
            *base_riff_index++ = 0x62773130;
            *base_riff_index++ = 0;
            *base_riff_index++ = (int) (v2 - base_movie_size);
            *base_riff_index++ = (int) f;
        }
    }
    total_frames++;

    /* Safeguard, stop AVI file before reaching 2 GB */
    if ((ftell(video_file) + audio_frame * audio_frames + (base_riff_index - riff_index) * 4) > 0x7fff0000) {
        message("Closing AVI near to 2GB");
        avi_end_video();
    }
}

/*
 ** End of video recording
 */
void avi_end_video(void)
{
    size_t c;
    size_t d;

    if (video_file == NULL)
        return;
    deflateEnd(&zstream);
    fseek(video_file, avi_b4, SEEK_SET);
    riff_write_int_32(total_frames);
    fseek(video_file, avi_b5, SEEK_SET);
    riff_write_int_32(total_frames);
    fseek(video_file, 0, SEEK_END);
    for (c = 0; c < audio_frames; c++)
        avi_record_video(NULL, 1);
    riff_close_structure();
    riff_start_element("idx1");
    c = base_riff_index - riff_index;
    for (d = 0; d < c; d++)
        riff_write_int_32(riff_index[d]);
    riff_close_element();
    riff_close_structure();
    fseek(video_file, avi_b0, SEEK_SET);
    riff_write_int_32((max_size_frame + 20 + 20 + max_size_audio + 2047) & ~0x07ff);
    fseek(video_file, avi_b1, SEEK_SET);
    riff_write_int_32(max_size_frame);
    fseek(video_file, avi_b2, SEEK_SET);
    riff_write_int_32(total_audio);
    fseek(video_file, avi_b3, SEEK_SET);
    riff_write_int_32(max_size_audio);
    free(riff_index);
    fclose(video_file);
    video_file = NULL;
}

/*
 ** Starts a RIFF structure
 */
void riff_start_structure(char *list, char *title)
{
    riff_start_element(list);
    riff_write_code(title);
}

/*
 ** Starts a RIFF element
 */
void riff_start_element(char *element)
{
    riff_write_code(element);
    riff_write_int_32(0);
    riff_nesting[riff_nesting_level++] = ftell(video_file);
}

/*
 ** Write a 32 bits code
 */
void riff_write_code(char *code)
{
    fwrite(code, 1, 4, video_file);
}

/*
 ** Write a 16 bits integer
 */
void riff_write_int_16(int value)
{
    char buf[2];

    buf[0] = value;
    buf[1] = value >> 8;
    fwrite(buf, 1, 2, video_file);
}

/*
 ** Write a 32 bits integer
 */
void riff_write_int_32(int value)
{
    char buf[4];

    buf[0] = value;
    buf[1] = value >> 8;
    buf[2] = value >> 16;
    buf[3] = value >> 24;
    fwrite(buf, 1, 4, video_file);
}

/*
 ** Writes a text string
 */
void riff_write_string(char *text)
{
    char *ap;

    ap = text;
    while (*ap++) ;
    fwrite(text, 1, ap - text, video_file);
}

/*
 ** Closes a RIFF element
 */
int riff_close_element(void)
{
    long c;
    long a;

    c = riff_nesting[--riff_nesting_level];
    a = ftell(video_file);
    if (a & 1)
        fwrite("\0", 1, 1, video_file);
    fseek(video_file, c - 4, SEEK_SET);
    riff_write_int_32((int) (a - c));
    if (a & 1)
        a++;
    fseek(video_file, a, SEEK_SET);
    return (int) (a - c);
}

/*
 ** Termina una estructura RIFF
 */
int riff_close_structure(void)
{
    return riff_close_element();
}
