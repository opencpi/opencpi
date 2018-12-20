
/* config.h.in is copied to config.h with substitutions made by
 * the configure script.
 */

#ifndef CONFIG_H
#define CONFIG_H

//#define _CPU_POWERPC
#if !defined(_CPU_IA64) && defined(__x86_64__)
#define _CPU_IA64
#endif
#if defined(__arm__) || defined(__ARM_ARCH)
#define _CPU_ARM
#endif

#include <stdint.h>

#endif
