/* Host layout measurement; not part of the machine. SPDX-License-Identifier: MIT */
#include "drift.h"
#include <stdio.h>
int main(void)
{
    printf("per device: %zu bytes; effect: %zu bytes\n",
           sizeof(drift_vm), sizeof(drift_effect));
    return 0;
}
