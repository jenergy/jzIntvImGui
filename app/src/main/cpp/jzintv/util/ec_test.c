#include <config.h>
#include "util/ecscable.h"

uint16_t data[65536];
uint16_t exec[4096];
void dump_data(int ofs, int len)
{
    FILE *f;
    int i;


    if (!(f = fopen("dump", "wb"))) return;
    for (i = 0; i < 65536; i++)
        data[i] = (data[i] >> 8) | (data[i] << 8);
    fwrite(data + ofs, 2, len, f);
    for (i = 0; i < 65536; i++)
        data[i] = (data[i] >> 8) | (data[i] << 8);
    fclose(f);

}


/*
 * ============================================================================
 *  ELAPSED      -- Returns amount of time that's elapsed since the program
 *                  started, in seconds.
 * ============================================================================
 */
double elapsed(int restart)
{
    static struct timeval start;
    static int init = 0;
    struct timeval now;
    uint32_t usec, sec;

    if (!init || restart)
    {
        gettimeofday(&start, NULL);
        init = 1;
    }

    gettimeofday(&now, NULL);

    if (now.tv_usec < start.tv_usec)
    {
        now.tv_usec += 1000000;
        now.tv_sec--;
    }

    usec = now.tv_usec - start.tv_usec;
    sec  = now.tv_sec  - start.tv_sec;

    return (sec + usec/1000000.);
}


