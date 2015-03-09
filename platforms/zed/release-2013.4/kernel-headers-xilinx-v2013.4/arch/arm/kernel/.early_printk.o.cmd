cmd_arch/arm/kernel/early_printk.o := /home/jek/mac/Xilinx/14.6/ISE_DS/EDK/gnu/arm/lin/bin/arm-xilinx-linux-gnueabi-gcc -Wp,-MD,arch/arm/kernel/.early_printk.o.d  -nostdinc -isystem /home/jek/mac/Xilinx/14.6/ISE_DS/EDK/gnu/arm/lin/bin/../lib/gcc/arm-xilinx-linux-gnueabi/4.7.2/include -I/home/jek/xilinx-git/linux-xlnx/arch/arm/include -Iarch/arm/include/generated  -Iinclude -I/home/jek/xilinx-git/linux-xlnx/arch/arm/include/uapi -Iarch/arm/include/generated/uapi -I/home/jek/xilinx-git/linux-xlnx/include/uapi -Iinclude/generated/uapi -include /home/jek/xilinx-git/linux-xlnx/include/linux/kconfig.h -D__KERNEL__ -mlittle-endian -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -Wno-format-security -fno-delete-null-pointer-checks -Os -Wno-maybe-uninitialized -fno-dwarf2-cfi-asm -mabi=aapcs-linux -mno-thumb-interwork -funwind-tables -marm -D__LINUX_ARM_ARCH__=7 -march=armv7-a -msoft-float -Uarm -Wframe-larger-than=1024 -fno-stack-protector -Wno-unused-but-set-variable -fomit-frame-pointer -Wdeclaration-after-statement -Wno-pointer-sign -fno-strict-overflow -fconserve-stack -DCC_HAVE_ASM_GOTO    -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(early_printk)"  -D"KBUILD_MODNAME=KBUILD_STR(early_printk)" -c -o arch/arm/kernel/.tmp_early_printk.o arch/arm/kernel/early_printk.c

source_arch/arm/kernel/early_printk.o := arch/arm/kernel/early_printk.c

