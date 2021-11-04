/* ======================================================================== */
/*  ECScable Interface and Protocol Routines.                               */
/*  By Joe Zbiciak                                                          */
/* ======================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include "../config.h"
#include "util/ecscable.h"

#if (!defined(PLAT_LINUX) && !defined(WIN32)) || !defined(i386)

void ec_sleep(long len)
{
    UNUSED(len);
}

unsigned ec_detect(unsigned port)
{
    UNUSED(port);
    return 0;
}

void ec_init_ports(unsigned long base)
{
    UNUSED(base);
}

int ec_upload
(
    ecscable_t *ec,        /* ECScable structure                            */
    uint16_t    addr,      /* Address to upload to.                         */
    uint16_t    len,       /* Total length of upload.                       */
    uint16_t   *data,      /* Data to upload.                               */
    int         width,     /* Width of data to upload.                      */
    int         icart      /* If non-zero, address is in Intellicart space. */
)
{
    UNUSED(ec); UNUSED(addr); UNUSED(len);
    UNUSED(data); UNUSED(width); UNUSED(icart);
    return -1;
}

int ec_download
(
    ecscable_t *ec,        /* ECScable structure                            */
    uint16_t    addr,      /* Address to upload to.                         */
    uint16_t    len,       /* Total length of download.                     */
    uint16_t   *data,      /* Data to download.                             */
    int         width,     /* Width of data to download.                    */
    int         icart      /* If non-zero, address is in Intellicart space. */
)
{
    UNUSED(ec); UNUSED(addr); UNUSED(len);
    UNUSED(data); UNUSED(width); UNUSED(icart);
    return -1;
}

int ec_reset_intv(ecscable_t *ec, int to_monitor)
{
    UNUSED(ec); UNUSED(to_monitor);
    return -1;
}

int ec_ping(ecscable_t *ec)
{
    UNUSED(ec);
    return -1;
}

int ec_ecs_roms(ecscable_t *ec, int enable)
{
    UNUSED(ec); UNUSED(enable);
    return -1;
}

int ec_ecs_setpage(ecscable_t *ec, int addr, int page)
{
    UNUSED(ec); UNUSED(addr); UNUSED(page);
    return -1;
}

int ec_video(ecscable_t *ec, int enable)
{
    UNUSED(ec); UNUSED(enable);
    return -1;
}

void ec_idle(ecscable_t *ec)
{
    UNUSED(ec);
}

#else


static unsigned long ports[3] = { 0x378, 0x278, 0x3bc }; /* printer ports  */

#define ec_outb(x,y) outb(y,x)
#define ec_inb(x)    inb(x)

#define EC_CTRL(e,x) (ec_outb((e)->port + 2, (x)))
#if 1
#define EC_SEND(e,x)  (ec_outb((e)->port, (x)))
#define EC_RECV(e)    ((ec_inb((e)->port + 1) ^ 0x80) >> 3)
#else
static char buf[100];
//#define EC_SEND(e,x) (gets(buf),printf("send(%.8x)\n", (x)), (ec_outb((e)->port, (x))))
#define EC_SEND(e,x) (printf("send(%.8x)\n", (x)), (ec_outb((e)->port, (x))))
#define EC_RECV_(e)   ((ec_inb((e)->port + 1) ^ 0x80) >> 3)
static inline uint32_t EC_RECV(ecscable_t *e)
{
    uint32_t val = ((ec_inb((e)->port + 1) ^ 0x80) >> 3);
    printf("recv(%.8x)\n", val);
    return val;
}
#endif

#define EC_TIMEOUT (2000000)

