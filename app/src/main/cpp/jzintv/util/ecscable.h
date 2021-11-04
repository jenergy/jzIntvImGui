/* ======================================================================== */
/*  ECScable Interface and Protocol Routines.                               */
/*  By Joe Zbiciak                                                          */
/* ======================================================================== */
#ifndef ECS_CABLE_H_
#define ECS_CABLE_H_ 1

typedef struct ecscable_t
{
    unsigned long   port;               /* Current ECS cable port number.   */
} ecscable_t;

/* ======================================================================== */
/*  INITIALIZATION AND STARTUP COMMANDS.                                    */
/* ======================================================================== */

/* ------------------------------------------------------------------------ */
/*  EC_DETECT        -- Detect the ECS cable on a given port.               */
/*                      Set the port to 0 to do an autodetect.              */
/* ------------------------------------------------------------------------ */
unsigned ec_detect(unsigned port);

/* ------------------------------------------------------------------------ */
/*  EC_INIT_PORTS    -- Get access to the desired I/O ports and drop        */
/*                      root.  Don't bother returning an error if we fail.  */
/*                      Just abort.                                         */
/* ------------------------------------------------------------------------ */
void ec_init_ports(unsigned long base);

/* ======================================================================== */
/*  HIGH LEVEL PROTOCOL COMMANDS.                                           */
/* ======================================================================== */

/* ------------------------------------------------------------------------ */
/*  EC_UPLOAD        -- Upload data over the ECScable.                      */
/* ------------------------------------------------------------------------ */
int ec_upload
(
    ecscable_t *ec,        /* ECScable structure                            */
    uint16_t    addr,      /* Address to upload to.                         */
    uint16_t    len,       /* Total length of upload.                       */
    uint16_t   *data,      /* Data to upload.                               */
    int         width,     /* Width of data to upload.                      */
    int         icart      /* If non-zero, address is in Intellicart space. */
);

/* ------------------------------------------------------------------------ */
/*  EC_DOWNLOAD      -- Download data over the ECScable.                    */
/* ------------------------------------------------------------------------ */
int ec_download
(
    ecscable_t *ec,        /* ECScable structure                            */
    uint16_t    addr,      /* Address to upload to.                         */
    uint16_t    len,       /* Total length of download.                     */
    uint16_t   *data,      /* Data to download.                             */
    int         width,     /* Width of data to download.                    */
    int         icart      /* If non-zero, address is in Intellicart space. */
);

/* ------------------------------------------------------------------------ */
/*  EC_RESET_INTV    -- Reset the Intellivision via ECScable                */
/* ------------------------------------------------------------------------ */
int ec_reset_intv(ecscable_t *ec, int to_monitor);

/* ------------------------------------------------------------------------ */
/*  EC_PING          -- Sends a simple ping over the ECScable.              */
/* ------------------------------------------------------------------------ */
int ec_ping(ecscable_t *ec);

/* ------------------------------------------------------------------------ */
/*  EC_ECS_ROMS      -- Enable/Disable ECS ROMs via ECScable.               */
/* ------------------------------------------------------------------------ */
int ec_ecs_roms(ecscable_t *ec, int enable);

/* ------------------------------------------------------------------------ */
/*  EC_ECS_SETPAGE   -- Set the Page # on an ECS paged ROM.                 */
/* ------------------------------------------------------------------------ */
int ec_ecs_setpage(ecscable_t *ec, int addr, int page);

/* ------------------------------------------------------------------------ */
/*  EC_VIDEO         -- Enable/Disable active display via ECScable.         */
/* ------------------------------------------------------------------------ */
int ec_video(ecscable_t *ec, int enable);


/* ======================================================================== */
/*  LOWER LEVEL INTERNAL COMMANDS USED BY THE PROTOCOL                      */
/* ======================================================================== */

/* ------------------------------------------------------------------------ */
/*  EC_XFER_DATA     -- Send a 7-bit quantity while receiving a 4-bit qty   */
/* ------------------------------------------------------------------------ */
int ec_xfer_data(ecscable_t *ec, int value);

/* ------------------------------------------------------------------------ */
/*  EC_IDLE          -- Go into the idle state.                             */
/* ------------------------------------------------------------------------ */
void ec_idle(ecscable_t *ec);

