cmd_arch/arm/vfp/vfphw.o := /home/jek/mac/Xilinx/14.6/ISE_DS/EDK/gnu/arm/lin/bin/arm-xilinx-linux-gnueabi-gcc -Wp,-MD,arch/arm/vfp/.vfphw.o.d  -nostdinc -isystem /home/jek/mac/Xilinx/14.6/ISE_DS/EDK/gnu/arm/lin/bin/../lib/gcc/arm-xilinx-linux-gnueabi/4.7.2/include -I/home/jek/xilinx-git/linux-xlnx/arch/arm/include -Iarch/arm/include/generated  -Iinclude -I/home/jek/xilinx-git/linux-xlnx/arch/arm/include/uapi -Iarch/arm/include/generated/uapi -I/home/jek/xilinx-git/linux-xlnx/include/uapi -Iinclude/generated/uapi -include /home/jek/xilinx-git/linux-xlnx/include/linux/kconfig.h -D__KERNEL__ -mlittle-endian  -D__ASSEMBLY__ -mabi=aapcs-linux -mno-thumb-interwork -funwind-tables -marm -D__LINUX_ARM_ARCH__=7 -march=armv7-a -include asm/unified.h -Wa,-mfpu=softvfp+vfp -mfloat-abi=soft        -c -o arch/arm/vfp/vfphw.o arch/arm/vfp/vfphw.S

source_arch/arm/vfp/vfphw.o := arch/arm/vfp/vfphw.S

deps_arch/arm/vfp/vfphw.o := \
    $(wildcard include/config/smp.h) \
    $(wildcard include/config/cpu/feroceon.h) \
    $(wildcard include/config/preempt/count.h) \
    $(wildcard include/config/thumb2/kernel.h) \
    $(wildcard include/config/vfpv3.h) \
  /home/jek/xilinx-git/linux-xlnx/arch/arm/include/asm/unified.h \
    $(wildcard include/config/arm/asm/unified.h) \
  /home/jek/xilinx-git/linux-xlnx/arch/arm/include/asm/thread_info.h \
    $(wildcard include/config/crunch.h) \
    $(wildcard include/config/arm/thumbee.h) \
  include/linux/compiler.h \
    $(wildcard include/config/sparse/rcu/pointer.h) \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
    $(wildcard include/config/kprobes.h) \
  /home/jek/xilinx-git/linux-xlnx/arch/arm/include/asm/fpstate.h \
    $(wildcard include/config/iwmmxt.h) \
  /home/jek/xilinx-git/linux-xlnx/arch/arm/include/asm/vfpmacros.h \
  /home/jek/xilinx-git/linux-xlnx/arch/arm/include/asm/hwcap.h \
  /home/jek/xilinx-git/linux-xlnx/arch/arm/include/uapi/asm/hwcap.h \
  /home/jek/xilinx-git/linux-xlnx/arch/arm/include/asm/vfp.h \
  include/linux/kern_levels.h \
  arch/arm/vfp/../kernel/entry-header.S \
    $(wildcard include/config/frame/pointer.h) \
    $(wildcard include/config/alignment/trap.h) \
    $(wildcard include/config/cpu/v7m.h) \
    $(wildcard include/config/trace/irqflags.h) \
    $(wildcard include/config/cpu/v6.h) \
    $(wildcard include/config/cpu/32v6k.h) \
    $(wildcard include/config/context/tracking.h) \
  include/linux/init.h \
    $(wildcard include/config/broken/rodata.h) \
    $(wildcard include/config/modules.h) \
  include/linux/types.h \
    $(wildcard include/config/uid16.h) \
    $(wildcard include/config/lbdaf.h) \
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
  include/linux/linkage.h \
  include/linux/stringify.h \
  include/linux/export.h \
    $(wildcard include/config/have/underscore/symbol/prefix.h) \
    $(wildcard include/config/modversions.h) \
    $(wildcard include/config/unused/symbols.h) \
  /home/jek/xilinx-git/linux-xlnx/arch/arm/include/asm/linkage.h \
  /home/jek/xilinx-git/linux-xlnx/arch/arm/include/asm/assembler.h \
    $(wildcard include/config/cpu/use/domains.h) \
  /home/jek/xilinx-git/linux-xlnx/arch/arm/include/asm/ptrace.h \
    $(wildcard include/config/arm/thumb.h) \
  /home/jek/xilinx-git/linux-xlnx/arch/arm/include/uapi/asm/ptrace.h \
    $(wildcard include/config/cpu/endian/be8.h) \
  /home/jek/xilinx-git/linux-xlnx/arch/arm/include/asm/domain.h \
    $(wildcard include/config/io/36.h) \
  /home/jek/xilinx-git/linux-xlnx/arch/arm/include/asm/opcodes-virt.h \
  /home/jek/xilinx-git/linux-xlnx/arch/arm/include/asm/opcodes.h \
    $(wildcard include/config/cpu/endian/be32.h) \
  /home/jek/xilinx-git/linux-xlnx/arch/arm/include/asm/asm-offsets.h \
  include/generated/asm-offsets.h \
  arch/arm/include/generated/asm/errno.h \
  /home/jek/xilinx-git/linux-xlnx/include/uapi/asm-generic/errno.h \
  /home/jek/xilinx-git/linux-xlnx/include/uapi/asm-generic/errno-base.h \
  /home/jek/xilinx-git/linux-xlnx/arch/arm/include/asm/v7m.h \

arch/arm/vfp/vfphw.o: $(deps_arch/arm/vfp/vfphw.o)

$(deps_arch/arm/vfp/vfphw.o):