int main()
{
    int i;
    ecscable_t ec;

    /* First, get set up. */
    ec_init_ports(0);
    if (!(ec.port = ec_detect(0)))
    {
        fprintf(stderr, "No ECS cable\n");
        exit(1);
    }
    ec_idle(&ec);


    printf("%-60s\n", "Reset to monitor..."); fflush(stdout);
    ec_reset_intv(&ec, 1);
    ec_video(&ec, 0); /* disable active video */

    /* Start off by reading the EXEC from INTY. */
    printf("%-60s", "Downloading the EXEC ROM as words... "); fflush(stdout);
    memset(exec, 0, 8192);
    elapsed(1);
    if (ec_download(&ec, 0x1000, 0x1000, exec, 16, 0))
    {
        fprintf(stderr, "Error during download 0\n");
        exit(1);
    }
    printf("%7.1f kbps\n", 16*4096. / elapsed(0));

    printf("%-60s", "Re-downloading the EXEC ROM as words... "); fflush(stdout);
    memset(data, 0, 8192);
    elapsed(1);
    if (ec_download(&ec, 0x1000, 0x1000, data, 16, 0))
    {
        fprintf(stderr, "Error during download 1\n");
        exit(1);
    }
    printf("%7.1f kbps\n", 16*4096. / elapsed(0));

    ec_video(&ec, 1); /* enable active video */
    if (memcmp(exec, data, 8192))
    {
        dump_data(0, 0x1000);
        fprintf(stderr, "EXEC miscompares (test 1)\n");
        exit(1);
    }

    printf("%-60s", "Downloading the EXEC ROM as decles... "); fflush(stdout);
    memset(data, 0, 8192);
    elapsed(1);
    if (ec_download(&ec, 0x1000, 0x1000, data, 10, 0))
    {
        fprintf(stderr, "Error during download 2\n");
        exit(1);
    }
    printf("%7.1f kbps\n", 10*4096. / elapsed(0));

    if (memcmp(exec, data, 8192))
    {
        dump_data(0, 0x1000);
        fprintf(stderr, "EXEC miscompares (test 2)\n");
        exit(1);
    }

    printf("%-60s", "Downloading the EXEC ROM as bytes... "); fflush(stdout);
    memset(data, 0, 8192);
    elapsed(1);
    if (ec_download(&ec, 0x1000, 0x1000, data, 8, 0))
    {
        fprintf(stderr, "Error during download 3\n");
        exit(1);
    }
    printf("%7.1f kbps\n", 8*4096. / elapsed(0));

    for (i = 0; i < 4096; i++)
    {
        if ((exec[i] & 0xFF) != data[i])
        {
            dump_data(0, 0x1000);
            fprintf(stderr, "EXEC miscompares (test 3)\n");
            exit(1);
        }
    }

    /* Next, upload "random" garbage to the Intellicart at 0x1000 */
    srand48(time(0));
    for (i = 0; i < 0x1000; i++)
        data[i] = lrand48();


    printf("%-60s", "Uploading random data as words to Intellicart... ");
    elapsed(1);
    if (ec_upload(&ec, 0x1000, 0x1000, data, 16, 1))
    {
        fprintf(stderr, "Error during upload 4a\n");
        exit(1);
    }
    printf("%7.1f kbps\n", 16*4096. / elapsed(0));
    printf("%-60s", "Downloading random data as words from Intellicart... ");
    elapsed(1);
    if (ec_download(&ec, 0x1000, 0x1000, data + 0x1000, 16, 1))
    {
        fprintf(stderr, "Error during download 5a\n");
        exit(1);
    }
    printf("%7.1f kbps\n", 16*4096. / elapsed(0));
    if (memcmp(data, data + 0x1000, 0x1000 * 2))
    {
        dump_data(0x0, 0x2000);
        fprintf(stderr, "DATA miscompares (test 5a)\n");
        exit(1);
    }

    printf("%-60s", "Downloading random data as decles from Intellicart... ");
    elapsed(1);
    if (ec_download(&ec, 0x1000, 0x1000, data + 0x1000, 10, 1))
    {
        fprintf(stderr, "Error during download 6a\n");
        exit(1);
    }
    printf("%7.1f kbps\n", 10*4096. / elapsed(0));
    for (i = 0; i < 0x2000; i++)
        data[i] &= 0x3FF;
    if (memcmp(data, data + 0x1000, 0x1000 * 2))
    {
        dump_data(0x0, 0x2000);
        fprintf(stderr, "DATA miscompares (test 6a)\n");
        exit(1);
    }

    printf("%-60s", "Downloading random data as bytes from Intellicart... ");
    elapsed(1);
    if (ec_download(&ec, 0x1000, 0x1000, data + 0x1000, 8, 1))
    {
        fprintf(stderr, "Error during download 7a\n");
        exit(1);
    }
    printf("%7.1f kbps\n", 8*4096. / elapsed(0));
    for (i = 0; i < 0x2000; i++)
        data[i] &= 0xFF;
    if (memcmp(data, data + 0x1000, 0x1000 * 2))
    {
        dump_data(0x0, 0x2000);
        fprintf(stderr, "DATA miscompares (test 7a)\n");
        exit(1);
    }

    /* Next, upload "random" garbage to the Intellicart at 0x1000 */
    srand48(time(0));
    for (i = 0; i < 0x1000; i++)
        data[i] = lrand48() & 0x3FF;


    printf("%-60s", "Uploading random data as decles to Intellicart... ");
    elapsed(1);
    if (ec_upload(&ec, 0x1000, 0x1000, data, 10, 1))
    {
        fprintf(stderr, "Error during upload 4b\n");
        exit(1);
    }
    printf("%7.1f kbps\n", 10*4096. / elapsed(0));
    printf("%-60s", "Downloading random data as words from Intellicart... ");
    elapsed(1);
    if (ec_download(&ec, 0x1000, 0x1000, data + 0x1000, 16, 1))
    {
        fprintf(stderr, "Error during download 5b\n");
        exit(1);
    }
    printf("%7.1f kbps\n", 16*4096. / elapsed(0));
    if (memcmp(data, data + 0x1000, 0x1000 * 2))
    {
        dump_data(0x0, 0x2000);
        fprintf(stderr, "DATA miscompares (test 5b)\n");
        exit(1);
    }

    printf("%-60s", "Downloading random data as decles from Intellicart... ");
    elapsed(1);
    if (ec_download(&ec, 0x1000, 0x1000, data + 0x1000, 10, 1))
    {
        fprintf(stderr, "Error during download 6b\n");
        exit(1);
    }
    printf("%7.1f kbps\n", 10*4096. / elapsed(0));
    if (memcmp(data, data + 0x1000, 0x1000 * 2))
    {
        dump_data(0x0, 0x2000);
        fprintf(stderr, "DATA miscompares (test 6b)\n");
        exit(1);
    }

    printf("%-60s", "Downloading random data as bytes from Intellicart... ");
    elapsed(1);
    if (ec_download(&ec, 0x1000, 0x1000, data + 0x1000, 8, 1))
    {
        fprintf(stderr, "Error during download 7b\n");
        exit(1);
    }
    printf("%7.1f kbps\n", 8*4096. / elapsed(0));
    for (i = 0; i < 0x2000; i++)
        data[i] &= 0xFF;
    if (memcmp(data, data + 0x1000, 0x1000 * 2))
    {
        dump_data(0x0, 0x2000);
        fprintf(stderr, "DATA miscompares (test 7b)\n");
        exit(1);
    }

    /* Next, upload "random" garbage to the Intellicart at 0x1000 */
    srand48(time(0));
    for (i = 0; i < 0x1000; i++)
        data[i] = lrand48() & 0xFF;


    printf("%-60s", "Uploading random data as bytes to Intellicart... ");
    elapsed(1);
    if (ec_upload(&ec, 0x1000, 0x1000, data, 8, 1))
    {
        fprintf(stderr, "Error during upload 4c\n");
        exit(1);
    }
    printf("%7.1f kbps\n", 8*4096. / elapsed(0));
    printf("%-60s", "Downloading random data as words from Intellicart... ");
    elapsed(1);
    if (ec_download(&ec, 0x1000, 0x1000, data + 0x1000, 16, 1))
    {
        fprintf(stderr, "Error during download 5c\n");
        exit(1);
    }
    printf("%7.1f kbps\n", 16*4096. / elapsed(0));
    if (memcmp(data, data + 0x1000, 0x1000 * 2))
    {
        dump_data(0x0, 0x2000);
        fprintf(stderr, "DATA miscompares (test 5c)\n");
        exit(1);
    }

    printf("%-60s", "Downloading random data as decles from Intellicart... ");
    elapsed(1);
    if (ec_download(&ec, 0x1000, 0x1000, data + 0x1000, 10, 1))
    {
        fprintf(stderr, "Error during download 6c\n");
        exit(1);
    }
    printf("%7.1f kbps\n", 10*4096. / elapsed(0));
    if (memcmp(data, data + 0x1000, 0x1000 * 2))
    {
        dump_data(0x0, 0x2000);
        fprintf(stderr, "DATA miscompares (test 6c)\n");
        exit(1);
    }

    printf("%-60s", "Downloading random data as bytes from Intellicart... ");
    elapsed(1);
    if (ec_download(&ec, 0x1000, 0x1000, data + 0x1000, 8, 1))
    {
        fprintf(stderr, "Error during download 7c\n");
        exit(1);
    }
    printf("%7.1f kbps\n", 8*4096. / elapsed(0));
    if (memcmp(data, data + 0x1000, 0x1000 * 2))
    {
        dump_data(0x0, 0x2000);
        fprintf(stderr, "DATA miscompares (test 7c)\n");
        exit(1);
    }


    /* Next, upload "random" garbage to the Intellivision at 0x0340 */
    srand48(time(0));
    for (i = 0; i < 0x20; i++)
        data[i] = lrand48();


    printf("%-60s", "Uploading random data as words to Intellivision... ");
    elapsed(1);
    if (ec_upload(&ec, 0x0340, 0x0020, data, 16, 1))
    {
        fprintf(stderr, "Error during upload 8\n");
        exit(1);
    }
    printf("%7.1f kbps\n", 16*32. / elapsed(0));
    printf("%-60s", "Downloading random data as words from Intellivision... ");
    elapsed(1);
    if (ec_download(&ec, 0x0340, 0x0020, data + 0x0020, 16, 1))
    {
        fprintf(stderr, "Error during download 9\n");
        exit(1);
    }
    printf("%7.1f kbps\n", 16*32. / elapsed(0));
    if (memcmp(data, data + 0x0020, 0x0020 * 2))
    {
        dump_data(0x0, 0x2000);
        fprintf(stderr, "DATA miscompares (test 9)\n");
        exit(1);
    }

    printf("%-60s", "Downloading random data as decles from Intellivision... ");
    elapsed(1);
    if (ec_download(&ec, 0x0340, 0x0020, data + 0x0020, 10, 1))
    {
        fprintf(stderr, "Error during download 10\n");
        exit(1);
    }
    printf("%7.1f kbps\n", 10*32. / elapsed(0));
    for (i = 0; i < 0x0040; i++)
        data[i] &= 0x3FF;
    if (memcmp(data, data + 0x0020, 0x0020 * 2))
    {
        dump_data(0x0, 0x2000);
        fprintf(stderr, "DATA miscompares (test 10)\n");
        exit(1);
    }

    printf("%-60s", "Downloading random data as bytes from Intellivision... ");
    elapsed(1);
    if (ec_download(&ec, 0x0340, 0x0020, data + 0x0020, 10, 1))
    {
        fprintf(stderr, "Error during download 11\n");
        exit(1);
    }
    printf("%7.1f kbps\n", 8*32. / elapsed(0));
    for (i = 0; i < 0x0040; i++)
        data[i] &= 0xFF;
    if (memcmp(data, data + 0x0020, 0x0020 * 2))
    {
        dump_data(0x0, 0x2000);
        fprintf(stderr, "DATA miscompares (test 11)\n");
        exit(1);
    }


    return 0;
}