/* ------------------------------------------------------------------------ */
/*  EC_DEIDLE        -- Come out of the idle state.  Return non-zero if we  */
/*                      time out (eg. we de-idle but don't see ECS de-idle) */
/* ------------------------------------------------------------------------ */
int ec_deidle(ecscable_t *ec);

/* ------------------------------------------------------------------------ */
/*  EC_SEND_COMMAND  -- Send a 1-byte command to the ECS cable.             */
/* ------------------------------------------------------------------------ */
int ec_send_command(ecscable_t *ec, int command);

/* ------------------------------------------------------------------------ */
/*  EC_SEND_CMDHDR   -- Send a 1-byte command to the ECS cable.             */
/*                                                                          */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  |   A7   |   A6   |   A5   |   A4   |   A3   |   A2   |   A1   |(clk)|  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  |   A15  |   A14  |   A13  |   A12  |   A11  |   A10  |   A9   |(clk)|  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  |             "Length - 1"                   |   A0   |   A8   |(clk)|  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  |                      Header Checksum                         |(clk)|  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*                                                                          */
/*  The header checksum is a simple 2s complement arithmetic sum of         */
/*  the first three bytes.  Not horribly robust, but hey.  After the        */
/*  header, the PC expects to read back 0xA (as above).  If it doesn't,     */
/*  then it goes back to the idle state, waits a moment, and tries          */
/*  sending the command again.                                              */
/*                                                                          */
/* ------------------------------------------------------------------------ */
int ec_send_cmdhdr(ecscable_t *ec, int addr, int len);

/* ------------------------------------------------------------------------ */
/*  EC_SEND_BYTES    -- Send an array of byte data.                         */
/*                                                                          */
/*  For BYTE data, up to 7 bytes in a row in this format:                   */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  |   D7a  |   D6a  |   D5a  |   D4a  |   D3a  |   D2a  |   D1a  |(clk)|  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  |   D7b  |   D6b  |   D5b  |   D4b  |   D3b  |   D2b  |   D1b  |(clk)|  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*        ....                                                              */
/*                                                                          */
/*  Followed by one byte in this format to give the LSBs:                   */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  |   D0g  |   D0f  |   D0e  |   D0d  |   D0c  |   D0b  |   D0a  |(clk)|  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*                                                                          */
/*  If there are more than 7 bytes remaining in a packet, 7 are always      */
/*  sent, otherwise only the remaining bytes are sent.  This eliminates     */
/*  the need for length bytes in the stream.                                */
/* ------------------------------------------------------------------------ */
int ec_send_bytes(ecscable_t *ec, uint16_t *data, int len);

/* ------------------------------------------------------------------------ */
/*  EC_SEND_DECLES   -- Send an array of 10-bit data.                       */
/*                                                                          */
/*  For DECLE data, up to 7 bytes in a row in this format:                  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  |   D7a  |   D6a  |   D5a  |   D4a  |   D3a  |   D2a  |   D1a  |(clk)|  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  |   D7b  |   D6b  |   D5b  |   D4b  |   D3b  |   D2b  |   D1b  |(clk)|  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*        ....                                                              */
/*                                                                          */
/*  Followed by one byte in this format to give the LSBs:                   */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  |   D0g  |   D0f  |   D0e  |   D0d  |   D0c  |   D0b  |   D0a  |(clk)|  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*                                                                          */
/*  Followed by two bytes in this format to give the MSBs:                  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  |   D9d  |   D8c  |   D9c  |   D8b  |   D9b  |   D8a  |   D9a  |(clk)|  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  |   D8g  |   D9g  |   D8f  |   D9f  |   D8e  |   D9e  |   D8d  |(clk)|  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*                                                                          */
/*  If there are more than 7 decles remaining in a packet, 7 are always     */
/*  sent, otherwise only the remaining decles are sent.  This eliminates    */
/*  the need for length bytes in the stream.                                */
/*                                                                          */
/* ------------------------------------------------------------------------ */
int ec_send_decles(ecscable_t *ec, uint16_t *data, int len);

