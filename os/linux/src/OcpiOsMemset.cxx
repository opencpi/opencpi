#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include "ocpi-config.h"


#if defined(OCPI_ARCH_arm64)
/* There is an invalid assumption made by arm64 on certain systems that results in an alignment
 * fault when memset's "DC ZVA" instruction is used on uncached memory.
 *
 * So, we override memset for arm64 instead of using the assembly routine that calls that instruction.
 * arm64's DC ZCA causes an alignment fault if used on any type of device memory.... Well it treats
 * uncached memory as device memory here and causes an alignment fault for valid calls to memset on
 * uncached memory. See https://patchwork.kernel.org/patch/6362401/ for more information on this
 * issue and solutions that have been explored. Also see comments on zeromem_dczva here
 * https://github.com/ARM-software/arm-trusted-firmware/blob/master/lib/aarch64/misc_helpers.S
 *
 * OpenCPI creates uncached DMA mappings for the CPU using the O_SYNC flag to the open() calls to the
 * driver (e.g. in DtDmaXfer.cxx).
 */

#ifdef __cplusplus
extern "C" {
#endif

void *memset(void *s, int c, size_t n) {
    // This implementation could be optimized in the following ways:
    // Check if 'n' bytes is aligned on 16, 32, 64 bit boundaries, and use uint* types matching that
    // alignment to reduce the number of iterations in the loop below.
    uint8_t *p = (uint8_t *) s; while (n--) *p++ = c;
    return s;
}

#ifdef __cplusplus
}
#endif

#endif // OCPI_ARCH_arm64