/* ------------------------------------------------------------------------ */
/*  GENERAL ECSCABLE DESCRIPTION                                            */
/*                                                                          */
/*  The ECS cable has 8 lines going from PC->ECS, and 5 lines going from    */
/*  ECS->PC.  One line in each direction is used as a clock line, leaving   */
/*  7 data lines from PC->ECS and 4 data lines from ECS->PC.                */
/*                                                                          */
/*  For reliability reasons (as it seems that a 0->1 transition is faster   */
/*  than a 1->0 transition, owing to pullup resistors on the bus), all      */
/*  data transactions occur over a full "clock" period, with valid data     */
/*  being read at the end of a 1->0 clock transition.                       */
/*                                                                          */
/*  PC MASTER MODE                                                          */
/*                                                                          */
/*  This is the mode that the existing ECScable Monitor operates in, and    */
/*  around which I've presently constructed my protocols and command        */
/*  structure.  Other protocols are certainly possible with this cable.     */
/*                                                                          */
/*  In this protocol, the PC always provides the clock, and the ECS         */
/*  echos the clock.  The following diagram shows one read/write cycle      */
/*  from the PC's perspective:                                              */
/*                            _____                                         */
/*          PC Clock:  ______|     |_________                               */
/*                           ________________                               */
/*          PC Data:   XXXXX>>>>>>>__________<                              */
/*                               ______                                     */
/*         ECS Clock:  _________|      |_____                               */
/*                           ________________                               */
/*         ECS Data:   XXXXX>>>>>>>>>>_______<                              */
/*                             ^^         ^^                                */
/*                       Data changing   Data valid                         */
/*                                                                          */
/*                                                                          */
/*  (Engineering aside:  The following code seems to reliably do a          */
/*   loopback.  Other clock protocols seem to be glitchy, probably          */
/*   owing to the pullups on the Inty side?? as noted above.                */
/*                                                                          */
/*      PC side:                                                            */
/*                                                                          */
/*      do {                                                                */
/*          outb(i | c, base + 0);                                          */
/*          do {                                                            */
/*              j = (0x80 ^ inb(base + 1)) >> 3;                            */
/*          } while ((j & 1) != c);                                         */
/*          c = !c;                                                         */
/*      } while (c == 0);                                                   */
/*                                                                          */
/*     INTV side:                                                           */
/*                                                                          */
/*      @@l MVI     $FF,  R0                                                */
/*          MVO     R0,   $FE                                               */
/*          B       @@l                                                     */
/*                                                                          */
/*  That's all it takes to do a simple loopback to verify operation.)       */
/*                                                                          */
/*  All data transmissions are simultaneously transmits and receives.       */
/*  If a given direction isn't sending or receiving something, it should    */
/*  set the lines to all 1s.  (This isn't mandatory.  Several of the I/O    */
/*  routines I currently have output some internal variable, such as a      */
/*  loop count or partial value, since the CP-1600 side tries to keep as    */
/*  much in registers as possible.  This actually is sometimes useful       */
/*  for debugging the protocol.)                                            */
/*                                                                          */
/*  Also, when the line is in the idle state, both directions should leave  */
/*  the line floating at all 1s.  (eg. 0xFF)  The Inty should set its xmit  */
/*  lines to the "input" direction to let the drivers on the PSG rest.      */
/*                                                                          */
/*  How to distinguish IDLE from sending 0xFE with clock hi?  Good          */
/*  question.  There is no good answer.  Currently, the Inty side has no    */
/*  timeouts, and the PC side implements a fairly long timeout.             */
/*                                                                          */
/*  COMMAND SEQUENCE                                                        */
/*                                                                          */
/*  The PC alerts the ECS that it wants to issue a command by sending       */
/*  0x00 on its line, thereby coming out of idle and alerting the ECS       */
/*  that it wants to do something.  It waits for the ECS to respond in      */
/*  kind by responding with 0x00.  Once this short handshake completes,     */
/*  the PC can send a command.                                              */
/*                                                                          */
/*  For robustness, each command is sent twice -- as the command word       */
/*  and as the inverse of the command word.  The PC then sends up           */
/*  0xAA and expects to read back a single nibble from the ECS.  If         */
/*  it reads back a 0x0A, it assumes the Inty read the command ok.          */
/*  If it reads back anything else, it assumes the Inty did not read        */
/*  the command correctly.  If the PC sees that the Inty did not read       */
/*  the command correctly, it returns to the idle state and then retries.   */
/*  Otherwise, the PC sends the body of the command packet (if any).        */
/*                                                                          */
/*  Example:                                                                */
/*                                                                          */
/*      PC->ECS     ECS->PC    Description                                  */
/*      11111111    11111      Idle                                         */
/*      00000000    11111      PC initiates handshake                       */
/*      00000000    00000      ECS responds to handshake                    */
/*   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */
/*      01011101    00000      PC sends a command, clock HI                 */
/*      01011101    xxxx1      ECS responds by bringing clock HI            */
/*      01011100    xxxx1      PC sends a command, clock LO                 */
/*      01011100    xxxx0      ECS responds by bringing clock LO            */
/*   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */
/*      10100011    xxxx0      PC sends command inverse, clock HI           */
/*      10100011    xxxx1      ECS responds by bringing clock HI            */
/*      10100010    xxxx1      PC sends command inverse, clock LO           */
/*      10100010    xxxx0      ECS responds by bringing clock LO            */
/*   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */
/*      10101011    xxxx0      PC sends a zero to finish cmd, clock HI      */
/*      10101011    01011      ECS responds with 0xA, clock HI              */
/*      10101010    01011      PC sends a zero, clock LO                    */
/*      10101010    01010      ECS responds with 0xA, clock LO              */
/*   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */
/*                                                                          */
/*                                                                          */
/*  Commands:                                                               */
/*                                                                          */
/*  (Note:  LSB is clock, and is shown as zero in commands below.           */
/*   This is consistent with how the PC-side code is written.)              */
/*                                                                          */
/*   0x00:  PING (just echo back "OK!")                                     */
/*   0x02:  Soft Reset to GAME                                              */
/*   0x04:  Soft Reset to MONITOR                                           */
/*   0x06:  Active video OFF                                                */
/*   0x08:  Active video ON                                                 */
/*   0x0A:  Disable ECS ROMs                                                */
/*   0x0C:  Enable ECS ROMs                                                 */
/*   0x0E:  Set ECS Page                                                    */
/*                                                                          */
/*   0x10:  PC->ECS xfer bytes  within Inty address space                   */
/*   0x12:  PC->ECS xfer decles within Inty address space                   */
/*   0x14:  PC->ECS xfer words  within Inty address space                   */
/*   0x16:  NOP (reserved)                                                  */
/*                                                                          */
/*   0x18:  ECS->PC xfer bytes  within Inty address space                   */
/*   0x1A:  ECS->PC xfer decles within Inty address space                   */
/*   0x1C:  ECS->PC xfer words  within Inty address space                   */
/*   0x1E:  NOP (reserved)                                                  */
/*                                                                          */
/*   0x20:  PC->ECS xfer bytes  within Intellicart address space            */
/*   0x22:  PC->ECS xfer decles within Intellicart address space            */
/*   0x24:  PC->ECS xfer words  within Intellicart address space            */
/*   0x26:  NOP (reserved)                                                  */
/*                                                                          */
/*   0x28:  ECS->PC xfer bytes  within Intellicart address space            */
/*   0x2A:  ECS->PC xfer decles within Intellicart address space            */
/*   0x2C:  ECS->PC xfer words  within Intellicart address space            */
/*   0x2E:  NOP (reserved)                                                  */
/*                                                                          */
/*  The two RESET commands have no command body.  The PC side should        */
/*  go to idle after issuing the command.                                   */
/*                                                                          */
/*  The Set ECS Page command is followed by a two-word command body which   */
/*  specifies which 4K bank to switch, and which ROM page to select into    */
/*  that bank.  The switching scheme allows for 1 of 16 different 4K pages  */
/*  to be selected within a 4K bank.  The following header is sent.         */
/*                                                                          */
/*  First word is the upper four bits of the address range being switched:  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  |    0   |    0   |    0   |  A15   |  A14   |  A13   |  A12   |(clk)|  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*                                                                          */
/*  Next word is the bank number being selected into the page:              */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  |    0   |    0   |    0   |   B3   |   B2   |   B1   |   B0   |(clk)|  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*                                                                          */
/*  The remaining memory transfer commands are followed by short header     */
/*  which gives the address and length.  The data is sent as follows:       */
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
/*  The header checksum is a simple 2s complement arithmetic sum of the     */
/*  first three bytes.  Not horribly robust, but hey.  After the header,    */
/*  the PC sends 0x00 and expects to read back 0xA (as above).  If it       */
/*  doesn't, then it goes back to the idle state, waits a moment, and       */
/*  tries sending the command again.                                        */
/*                                                                          */
/*  After the header, the PC and the ECS send data back and forth.  For     */
/*  the PC->ECS the following formats are used:                             */
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
/*                                                                          */
/*  - - - - - - - - - - - - -                                               */
/*                                                                          */
/*  For DECLE data, up to 7 bytes in a row in this format:                  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  |   D9a  |   D8a  |   D7a  |   D6a  |   D5a  |   D4a  |   D3a  |(clk)|  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  |   D9b  |   D8b  |   D7b  |   D6b  |   D5b  |   D4b  |   D3b  |(clk)|  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*        ....                                                              */
/*                                                                          */
/*  Followed by one byte in this format to give bit2 for the DECLEs:        */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  |   D2g  |   D2f  |   D2e  |   D2d  |   D2c  |   D2b  |   D2a  |(clk)|  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*                                                                          */
/*  Followed by two bytes in this format to give bit1 and bit0 for the      */
/*  DECLEs:                                                                 */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  |   D1d  |   D0c  |   D1c  |   D0b  |   D1b  |   D0a  |   D1a  |(clk)|  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  |   D0g  |   D1g  |   D0f  |   D1f  |   D0e  |   D1e  |   D0d  |(clk)|  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*                                                                          */
/*  If there are more than 7 decles remaining in a packet, 7 are always     */
/*  sent, otherwise only the remaining decles are sent.  This eliminates    */
/*  the need for length bytes in the stream.                                */
/*                                                                          */
/*  - - - - - - - - - - - - -                                               */
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
/*    If there are more than 7 words remaining in a packet, 7 are always    */
/*    sent, otherwise only the remaining words are sent.  This eliminates   */
/*    the need for length bytes in the stream.                              */
/*                                                                          */
/*  For ECS->PC the following formats are used:                             */
/*                                                                          */
/*    For BYTE data, data is just sent in "nickle" pairs:                   */
/*    +--------+--------+--------+--------+--------+                        */
/*    |   D3a  |   D2a  |   D1a  |   D0a  | (clk)  |                        */
/*    +--------+--------+--------+--------+--------+                        */
/*    +--------+--------+--------+--------+--------+                        */
/*    |   D7a  |   D6a  |   D5a  |   D4a  | (clk)  |                        */
/*    +--------+--------+--------+--------+--------+                        */
/*                                                                          */
/*    For DECLE data, data is just sent in "nickle" triads:                 */
/*    +--------+--------+--------+--------+--------+                        */
/*    |   D3a  |   D2a  |   D1a  |   D0a  | (clk)  |                        */
/*    +--------+--------+--------+--------+--------+                        */
/*    +--------+--------+--------+--------+--------+                        */
/*    |   D7a  |   D6a  |   D5a  |   D4a  | (clk)  |                        */
/*    +--------+--------+--------+--------+--------+                        */
/*    +--------+--------+--------+--------+--------+                        */
/*    |   n/a  |   n/a  |   D9a  |   D8a  | (clk)  |                        */
/*    +--------+--------+--------+--------+--------+                        */
/*                                                                          */
/*    For WORD data, data is just sent in "nickle" quads:                   */
/*    +--------+--------+--------+--------+--------+                        */
/*    |   D3a  |   D2a  |   D1a  |   D0a  | (clk)  |                        */
/*    +--------+--------+--------+--------+--------+                        */
/*    +--------+--------+--------+--------+--------+                        */
/*    |   D7a  |   D6a  |   D5a  |   D4a  | (clk)  |                        */
/*    +--------+--------+--------+--------+--------+                        */
/*    +--------+--------+--------+--------+--------+                        */
/*    |   D11a |   D10a |   D9a  |   D8a  | (clk)  |                        */
/*    +--------+--------+--------+--------+--------+                        */
/*    +--------+--------+--------+--------+--------+                        */
/*    |   D15a |   D14a |   D13a |   D12a | (clk)  |                        */
/*    +--------+--------+--------+--------+--------+                        */
/*                                                                          */
/*  After each transfer (in either direction) the transfer is ACKd by the   */
/*  PC sending 0xAA and the ECS sending 0xA, in what I call "the Standard   */
/*  1010 Handshake."                                                        */
/*                                                                          */
/*  STANDARD 1010 HANDSHAKE                                                 */
/*                                                                          */
/*  No, this has nothing to do with those cheese long distance phone        */
/*  companies.  The 1010 handshake is simple.  The ECS transfer protocol    */
/*  actually transfers data both directions simultaneously.  This is an     */
/*  artifact of how the clock protocol works.  For most things in the       */
/*  ECScable Monitor protocol, we only use one direction at a time.         */
/*  However, the "handshake" that's used to terminate most of the commands  */
/*  below uses both directions simultaneously.                              */
/*                                                                          */
/*  Basically, this handshake is quite simple.  The PC and the ECS send     */
/*  the following bit patterns to each other:                               */
/*                                                                          */
/*      +---+---+---+---+---+---+---+-----+                                 */
/*      | 1 | 0 | 1 | 0 | 1 | 0 | 1 |(clk)|   PC -> ECS                     */
/*      +---+---+---+---+---+---+---+-----+                                 */
/*                                                                          */
/*                  +---+---+---+---+-----+                                 */
/*                  | 0 | 1 | 0 | 1 |(clk)|   ECS -> PC                     */
/*                  +---+---+---+---+-----+                                 */
/*                                                                          */
/*  For some of the transfers below (such as the command header), the       */
/*  PC -> ECS transfer may be a different value.  The standard ECS->PC      */
/*  "All is OK!" response is the one shown above though.                    */
/* ------------------------------------------------------------------------ */

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
)
{
    int i, a, l, a_hi;
    int cmd;
    int (*fxn)(ecscable_t*, uint16_t*, int);
    int (*snd_fxn[16])(ecscable_t*, uint16_t*, int) =
    {
        ec_send_bytes,  ec_send_bytes,  ec_send_bytes,  ec_send_bytes,
        ec_send_bytes,  ec_send_bytes,  ec_send_bytes,  ec_send_bytes,
        ec_send_decles, ec_send_decles, ec_send_words,  ec_send_words,
        ec_send_words,  ec_send_words,  ec_send_words,  ec_send_words,
    };
    int snd_cmd[16] =
    {
        0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
        0x12, 0x12, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14,
    };

    a_hi = addr + len;
    for (a = addr, i = 0; a < a_hi; a += l, i += l)
    {
        l = a + 32 >= a_hi ? a_hi - a : 32;

        if (width > 0 && width <= 16)
        {
            cmd = snd_cmd[width - 1];
            fxn = snd_fxn[width - 1];
        } else
        {
            int j, m = 0;
            for (j = 0; j < l; j++)
                m |= data[i + j];

            for (j = 0; j < 15; j++)
                if ((2 << j) > m)
                    break;

            cmd = snd_cmd[j];
            fxn = snd_fxn[j];
        }
        if (icart) cmd += 0x10;

        ec_idle(ec);
        if (ec_send_command(ec, cmd)  < 0)       { ec_idle(ec); return -1; }
        if (ec_send_cmdhdr(ec, a, l)  < 0)       { ec_idle(ec); return -1; }
        if (fxn           (ec, data + i, l) < 0) { ec_idle(ec); return -1; }
    }
    ec_idle(ec);

    return 0;
}

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
)
{
    int i, a, l, a_hi;
    int cmd = 0x1C;
    int (*ec_recv_width)(ecscable_t*, uint16_t*, int) = ec_recv_words;

    if (width <= 10) { cmd = 0x1A; ec_recv_width = ec_recv_decles; }
    if (width <=  8) { cmd = 0x18; ec_recv_width = ec_recv_bytes;  }
    if (icart)       { cmd += 0x10;                                }

    a_hi = addr + len;
    for (a = addr, i = 0; a < a_hi; a += l, i += l)
    {
        l = a + 32 >= a_hi ? a_hi - a : 32;

        ec_idle(ec);
        if (ec_send_command(ec, cmd)  < 0)       { ec_idle(ec); return -1; }
        if (ec_send_cmdhdr(ec, a, l)  < 0)       { ec_idle(ec); return -1; }
        if (ec_recv_width (ec, data + i, l) < 0) { ec_idle(ec); return -1; }
    }
    ec_idle(ec);

    return 0;
}

