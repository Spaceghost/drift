/* Negative compile fixture: intentionally outside the closed Linux API. */
#include <linux/slab.h>
void *drift_unsupported_linux_api(void) { return kmalloc(16, GFP_KERNEL); }
