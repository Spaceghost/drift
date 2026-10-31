/* Descriptor acceptance tests independent of any physical USB device.
 * SPDX-License-Identifier: MIT */
#include "../hosts/hid_descriptor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
static const uint8_t keyboard[] = {
    0x05,0x01, 0x09,0x06, 0xa1,0x01,
    0x05,0x07, 0x19,0xe0, 0x29,0xe7, 0x15,0x00, 0x25,0x01,
    0x75,0x01, 0x95,0x08, 0x81,0x02,
    0x95,0x01, 0x75,0x08, 0x81,0x01,
    0x95,0x05, 0x75,0x01, 0x05,0x08, 0x19,0x01, 0x29,0x05, 0x91,0x02,
    0x95,0x01, 0x75,0x03, 0x91,0x01,
    0x95,0x06, 0x75,0x08, 0x15,0x00, 0x25,0x65, 0x05,0x07,
    0x19,0x00, 0x29,0x65, 0x81,0x00, 0xc0
};

static void check(int condition, const char *name)
{
    ++checks;
    if (!condition) { fprintf(stderr, "descriptor: %s\n", name); exit(1); }
}

static void accept(const uint8_t *data, size_t size, const char *name)
{
    check(drift_boot_keyboard_descriptor(data,size),name);
}

static void reject(const uint8_t *data, size_t size, const char *name)
{
    check(!drift_boot_keyboard_descriptor(data,size),name);
}

static void change(size_t at, uint8_t value, const char *name)
{
    uint8_t copy[sizeof keyboard];
    memcpy(copy,keyboard,sizeof copy);
    copy[at] = value;
    reject(copy,sizeof copy,name);
}

static size_t splice(uint8_t *out, size_t at, size_t removed,
                     const uint8_t *extra, size_t added)
{
    memcpy(out,keyboard,at);
    if (added) memcpy(out+at,extra,added);
    memcpy(out+at+added,keyboard+at+removed,sizeof keyboard-at-removed);
    return sizeof keyboard-removed+added;
}

