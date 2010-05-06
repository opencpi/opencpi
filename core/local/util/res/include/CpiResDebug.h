#ifndef UTL_DEBUG_H
#define UTL_DEBUG_H

//#include "utl_defs.h"

/*
 * The follow macros are intended to be used for debugging the kernel
 * portion of the driver
 */

#ifdef KERNEL
#define Execdbg_level utl_kernel_debug
#define UTL_DEBUGVAR utl_kernel_debug
#else
#define UTL_DEBUGVAR utl_user_debug
#endif /* KERNEL */

extern long UTL_DEBUGVAR;

#ifdef UTL_DEBUG
#define UTL_DPRINT(mask, args) \
  do if (UTL_DEBUGVAR & (mask)) \
     os_error_printf args; while (0)
#else
#define UTL_DPRINT(mask, args) \
  do {} while (0)
#endif /* UTL_DEBUG */


/* The following are the bit flags that are used with the UTL_DEBUGVAR */
#define UTLD_ALL        0xffffffff

#define UTLD_PRINT        (1 << 0)        /* General print */
#define UTLD_MAP        (1 << 1)        /* Mapping functions */
#define UTLD_OSM        (1 << 2)        /* OSM functions */
#define UTLD_SYS        (1 << 3)        /* SYS functions */
#define UTLD_PRC        (1 << 4)        /* PRC functions */
#define UTLD_IMG        (1 << 5)        /* IMG functions */
#define UTLD_PROCESS        (1 << 6)        /* Process & thread functions */
#define UTLD_GRM        (1 << 7)        /* GRM functions */


/*
 * Below is the old dbg stuff which violates varous
 * naming conventions.
 */
/* This should be more properly coordinated... */
#ifndef DBG_PRINTF
#if defined(MC_HOST) || !defined(KERNEL)
#define DBG_PRINTF if(0) utl_printf
#else
#define        DBG_PRINTF        if (UTL_DEBUGVAR & UTL_DBG_PRINTF) utl_printf
#endif
#endif

extern        long        UTL_DEBUGVAR;

#define        UTL_DBG_PRINTF                (1<<0)
#define        UTL_DBG_TIME_CONSOLE        (1<<1)  // Do console output w/ timestamp
#define        UTL_DBG_MAPWRITTABLE        (1<<2)
#define        UTL_DBG_SAVE_FPREGS        (1<<3)
#define        UTL_DBG_MEMPROBE        (1<<4)
#define UTL_DBG_ENABLE_EDB        (1<<5)
#define UTL_DBG_NO_INTR_PANIC   (1<<6)
#define UTL_DBG_CONFIG                (1<<7)
#define UTL_DBG_SYNC_CONSOLE        (1<<8)        // 0x00000100
#define UTL_DBG_GRM                (1<<9)        // 0x00000200
#define UTL_DBG_MEM                (1<<10) // 0x00000400
#define UTL_DBG_MAP                (1<<11) // 0x00000800
#define UTL_DBG_IOM                (1<<12) // Free
#define UTL_DBG_NK                (1<<13) // 0x00002000
#define UTL_DBG_RSR                (1<<14)        // 0x00004000
#define UTL_DBG_LD                (1<<15)
#define UTL_DBG_PATH                (1<<16)
#define UTL_DBG_DX                (1<<17)
#define UTL_DBG_VM                (1<<18)
#define UTL_DBG_SHARC                (1<<19)
#define UTL_DBG_CE_DX_ENG        (1<<20)  /* CE xfer engine drivers */
#define UTL_DBG_DEV                (1<<21)         /* device facility */
#define UTL_DBG_EBI                (1<<22)        // 0x00400000
#define UTL_DBG_EBI_EP                (1<<23)        // 0x00800000
#define UTL_DBG_EBI_IC                (1<<24) // 0x01000000
#define UTL_DBG_EBI_BRIDGE        (1<<25) // 0x02000000
#define UTL_DBG_IPI                (1<<26)        // 0x04000000
#define UTL_DBG_OSM                (1<<27) // 0x08000000
#define UTL_DBG_IMG                (1<<28) // 0x10000000
#define UTL_DBG_PRC                (1<<29)        // 0x20000000
#define UTL_DBG_EBI_ASM                (1<<30) // 0x40000000