deps_arch/arm/kernel/early_printk.o := \
  include/linux/kernel.h \
    $(wildcard include/config/lbdaf.h) \
    $(wildcard include/config/preempt/voluntary.h) \
    $(wildcard include/config/debug/atomic/sleep.h) \
    $(wildcard include/config/prove/locking.h) \
    $(wildcard include/config/ring/buffer.h) \
    $(wildcard include/config/tracing.h) \
    $(wildcard include/config/ftrace/mcount/record.h) \
  /home/jek/mac/Xilinx/14.6/ISE_DS/EDK/gnu/arm/lin/bin/../lib/gcc/arm-xilinx-linux-gnueabi/4.7.2/include/stdarg.h \
  include/linux/linkage.h \
  include/linux/compiler.h \
    $(wildcard include/config/sparse/rcu/pointer.h) \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
    $(wildcard include/config/kprobes.h) \
  include/linux/compiler-gcc.h \
    $(wildcard include/config/arch/supports/optimized/inlining.h) \
    $(wildcard include/config/optimize/inlining.h) \
  include/linux/compiler-gcc4.h \
    $(wildcard include/config/arch/use/builtin/bswap.h) \
  include/linux/stringify.h \
  include/linux/export.h \
    $(wildcard include/config/have/underscore/symbol/prefix.h) \
    $(wildcard include/config/modules.h) \
    $(wildcard include/config/modversions.h) \
    $(wildcard include/config/unused/symbols.h) \
  /home/jek/xilinx-git/linux-xlnx/arch/arm/include/asm/linkage.h \
  include/linux/stddef.h \
  include/uapi/linux/stddef.h \
  include/linux/types.h \
    $(wildcard include/config/uid16.h) \
    $(wildcard include/config/arch/dma/addr/t/64bit.h) \
    $(wildcard include/config/phys/addr/t/64bit.h) \
    $(wildcard include/config/64bit.h) \
  include/uapi/linux/types.h \
  /home/jek/xilinx-git/linux-xlnx/arch/arm/include/asm/types.h \
  include/asm-generic/int-ll64.h \
  include/uapi/asm-generic/int-ll64.h \
  arch/arm/include/generated/asm/bitsperlong.h \
  include/asm-generic/bitsperlong.h \
  include/uapi/asm-generic/bitsperlong.h \
  /home/jek/xilinx-git/linux-xlnx/include/uapi/linux/posix_types.h \
  /home/jek/xilinx-git/linux-xlnx/arch/arm/include/uapi/asm/posix_types.h \
  /home/jek/xilinx-git/linux-xlnx/include/uapi/asm-generic/posix_types.h \
  include/linux/bitops.h \
  /home/jek/xilinx-git/linux-xlnx/arch/arm/include/asm/bitops.h \
    $(wildcard include/config/smp.h) \
  include/linux/irqflags.h \
    $(wildcard include/config/trace/irqflags.h) \
    $(wildcard include/config/irqsoff/tracer.h) \
    $(wildcard include/config/preempt/tracer.h) \
    $(wildcard include/config/trace/irqflags/support.h) \
  include/linux/typecheck.h \
  /home/jek/xilinx-git/linux-xlnx/arch/arm/include/asm/irqflags.h \
    $(wildcard include/config/cpu/v7m.h) \
  /home/jek/xilinx-git/linux-xlnx/arch/arm/include/asm/ptrace.h \
    $(wildcard include/config/arm/thumb.h) \
  /home/jek/xilinx-git/linux-xlnx/arch/arm/include/uapi/asm/ptrace.h \
    $(wildcard include/config/cpu/endian/be8.h) \
  /home/jek/xilinx-git/linux-xlnx/arch/arm/include/asm/hwcap.h \
  /home/jek/xilinx-git/linux-xlnx/arch/arm/include/uapi/asm/hwcap.h \
  include/asm-generic/bitops/non-atomic.h \
  include/asm-generic/bitops/fls64.h \
  include/asm-generic/bitops/sched.h \
  include/asm-generic/bitops/hweight.h \
  include/asm-generic/bitops/arch_hweight.h \
  include/asm-generic/bitops/const_hweight.h \
  include/asm-generic/bitops/lock.h \
  include/asm-generic/bitops/le.h \
  /home/jek/xilinx-git/linux-xlnx/arch/arm/include/uapi/asm/byteorder.h \
  include/linux/byteorder/little_endian.h \
  include/uapi/linux/byteorder/little_endian.h \
  include/linux/swab.h \
  include/uapi/linux/swab.h \
  /home/jek/xilinx-git/linux-xlnx/arch/arm/include/asm/swab.h \
  /home/jek/xilinx-git/linux-xlnx/arch/arm/include/uapi/asm/swab.h \
  include/linux/byteorder/generic.h \
  include/asm-generic/bitops/ext2-atomic-setbit.h \
  include/linux/log2.h \
    $(wildcard include/config/arch/has/ilog2/u32.h) \
    $(wildcard include/config/arch/has/ilog2/u64.h) \
  include/linux/printk.h \
    $(wildcard include/config/early/printk.h) \
    $(wildcard include/config/printk.h) \
    $(wildcard include/config/dynamic/debug.h) \
  include/linux/init.h \
    $(wildcard include/config/broken/rodata.h) \
  include/linux/kern_levels.h \
  include/linux/dynamic_debug.h \
  include/uapi/linux/kernel.h \
  /home/jek/xilinx-git/linux-xlnx/include/uapi/linux/sysinfo.h \
  /home/jek/xilinx-git/linux-xlnx/arch/arm/include/asm/div64.h \
    $(wildcard include/config/aeabi.h) \
  /home/jek/xilinx-git/linux-xlnx/arch/arm/include/asm/compiler.h \
  /home/jek/xilinx-git/linux-xlnx/arch/arm/include/asm/bug.h \
    $(wildcard include/config/bug.h) \
    $(wildcard include/config/thumb2/kernel.h) \
    $(wildcard include/config/debug/bugverbose.h) \
    $(wildcard include/config/arm/lpae.h) \
  include/asm-generic/bug.h \
    $(wildcard include/config/generic/bug.h) \
    $(wildcard include/config/generic/bug/relative/pointers.h) \
  include/linux/console.h \
    $(wildcard include/config/hw/console.h) \
    $(wildcard include/config/tty.h) \
    $(wildcard include/config/vga/console.h) \

arch/arm/kernel/early_printk.o: $(deps_arch/arm/kernel/early_printk.o)

$(deps_arch/arm/kernel/early_printk.o):