/* ------------------------------------------------------------------------ */
/*  EC_SEND_WORDS    -- Send an array of 16-bit data.                       */
/*                                                                          */
/*  For WORD data, up to 14 bytes in a row in pairs in this format:         */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  |   D7a  |   D6a  |   D5a  |   D4a  |   D3a  |   D2a  |   D1a  |(clk)|  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  |   D15a |   D14a |   D13a |   D12a |   D11a |   D10a |   D9a  |(clk)|  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*        ....                                                              */
/*                                                                          */
/*  Followed by two decles in this format to give the missing bits:         */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  |   D8d  |   D0c  |   D8c  |   D0b  |   D8b  |   D0a  |   D8a  |(clk)|  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  |   D0g  |   D8g  |   D0f  |   D8f  |   D0e  |   D8e  |   D0d  |(clk)|  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*                                                                          */
/*  If there are more than 7 words remaining in a packet, 7 are always      */
/*  sent, otherwise only the remaining words are sent.  This eliminates     */
/*  the need for length bytes in the stream.                                */
/* ------------------------------------------------------------------------ */
int ec_send_words(ecscable_t *ec, uint16_t *data, int len);

/* ------------------------------------------------------------------------ */
/*  EC_RECV_BYTES    -- Send an array of 8-bit data.                        */
/*                                                                          */
/*   For BYTE data, data is just sent in "nickle" pairs:                    */
/*   +--------+--------+--------+--------+--------+                         */
/*   |   D3a  |   D2a  |   D1a  |   D0a  | (clk)  |                         */
/*   +--------+--------+--------+--------+--------+                         */
/*   +--------+--------+--------+--------+--------+                         */
/*   |   D7a  |   D6a  |   D5a  |   D4a  | (clk)  |                         */
/*   +--------+--------+--------+--------+--------+                         */
/* ------------------------------------------------------------------------ */
int ec_recv_bytes(ecscable_t *ec, uint16_t *data, int len);

/* ------------------------------------------------------------------------ */
/*  EC_RECV_DECLES   -- Send an array of 10-bit data.                       */
/*                                                                          */
/*   For DECLE data, data is just sent in "nickle" triads:                  */
/*   +--------+--------+--------+--------+--------+                         */
/*   |   D3a  |   D2a  |   D1a  |   D0a  | (clk)  |                         */
/*   +--------+--------+--------+--------+--------+                         */
/*   +--------+--------+--------+--------+--------+                         */
/*   |   D7a  |   D6a  |   D5a  |   D4a  | (clk)  |                         */
/*   +--------+--------+--------+--------+--------+                         */
/*   +--------+--------+--------+--------+--------+                         */
/*   |   n/a  |   n/a  |   D9a  |   D8a  | (clk)  |                         */
/*   +--------+--------+--------+--------+--------+                         */
/* ------------------------------------------------------------------------ */
int ec_recv_decles(ecscable_t *ec, uint16_t *data, int len);

/* ------------------------------------------------------------------------ */
/*  EC_RECV_WORDS    -- Send an array of 16-bit data.                       */
/*                                                                          */
/*   For DECLE data, data is just sent in "nickle" triads:                  */
/*   +--------+--------+--------+--------+--------+                         */
/*   |   D3a  |   D2a  |   D1a  |   D0a  | (clk)  |                         */
/*   +--------+--------+--------+--------+--------+                         */
/*   +--------+--------+--------+--------+--------+                         */
/*   |   D7a  |   D6a  |   D5a  |   D4a  | (clk)  |                         */
/*   +--------+--------+--------+--------+--------+                         */
/*   +--------+--------+--------+--------+--------+                         */
/*   |   D11a |   D10a |   D9a  |   D8a  | (clk)  |                         */
/*   +--------+--------+--------+--------+--------+                         */
/*   +--------+--------+--------+--------+--------+                         */
/*   |   D15a |   D14a |   D13a |   D12a | (clk)  |                         */
/*   +--------+--------+--------+--------+--------+                         */
/* ------------------------------------------------------------------------ */
int ec_recv_words(ecscable_t *ec, uint16_t *data, int len);

/* ------------------------------------------------------------------------ */
/*  EC_SLEEP         -- Wrapper around nanosleep().                         */
/* ------------------------------------------------------------------------ */
void ec_sleep(long len);

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
/*                 Copyright (c) 1998-2001, Joseph Zbiciak                  */
/* ======================================================================== */