#if defined(NDEBUG)
#define DBG_CONFIG if(0) utl_printf
#define DBG_GRM if (0) utl_printf
#define DBG_MEM        if (0) utl_printf
#define DBG_MAP        if (0) utl_printf
#define DBG_IOM        if (0) utl_printf
#define DBG_NK if (0) utl_printf
#define DBG_RSR        if (0) utl_printf
#define DBG_LD        if (0) utl_printf
#define DBG_PATH if (0) utl_printf
#define DBG_DX if (0) utl_printf
#define DBG_VM if (0) utl_printf
#define DBG_SHARC if (0) utl_printf
#define DBG_CE_DX_ENG if (0) utl_printf
#define DBG_DEV if (0) utl_printf
#define DBG_EBI if (0) utl_printf
#define DBG_EBI_EP if (0) utl_printf
#define DBG_EBI_BRIDGE if (0) utl_printf
#define DBG_EBI_IC if (0) utl_printf
#define DBG_IPI if (0) utl_printf
#define DBG_OSM if (0) utl_printf
#define DBG_IMG if (0) utl_printf
#define DBG_PRC if (0) utl_printf
#define DBG_EBI_ASM if (0) utl_printf

#else

#define        DBG_CONFIG        if (UTL_DEBUGVAR & UTL_DBG_CONFIG) utl_printf
#define DBG_GRM                if (UTL_DEBUGVAR & UTL_DBG_GRM) utl_printf
#define DBG_MEM                if (UTL_DEBUGVAR & UTL_DBG_MEM) utl_printf
#define DBG_MAP                if (UTL_DEBUGVAR & UTL_DBG_MAP) utl_printf
#define DBG_IOM                if (UTL_DEBUGVAR & UTL_DBG_IOM) utl_printf
#define DBG_NK                if (UTL_DEBUGVAR & UTL_DBG_NK) utl_printf
#define DBG_RSR                if (UTL_DEBUGVAR & UTL_DBG_RSR) utl_printf
#define DBG_LD                if (UTL_DEBUGVAR & UTL_DBG_LD) utl_printf
#define DBG_PATH        if (UTL_DEBUGVAR & UTL_DBG_PATH) utl_printf
#define DBG_DX                if (UTL_DEBUGVAR & UTL_DBG_DX) utl_printf
#define DBG_VM                if (UTL_DEBUGVAR & UTL_DBG_VM) utl_printf
#define DBG_SHARC        if (UTL_DEBUGVAR & UTL_DBG_SHARC) utl_printf
#define DBG_CE_DX_ENG        if (UTL_DEBUGVAR & UTL_DBG_CE_DX_ENG) utl_printf
#define DBG_DEV                if (UTL_DEBUGVAR & UTL_DBG_DEV) utl_printf
#define DBG_EBI                if (UTL_DEBUGVAR & UTL_DBG_EBI) utl_printf
#define DBG_EBI_EP        if (UTL_DEBUGVAR & UTL_DBG_EBI_EP) utl_printf
#define DBG_EBI_IC        if (UTL_DEBUGVAR & UTL_DBG_EBI_IC) utl_printf
#define DBG_EBI_BRIDGE        if (UTL_DEBUGVAR & UTL_DBG_EBI_BRIDGE) utl_printf
#define DBG_IPI                if (UTL_DEBUGVAR & UTL_DBG_IPI) utl_printf
#define DBG_OSM                if (UTL_DEBUGVAR & UTL_DBG_OSM) utl_printf
#define DBG_IMG                if (UTL_DEBUGVAR & UTL_DBG_IMG) utl_printf
#define DBG_PRC                if (UTL_DEBUGVAR & UTL_DBG_PRC) utl_printf
#define DBG_EBI_ASM        if (UTL_DEBUGVAR & UTL_DBG_EBI_ASM) utl_printf
#endif

#endif
