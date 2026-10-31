/* Drift POSIX host. SPDX-License-Identifier: MIT */
#define _POSIX_C_SOURCE 200809L
#include "drift.h"
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#endif

#define IMAGE_MAX (32u + 8u*DRIFT_CODE_MAX + DRIFT_MEMORY_MAX)
#define STEP_MAX UINT64_C(10000000)
#ifndef DRIFT_MODULE_DIR
#define DRIFT_MODULE_DIR "/usr/local/share/drift"
#endif

static volatile sig_atomic_t stopping;

static void stop_requested(int signal_number)
{
    (void)signal_number;
    stopping=1;
}

typedef struct host {
    FILE *trace;
    const char *name;
    unsigned line;
    int fd;
    uint32_t speed;
    FILE *dump;
    uint32_t total, written;
    int flash, demo;
} host;

static int fail(const char *message)
{
    fprintf(stderr, "drift: %s\n", message);
    return -1;
}

static int number(const char *s, uint32_t *out)
{
    char *end;
    unsigned long value;
    if (!s || !*s || *s == '-') return -1;
    errno = 0;
    value = strtoul(s, &end, 0);
    if (errno || *end || value > UINT32_MAX) return -1;
    *out = (uint32_t)value;
    return 0;
}

static int signed_number(const char *s, int32_t *out)
{
    char *end;
    long value;
    if (!s || !*s) return -1;
    errno = 0;
    value = strtol(s, &end, 0);
    if (errno || *end || value < INT32_MIN || value > 0) return -1;
    *out = (int32_t)value;
    return 0;
}

static int digit(int c)
{
    if (c >= '0' && c <= '9') return c-'0';
    if (c >= 'a' && c <= 'f') return c-'a'+10;
    if (c >= 'A' && c <= 'F') return c-'A'+10;
    return -1;
}

static int hex(const char *s, uint8_t *out, uint32_t *length)
{
    size_t n, i;
    if (!s) return -1;
    if (!strcmp(s,"-")) { *length = 0; return 0; }
    n = strlen(s);
    if (!n || (n & 1) || n/2 > DRIFT_MEMORY_MAX) return -1;
    for (i=0; i<n/2; ++i) {
        int a=digit((unsigned char)s[2*i]), b=digit((unsigned char)s[2*i+1]);
        if (a < 0 || b < 0) return -1;
        out[i] = (uint8_t)((a<<4)|b);
    }
    *length = (uint32_t)(n/2);
    return 0;
}

/* Return one non-comment line; no implicit EOF means a successful transfer. */
static int trace_line(host *h, char *line, size_t capacity)
{
    while (fgets(line, (int)capacity, h->trace)) {
        char *p=line, *comment;
        ++h->line;
        if (!strchr(line,'\n') && !feof(h->trace)) return fail("trace line too long");
        comment = strchr(line,'#');
        if (comment) *comment = 0;
        while (*p==' ' || *p=='\t' || *p=='\r' || *p=='\n') ++p;
        if (*p) { memmove(line,p,strlen(p)+1); return 1; }
    }
    return ferror(h->trace) ? fail("cannot read trace") : 0;
}

/* Replay validates the actual protocol request; it never supplies magic data
 * based on a module name. stop is explicit and only legal at the next XFER. */
static int replay(host *h, drift_vm *vm, uint8_t *rx,
                  uint32_t *length, int32_t *status)
{
    char line[17000], *word[6], *save=NULL;
    uint8_t tx[DRIFT_MEMORY_MAX];
    uint32_t txlen, capacity;
    int got=trace_line(h,line,sizeof line), i;
    if (got <= 0) return fail("trace ended before a requested transfer");
    word[0] = strtok_r(line," \t\r\n",&save);
    for (i=1; i<6; ++i) word[i] = strtok_r(NULL," \t\r\n",&save);
    if (word[0] && !strcmp(word[0],"stop") && !word[1]) return 1;
    if (!word[0] || strcmp(word[0],"xfer") || !word[4] || word[5] ||
        hex(word[1],tx,&txlen) || number(word[2],&capacity) ||
        signed_number(word[3],status) || hex(word[4],rx,length) ||
        (*status < 0 && *length != 0)) return fail("invalid trace row");
    if (txlen != vm->effect.tx_len || capacity != vm->effect.rx_len ||
        memcmp(tx,vm->memory+vm->effect.tx,txlen) || *length > capacity) {
        fprintf(stderr,"drift: %s:%u: transfer differs from trace\n",h->name,h->line);
        return -1;
    }
    return 0;
}

