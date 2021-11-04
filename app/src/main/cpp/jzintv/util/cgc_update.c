/* ======================================================================== */
/*  CGC Update                                                              */
/*                                                                          */
/*  Downloads firmware updates to the Classic Game Controller.              */
/*  Currently only known to work on Linux.  May work on other platforms.    */
/*                                                                          */
/*  Only built on platforms that define CGC_THREAD for now.                 */
/*                                                                          */
/* ------------------------------------------------------------------------ */
/*                                                                          */
/*  CGC flash update protocol:                                              */
/*                                                                          */
/*  1.  Read flash image into memory and verify size.                       */
/*       -- First byte is # of 64-byte blocks (num_blocks)                  */
/*       -- Next N * 64 bytes are firmware update                           */
/*       -- Possible byte of garbage beyond end of image                    */
/*                                                                          */
/*  2.  Open CGC device                                                     */
/*                                                                          */
/*  3.  Send REWRITE command byte (0x01)                                    */
/*                                                                          */
/*  4.  Number-of-blocks handshake:                                         */
/*       -- Send num_blocks byte                                            */
/*       -- Read back num_blocks from CGC                                   */
/*                                                                          */
/*  5.  Send blocks.  For each block do:                                    */
/*       -- Send 64 bytes for block                                         */
/*       -- Read back status byte (see RESP_xxx defines below)              */
/*                                                                          */
/*  6.  Close the CGC.                                                      */
/*                                                                          */
/* ======================================================================== */
#include "config.h"

#ifdef CGC_THREAD

#include <errno.h>
#include <termios.h>
#include <fcntl.h>

#define CMD_REWRITE         (1)         /* Byte to send to initiate rewrite */

#define RESP_READY          (0x50)      /* Unused                           */
#define RESP_PASS           (0x51)      /* Flash rewrite succeeded          */
#define RESP_FAIL           (0x52)      /* Unused                           */
#define RESP_BLOCKWAITFAIL  (0x53)      /* Timeout during block transfer    */
#define RESP_FLASHWAITFAIL  (0x54)      /* Timeout during flash write       */
#define RESP_FLASHWRITEFAIL (0x55)      /* Flash write itself failed        */


#define MAX_FIRMWARE (64 * 255)         /* Maximum possible image size      */
uint8_t firmware[MAX_FIRMWARE];
uint8_t num_blocks = 0;

/* ======================================================================== */
/*  DIE             Ok, so I write too much perl                            */
/* ======================================================================== */
LOCAL void die(const char *error)
{
    if (errno != 0) perror(error);
    else            fprintf(stderr, "%s\n", error);

    exit(1);
}

/* ======================================================================== */
/*  WRITE_FIRMWARE  Send the firmware to the CGC.                           */
/* ======================================================================== */
LOCAL void write_firmware(int fd)
{
    uint8_t cmd  = CMD_REWRITE;
    uint8_t resp = -1;
    int i, j, errsv;
    const char *error;

    errno = 0;

    tcflush(fd, TCIOFLUSH);
    if (write(fd, &cmd,        1) != 1) die("Error writing command byte");
    tcflush(fd, TCIOFLUSH);
    if (write(fd, &num_blocks, 1) != 1) die("Error writing number of blocks");
    tcflush(fd, TCIOFLUSH);
    if (read (fd, &resp,       1) != 1) die("Error reading number of blocks");

    if (resp != num_blocks)
    {
        errsv = errno;
        printf("Expected %.2X, got %.2X\n", num_blocks, resp);
        errno = errsv;
        die("Error during number-of-blocks handshake");
    }

    for (i = 0; i < num_blocks; i++)
    {
        printf("\rSending block %d of %d...", i+1, num_blocks);
        fflush(stdout);

        /* Send a byte at a time for paranoia's sake. */
        for (j = 0; j < 64; j++)
        {
            tcflush(fd, TCIOFLUSH);
            if (write(fd, firmware + 64*i+j, 1) != 1 || tcflush(fd, TCOFLUSH))
                die("Error sending block");
        }

        tcflush(fd, TCIOFLUSH);
        if (read(fd, &resp, 1) != 1 || tcflush(fd, TCIFLUSH))
            die("Error reading block handshake byte");

        switch (resp)
        {
            case RESP_PASS: continue;

            case RESP_BLOCKWAITFAIL:    error = "Block timeout failure"; break;
            case RESP_FLASHWAITFAIL:    error = "Flash timeout failure"; break;
            case RESP_FLASHWRITEFAIL:   error = "Flash write failure";   break;

            default:
            {
                errsv = errno;
                printf("Unexpected response from CGC: %.2X\n", resp);
                errno = errsv;
                error = "Bad block handshake";
            }
        }

        die(error);
    }

    printf("\nFirmware update successful!\n");
    fflush(stdout);
}

