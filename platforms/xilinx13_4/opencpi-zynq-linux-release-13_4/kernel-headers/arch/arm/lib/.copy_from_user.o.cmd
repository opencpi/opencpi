cmd_arch/arm/lib/copy_from_user.o := /opt/Xilinx/14.7/ISE_DS/EDK/gnu/arm/lin/bin/arm-xilinx-linux-gnueabi-gcc -Wp,-MD,arch/arm/lib/.copy_from_user.o.d  -nostdinc -isystem /home/jek/mac/Xilinx/14.7/ISE_DS/EDK/gnu/arm/lin/bin/../lib/gcc/arm-xilinx-linux-gnueabi/4.7.3/include -I/home/jek/git/linux-xlnx/arch/arm/include -Iarch/arm/include/generated  -Iinclude -I/home/jek/git/linux-xlnx/arch/arm/include/uapi -Iarch/arm/include/generated/uapi -I/home/jek/git/linux-xlnx/include/uapi -Iinclude/generated/uapi -include /home/jek/git/linux-xlnx/include/linux/kconfig.h -D__KERNEL__ -mlittle-endian  -D__ASSEMBLY__ -mabi=aapcs-linux -mno-thumb-interwork -funwind-tables -marm -D__LINUX_ARM_ARCH__=7 -march=armv7-a  -include asm/unified.h -msoft-float         -c -o arch/arm/lib/copy_from_user.o arch/arm/lib/copy_from_user.S

source_arch/arm/lib/copy_from_user.o := arch/arm/lib/copy_from_user.S

deps_arch/arm/lib/copy_from_user.o := \
    $(wildcard include/config/thumb2/kernel.h) \
  /home/jek/git/linux-xlnx/arch/arm/include/asm/unified.h \
    $(wildcard include/config/arm/asm/unified.h) \
  include/linux/linkage.h \
  include/linux/compiler.h \
    $(wildcard include/config/sparse/rcu/pointer.h) \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
    $(wildcard include/config/kprobes.h) \
  include/linux/stringify.h \
  include/linux/export.h \
    $(wildcard include/config/have/underscore/symbol/prefix.h) \
    $(wildcard include/config/modules.h) \
    $(wildcard include/config/modversions.h) \
    $(wildcard include/config/unused/symbols.h) \
  /home/jek/git/linux-xlnx/arch/arm/include/asm/linkage.h \
  /home/jek/git/linux-xlnx/arch/arm/include/asm/assembler.h \
    $(wildcard include/config/cpu/feroceon.h) \
    $(wildcard include/config/trace/irqflags.h) \
    $(wildcard include/config/cpu/v7m.h) \
    $(wildcard include/config/smp.h) \
    $(wildcard include/config/cpu/use/domains.h) \
  /home/jek/git/linux-xlnx/arch/arm/include/asm/ptrace.h \
    $(wildcard include/config/arm/thumb.h) \
  /home/jek/git/linux-xlnx/arch/arm/include/uapi/asm/ptrace.h \
    $(wildcard include/config/cpu/endian/be8.h) \
  /home/jek/git/linux-xlnx/arch/arm/include/asm/hwcap.h \
  /home/jek/git/linux-xlnx/arch/arm/include/uapi/asm/hwcap.h \
  /home/jek/git/linux-xlnx/arch/arm/include/asm/domain.h \
    $(wildcard include/config/io/36.h) \
  /home/jek/git/linux-xlnx/arch/arm/include/asm/opcodes-virt.h \
  /home/jek/git/linux-xlnx/arch/arm/include/asm/opcodes.h \
    $(wildcard include/config/cpu/endian/be32.h) \
  arch/arm/lib/copy_template.S \

arch/arm/lib/copy_from_user.o: $(deps_arch/arm/lib/copy_from_user.o)

$(deps_arch/arm/lib/copy_from_user.o):