int main(void)
{
    uint8_t copy[4097], extra[64];
    size_t i, length;
    uint32_t random = 0x5eed1234;
    accept(keyboard,sizeof keyboard,"standard keyboard with LEDs");
    reject(NULL,sizeof keyboard,"null descriptor");
    reject(keyboard,0,"empty descriptor");
    reject(copy,sizeof copy,"oversized descriptor");
    for (i=0; i<sizeof keyboard; ++i)
        reject(keyboard,i,"every truncated prefix");
    change(1,2,"wrong application page");
    change(3,2,"mouse application");
    change(5,0,"physical top-level collection");
    change(7,8,"LED-page modifier bits");
    change(9,0xe1,"shifted modifier range");
    change(11,0xe8,"oversized modifier range");
    change(13,1,"nonzero modifier logical minimum");
    change(15,2,"modifier logical maximum");
    change(17,2,"two-bit modifiers");
    change(19,7,"seven modifiers");
    change(21,0,"array modifiers");
    change(25,7,"seven-bit reserved field");
    change(27,0,"reserved byte is data");
    change(47,5,"five key slots");
    change(49,7,"seven-bit key slots");
    change(51,1,"nonzero array logical minimum");
    change(53,0x64,"array logical and usage maxima disagree");
    change(55,8,"LED-page key array");
    change(57,1,"array usage mapping offset");
    change(59,0x64,"array usage and logical maxima disagree");
    change(61,2,"variable key slots");
    change(61,4,"relative key slots");
    change(61,0x40,"unsupported null-state key array");
    change(62,0xc1,"invalid End Collection size");

    /* LED output fields do not change input offsets. */
    length=splice(copy,28,18,NULL,0);
    accept(copy,length,"keyboard with no output report");
    length=splice(copy,8,4,(const uint8_t[]){0x09,0xe0,0x09,0xe1,0x09,0xe2,
        0x09,0xe3,0x09,0xe4,0x09,0xe5,0x09,0xe6,0x09,0xe7},16);
    accept(copy,length,"explicit modifier usages");
    length=splice(copy,6,0,(const uint8_t[]){0x09,0x01,0xa1,0x02},4);
    copy[length++]=0xc0;
    accept(copy,length,"nested logical collection");
    length=splice(copy,8,4,(const uint8_t[]){
        0x1b,0xe0,0x00,0x07,0x00,0x2b,0xe7,0x00,0x07,0x00},10);
    accept(copy,length,"extended modifier usages");
    length=splice(copy,20,0,(const uint8_t[]){0xa4,0x95,0x09,0xb4},4);
    accept(copy,length,"global state restored by push/pop");
    length=splice(copy,46,0,(const uint8_t[]){0x15,0x00,0x25,0xff},4);
    /* Existing zero-based array range overwrites this maximum. */
    accept(copy,length,"unused logical state before array");
    memcpy(copy,keyboard,sizeof keyboard);
    copy[53]=copy[59]=0xdf;
    accept(copy,sizeof keyboard,"unsigned one-byte logical maximum 223");
    copy[53]=copy[59]=0xff;
    reject(copy,sizeof keyboard,"array range overlaps modifier usages");
    copy[53]=copy[59]=0xdf;
    copy[51]=0xff;
    reject(copy,sizeof keyboard,"negative array logical minimum");
    length=splice(copy,8,4,(const uint8_t[]){
        0x1b,0xe0,0x00,0x08,0x00,0x2b,0xe7,0x00,0x08,0x00},10);
    reject(copy,length,"extended usage page must match keyboard");

    length=splice(copy,6,0,(const uint8_t[]){0x85,0x01},2);
    reject(copy,length,"numbered input report");
    copy[7]=0;
    reject(copy,length,"invalid zero report ID");
    length=splice(copy,62,0,(const uint8_t[]){0x85,0x01},2);
    reject(copy,length,"report ID on output still rejected");
    length=splice(copy,0,0,(const uint8_t[]){0xb4},1);
    reject(copy,length,"pop underflow");
    length=splice(copy,0,0,(const uint8_t[]){0xa4},1);
    reject(copy,length,"unclosed global state stack");
    length=splice(copy,0,0,(const uint8_t[]){0xa5,0x00},2);
    reject(copy,length,"push with a payload");
    memset(extra,0xa4,9);
    length=splice(copy,0,0,extra,9);
    reject(copy,length,"bounded global stack");
    length=splice(copy,18,2,(const uint8_t[]){0x97,0xff,0xff,0xff,0xff},5);
    reject(copy,length,"overflowing report count");
    length=splice(copy,16,2,(const uint8_t[]){0x77,0xff,0xff,0xff,0xff},5);
    reject(copy,length,"overflowing report size");
    length=splice(copy,0,0,(const uint8_t[]){0x07,0,0,1,0},5);
    reject(copy,length,"usage page exceeds sixteen bits");
    length=splice(copy,8,0,(const uint8_t[]){0x09,0xe0},2);
    reject(copy,length,"mixed explicit and ranged local usages");
    length=splice(copy,8,0,(const uint8_t[]){0xa9,1},2);
    reject(copy,length,"alternative-usage delimiter");
    length=splice(copy,8,2,NULL,0);
    reject(copy,length,"usage range missing its minimum");
    length=splice(copy,62,0,(const uint8_t[]){0x81,1},2);
    reject(copy,length,"additional input padding changes report size");
    length=splice(copy,0,0,(const uint8_t[]){0xfe,0x00,0x00},3);
    reject(copy,length,"unsupported long item");
    length=splice(copy,0,0,(const uint8_t[]){0xfc},1);
    reject(copy,length,"reserved item type");
    length=splice(copy,0,0,(const uint8_t[]){0xc4},1);
    reject(copy,length,"reserved global tag");
    length=splice(copy,0,0,(const uint8_t[]){0xc0},1);
    reject(copy,length,"collection underflow");
    length=splice(copy,62,0,(const uint8_t[]){0xc0},1);
    reject(copy,length,"extra end collection");
    length=splice(copy,62,0,(const uint8_t[]){0x05,1,0x09,6,0xa1,1},6);
    reject(copy,length,"nested application collection");
    for (i=0; i<16; ++i) { extra[2*i]=0xa1; extra[2*i+1]=0; }
    length=splice(copy,6,0,extra,32);
    reject(copy,length,"bounded collection stack");
    memcpy(copy,keyboard,sizeof keyboard);
    memcpy(copy+sizeof keyboard,keyboard,sizeof keyboard);
    reject(copy,2*sizeof keyboard,"multiple application collections");

    /* Exercise parsing of malformed arbitrary data under sanitizers. The exact
     * accepted subset is covered above; do not assert random bytes are invalid. */
    for (i=0; i<20000; ++i) {
        size_t j, n=i%sizeof(copy);
        for (j=0; j<n; ++j) {
            random ^= random << 13; random ^= random >> 17; random ^= random << 5;
            copy[j]=(uint8_t)random;
        }
        (void)drift_boot_keyboard_descriptor(copy,n);
    }
    printf("hid descriptor: %u checks passed; 20,000 malformed inputs parsed\n",checks);
    return 0;
}
