/* HID 1.11 short-item parser for one proven keyboard layout.
 * https://www.usb.org/sites/default/files/hid1_11.pdf
 * Deliberately reject layouts the boot-keyboard module cannot interpret.
 * SPDX-License-Identifier: MIT */
#include "hid_descriptor.h"

typedef struct globals {
    uint32_t page, minimum, maximum, size, count;
    unsigned negative_minimum, have_minimum, have_maximum;
} globals;

typedef struct locals {
    uint32_t usages[8], minimum, maximum;
    unsigned count, have_minimum, have_maximum;
} locals;

static uint32_t usage(uint32_t value, unsigned bytes, uint32_t page)
{
    return bytes == 4 ? value : (page << 16) | value;
}

static int modifiers(const globals *g, const locals *l)
{
    unsigned i;
    if (g->page != 7 || !g->have_minimum || !g->have_maximum ||
        g->minimum || g->negative_minimum || g->maximum != 1)
        return 0;
    if (l->have_minimum || l->have_maximum)
        return l->count == 0 && l->have_minimum && l->have_maximum &&
               l->minimum == 0x700e0 && l->maximum == 0x700e7;
    if (l->count != 8) return 0;
    for (i = 0; i < 8; ++i)
        if (l->usages[i] != 0x700e0 + i) return 0;
    return 1;
}

static int key_array(const globals *g, const locals *l)
{
    /* Equal zero-based logical/usage ranges make each raw array value its HID
     * usage. Nonzero minima can silently remap array indices, so reject them.
     * Modifier usages E0..E7 belong only to the separate modifier bitmap. */
    return g->page == 7 && g->have_minimum && g->have_maximum &&
           !g->negative_minimum && !g->minimum &&
           g->maximum >= 0x65 && g->maximum < 0xe0 &&
           !l->count && l->have_minimum && l->have_maximum &&
           l->minimum == 0x70000 && l->maximum == 0x70000 + g->maximum;
}

int drift_boot_keyboard_descriptor(const uint8_t *data, size_t size)
{
    globals g = {0}, stack[8];
    locals l = {0};
    size_t at = 0;
    unsigned stack_size = 0, depth = 0, phase = 0, finished = 0;
    if (!data || !size || size > 4096) return 0;
    while (at < size) {
        uint32_t value = 0;
        unsigned prefix = data[at++], bytes = prefix & 3;
        unsigned type = (prefix >> 2) & 3, tag = prefix >> 4, i;
        if (finished || prefix == 0xfe || type == 3) return 0;
        if (bytes == 3) bytes = 4;
        if (size - at < bytes) return 0;
        for (i = 0; i < bytes; ++i) value |= (uint32_t)data[at++] << (8*i);
        if (type == 1) {
            if (tag == 10 || tag == 11) {
                if (bytes) return 0;
                if (tag == 10) {
                    if (stack_size == 8) return 0;
                    stack[stack_size++] = g;
                } else {
                    if (!stack_size) return 0;
                    g = stack[--stack_size];
                }
                continue;
            }
            if (!bytes) return 0;
            switch (tag) {
            case 0:
                if (value > 0xffff) return 0;
                g.page = value;
                break;
            case 1:
                g.minimum = value;
                g.negative_minimum = (value >> (bytes*8-1)) & 1;
                g.have_minimum = 1;
                break;
            case 2:
                g.maximum = value;
                g.have_maximum = 1;
                break;
            case 3: case 4: case 5: case 6:
                break; /* Physical bounds and units do not alter bit layout. */
            case 7:
                if (!value || value > 32) return 0;
                g.size = value;
                break;
            case 8:
                return 0; /* Any report ID adds a byte to hidraw input. */
            case 9:
                if (!value || value > 32768) return 0;
                g.count = value;
                break;
            default:
                return 0;
            }
        } else if (type == 2) {
            if (!bytes) return 0;
            switch (tag) {
            case 0:
                if (l.count == 8 || l.have_minimum || l.have_maximum) return 0;
                l.usages[l.count++] = usage(value, bytes, g.page);
                break;
            case 1:
                if (l.count || l.have_minimum) return 0;
                l.minimum = usage(value, bytes, g.page);
                l.have_minimum = 1;
                break;
            case 2:
                if (l.count || l.have_maximum) return 0;
                l.maximum = usage(value, bytes, g.page);
                l.have_maximum = 1;
                break;
            case 3: case 4: case 5: case 7: case 8: case 9:
                break; /* Designator/string metadata does not alter usages. */
            default:
                return 0; /* Includes alternative-usage delimiters. */
            }
        } else {
            if (l.have_minimum != l.have_maximum ||
                (l.have_minimum && l.minimum > l.maximum)) return 0;
            if (tag == 10) { /* Collection */
                if (bytes != 1 || depth == 16) return 0;
                if (!depth) {
                    if (value != 1 || l.count != 1 || l.have_minimum ||
                        l.usages[0] != 0x10006) return 0;
                } else if (value != 0 && value != 2) return 0;
                ++depth;
            } else if (tag == 12) { /* End Collection */
                if (bytes || !depth || l.count || l.have_minimum) return 0;
                if (!--depth) finished = 1;
            } else if (tag == 8 || tag == 9 || tag == 11) {
                if (!bytes || !depth || !g.size || !g.count ||
                    g.count > 32768/g.size || value > 0x1ff) return 0;
                if (tag == 8) {
                    if (phase == 0) {
                        if (value != 2 || g.size != 1 || g.count != 8 ||
                            !modifiers(&g, &l)) return 0;
                    } else if (phase == 1) {
                        if ((value != 1 && value != 3) || g.size*g.count != 8)
                            return 0;
                    } else if (phase == 2) {
                        if (value || g.size != 8 || g.count != 6 ||
                            !key_array(&g, &l)) return 0;
                    } else return 0;
                    ++phase;
                }
            } else return 0;
            l = (locals){0}; /* HID local state ends at every Main item. */
        }
    }
    return finished && !depth && !stack_size && phase == 3;
}
