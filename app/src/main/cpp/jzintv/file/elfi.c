/* ======================================================================== */
/*  EMU-LINK FILE I/O                                                       */
/*                                                                          */
/*  This module implements the Emu-Link File I/O API.  The API provides     */
/*  the following Emu-Link hooks:                                           */
/*                                                                          */
/*      10  OPEN   ("filename", flags)  => fd in R2                         */
/*      11  CLOSE  (fd)                                                     */
/*      12  READ   (fd, buf, bytes)     => bytes read in R1                 */
/*      13  READ16 (fd, buf, words)     => words read in R1                 */
/*      14  WRITE  (fd, buf, bytes)     => bytes written in R1              */
/*      15  WRITE16(fd, buf, words)     => words written in R1              */
/*      16  LSEEK  (fd, offset, whence) => byte offset in file in R2:R1     */
/*      17  UNLINK ("filename")                                             */
/*      18  RENAME ("old", "new")                                           */
/*                                                                          */
/*                                                                          */
/*  These provide essentially the same API as C does.                       */
/*                                                                          */
/*  The File I/O API tries to prevent the program from reading and          */
/*  writing arbitary files in the system.  When initialized, the user       */
/*  passes a directory prefix, and the File I/O API tries to ensure all     */
/*  files get written under that directory.                                 */
/*                                                                          */
/*  See the "ELFI_xxxx" functions below for the detailed API.               */
/*                                                                          */
/*  TODO:  ELFI doesn't really support a robust errno.  For now, just -1    */
/*  gets returned to the Intellivision.                                     */
/*                                                                          */
/* ======================================================================== */


#include "config.h"
#include "periph/periph.h"
#include "cp1600/cp1600.h"
#include "cp1600/emu_link.h"
#include "elfi.h"
#include <ctype.h>

LOCAL int   fd_map[MAX_ELFI_FD];     /* map Intellivision fd to system fd */
LOCAL char *elfi_fname = NULL;
LOCAL char *elfi_fname_end;

LOCAL int elfi_open   (cp1600_t *, int *, void *);
LOCAL int elfi_close  (cp1600_t *, int *, void *);
LOCAL int elfi_read   (cp1600_t *, int *, void *);
LOCAL int elfi_read16 (cp1600_t *, int *, void *);
LOCAL int elfi_write  (cp1600_t *, int *, void *);
LOCAL int elfi_write16(cp1600_t *, int *, void *);
LOCAL int elfi_lseek  (cp1600_t *, int *, void *);
LOCAL int elfi_unlink (cp1600_t *, int *, void *);
LOCAL int elfi_rename (cp1600_t *, int *, void *);

/* ======================================================================== */
/*  ELFI_INIT   Register all the Emu-Link APIs.                             */
/* ======================================================================== */
int elfi_init(const char *elfi_prefix)
{
    int i;
    int prefix_len = strlen(elfi_prefix);

    elfi_fname     = (char *)malloc(strlen(elfi_prefix) + MAX_ELFI_FNAME + 2);
    elfi_fname_end = elfi_fname + prefix_len;

    memcpy(elfi_fname, elfi_prefix, prefix_len);
    *elfi_fname_end++ = PATH_SEP;

    if (!elfi_fname)
    {
        fprintf(stderr, "elfi:  Out of memory\n");
        return -1;
    }

    for (i = 0; i < MAX_ELFI_FD; i++)
        fd_map[i] = 0;

    if (emu_link_register(elfi_open,    10, NULL) == 0 &&
        emu_link_register(elfi_close,   11, NULL) == 0 &&
        emu_link_register(elfi_read,    12, NULL) == 0 &&
        emu_link_register(elfi_read16,  13, NULL) == 0 &&
        emu_link_register(elfi_write,   14, NULL) == 0 &&
        emu_link_register(elfi_write16, 15, NULL) == 0 &&
        emu_link_register(elfi_lseek,   16, NULL) == 0 &&
        emu_link_register(elfi_unlink,  17, NULL) == 0 &&
        emu_link_register(elfi_rename,  18, NULL) == 0)
    {
        jzp_printf("elfi:  Emu-Link File I/O enabled and directed to %s\n",
                    elfi_prefix);
        return 0;
    }

    return -1;
}

