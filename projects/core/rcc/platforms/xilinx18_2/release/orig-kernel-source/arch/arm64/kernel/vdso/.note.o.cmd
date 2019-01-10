cmd_arch/arm64/kernel/vdso/note.o := /opt/Xilinx/SDK/2018.2/gnu/aarch64/lin/aarch64-linux/bin/aarch64-linux-gnu-gcc -Wp,-MD,arch/arm64/kernel/vdso/.note.o.d  -nostdinc -isystem /opt/Xilinx/SDK/2018.2/gnu/aarch64/lin/aarch64-linux/bin/../lib/gcc/aarch64-linux-gnu/7.2.1/include -I./arch/arm64/include -I./arch/arm64/include/generated  -I./include -I./arch/arm64/include/uapi -I./arch/arm64/include/generated/uapi -I./include/uapi -I./include/generated/uapi -include ./include/linux/kconfig.h -D__KERNEL__ -mlittle-endian -I/home/chinkey/git_stuff/opencpi/runtime/hdl/include -I/home/chinkey/git_stuff/opencpi/os/interfaces/include -Wformat=2 -D__ASSEMBLY__ -fno-PIE -DCONFIG_AS_LSE=1 -mabi=lp64 -DCC_HAVE_ASM_GOTO -Wa,-gdwarf-2   -c -o arch/arm64/kernel/vdso/note.o arch/arm64/kernel/vdso/note.S

source_arch/arm64/kernel/vdso/note.o := arch/arm64/kernel/vdso/note.S

deps_arch/arm64/kernel/vdso/note.o := \
  include/linux/uts.h \
    $(wildcard include/config/default/hostname.h) \
  include/generated/uapi/linux/version.h \
  include/linux/elfnote.h \

arch/arm64/kernel/vdso/note.o: $(deps_arch/arm64/kernel/vdso/note.o)

$(deps_arch/arm64/kernel/vdso/note.o):