static int stream(host *h, drift_vm *vm, uint8_t *rx,
                  uint32_t *length, int32_t *status)
{
    struct pollfd p={h->fd,POLLIN,0};
    ssize_t n;
    int ready;
    if (vm->effect.tx_len || !vm->effect.rx_len) return fail("stream host accepts reads only");
    do ready=poll(&p,1,5000); while (ready < 0 && errno==EINTR);
    if (ready <= 0) { *status=ready ? -errno : -ETIMEDOUT; *length=0; return 0; }
    do n=read(h->fd,rx,vm->effect.rx_len); while (n < 0 && errno==EINTR && !stopping);
    *status = n < 0 ? -errno : n == 0 ? -ENODEV : 0;
    *length = n > 0 ? (uint32_t)n : 0;
    return 0;
}

static int spi(host *h, drift_vm *vm, uint8_t *rx,
               uint32_t *length, int32_t *status)
{
#ifdef __linux__
    struct spi_ioc_transfer message[2]={{0},{0}};
    uint32_t mode=0;
    int n;
    /* The endpoint permits only identification and bounded read commands.
     * No write-enable, erase, status-register or mode-changing operations. */
    const uint8_t *tx=vm->memory+vm->effect.tx;
    if (!(vm->effect.tx_len == 1 && vm->effect.rx_len == 3 && tx[0] == 0x9f) &&
        !(h->flash && h->total && vm->effect.tx_len == 4 &&
          vm->effect.rx_len == 256 && tx[0] == 0x03 &&
          ((uint32_t)tx[1]<<16 | (uint32_t)tx[2]<<8 | tx[3]) <= h->total-256))
        return fail("SPI endpoint permits identification and bounded reads only");
    /* Inspect, never silently reconfigure. JEDEC read-ID accepts SPI modes 0
     * and 3, MSB first, active-low select. Reject all additional mode flags.
     * The caller must retain exclusive ownership of the endpoint/configuration. */
    do n=ioctl(h->fd,SPI_IOC_RD_MODE32,&mode); while (n < 0 && errno==EINTR && !stopping);
    if (n < 0) { *status=-errno; *length=0; return 0; }
    if (mode != 0 && mode != (SPI_CPOL|SPI_CPHA))
        return fail("JEDEC host requires SPI mode 0 or 3, MSB first, active-low select");
    message[0].tx_buf=(uintptr_t)(vm->memory+vm->effect.tx);
    message[0].len=vm->effect.tx_len;
    message[1].rx_buf=(uintptr_t)rx;
    message[1].len=vm->effect.rx_len;
    message[0].speed_hz=message[1].speed_hz=h->speed;
    message[0].bits_per_word=message[1].bits_per_word=8;
    /* One message keeps chip select asserted between the write and read. */
    do n=ioctl(h->fd,SPI_IOC_MESSAGE(2),message); while (n < 0 && errno==EINTR && !stopping);
    *status=n < 0 ? -errno : (uint32_t)n != vm->effect.tx_len+vm->effect.rx_len ? -EIO : 0;
    *length=*status ? 0 : vm->effect.rx_len;
    return 0;
#else
    (void)h; (void)vm; (void)rx; (void)length; (void)status;
    return fail("SPI host requires Linux spidev");
#endif
}

