cmd_arch/arm64/kernel/vdso/vdso.lds := /opt/Xilinx/SDK/2018.2/gnu/aarch64/lin/aarch64-linux/bin/aarch64-linux-gnu-gcc -E -Wp,-MD,arch/arm64/kernel/vdso/.vdso.lds.d  -nostdinc -isystem /opt/Xilinx/SDK/2018.2/gnu/aarch64/lin/aarch64-linux/bin/../lib/gcc/aarch64-linux-gnu/7.2.1/include -I./arch/arm64/include -I./arch/arm64/include/generated  -I./include -I./arch/arm64/include/uapi -I./arch/arm64/include/generated/uapi -I./include/uapi -I./include/generated/uapi -include ./include/linux/kconfig.h -D__KERNEL__ -mlittle-endian  -I/home/chinkey/git_stuff/opencpi/runtime/hdl/include -I/home/chinkey/git_stuff/opencpi/os/interfaces/include -Wformat=2   -P -C -Uarm64 -P -Uarm64 -D__ASSEMBLY__ -DLINKER_SCRIPT -o arch/arm64/kernel/vdso/vdso.lds arch/arm64/kernel/vdso/vdso.lds.S

source_arch/arm64/kernel/vdso/vdso.lds := arch/arm64/kernel/vdso/vdso.lds.S

deps_arch/arm64/kernel/vdso/vdso.lds := \
  include/uapi/linux/const.h \
  arch/arm64/include/asm/page.h \
    $(wildcard include/config/have/arch/pfn/valid.h) \
  arch/arm64/include/asm/page-def.h \
    $(wildcard include/config/arm64/page/shift.h) \
    $(wildcard include/config/arm64/cont/shift.h) \
  include/asm-generic/getorder.h \
  arch/arm64/include/asm/vdso.h \

arch/arm64/kernel/vdso/vdso.lds: $(deps_arch/arm64/kernel/vdso/vdso.lds)

$(deps_arch/arm64/kernel/vdso/vdso.lds):
