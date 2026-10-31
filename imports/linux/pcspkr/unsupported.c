/* Negative compile test: interrupt APIs are intentionally not implemented. */
#include <linux/io.h>
int unsupported(void) { return request_irq(1, 0, 0, "unsupported", 0); }
