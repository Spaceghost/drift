/* Optional formatter: unsupported instead of a fabricated formatted result.
 * SPDX-License-Identifier: MIT */
#include "types.h"
static inline NTSTATUS RtlStringCbVPrintfA(char *out,size_t capacity,const char *format,va_list arguments)
{ (void)format;(void)arguments;if(out&&capacity)out[0]=0;return STATUS_NOT_SUPPORTED; }
