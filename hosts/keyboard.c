/* Drift keyboard monitor: validated hidraw input, portable protocol module.
 * SPDX-License-Identifier: MIT */
#define _POSIX_C_SOURCE 200809L
#include "drift.h"
#include "hid_descriptor.h"
#include <errno.h>
#include <fcntl.h>
#include <glob.h>
#include <inttypes.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef __linux__
#include <linux/hidraw.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#endif

#ifndef DRIFT_MODULE_DIR
#define DRIFT_MODULE_DIR "/usr/local/share/drift"
#endif

#define IMAGE_MAX (32u + 8u * DRIFT_CODE_MAX + DRIFT_MEMORY_MAX)
#define REPORT_FUEL UINT64_C(65536)
static volatile sig_atomic_t stopping;
static uint8_t image[IMAGE_MAX + 1];
static drift_vm vm;

typedef struct endpoint {
    int fd;
    FILE *trace, *record;
    unsigned line;
    uint8_t held[256];
} endpoint;

static int fail(const char *message)
{
    fprintf(stderr, "drift-keyboard: %s\n", message);
    return -1;
}

static void stop_requested(int signal_number)
{
    (void)signal_number;
    stopping = 1;
}

#ifdef __linux__
static const char *inspect(int fd, char *name, size_t capacity)
{
    struct hidraw_devinfo info;
    struct hidraw_report_descriptor descriptor;
    int size = 0;
    memset(&info, 0, sizeof(info));
    memset(&descriptor, 0, sizeof(descriptor));
    memset(name, 0, capacity);
    if (ioctl(fd, HIDIOCGRAWNAME(capacity - 1), name) < 0)
        snprintf(name, capacity, "USB HID device");
    for (size_t i = 0; i < capacity && name[i]; ++i)
        if ((unsigned char)name[i] < 32 || (unsigned char)name[i] > 126) name[i] = '?';
    if (ioctl(fd, HIDIOCGRAWINFO, &info) < 0 || info.bustype != BUS_USB)
        return "endpoint is not a USB HID device";
    if (ioctl(fd, HIDIOCGRDESCSIZE, &size) < 0 ||
        size < 1 || size > HID_MAX_DESCRIPTOR_SIZE)
        return "cannot read HID descriptor size";
    descriptor.size = (uint32_t)size;
    if (ioctl(fd, HIDIOCGRDESC, &descriptor) < 0 ||
        descriptor.size != (uint32_t)size ||
        !drift_boot_keyboard_descriptor(descriptor.value, descriptor.size))
        return "descriptor is not an unnumbered 8-byte boot keyboard report";
    return NULL;
}
#endif