/* ------------------------------------------------------------------------ */
/*  EC_RESET_INTV    -- Reset the Intellivision via ECScable                */
/* ------------------------------------------------------------------------ */
int ec_reset_intv(ecscable_t *ec, int to_monitor)
{
    return ec_send_command(ec, to_monitor ? 0x04 : 0x02);
}

/* ------------------------------------------------------------------------ */
/*  EC_PING          -- Sends a simple ping over the ECScable.              */
/* ------------------------------------------------------------------------ */
int ec_ping(ecscable_t *ec)
{
    return ec_send_command(ec, 0);
}

/* ------------------------------------------------------------------------ */
/*  EC_ECS_ROMS      -- Enable/Disable ECS ROMs via ECScable.               */
/* ------------------------------------------------------------------------ */
int ec_ecs_roms(ecscable_t *ec, int enable)
{
    return ec_send_command(ec, enable ? 0x0C : 0x0A);
}

/* ------------------------------------------------------------------------ */
/*  EC_ECS_SETPAGE   -- Set the Page # on an ECS paged ROM.                 */
/*                                                                          */
/*  Command is followed by two words and the "standard 1010 handshake".     */
/*                                                                          */
/*  First word is the upper four bits of the address range being switched:  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  |    0   |    0   |    0   |  A15   |  A14   |  A13   |  A12   |(clk)|  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*                                                                          */
/*  Next word is the bank number being selected into the page:              */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  |    0   |    0   |    0   |   B3   |   B2   |   B1   |   B0   |(clk)|  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*                                                                          */
/* ------------------------------------------------------------------------ */
int ec_ecs_setpage(ecscable_t *ec, int addr, int page)
{
    if (ec_send_command(ec, 0xE) < 0)            { ec_idle(ec); return -1; }
    if (ec_xfer_data(ec, (addr>>11) & 0x1E) < 0) { ec_idle(ec); return -1; }
    if (ec_xfer_data(ec, (page<< 1) & 0x1E) < 0) { ec_idle(ec); return -1; }
    if (ec_xfer_data(ec, 0xAA) != 0x0A)          { ec_idle(ec); return -1; }

    return 0;
}