/* Explicitly synthetic endpoint: a 64 KiB NOR with a deterministic image. */
static int flash_demo(host *h, drift_vm *vm, uint8_t *rx,
                      uint32_t *length, int32_t *status)
{
    const drift_effect *e=&vm->effect;
    const uint8_t *tx=vm->memory+e->tx;
    uint32_t address, i;
    if (e->tx_len == 1 && e->rx_len == 3 && tx[0] == 0x9f) {
        rx[0]=0xef; rx[1]=0x40; rx[2]=16; *length=3;
    } else if (e->tx_len == 4 && e->rx_len == 256 && tx[0] == 0x03 && h->total) {
        address=(uint32_t)tx[1]<<16 | (uint32_t)tx[2]<<8 | tx[3];
        if (address > h->total-256) return fail("demo read outside flash");
        for (i=0; i<256; ++i) rx[i]=(uint8_t)((address+i)^((address+i)>>8));
        *length=256;
    } else return fail("demo received an unsupported transaction");
    *status=0;
    return 0;
}

/* The host owns the sink; module events describe validated chunks in RAM. */
static int flash_event(host *h, drift_vm *vm)
{
    const drift_effect *e=&vm->effect;
    if (e->tag == 2) {
        if (h->total || e->z < 16 || e->z > 24 || e->x == 0 || e->x >= 255 || e->y > 255)
            return fail("invalid flash identification event");
        h->total=UINT32_C(1)<<e->z;
        fprintf(stderr,"JEDEC %02" PRIx32 " %02" PRIx32 " %02" PRIx32
                       ": %" PRIu32 " bytes; reading each block twice\n",e->x,e->y,e->z,h->total);
        return 0;
    }
    if (e->tag != 3 || !h->total || e->x != h->written || e->y != 256 ||
        e->y > h->total-h->written || e->z > vm->memory_len || e->y > vm->memory_len-e->z)
        return fail("invalid or out-of-order flash chunk");
    if (fwrite(vm->memory+e->z,1,e->y,h->dump) != e->y || fflush(h->dump))
        return fail("cannot write backup");
    h->written+=e->y;
    return 0;
}

static int load_image(const char *path, uint8_t *image, uint32_t *length)
{
    FILE *file=fopen(path,"rb");
    size_t n;
    int extra, bad;
    if (!file) { perror(path); return -1; }
    n=fread(image,1,IMAGE_MAX,file);
    extra=fgetc(file);
    bad=ferror(file);
    fclose(file);
    if (bad || extra != EOF) return fail("module cannot be read or is too large");
    *length=(uint32_t)n;
    return 0;
}

static int finish_trace(host *h)
{
    char line[17000];
    int n=trace_line(h,line,sizeof line);
    return n < 0 ? -1 : n ? fail("unused trace row after driver completion") : 0;
}