static int list_devices(void)
{
#ifdef __linux__
    glob_t devices = {0};
    int result = glob("/dev/hidraw*", 0, NULL, &devices), found = 0;
    if (result != 0 && result != GLOB_NOMATCH) return fail("cannot enumerate hidraw devices");
    if (!result) for (size_t i = 0; i < devices.gl_pathc; ++i) {
        char name[256];
        const char *reason;
        int fd = open(devices.gl_pathv[i], O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (fd < 0) {
            if (printf("%s\tunavailable\t%s\n", devices.gl_pathv[i], strerror(errno)) < 0) {
                globfree(&devices); return fail("cannot list devices");
            }
            continue;
        }
        reason = inspect(fd, name, sizeof(name));
        if (printf("%s\t%s\t%s%s%s\n", devices.gl_pathv[i], reason ? "unsupported" : "ready",
                   name, reason ? ": " : "", reason ? reason : "") < 0) {
            close(fd); globfree(&devices); return fail("cannot list devices");
        }
        if (!reason) ++found;
        close(fd);
    }
    globfree(&devices);
    if (!found) fprintf(stderr, "No readable compatible keyboards found.\n");
    return fflush(stdout) ? fail("cannot list devices") : 0;
#else
    return fail("device discovery requires Linux hidraw");
#endif
}

static int load_module(const char *path)
{
    FILE *file = fopen(path, "rb");
    size_t size;
    int bad;
    if (!file) return fail("cannot open keyboard module (use --module PATH)");
    size = fread(image, 1, sizeof(image), file);
    bad = ferror(file);
    if (fclose(file)) bad = 1;
    if (bad || size == sizeof(image)) return fail("cannot read module");
    if (drift_load(&vm, image, (uint32_t)size, DRIFT_CAP_XFER | DRIFT_CAP_EMIT, 1) != DRIFT_READY)
        return fail("module rejected");
    return vm.bus == DRIFT_BUS_STREAM ? 0 : fail("keyboard module must use the record stream bus");
}

static int next_line(endpoint *host, char *line, size_t capacity)
{
    while (fgets(line, (int)capacity, host->trace)) {
        char *p = line, *comment;
        ++host->line;
        if (!strchr(line, '\n') && !feof(host->trace)) return fail("trace line too long");
        comment = strchr(line, '#');
        if (comment) *comment = 0;
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') ++p;
        if (*p) { memmove(line, p, strlen(p) + 1); return 1; }
    }
    return ferror(host->trace) ? fail("cannot read trace") : 0;
}

static int hex_digit(int c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* Return 1 only for an explicit stop at a pending transfer. */
static int replay(endpoint *host, uint8_t *reply, uint32_t *length, int32_t *status)
{
    char line[256], *word[6], *save = NULL, *end;
    long error;
    size_t size;
    int got = next_line(host, line, sizeof(line));
    if (got <= 0) return fail("trace ended without stop or a driver completion");
    word[0] = strtok_r(line, " \t\r\n", &save);
    for (int i = 1; i < 6; ++i) word[i] = strtok_r(NULL, " \t\r\n", &save);
    if (!strcmp(word[0], "stop") && !word[1]) {
        got = next_line(host, line, sizeof(line));
        return got < 0 ? -1 : got ? fail("unused trace row after stop") : 1;
    }
    if (strcmp(word[0], "xfer") || !word[4] || word[5] ||
        strcmp(word[1], "-") || strcmp(word[2], "8")) return fail("invalid keyboard trace row");
    errno = 0;
    error = strtol(word[3], &end, 0);
    if (!*word[3] || *end || errno || error < INT32_MIN || error > 0)
        return fail("invalid trace completion status");
    *status = (int32_t)error;
    if (!strcmp(word[4], "-")) size = 0;
    else {
        size = strlen(word[4]);
        if (!size || (size & 1) || size > 16) return fail("invalid trace report length");
        for (size_t i = 0; i < size / 2; ++i) {
            int a = hex_digit(word[4][i * 2]), b = hex_digit(word[4][i * 2 + 1]);
            if (a < 0 || b < 0) return fail("invalid trace report bytes");
            reply[i] = (uint8_t)((a << 4) | b);
        }
        size /= 2;
    }
    if (*status && size) return fail("failed transfers cannot carry report bytes");
    *length = (uint32_t)size;
    return 0;
}

static int receive(endpoint *host, uint8_t *reply, uint32_t *length, int32_t *status)
{
    for (;;) {
        struct pollfd input = {host->fd, POLLIN, 0};
        uint8_t packet[9];
        ssize_t count;
        int ready;
        if (stopping) return 1;
        /* Recheck signals every second; idle devices never time out. */
        ready = poll(&input, 1, 1000);
        if (ready < 0 && errno == EINTR) continue;
        if (ready < 0) return fail("cannot poll keyboard");
        if (!ready) continue;
        count = read(host->fd, packet, sizeof(packet));
        if (count < 0 && (errno == EINTR || errno == EAGAIN)) continue;
        if (count > 8) return fail("device supplied a report larger than its validated descriptor");
        *status = count < 0 ? -errno : count == 0 ? -ENODEV : 0;
        *length = count > 0 ? (uint32_t)count : 0;
        if (*length) memcpy(reply, packet, *length);
        return 0;
    }
}

static int record_report(endpoint *host, const uint8_t *reply, uint32_t length, int32_t status)
{
    if (!host->record) return 0;
    if (fprintf(host->record, "xfer - 8 %" PRId32 " ", status) < 0) return fail("cannot record report");
    if (!length && fputc('-', host->record) == EOF) return fail("cannot record report");
    for (uint32_t i = 0; i < length; ++i)
        if (fprintf(host->record, "%02x", reply[i]) < 0) return fail("cannot record report");
    if (fputc('\n', host->record) == EOF || fflush(host->record)) return fail("cannot record report");
    return 0;
}

static int event(endpoint *host, uint32_t usage, uint32_t down, uint32_t modifiers, const char *reason)
{
    if (printf("%s usage=0x%02" PRIx32 " modifiers=0x%02" PRIx32 "%s\n",
               down ? "press" : "release", usage, modifiers, reason) < 0 || fflush(stdout))
        return fail("cannot deliver keyboard event");
    host->held[usage] = (uint8_t)down;
    return 0;
}

static int drive(endpoint *host)
{
    uint64_t report_start = vm.steps;
    for (;;) {
        uint8_t reply[8];
        uint32_t length = 0;
        int32_t status = 0, state;
        int result;
        uint64_t used = vm.steps - report_start;
        if (used >= REPORT_FUEL) return fail("keyboard module exceeded its per-report instruction budget");
        state = drift_run(&vm, (uint32_t)(REPORT_FUEL - used < 256 ? REPORT_FUEL - used : 256));
        if (state == DRIFT_QUOTA) continue;
        if (state == DRIFT_DONE) {
            if (host->trace) {
                char line[256];
                result = next_line(host, line, sizeof(line));
                if (result < 0 || (result && fail("unused trace row after driver halt"))) return -1;
            }
            return vm.r[0] ? fail("keyboard driver reported a transport or protocol failure") : 0;
        }
        if (state != DRIFT_EFFECT) return fail("keyboard machine trapped");
        if (vm.effect.kind == DRIFT_XFER) {
            if (vm.effect.tx_len || vm.effect.rx_len != 8) return fail("keyboard endpoint permits 8-byte reads only");
            result = stopping ? 1 : host->trace ? replay(host, reply, &length, &status)
                                                       : receive(host, reply, &length, &status);
            if (result < 0) return -1;
            if (result) {
                if (host->record && (fputs("stop\n", host->record) == EOF || fflush(host->record)))
                    return fail("cannot finish recording");
                return 0;
            }
            if (record_report(host, reply, length, status)) return -1;
            if (!status && length == 8)
                for (uint32_t i = 2; i < 8; ++i)
                    if (reply[i] >= 0xe0)
                        return fail("report aliases modifiers in the key array");
            report_start = vm.steps;
        } else if (vm.effect.kind == DRIFT_EMIT) {
            if (vm.effect.tag != 1 || vm.effect.x < 4 || vm.effect.x > 255 ||
                vm.effect.y > 1 || vm.effect.z > 255)
                return fail("module emitted an invalid keyboard event");
            if (host->held[vm.effect.x] == vm.effect.y)
                return fail("module emitted a duplicate press or unmatched release");
            if (event(host, vm.effect.x, vm.effect.y, vm.effect.z, "")) return -1;
        } else return fail("unsupported keyboard effect");
        state = drift_complete(&vm, vm.effect.instance, vm.effect.ticket, status, reply, length);
        if (state != DRIFT_READY) return fail("keyboard completion rejected");
    }
}

static int usage(void)
{
    return puts("usage: drift-keyboard --list\n"
                "       drift-keyboard (--device PATH | --replay TRACE) [--record TRACE] [--module MODULE]\n"
                "\n"
                "List compatible USB keyboards, then monitor one with --device /dev/hidrawN.\n"
                "Use --replay fixtures/keyboard.trace to try it without hardware.\n"
                "--record creates a new replay trace; Ctrl-C stops and releases held usages.\n"
                "Events are HID usages, not layout-translated characters.") == EOF || fflush(stdout) ? 1 : 0;
}

int main(int argc, char **argv)
{
    const char *module = NULL, *device = NULL, *trace = NULL, *record = NULL;
    endpoint host = {-1, NULL, NULL, 0, {0}};
    struct sigaction action;
    int result, listing = 0;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--help")) return usage();
        if (!strcmp(argv[i], "--list")) listing = 1;
        else if (i + 1 < argc && !strcmp(argv[i], "--module")) module = argv[++i];
        else if (i + 1 < argc && !strcmp(argv[i], "--device")) device = argv[++i];
        else if (i + 1 < argc && !strcmp(argv[i], "--replay")) trace = argv[++i];
        else if (i + 1 < argc && !strcmp(argv[i], "--record")) record = argv[++i];
        else {
            fprintf(stderr, "usage: drift-keyboard --list\n"
                            "       drift-keyboard (--device PATH | --replay TRACE) [--record TRACE] [--module MODULE]\n");
            return 2;
        }
    }
    if (listing) return device || trace || record ? 2 : list_devices() ? 1 : 0;
    if ((!device && !trace) || (device && trace)) return fail("choose --device PATH or --replay TRACE") ? 2 : 0;
    if (!module) module = access(DRIFT_MODULE_DIR "/keyboard.drv", R_OK) == 0
                         ? DRIFT_MODULE_DIR "/keyboard.drv" : "modules/keyboard.drv";
    if (load_module(module)) return 1;
    if (trace) {
        host.trace = fopen(trace, "r");
        if (!host.trace) return fail("cannot open trace") ? 1 : 0;
    } else {
#ifdef __linux__
        char name[256];
        const char *reason;
        host.fd = open(device, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (host.fd < 0) { perror(device); return 1; }
        reason = inspect(host.fd, name, sizeof(name));
        if (reason) { close(host.fd); return fail(reason) ? 1 : 0; }
        fprintf(stderr, "Monitoring %s (%s); Ctrl-C to stop.\n", device, name);
#else
        return fail("live keyboard input requires Linux hidraw") ? 1 : 0;
#endif
    }
    if (record) {
        int fd = open(record, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
        if (fd < 0) { result = fail("cannot create recording (path must not exist)"); goto close_input; }
        host.record = fdopen(fd, "w");
        if (!host.record) { close(fd); result = fail("cannot open recording stream"); goto close_input; }
        if (fputs("# Recorded Drift keyboard report stream.\n", host.record) == EOF || fflush(host.record)) {
            result = fail("cannot start recording"); goto close_record;
        }
    }
    memset(&action, 0, sizeof(action));
    action.sa_handler = stop_requested;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGINT, &action, NULL) || sigaction(SIGTERM, &action, NULL)) {
        result = fail("cannot install stop handler"); goto close_record;
    }
    result = drive(&host);
    /* These releases are host cleanup, not fabricated driver events. */
    for (uint32_t usage = 4; usage < 256; ++usage)
        if (host.held[usage] && event(&host, usage, 0, 0, " cleanup=stop")) result = -1;
close_record:
    if (host.record && fclose(host.record)) result = fail("cannot close recording");
close_input:
    if (host.trace && fclose(host.trace)) result = fail("cannot close trace");
    if (host.fd >= 0) close(host.fd);
    if (fclose(stdout)) result = fail("cannot close event output");
    return result ? 1 : 0;
}
