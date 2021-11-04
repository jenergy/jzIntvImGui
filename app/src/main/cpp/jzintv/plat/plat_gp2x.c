#include "config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

/* Most of the code in this file is based on code from wiki.gp2x.org */

static volatile uint32_t *memregs32;
static volatile uint16_t *memregs16;
static int memfd;

int gp2x_speed(int speed)
{
    int m = 0;

    // Calculating the FPLL value from frequency is a bit complex
    // so we just use some known-to-work values instead.
    switch(speed)
    {
        case 250: m = 0x5D04; break;
        case 225: m = 0x5304; break;
        case 200: m = 0x4904; break;
        case 175: m = 0x3F04; break;
        case 150: m = 0x4901; break;
        case 125: m = 0x3c01; break;
        case 100: m = 0x6502; break;
        case 75:  m = 0x4902; break;
        case 50:  m = 0x6503; break;
        default:  m = 0;      break;
    }

    if(m)
    {
        // Get interupt flags
        unsigned int l = memregs32[0x808>>2];

        // Turn off interrupts
        memregs32[0x808>>2] = 0xFF8FFFE7;

        // Set new clock frequency
        memregs16[0x910>>1]=m;

        // Wait for it to take
        while(memregs16[0x0902>>1] & 1);

        // Turn on interrupts again
        memregs32[0x808>>2] = l;
    }

    return m == 0 ? -1 : 0;
}

static uint16_t saved_clock = 0;

void gp2x_save_clock(void)
{
    saved_clock = memregs16[0x910 >> 1];
}

void gp2x_restore_clock(void)
{
    if (saved_clock)
    {
        // Get interupt flags
        unsigned int l = memregs32[0x808>>2];

        // Turn off interrupts
        memregs32[0x808>>2] = 0xFF8FFFE7;

        // Set new clock frequency
        memregs16[0x910>>1] = saved_clock;

        // Wait for it to take
        while(memregs16[0x0902>>1] & 1);

        // Turn on interrupts again
        memregs32[0x808>>2] = l;
    }
}

void *trymmap (void *start, size_t length, int prot, int flags, int fd, off_t offset)
{
    char *p;
    int aa;

    jzp_printf ("mmap(%X, %X, %X, %X, %X, %X) ... ", (unsigned int)start, length, prot, flags, fd, (unsigned int)offset);
    p = mmap (start, length, prot, flags, fd, offset);
    if (p == (char *)0xFFFFFFFF)
    {
        aa = errno;
        jzp_printf ("failed. errno = %d\n", aa);
    }
    else
    {
        jzp_printf ("OK! (%X)\n", (unsigned int)p);
    }

    return p;
}

unsigned char gp2x_init (void)
{
    memfd = open("/dev/mem", O_RDWR);
    if (memfd == -1)
    {
        jzp_printf ("Open failed\n");
        return 0;
    }

    jzp_printf ("/dev/mem opened successfully - fd = %d\n", memfd);

    memregs32 = trymmap(0, 0x10000, PROT_READ|PROT_WRITE, MAP_SHARED, memfd, 0xc0000000);
    if (memregs32 == (unsigned long *)0xFFFFFFFF) return 0;

    memregs16 = (unsigned short *)memregs32;

    gp2x_save_clock();

    atexit(gp2x_restore_clock);
    return 1;
}