/* ------------------------------------------------------------------------ */
/*  EC_VIDEO         -- Enable/Disable active display via ECScable.         */
/* ------------------------------------------------------------------------ */
int ec_video(ecscable_t *ec, int enable)
{
    return ec_send_command(ec, enable ? 0x08 : 0x06);
}

/* ------------------------------------------------------------------------ */
/*  EC_XFER_DATA     -- Send a 7-bit quantity while receiving a 4-bit qty   */
/* ------------------------------------------------------------------------ */
int ec_xfer_data(ecscable_t *ec, int value)
{
    int rvalue;
    struct timeval x, y;
    unsigned long diff;

    /* -------------------------------------------------------------------- */
    /*  Output with clock==1.                                               */
    /* -------------------------------------------------------------------- */
    EC_SEND(ec, (value & 0xFE) | 0);
    EC_SEND(ec, (value & 0xFE) | 1);

    gettimeofday(&x, 0);
    while (((rvalue = EC_RECV(ec)) & 1) == 0)
    {
        gettimeofday(&y, 0);
        diff = (y.tv_sec  - x.tv_sec ) * 1000000 +
               (y.tv_usec - x.tv_usec);
        if (diff > EC_TIMEOUT)  /* wait 1/20th of a second */
        {
fprintf(stderr, "read clkhi timeout: rvalue = %.8X\n", rvalue);
            ec_idle(ec);
            return -1;
        }
    }

    /* -------------------------------------------------------------------- */
    /*  Output with clock==0.                                               */
    /* -------------------------------------------------------------------- */
    EC_SEND(ec, (value & 0xFE) | 0);
    while (((rvalue = EC_RECV(ec)) & 1) == 1)
    {
        gettimeofday(&y, 0);
        diff = (y.tv_sec  - x.tv_sec ) * 1000000 +
               (y.tv_usec - x.tv_usec);
        if (diff > EC_TIMEOUT)  /* wait 1/20th of a second */
        {
fprintf(stderr, "read clklo timeout: rvalue = %.8X\n", rvalue);
            ec_idle(ec);
            return -1;
        }
    }
//printf("recv=%.2X send=%.2X\n", rvalue & 0x1E, value & 0xFE);

    return rvalue & 0x1E;
}