/* ======================================================================== */
/*  ELFI_DTOR   Close any open files and reset the fd_map table.            */
/* ======================================================================== */
void elfi_dtor(void)
{
    int i;

    for (i = 0; i < MAX_ELFI_FD; i++)
    {
        if (fd_map[i] > 0)
            close(fd_map[i] - 1);

        fd_map[i] = 0;
    }

    CONDFREE(elfi_fname);
}

/* ======================================================================== */
/*  GET_FNAME   Convert the filename to a string and make sure it's sane.   */
/* ======================================================================== */
LOCAL int get_fname(cp1600_t *cp, uint32_t addr)
{
    int i;
    uint16_t ch = 0;

    /* -------------------------------------------------------------------- */
    /*  Disallow everything but alphanumerics, '-', '_' and '.'.            */
    /* -------------------------------------------------------------------- */
    for (i = 0; i < MAX_ELFI_FNAME; i++)
    {
        if (addr + i > 0xFFFF)  /* don't wrap end-of-memory */
            return -1;

        elfi_fname_end[i] = ch = CP1600_RD(cp, addr + i);

        if (ch == 0)            /* break on NUL */
            break;

        if (ch > 0x7F)          /* Don't trust isalnum for > 0x7F */
            return -1;

        if (!isalnum(ch)  &&
            ch != '-'     &&
            ch != '_'     &&
            (i == 0 || ch != '.'))  /* no leading '.' */
            return -1;
    }

    /* -------------------------------------------------------------------- */
    /*  Make sure it was NUL terminated.                                    */
    /* -------------------------------------------------------------------- */
    if (ch)
        return -1;

    return 0;
}

#define FAIL                do { *fail = 1; return -1; } while (0)
#define SUCCESS             do { *fail = 0; return  0; } while (0)
#define IFD_TO_FD           do {                                        \
                                int ifd = cp->r[2];                     \
                                if (ifd >= MAX_ELFI_FD) FAIL;           \
                                if ((fd = fd_map[ifd] - 1) < 0) FAIL;   \
                            } while (0);


/* ======================================================================== */
/*  ELFI_OPEN       Open a new file on behalf of the Intellivision.         */
/*                                                                          */
/*  INPUTS:                                                                 */
/*      R1  Constant:  10                                                   */
/*      R2  Pointer to ASCIIZ filename.                                     */
/*      R3  Flags.  Flags must come from the following set, ORed together:  */
/*                                                                          */
/*                  O_RDONLY   (1 << 0)                                     */
/*                  O_WRONLY   (2 << 0)                                     */
/*                  O_RDWR     (3 << 0)                                     */
/*                  O_APPEND   (1 << 2)                                     */
/*                  O_CREAT    (1 << 3)                                     */
/*                  O_EXCL     (1 << 4)                                     */
/*                  O_TRUNC    (1 << 5)                                     */
/*                                                                          */
/*  OUTPUTS:                                                                */
/*      C   Clear on success, set on failure                                */
/*      R0  errno on failure                                                */
/*      R2  File descriptor on success                                      */
/*                                                                          */
/* ======================================================================== */
LOCAL int elfi_open   (cp1600_t *cp, int *fail, void *opaque)
{
    int flags, iflags;
    int fd, ifd;
    UNUSED(opaque);

    /* -------------------------------------------------------------------- */
    /*  Allocate an Intellivision file descriptor slot.                     */
    /* -------------------------------------------------------------------- */
    for (ifd = 0; ifd < MAX_ELFI_FD; ifd++)
        if (!fd_map[ifd])
            break;

    if (ifd == MAX_ELFI_FD) FAIL;

    /* -------------------------------------------------------------------- */
    /*  Get the filename from the target.                                   */
    /* -------------------------------------------------------------------- */
    if (get_fname(cp, cp->r[2])) FAIL;

    /* -------------------------------------------------------------------- */
    /*  Remap the target's flags to the host OS.                            */
    /* -------------------------------------------------------------------- */
    iflags = cp->r[3];
    if (iflags & (-(1u << 6))) FAIL;
    if ((iflags & ELFI_O_RDWR) == 0) FAIL;

    flags = ((iflags & ELFI_O_RDWR)    == ELFI_O_RDWR   ? O_RDWR   : 0)
          | ((iflags & ELFI_O_RDWR)    == ELFI_O_RDONLY ? O_RDONLY : 0)
          | ((iflags & ELFI_O_RDWR)    == ELFI_O_WRONLY ? O_WRONLY : 0)
          | ((iflags & ELFI_O_APPEND)  == ELFI_O_APPEND ? O_APPEND : 0)
          | ((iflags & ELFI_O_CREAT)   == ELFI_O_CREAT  ? O_CREAT  : 0)
          | ((iflags & ELFI_O_EXCL)    == ELFI_O_EXCL   ? O_EXCL   : 0)
          | ((iflags & ELFI_O_TRUNC)   == ELFI_O_TRUNC  ? O_TRUNC  : 0);

#ifdef O_BINARY
    flags |= O_BINARY;
#endif

    /* -------------------------------------------------------------------- */
    /*  Ok, let's try to open this file, shall we?                          */
    /* -------------------------------------------------------------------- */
    fd = open(elfi_fname, flags, 0666);     /* Let user modify via umask    */

    if (fd < 0) FAIL;

    fd_map[ifd] = fd + 1;

    /* -------------------------------------------------------------------- */
    /*  Success:  Return the Intellivision file descriptor.                 */
    /* -------------------------------------------------------------------- */
    cp->r[2] = ifd;

    SUCCESS;
}