/* ======================================================================== */
/*  READ_FIRMWARE                                                           */
/* ======================================================================== */
LOCAL void read_firmware(const char *fn)
{
    FILE *f;

    f = fopen(fn, "rb");
    if (!f)
        die("Could not open firmware file");

    if (fread(&num_blocks, 1, 1, f) != 1)
        die("Could not get firmware block count");

    printf("Firmware size: %d blocks\n", num_blocks); fflush(stdout);

    if (fread(firmware, 64, num_blocks, f) != num_blocks)
        die("Error reading firmware file:  Too short?");

    fseek(f, 0, SEEK_END);

    /* Add 2 to the 64*num_blocks to account for garbage byte */
    if (ftell(f) > 2 + 64*num_blocks)
        die("Firmware file appears to be the wrong size");

    fclose(f);
}

/* ======================================================================== */
/*  OPEN_CGC    Establish a file descriptor for the CGC and set it up for   */
/*              raw binary access.  Same code as pads_cgc_linux.            */
/* ======================================================================== */
LOCAL int open_cgc(const char *cgc_dev)
{
    int fd, i;
    struct termios tio;
    char o_byte, i_byte = 0;

    /* -------------------------------------------------------------------- */
    /*  Establish descriptor to the device node.                            */
    /* -------------------------------------------------------------------- */
    fd = open(cgc_dev, O_RDWR|O_SYNC);

    if (fd < 0)
    {
        perror("open()");
        fprintf(stderr, "Could not open CGC device \"%s\".\n", cgc_dev);
        return -1;
    }

    /* -------------------------------------------------------------------- */
    /*  Ugh.  CGC is over a tty, so we need to set the terminal attribs.    */
    /* -------------------------------------------------------------------- */
    if (tcgetattr(fd, &tio))
    {
        perror("tcgetattr()");
        fprintf(stderr, "Could not control CGC device \"%s\".\n", cgc_dev);
        return -1;
    }

    cfmakeraw(&tio);
    tio.c_cc[VMIN    ] = 1;
    tio.c_cc[VTIME   ] = 0;
    tio.c_cflag |= CRTSCTS;

    if (cfsetispeed(&tio, B9600) ||
        cfsetospeed(&tio, B9600) ||
        tcsetattr(fd, TCSANOW, &tio))
    {
        perror("tcsetattr()");
        fprintf(stderr, "Could not control CGC device \"%s\".\n", cgc_dev);
        return -1;
    }

    tcflow(fd, TCOON);
    tcflow(fd, TCION);

    tcflush(fd, TCOFLUSH);
    tcflush(fd, TCIFLUSH);

    /* -------------------------------------------------------------------- */
    /*  Synchronize with the CGC.                                           */
    /* -------------------------------------------------------------------- */
    o_byte = 0;
    for (i = 0; i < 10; i++)
    {
        tcflush(fd, TCIOFLUSH);
        if (write(fd, &o_byte, 1) != 1 ||
            read (fd, &i_byte, 1) != 1)
        {
            fprintf(stderr, "Could not synchronize with CGC %s\n", cgc_dev);
            return -1;
        }
    }
    if (i_byte != 0x52)
    {
        fprintf(stderr, "Unexpected sync byte %.2X synchronizing with %s\n",
                0xFF & i_byte, cgc_dev);
        return -1;
    }

    return fd;
}

int main(int argc, char *argv[])
{
    int fd;

    if (argc != 3)
        die("Usage:  cgc_update /path/to/cgc /path/to/firmware");

    read_firmware(argv[2]);
    fd = open_cgc(argv[1]);

    if (fd < 0)
        die("Unable to establish reliable connection to CGC");

    write_firmware(fd);

    close(fd);
    return 0;
}

#else /* CGC_THREAD not defined:  Make a stub executable */

int main(void)
{
    printf("cgc_update unsupported on this platform\n");
    return 1;
}

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
/*                 Copyright (c) 2004-+Inf, Joseph Zbiciak                  */
/* ======================================================================== */
