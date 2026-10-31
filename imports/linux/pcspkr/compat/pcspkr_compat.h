/* A deliberately closed source-API surface for Linux v6.18 pcspkr.c.
 * This is not a general Linux header set. SPDX-License-Identifier: MIT */
#ifndef DRIFT_PCSPKR_COMPAT_H
#define DRIFT_PCSPKR_COMPAT_H
#include "drift_linux_pcspkr.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define EINVAL 22
#define ENOMEM 12
#define EV_SND 18
#define SND_BELL 1
#define SND_TONE 2
#define BUS_ISA 0x10
#define PIT_TICK_RATE 1193182
#define BIT_MASK(nr) (1ul << ((nr) % (8 * sizeof(unsigned long))))

struct device { drift_linux_pcspkr *owner; struct device *parent; };
struct input_id { uint16_t bustype, vendor, product, version; };
struct input_dev {
    const char *name, *phys;
    struct input_id id;
    struct device dev;
    unsigned long evbit[1], sndbit[1];
    int (*event)(struct input_dev *, unsigned int, unsigned int, int);
    drift_linux_pcspkr *owner;
};
typedef struct input_dev drift_pcspkr_input;
struct platform_device { struct device dev; void *driver_data; };
struct dev_pm_ops { int (*suspend)(struct device *); };
struct platform_driver {
    struct { const char *name; const struct dev_pm_ops *pm; } driver;
    int (*probe)(struct platform_device *);
    void (*remove)(struct platform_device *);
    void (*shutdown)(struct platform_device *);
};
typedef struct { int marker; } raw_spinlock_t;
extern raw_spinlock_t i8253_lock;
_Static_assert(sizeof(struct input_dev) <= sizeof(((drift_linux_pcspkr *)0)->input_slot), "input slot");
_Static_assert(sizeof(struct platform_device) <= sizeof(((drift_linux_pcspkr *)0)->platform_slot), "platform slot");
_Static_assert(_Alignof(struct input_dev) <= _Alignof(drift_linux_pcspkr), "input alignment");
_Static_assert(offsetof(drift_linux_pcspkr,platform_slot)==offsetof(drift_linux_pcspkr,input_slot)+256, "contiguous slots");

drift_linux_pcspkr *drift_lx_current(void);
int32_t drift_lx_prepare(drift_linux_pcspkr *, drift_linux_pcspkr_host *);
int32_t drift_lx_finish(drift_linux_pcspkr *, int32_t);
struct input_dev *drift_lx_input_allocate_device(void);
int32_t drift_lx_input_register_device(struct input_dev *);
void drift_lx_input_free_device(struct input_dev *);
void drift_lx_input_unregister_device(struct input_dev *);
uint8_t drift_lx_inb(uint16_t);
void drift_lx_outb(uint8_t, uint16_t);
void drift_lx_delay(void);
unsigned long drift_lx_lock(void);
void drift_lx_unlock(unsigned long);

static inline struct input_dev *drift_lx_slot(drift_linux_pcspkr *s) { return (struct input_dev *)(void *)s->input_slot; }
static inline uint8_t *drift_lx_storage(drift_linux_pcspkr *s) { return (uint8_t *)(void *)s->input_slot; }
static inline void drift_lx_set_owner(struct input_dev *d, drift_linux_pcspkr *s) { d->owner=s; }
static inline int32_t drift_lx_is_owned(struct input_dev *d, drift_linux_pcspkr *s) { return d==drift_lx_slot(s) && d->owner==s; }
static inline int32_t drift_lx_metadata_valid(struct input_dev *d, drift_linux_pcspkr *s) {
    struct platform_device *p=(void *)s->platform_slot;
    return d->name && d->phys && d->event && d->dev.parent==&p->dev &&
           (d->evbit[0]&BIT_MASK(EV_SND)) && (d->sndbit[0]&BIT_MASK(SND_BELL)) &&
           (d->sndbit[0]&BIT_MASK(SND_TONE));
}
static inline int32_t drift_lx_host_valid(drift_linux_pcspkr_host *h) { return h && h->read8 && h->write8 && h->lock && h->unlock && h->wait_us; }
static inline int32_t drift_lx_host_read(drift_linux_pcspkr_host *h, uint16_t p) { return h->read8(h->context,p); }
static inline int32_t drift_lx_host_write(drift_linux_pcspkr_host *h,uint16_t p,uint8_t v) { return h->write8(h->context,p,v); }
static inline int32_t drift_lx_host_lock(drift_linux_pcspkr_host *h,unsigned long *f) { return h->lock(h->context,f); }
static inline int32_t drift_lx_host_unlock(drift_linux_pcspkr_host *h,unsigned long f) { return h->unlock(h->context,f); }
static inline int32_t drift_lx_host_wait(drift_linux_pcspkr_host *h) { return h->wait_us(h->context,1); }

#define input_allocate_device drift_lx_input_allocate_device
#define input_register_device drift_lx_input_register_device
#define input_free_device drift_lx_input_free_device
#define input_unregister_device drift_lx_input_unregister_device
static inline void platform_set_drvdata(struct platform_device *d,void *p) { d->driver_data=p; }
static inline void *platform_get_drvdata(struct platform_device *d) { return d->driver_data; }
static inline uint8_t inb_p(uint16_t port) { uint8_t value=drift_lx_inb(port); drift_lx_delay(); return value; }
#define outb(value,port) drift_lx_outb((value),(port))
#define outb_p(value,port) do { drift_lx_outb((value),(port)); drift_lx_delay(); } while (0)
#define raw_spin_lock_irqsave(lock,flags) do { (void)(lock); (flags)=drift_lx_lock(); } while (0)
#define raw_spin_unlock_irqrestore(lock,flags) do { (void)(lock); drift_lx_unlock((flags)); } while (0)

/* Metadata remains data. Module registration is performed explicitly by the
 * host-facing lifecycle wrapper using this descriptor, not a Linux loader. */
#define MODULE_AUTHOR(value) static const char drift_lx_author[] __attribute__((unused))=value
#define MODULE_DESCRIPTION(value) static const char drift_lx_description[] __attribute__((unused))=value
#define MODULE_LICENSE(value) static const char drift_lx_license[] __attribute__((unused))=value
#define MODULE_ALIAS(value) static const char drift_lx_alias[] __attribute__((unused))=value
#define module_platform_driver(value) static struct platform_driver *const drift_lx_driver=&(value)
#endif