static int drive(host *h, drift_vm *vm)
{
    uint8_t rx[DRIFT_MEMORY_MAX];
    uint32_t waited=0;
    uint64_t since_effect=vm->steps;
    for (;;) {
        uint32_t length=0;
        int32_t status=0, state;
        int stopped=0;
        if (h->flash && stopping) return fail("backup interrupted");
        uint64_t remaining=h->flash ? UINT64_C(65536)-(vm->steps-since_effect) : STEP_MAX-vm->steps;
        if ((h->flash && vm->steps-since_effect >= 65536) || (!h->flash && vm->steps >= STEP_MAX))
            return fail("instruction limit reached");
        state=drift_run(vm,(uint32_t)(remaining < 4096 ? remaining : 4096));
        if (state == DRIFT_QUOTA) continue;
        if (state == DRIFT_DONE) {
            if (h->trace && finish_trace(h)) return -1;
            if (h->flash) {
                if (vm->r[0] || !h->total || h->written != h->total) {
                    fprintf(stderr,"drift: backup rejected (driver result %" PRIu32 ", bytes %" PRIu32 ")\n",vm->r[0],h->written);
                    return -1;
                }
                return 0;
            }
            if (printf("done %" PRIu32 " steps=%" PRIu64 "\n",vm->r[0],vm->steps) < 0 ||
                fflush(stdout) == EOF) return fail("cannot write stdout");
            return vm->r[0] ? 1 : 0;
        }
        if (state != DRIFT_EFFECT) {
            fprintf(stderr,"drift: machine error %" PRId32 " at pc=%" PRIu32 "\n",state,vm->pc);
            return -1;
        }
        if (vm->effect.kind == DRIFT_XFER) {
            if (h->trace) stopped=replay(h,vm,rx,&length,&status);
            else if (h->demo) stopped=flash_demo(h,vm,rx,&length,&status);
            else if (vm->bus==DRIFT_BUS_SPI) stopped=spi(h,vm,rx,&length,&status);
            else stopped=stream(h,vm,rx,&length,&status);
            if (stopped < 0) return -1;
            if (stopped) {
                if (h->flash) return fail("backup trace stopped before completion");
                if (finish_trace(h)) return -1;
                if (printf("stopped steps=%" PRIu64 "\n",vm->steps) < 0 ||
                    fflush(stdout) == EOF) return fail("cannot write stdout");
                return 0;
            }
        } else if (vm->effect.kind == DRIFT_EMIT) {
            if (h->flash) {
                if (flash_event(h,vm)) return -1;
            } else {
            /* Delivery must succeed before we acknowledge this effect. */
            if (printf("event %" PRIu32 " %" PRIu32 " %" PRIu32 " %" PRIu32 "\n",
                       vm->effect.tag,vm->effect.x,vm->effect.y,vm->effect.z) < 0 ||
                fflush(stdout) == EOF) return fail("cannot deliver event to stdout");
            }
        } else if (vm->effect.kind == DRIFT_WAIT) {
            struct timespec delay;
            if (vm->effect.x > 60000-waited) return fail("wait budget exceeded (60000 ms total)");
            waited+=vm->effect.x;
            delay.tv_sec=vm->effect.x/1000;
            delay.tv_nsec=(long)(vm->effect.x%1000)*1000000L;
            if (!h->trace) {
                while (nanosleep(&delay,&delay) < 0) {
                    if (errno != EINTR) { status=-errno; break; }
                }
            }
        } else return fail("unknown effect kind");
        state=drift_complete(vm,vm->effect.instance,vm->effect.ticket,status,rx,length);
        if (state != DRIFT_READY) return fail("completion rejected");
        since_effect=vm->steps;
    }
}

/* Publish a complete backup without overwriting an existing path. Partial
 * data stays in a private sibling file and is removed on handled failures. */
static int backup(int argc, char **argv)
{
    uint8_t image[IMAGE_MAX];
    uint32_t length=0;
    drift_vm vm;
    host h={0};
    const char *output, *module=DRIFT_MODULE_DIR "/flash.drv";
    char *temporary;
    size_t capacity;
    struct sigaction action;
    struct stat existing;
    int fd, result=-1;
    h.fd=-1; h.speed=1000000; h.flash=1;
    h.demo=!strcmp(argv[1],"flash-demo");
    if ((h.demo && argc != 3) || (!h.demo && argc != 4 && argc != 5))
        return fail("usage: drift flash DEVICE OUTPUT [HZ] | flash-replay TRACE OUTPUT | flash-demo OUTPUT");
    if (argc == 5 && (!strcmp(argv[1],"flash-replay") || number(argv[4],&h.speed) || !h.speed || h.speed>50000000))
        return fail("invalid SPI speed (1..50000000 Hz)");
    output=argv[h.demo ? 2 : 3];
    if (!lstat(output,&existing) || errno != ENOENT) return fail("output path must not exist");
    memset(&action,0,sizeof(action));
    action.sa_handler=stop_requested;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGINT,&action,NULL) || sigaction(SIGTERM,&action,NULL))
        return fail("cannot install stop handler");
    if (access(module,R_OK)) module="modules/flash.drv";
    if (load_image(module,image,&length)) return -1;
    if (drift_load(&vm,image,length,DRIFT_CAP_XFER|DRIFT_CAP_EMIT,1) != DRIFT_READY || vm.bus != DRIFT_BUS_SPI)
        return fail("flash module rejected");
    if (!h.demo) {
        if (!strcmp(argv[1],"flash-replay")) {
            h.name=argv[2]; h.trace=fopen(h.name,"r");
            if (!h.trace) { perror(h.name); return -1; }
        } else {
            h.fd=open(argv[2],O_RDWR);
            if (h.fd < 0) { perror(argv[2]); return -1; }
        }
    }
    capacity=strlen(output)+sizeof(".part-XXXXXX");
    temporary=malloc(capacity);
    if (!temporary) { fail("cannot allocate output path"); goto close_input; }
    snprintf(temporary,capacity,"%s.part-XXXXXX",output);
    fd=mkstemp(temporary);
    if (fd < 0) { perror(output); goto free_path; }
    h.dump=fdopen(fd,"wb");
    if (!h.dump) { close(fd); unlink(temporary); goto free_path; }
    result=drive(&h,&vm);
    if (!result && (fflush(h.dump) || fsync(fileno(h.dump)))) result=fail("cannot flush backup");
    if (fclose(h.dump)) result=fail("cannot close backup");
    if (h.trace) {
        if (fclose(h.trace)) result=fail("cannot close trace");
        h.trace=NULL;
    }
    if (h.fd >= 0) {
        if (close(h.fd)) result=fail("cannot close endpoint");
        h.fd=-1;
    }
    if (stopping) result=fail("backup interrupted");
    if (!result && link(temporary,output)) { perror(output); result=-1; }
    /* A successful link commits a complete image. A failed temporary-file
     * cleanup cannot invalidate that image, so report it without undoing it. */
    if (unlink(temporary)) perror(temporary);
    if (!result) fprintf(stderr,"saved %" PRIu32 " verified bytes to %s%s\n",h.written,output,h.demo ? " (synthetic demo)" : "");