/* ------------------------------------------------------------------------ */
/*  EC_IDLE          -- Go into the idle state.                             */
/* ------------------------------------------------------------------------ */
void ec_idle(ecscable_t *ec)
{
    EC_SEND(ec, 0xFF);
}

/* ------------------------------------------------------------------------ */
/*  EC_DEIDLE        -- Come out of the idle state.  Return non-zero if we  */
/*                      time out (eg. we de-idle but don't see ECS de-idle) */
/* ------------------------------------------------------------------------ */
int ec_deidle(ecscable_t *ec)
{
    struct timeval x, y;
    unsigned long diff;
    int r;

    EC_SEND(ec, 0x00);

    gettimeofday(&x, 0);
    while ((r = EC_RECV(ec)) != 0x00)
    {
        /*printf("\r%.2X", r); fflush(stdout);*/
        gettimeofday(&y, 0);
        diff = (y.tv_sec  - x.tv_sec ) * 1000000 +
               (y.tv_usec - x.tv_usec);
        if (diff > EC_TIMEOUT)  /* wait 1/20th of a second */
        {
            ec_idle(ec);
            return -1;
        }
    }
    return 0;
}

/* ------------------------------------------------------------------------ */
/*  EC_SEND_COMMAND  -- Send a 1-byte command to the ECS cable.             */
/*                                                                          */
/*  The command sequence is followed by the "standard 1010 handshake".      */
/* ------------------------------------------------------------------------ */
int ec_send_command(ecscable_t *ec, int command)
{
    command &= 0xFE;

    if (ec_deidle(ec))                            { ec_idle(ec); return -1; }
    if (ec_xfer_data(ec,        command) < 0)     { ec_idle(ec); return -1; }
    if (ec_xfer_data(ec, 0xFE ^ command) < 0)     { ec_idle(ec); return -1; }
    if (ec_xfer_data(ec,           0xAA) != 0x0A) { ec_idle(ec); return -1; }

    return 0;
}

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
/*  This transfer is followed by a modified "1010 handshake".  Instead of   */
/*  the usual PC->ECS pattern, this sequence sends all-zeros.               */
/* ------------------------------------------------------------------------ */
int ec_send_cmdhdr(ecscable_t *ec, int addr, int len)
{
    int b0, b1, b2, b3;

    b0 =  addr       & 0xFE;
    b1 = (addr >> 8) & 0xFE;
    b2 = ((addr & 1) << 2) | ((addr & 0x100) >> 7) | (((len-1) & 31) << 3);
    b3 = (b0 + b1 + b2) & 0xFE;

    if (ec_xfer_data(ec, b0) ==  -1) { ec_idle(ec); return -5; }
    if (ec_xfer_data(ec, b1) ==  -1) { ec_idle(ec); return -4; }
    if (ec_xfer_data(ec, b2) ==  -1) { ec_idle(ec); return -3; }
    if (ec_xfer_data(ec, b3) ==  -1) { ec_idle(ec); return -2; }
    if (ec_xfer_data(ec,  0) != 0xA) { ec_idle(ec); return -1; }
    return 0;
}

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
/*                                                                          */
/*  This transfer is followed by the "standard 1010 handshake".             */
/* ------------------------------------------------------------------------ */
int ec_send_bytes(ecscable_t *ec, uint16_t *data, int len)
{
    int i = 0, lsb = 0;

    while (len-- > 0)
    {
        lsb |= (*data & 1) << ++i;
        if (ec_xfer_data(ec, *data++ & 0xFE) < 0) { ec_idle(ec); return -1; }
        if (i == 7)
        {
            if (ec_xfer_data(ec, lsb & 0xFE) < 0) { ec_idle(ec); return -1; }
            lsb = i = 0;
        }
    }
    if (i && ec_xfer_data(ec, lsb & 0xFE) < 0) { ec_idle(ec); return -1; }
    if (     ec_xfer_data(ec, 0xAA)    != 0xA) { ec_idle(ec); return -1; }

    return 0;
}