/* ======================================================================== */
/*  ELFI_CLOSE      Close the requested file.                               */
/*                                                                          */
/*  INPUTS:                                                                 */
/*      R1  Constant:  11                                                   */
/*      R2  File descriptor                                                 */
/*                                                                          */
/*  OUTPUTS:                                                                */
/*      C   Clear on success, set on failure                                */
/*      R0  errno on failure                                                */
/* ======================================================================== */
LOCAL int elfi_close  (cp1600_t *cp, int *fail, void *opaque)
{
    int fd;
    UNUSED(opaque);

    IFD_TO_FD;

    fd_map[cp->r[2]] = 0;  /* deallocate FD */

    if (close(fd) != 0) FAIL;

    SUCCESS;
}


/* ======================================================================== */
/*  ELFI_READ       Read from the file into a buffer as bytes.              */
/*  ELFI_READ16     Read from the file into a buffer as big endian words.   */
/*                                                                          */
/*  The READ API reads in bytes, populating the lower 8 bits of the         */
/*  specified locations.  The READ16 API reads in words in big-endian,      */
/*  populating all 16 bits of the specified locations, if said locations    */
/*  are in 16-bit RAM.                                                      */
/*                                                                          */
/*  The buffer gets written to memory in the same manner as if the CPU      */
/*  wrote it, with all the same restrictions therein (ie. you can't         */
/*  overwrite ROM or violate STIC access windows).  The writes take zero    */
/*  time, however.                                                          */
/*                                                                          */
/*  INPUTS:                                                                 */
/*      R1  Constant:  12 (READ) or 13 (READ16)                             */
/*      R2  File descriptor                                                 */
/*      R3  Pointer to buffer                                               */
/*      R4  Number of bytes (READ) or words (READ16) to read.               */
/*                                                                          */
/*  OUTPUTS:                                                                */
/*      C   Clear on success, set on failure                                */
/*      R0  errno on failure                                                */
/*      R1  Number of bytes/words read                                      */
/*      @R3 Data read                                                       */
/*                                                                          */
/* ======================================================================== */
LOCAL int elfi_read   (cp1600_t *cp, int *fail, void *opaque)
{
    int fd, bytes, addr, total = 0, r;
    uint8_t b;
    UNUSED(opaque);

    /* -------------------------------------------------------------------- */
    /*  Get arguments.                                                      */
    /* -------------------------------------------------------------------- */
    IFD_TO_FD;
    addr  = cp->r[3];
    bytes = cp->r[4];

    /* -------------------------------------------------------------------- */
    /*  For now, do this slow and stupid, because it's also easy and not    */
    /*  performance critical in the least.                                  */
    /* -------------------------------------------------------------------- */
    while (bytes--)
    {
        cp->r[1] = total;
        if ((r = read(fd, &b, 1)) == 1)
            CP1600_WR(cp, (addr + total++) & 0xFFFF, b & 0xFF);
        else if (r == 0) break;
        else             { cp->r[1] = total; FAIL; }
    }

    cp->r[1] = total;
    SUCCESS;
}

