#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>


#if defined(OCPI_ARCH_arm64)
// Due an arm64 bug that results in an alignment fault when memset's "DC ZVA" instruction is used,
// we override memset for arm64 instead of using the assembly routine that calls that instruction.
// arm64's DC ZCA seems to be handling uncached memory as if it were device memory.

#ifdef __cplusplus
extern "C" {
#endif

void *memset(void *s, int c, size_t n) {
    uint8_t *p = (uint8_t *) s; while (n--) *p++ = c;
    return s;
}

#ifdef __cplusplus
}
#endif

#endif // OCPI_ARCH_arm64