/* ------------------------------------------------------------------------ */
/*  EC_SEND_DECLES   -- Send an array of 10-bit data.                       */
/*                                                                          */
/*  For DECLE data, up to 7 bytes in a row in this format:                  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  |   D9a  |   D8a  |   D7a  |   D6a  |   D5a  |   D4a  |   D3a  |(clk)|  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  |   D9b  |   D8b  |   D7b  |   D6b  |   D5b  |   D4b  |   D3b  |(clk)|  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*        ....                                                              */
/*                                                                          */
/*  Followed by one byte in this format to give the LSBs:                   */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  |   D2g  |   D2f  |   D2e  |   D2d  |   D2c  |   D2b  |   D2a  |(clk)|  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*                                                                          */
/*  Followed by two bytes in this format to give the MSBs:                  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  |   D1d  |   D0c  |   D1c  |   D0b  |   D1b  |   D0a  |   D1a  |(clk)|  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*  |   D0g  |   D1g  |   D0f  |   D1f  |   D0e  |   D1e  |   D0d  |(clk)|  */
/*  +--------+--------+--------+--------+--------+--------+--------+-----+  */
/*                                                                          */
/*  If there are more than 7 decles remaining in a packet, 7 are always     */
/*  sent, otherwise only the remaining decles are sent.  This eliminates    */
/*  the need for length bytes in the stream.                                */
/*                                                                          */
/*  This transfer is followed by the "standard 1010 handshake".             */
/* ------------------------------------------------------------------------ */
int ec_send_decles(ecscable_t *ec, uint16_t *data, int len)
{
    int i = 0, lsb0 = 0, lsb1 = 0;

    while (len-- > 0)
    {
        ++i;
        lsb0 |= ((*data & 4) >> 2) << i;
        lsb1 |= ((*data & 2) >> 1) << (2*i - 1);
        lsb1 |= ((*data & 1) >> 0) << (2*i);

        if (ec_xfer_data(ec, (*data++ >>2)&0xFE) < 0) {ec_idle(ec); return -1;}
        if (i == 7)
        {
            if (ec_xfer_data(ec, lsb0     &0xFE) < 0) {ec_idle(ec); return -1;}
            if (ec_xfer_data(ec,(lsb1    )&0xFE) < 0) {ec_idle(ec); return -1;}
            if (ec_xfer_data(ec,(lsb1>> 7)&0xFE) < 0) {ec_idle(ec); return -1;}
            lsb0 = lsb1 = i = 0;
        }
    }
    if (i && ec_xfer_data(ec,  lsb0      & 0xFE) < 0) {ec_idle(ec); return -1;}
    if (i && ec_xfer_data(ec, (lsb1    ) & 0xFE) < 0) {ec_idle(ec); return -1;}
    if (i && ec_xfer_data(ec, (lsb1>> 7) & 0xFE) < 0) {ec_idle(ec); return -1;}
    if (     ec_xfer_data(ec, 0xAA)           != 0xA) {ec_idle(ec); return -1;}

    return 0;
}

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
/*                                                                          */
/*  This transfer is followed by the "standard 1010 handshake".             */
/* ------------------------------------------------------------------------ */
int ec_send_words(ecscable_t *ec, uint16_t *data, int len)
{
    int i = 0, lsb = 0;

    while (len-- > 0)
    {
        ++i;
        lsb |= ((*data & 0x001)>> 0) << (2*i    );
        lsb |= ((*data & 0x100)>> 8) << (2*i - 1);

        if (ec_xfer_data(ec,(*data       )&0xFE) < 0) {ec_idle(ec); return -1;}
        if (ec_xfer_data(ec,(*data++ >> 8)&0xFE) < 0) {ec_idle(ec); return -1;}
        if (i == 7)
        {
            if (ec_xfer_data(ec,(lsb     )&0xFE) < 0) {ec_idle(ec); return -1;}
            if (ec_xfer_data(ec,(lsb >> 7)&0xFE) < 0) {ec_idle(ec); return -1;}
            lsb = i = 0;
        }
    }
    if (i && ec_xfer_data(ec, (lsb     ) & 0xFE) < 0) {ec_idle(ec); return -1;}
    if (i && ec_xfer_data(ec, (lsb >> 7) & 0xFE) < 0) {ec_idle(ec); return -1;}
    if (     ec_xfer_data(ec, 0xAA)           != 0xA) {ec_idle(ec); return -1;}

    return 0;
}

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
/*                                                                          */
/*  This transfer is followed by the "standard 1010 handshake".             */
/* ------------------------------------------------------------------------ */
int ec_recv_bytes(ecscable_t *ec, uint16_t *data, int len)
{
    int b0, b1;

    while (len-- > 0)
    {
        if ((b0 = ec_xfer_data(ec, 0xFC)) < 0) { ec_idle(ec); return -1; }
        if ((b1 = ec_xfer_data(ec, 0xFA)) < 0) { ec_idle(ec); return -1; }
        *data++ = (0x0F & (b0 >> 1)) | (0xF0 & (b1 << 3));
    }
    if (ec_xfer_data(ec, 0xAA) != 0xA) { ec_idle(ec); return -1; }
    return 0;
}

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
/*                                                                          */
/*  This transfer is followed by the "standard 1010 handshake".             */
/* ------------------------------------------------------------------------ */
int ec_recv_decles(ecscable_t *ec, uint16_t *data, int len)
{
    int b0, b1, b2;

    while (len-- > 0)
    {
        if ((b0 = ec_xfer_data(ec, 0xF6)) < 0) { ec_idle(ec); return -1; }
        if ((b1 = ec_xfer_data(ec, 0xEE)) < 0) { ec_idle(ec); return -1; }
        if ((b2 = ec_xfer_data(ec, 0xDE)) < 0) { ec_idle(ec); return -1; }
        *data++ = (0x0F & (b0 >> 1)) | (0xF0 & (b1 << 3)) | (0x300 & (b2 << 7));
    }
    if (ec_xfer_data(ec, 0xAA) != 0xA) { ec_idle(ec); return -1; }
    return 0;
}

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
/*                                                                          */
/*  This transfer is followed by the "standard 1010 handshake".             */
/* ------------------------------------------------------------------------ */
int ec_recv_words(ecscable_t *ec, uint16_t *data, int len)
{
    int b0, b1, b2, b3;

    while (len-- > 0)
    {
        if ((b0 = ec_xfer_data(ec, 0xBE)) < 0) { ec_idle(ec); return -1; }
        if ((b1 = ec_xfer_data(ec, 0x7E)) < 0) { ec_idle(ec); return -1; }
        if ((b2 = ec_xfer_data(ec, 0xFC)) < 0) { ec_idle(ec); return -1; }
        if ((b3 = ec_xfer_data(ec, 0xFA)) < 0) { ec_idle(ec); return -1; }
        *data++ = (0x000F & (b0 >> 1)) | (0x00F0 & (b1 <<  3)) |
                  (0x0F00 & (b2 << 7)) | (0xF000 & (b3 << 11));
    }
    if (ec_xfer_data(ec, 0xAA) != 0xA) { ec_idle(ec); return -1; }
    return 0;
}