LOCAL int elfi_read16 (cp1600_t *cp, int *fail, void *opaque)
{
    int fd, words, addr, total = 0, r0, r1;
    uint8_t b0, b1;
    uint16_t wo;
    UNUSED(opaque);

    /* -------------------------------------------------------------------- */
    /*  Get arguments.                                                      */
    /* -------------------------------------------------------------------- */
    IFD_TO_FD;
    addr  = cp->r[3];
    words = cp->r[4];

    /* -------------------------------------------------------------------- */
    /*  For now, do this slow and stupid, because it's also easy and not    */
    /*  performance critical in the least.                                  */
    /* -------------------------------------------------------------------- */
    while (words--)
    {
        if ((r0 = read(fd, &b0, 1)) == 1 &&
            (r1 = read(fd, &b1, 1)) == 1)
        {
            wo = ((uint16_t)b0 << 8) | b1;
            CP1600_WR(cp, (addr + total++) & 0xFFFF, wo);
        }
        else if (r0 == 0) break;
        else              { cp->r[1] = total; FAIL; }
    }

    cp->r[1] = total;
    SUCCESS;
}


/* ======================================================================== */
/*  ELFI_WRITE, ELFI_WRITE16                                                */
/*                                                                          */
/*  The WRITE API writes out bytes, taking them from the lower 8 bits of    */
/*  the specified locations.  The WRITE16 API writes out words in big-      */
/*  endian, writing out the full 16-bit value read from the specified       */
/*  locations.                                                              */
/*                                                                          */
/*  The buffer gets read from memory in the same manner as if the CPU       */
/*  read it, with all the same restrictions therein (ie. you can't          */
/*  violate STIC access windows).  The reads take zero time, however.       */
/*                                                                          */
/*  INPUTS:                                                                 */
/*      R1  Constant:  14 (WRITE) or 15 (WRITE16)                           */
/*      R2  File descriptor                                                 */
/*      R3  Pointer to buffer                                               */
/*      R4  Number of bytes (WRITE) or words (WRITE16) to write             */
/*                                                                          */
/*  OUTPUTS                                                                 */
/*      C   Clear on success, set on failure                                */
/*      R0  errno on failure                                                */
/*      R1  Number of bytes/words written                                   */
/*                                                                          */
/* ======================================================================== */
LOCAL int elfi_write  (cp1600_t *cp, int *fail, void *opaque)
{
    int fd, bytes, addr, total = 0, w;
    uint8_t b;
    UNUSED(opaque);

    /* -------------------------------------------------------------------- */
    /*  Get arguments.                                                      */
    /* -------------------------------------------------------------------- */
    IFD_TO_FD;
    addr  = cp->r[3];
    bytes = cp->r[4];

    if (addr + bytes > 0xFFFF) { cp->r[1] = 0; FAIL; }

    /* -------------------------------------------------------------------- */
    /*  For now, do this slow and stupid, because it's also easy and not    */
    /*  performance critical in the least.                                  */
    /* -------------------------------------------------------------------- */
    while (bytes--)
    {
        b = CP1600_RD(cp, (addr + total++)) & 0xFF;
        w = write(fd, &b, 1);
        cp->r[1] = total;

        if (w == 1) continue;
        if (w == 0) break;
        FAIL;
    }

    SUCCESS;
}

LOCAL int elfi_write16(cp1600_t *cp, int *fail, void *opaque)
{
    int fd, words, addr, total = 0, w;
    uint8_t  b[2];
    uint16_t wo;
    UNUSED(opaque);

    /* -------------------------------------------------------------------- */
    /*  Get arguments.                                                      */
    /* -------------------------------------------------------------------- */
    IFD_TO_FD;
    addr  = cp->r[3];
    words = cp->r[4];

    if (addr + words > 0xFFFF) { cp->r[1] = 0; FAIL; }

    /* -------------------------------------------------------------------- */
    /*  For now, do this slow and stupid, because it's also easy and not    */
    /*  performance critical in the least.                                  */
    /* -------------------------------------------------------------------- */
    while (words--)
    {
        wo   = CP1600_RD(cp, (addr + total++));
        b[0] = (wo >> 8) & 0xFF;
        b[1] = (wo >> 0) & 0xFF;
        w    = write(fd, b, 2);
        cp->r[1] = total;

        if (w == 2) continue;
        if (w == 0) break;
        FAIL;
    }

    SUCCESS;
}

