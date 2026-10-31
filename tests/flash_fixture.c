/* Independent flash endpoint traces and expected demo bytes.
 * SPDX-License-Identifier: MIT */
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static unsigned byte(unsigned address)
{
    return (address ^ (address >> 8)) & 255;
}

int main(int argc, char **argv)
{
    const char *mode;
    unsigned address, i, pass;
    if (argc != 2) return 2;
    mode = argv[1];
    if (!strcmp(mode, "hold")) {
        puts("xfer 9f 3 0 ef4010");
        if (fflush(stdout)) return 1;
        for (;;) pause(); /* Keep a FIFO open until the test releases us. */
    }
    if (!strcmp(mode, "expected")) {
        for (address = 0; address < 65536; ++address)
            if (putchar((int)byte(address)) == EOF) return 1;
        return fclose(stdout) != 0;
    }
    if (!strcmp(mode, "invalid-id")) puts("xfer 9f 3 0 000000");
    else if (!strcmp(mode, "unsupported-capacity")) puts("xfer 9f 3 0 ef4019");
    else if (!strcmp(mode, "short-id")) puts("xfer 9f 3 0 ef40");
    else if (!strcmp(mode, "good") || !strcmp(mode, "truncated") ||
             !strcmp(mode, "short") || !strcmp(mode, "mismatch") ||
             !strcmp(mode, "transport") || !strcmp(mode, "stop") ||
             !strcmp(mode, "extra")) {
        puts("xfer 9f 3 0 ef4010");
        for (address = 0; address < 65536; address += 256) {
            if (address == 256) {
                if (!strcmp(mode, "truncated")) break;
                if (!strcmp(mode, "stop")) { puts("stop"); break; }
            }
            for (pass = 0; pass < 2; ++pass) {
                if (address == 256 && pass && !strcmp(mode, "transport")) {
                    puts("xfer 03000100 256 -5 -");
                    return fclose(stdout) != 0;
                }
                printf("xfer 03%06x 256 0 ", address);
                for (i = 0; i < 256; ++i) {
                    unsigned value = byte(address + i);
                    if (address == 256 && pass && i == 255) {
                        if (!strcmp(mode, "short")) break;
                        if (!strcmp(mode, "mismatch")) value ^= 1;
                    }
                    printf("%02x", value);
                }
                putchar('\n');
                if (address == 256 && pass &&
                    (!strcmp(mode, "short") || !strcmp(mode, "mismatch")))
                    return fclose(stdout) != 0;
            }
        }
        if (!strcmp(mode, "extra")) puts("xfer 9f 3 0 ef4010");
    } else return 2;
    return fclose(stdout) != 0;
}
