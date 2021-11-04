/* ======================================================================== */
/*  EMU-LINK FILE I/O                                                       */
/*                                                                          */
/*  This module implements the Emu-Link File I/O API.  The API provides     */
/*  the following Emu-Link hooks:                                           */
/*                                                                          */
/*      10  OPEN("filename", flags)     => fd                               */
/*      11  CLOSE(fd)                                                       */
/*      12  READ(fd, buf, bytes)        => bytes read                       */
/*      13  READ16(fd, buf, words)      => words read                       */
/*      14  WRITE(fd, buf, bytes)       => bytes written                    */
/*      15  WRITE16(fd, buf, words)     => words written                    */
/*      16  LSEEK(fd, offset, whence)   => byte offset in file              */
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
#ifndef ELFI_H_
#define ELFI_H_ 1


#define MAX_ELFI_FD     (8)
#define MAX_ELFI_FNAME  (31)

#define ELFI_O_RDONLY   (1 << 0)
#define ELFI_O_WRONLY   (1 << 1)
#define ELFI_O_RDWR     (3 << 0)
#define ELFI_O_APPEND   (1 << 2)
#define ELFI_O_CREAT    (1 << 3)
#define ELFI_O_EXCL     (1 << 4)
#define ELFI_O_TRUNC    (1 << 5)

#define ELFI_SEEK_SET   (0)
#define ELFI_SEEK_CUR   (1)
#define ELFI_SEEK_END   (2)

/* ======================================================================== */
/*  ELFI_INIT   Register all the Emu-Link APIs.                             */
/* ======================================================================== */
int elfi_init(const char *elfi_prefix);

/* ======================================================================== */
/*  ELFI_DTOR   Close any open files and reset the fd_map table.            */
/* ======================================================================== */
void elfi_dtor(void);

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
/*                 Copyright (c) 2011-+Inf, Joseph Zbiciak                  */
/* ======================================================================== */