/* ------------------------------------------------------------------------ */
/*  EC_DETECT        -- Detect the ECS cable on a given port.               */
/*                      Set the port to 0 to do an autodetect.              */
/* ------------------------------------------------------------------------ */
unsigned ec_detect(unsigned port)
{
    int i = 0, j;
    ecscable_t try;

    memset(&try, 0, sizeof(try));

    if (port) i = 4;
    do
    {
        if (!port) port = ports[i];
        printf("> Scanning for an ECS cable at port $%.4X\n", port);

        try.port = port;

        EC_CTRL(&try, 0x00);
        ec_idle(&try);
        for (j = 0; j < 4; j++)
        {
            if (ec_deidle(&try))                  break;
            if (ec_xfer_data(&try, 0x00) < 0)     break;
            if (ec_xfer_data(&try, 0xFF) < 0)     break;
            if (ec_xfer_data(&try, 0xAA) != 0x0A) break;
            ec_idle(&try);
        }
        ec_idle(&try);

        if (j > 0 && j != 4)
        {
            printf("> Partial detect on port $%.4X. "
                   "%d of 4 pings returned\n", port, j);
        }

        if (j == 4) { break; } else port = 0;
    } while (++i < 3);

    if (port)   printf("> Found active ECS cable at port $%.4X\n", port);
    else        printf("> No active ECS cable found!\n");

    return port;
}