free_path:
    free(temporary);
close_input:
    if (h.trace && fclose(h.trace)) result=-1;
    if (h.fd >= 0) close(h.fd);
    return result;
}

int main(int argc, char **argv)
{
    uint8_t image[IMAGE_MAX];
    uint32_t length=0;
    drift_vm vm;
    host h={0};
    int result;
    h.fd=-1; h.speed=1000000;
    if (argc >= 2 && (!strcmp(argv[1],"flash") || !strcmp(argv[1],"flash-replay") || !strcmp(argv[1],"flash-demo")))
        return backup(argc,argv) ? 1 : 0;
    if (argc < 4 || argc > 5 ||
        (strcmp(argv[1],"replay") && strcmp(argv[1],"spi") && strcmp(argv[1],"stream")) ||
        (argc==5 && strcmp(argv[1],"spi"))) {
        fprintf(stderr,"usage: drift replay MODULE TRACE\n"
                       "       drift spi MODULE DEVICE [HZ]\n"
                       "       drift stream MODULE PATH\n"
                       "       drift flash DEVICE OUTPUT [HZ]\n"
                       "       drift flash-replay TRACE OUTPUT\n"
                       "       drift flash-demo OUTPUT\n");
        return 2;
    }
    if (load_image(argv[2],image,&length)) return 1;
    result=drift_load(&vm,image,length,DRIFT_CAP_XFER|DRIFT_CAP_EMIT|DRIFT_CAP_WAIT,1);
    if (result != DRIFT_READY) { fprintf(stderr,"drift: module rejected (%d)\n",result); return 1; }
    if (!strcmp(argv[1],"replay")) {
        h.name=argv[3]; h.trace=fopen(argv[3],"r");
        if (!h.trace) { perror(argv[3]); return 1; }
    } else {
        uint32_t bus=!strcmp(argv[1],"spi") ? DRIFT_BUS_SPI : DRIFT_BUS_STREAM;
        if (vm.bus != bus) return fail("module bus does not match host") ? 1 : 0;
        if (argc==5 && (number(argv[4],&h.speed) || !h.speed || h.speed>50000000))
            return fail("invalid SPI speed (1..50000000 Hz)") ? 1 : 0;
        h.fd=open(argv[3],bus==DRIFT_BUS_SPI ? O_RDWR : O_RDONLY|O_NONBLOCK);
        if (h.fd < 0) { perror(argv[3]); return 1; }
    }
    result=drive(&h,&vm);
    if (h.trace) fclose(h.trace);
    if (h.fd >= 0) close(h.fd);
    if (fclose(stdout) == EOF) { fail("cannot close stdout"); result=1; }
    return result ? 1 : 0;
}