/* ======================================================================== */
/*  ELFI_LSEEK                                                              */
/*                                                                          */
/*  Changes the offset within the file, and returns the new offset.         */
/*  Offsets are 32 bits, signed.                                            */
/*                                                                          */
/*  INPUTS:                                                                 */
/*      R1  Constant:  16 (LSEEK)                                           */
/*      R2  File descriptor                                                 */
/*      R3  Lower 16 bits of signed offset                                  */
/*      R4  Upper 16 bits of signed offset                                  */
/*      R5  "Whence":  0 == SEEK_SET, 1 == SEEK_CUR, 2 == SEEK_END          */
/*                                                                          */
/*  OUTPUTS:                                                                */
/*      C   Clear on success, set on failure                                */
/*      R0  errno on failure                                                */
/*      R1  Lower 16 bits of new file offset                                */
/*      R2  Upper 16 bits of new file offset                                */
/*                                                                          */
/* ======================================================================== */
LOCAL int elfi_lseek  (cp1600_t *cp, int *fail, void *opaque)
{
    int fd, ofs, whence, fpos;
    UNUSED(opaque);

    /* -------------------------------------------------------------------- */
    /*  Get arguments.                                                      */
    /* -------------------------------------------------------------------- */
    IFD_TO_FD;
    ofs = cp->r[3] | ((int)cp->r[4] << 16);

    switch (cp->r[5])
    {
        case ELFI_SEEK_SET: whence = SEEK_SET; break;
        case ELFI_SEEK_CUR: whence = SEEK_CUR; break;
        case ELFI_SEEK_END: whence = SEEK_END; break;
        default:            FAIL;
    }

    /* -------------------------------------------------------------------- */
    /*  Seek it.                                                            */
    /* -------------------------------------------------------------------- */
    fpos = lseek(fd, ofs, whence);

    if (fpos == -1) FAIL;

    cp->r[1] = (fpos >>  0) & 0xFFFF;
    cp->r[2] = (fpos >> 16) & 0xFFFF;

    SUCCESS;
}

/* ======================================================================== */
/*  ELFI_UNLINK                                                             */
/*                                                                          */
/*  Tries to unlink (ie. remove) a file from the file system.  No check is  */
/*  made to determine if the file is currently open.  Behavior is defined   */
/*  by jzIntv's host OS.                                                    */
/*                                                                          */
/*  INPUTS:                                                                 */
/*      R1  Constant:  17 (UNLINK)                                          */
/*      R2  Pointer to ASCIIZ file name                                     */
/*                                                                          */
/*  OUTPUTS:                                                                */
/*      C   Clear on success, set on failure                                */
/*      R0  errno on failure                                                */
/* ======================================================================== */
LOCAL int elfi_unlink (cp1600_t *cp, int *fail, void *opaque)
{
    UNUSED(opaque);
    if (get_fname(cp, cp->r[2])) FAIL;
    if (unlink(elfi_fname) != 0) FAIL;

    SUCCESS;
}

/* ======================================================================== */
/*  ELFI_RENAME                                                             */
/*                                                                          */
/*  Tries to rename a file in the file system.  No check is made to         */
/*  determine if the file is currently open.  Behavior is defined by        */
/*  jzIntv's host OS.                                                       */
/*                                                                          */
/*  INPUTS:                                                                 */
/*      R1  Constant:  18 (RENAME)                                          */
/*      R2  Pointer to ASCIIZ file name for old name                        */
/*      R3  Pointer to ASCIIZ file name for new name                        */
/*                                                                          */
/*  OUTPUTS:                                                                */
/*      C   Clear on success, set on failure                                */
/*      R0  errno on failure                                                */
/* ======================================================================== */
LOCAL int elfi_rename (cp1600_t *cp, int *fail, void *opaque)
{
    char *old_fname;
    UNUSED(opaque);

    if (get_fname(cp, cp->r[2]))            FAIL;
    if (!(old_fname = strdup(elfi_fname)))  FAIL;
    if (get_fname(cp, cp->r[3]))            { free(old_fname); FAIL; }
    if (rename(old_fname, elfi_fname) != 0) { free(old_fname); FAIL; }

    free(old_fname);
    SUCCESS;
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
/*                 Copyright (c) 2011-+Inf, Joseph Zbiciak                  */
/* ======================================================================== */