/* ------------------------------------------------------------------------ */
/*  EC_SLEEP         -- Wrapper around nanosleep().                         */
/* ------------------------------------------------------------------------ */
inline void ec_sleep(long len)
{
#ifndef NO_NANOSLEEP
    struct timespec delay, remain;

    delay.tv_sec  = 0;
    delay.tv_nsec = len * 1000000;

    while (nanosleep(&delay, &remain) == -1 && errno == EINTR)
    {
        delay = remain;
        if (remain.tv_nsec < 10000) break;
    }
#else
    struct timeval x, y;
    long diff = 0;

    gettimeofday(&x, 0);
    while (diff < len)
    {
        gettimeofday(&y, 0);
        diff = (y.tv_sec  - x.tv_sec ) * 1000000 +
               (y.tv_usec - x.tv_usec);
    }
#endif
    return;
}

/* ======================================================================== */
/*  EC_INIT_PORTS    -- Get access to the desired I/O ports and drop        */
/*                      root.  Don't bother returning an error if we fail.  */
/*                      Just abort.                                         */
/* ======================================================================== */
void ec_init_ports(unsigned long base)
{
    int i;

    /* -------------------------------------------------------------------- */
    /*  First, sanity check the port number.  If it is not one of the       */
    /*  standard printer port #'s, then abort.                              */
    /* -------------------------------------------------------------------- */
    if (base && base != 0x378 && base != 0x278 && base != 0x3BC)
    {
        fprintf(stderr, "ec_init_ports:  Invalid base address 0x%.4lX\n", base);
        exit(1);
    }

#ifndef NO_SETUID
    if (base)
    {
        /* ---------------------------------------------------------------- */
        /*  Grant ourself perms to access ports 'base' through 'base+2'.    */
        /* ---------------------------------------------------------------- */
        if (ioperm(base, 3, 1))
        {
            fprintf(stderr, "ec_init_ports:  Unable to set I/O permissions\n");
            perror("ec_init_ports: ioperm()");
            exit(1);
        }
        printf("> Granted access to $%.4lX (%ld)\n", base, base);
    } else
    {
        /* ---------------------------------------------------------------- */
        /*  Grant ourself perms to access ports 'base' through 'base+2'.    */
        /* ---------------------------------------------------------------- */
        for (i = 0; i < 3; i++)
        {
            if (ioperm(ports[i], 3, 1))
            {
                fprintf(stderr, "ec_init_ports: "
                                " Unable to set I/O permissions\n");
                perror("ec_init_ports: ioperm()");
                exit(1);
            }
            printf("> Granted access to $%.4lX (%ld)\n", ports[i], ports[i]);
        }
    }


    /* -------------------------------------------------------------------- */
    /*  Drop elevated privs if we have them.                                */
    /* -------------------------------------------------------------------- */
    if (getegid() != getgid()) setgid(getgid());
    if (geteuid() != getuid()) setuid(getuid());
#endif

    return;
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
/*                 Copyright (c) 2001-+Inf, Joseph Zbiciak                  */
/* ======================================================================== */
