# The default definitions for native RCC compilation
GCC_$(OCPI_TOOL_HOST)=gcc
GCCLINK_$(OCPI_TOOL_HOST)=gcc
# ifeq ($(RccTarget),Linux-MCS_864x)
# GCC=/opt/timesys/toolchains/ppc86xx-linux/bin/ppc86xx-linux-gcc
# GCCLINK=$(GCC)
# else ifeq ($(RccTarget),Linux-x86_32)
# GCC=gcc -m32
# GCCLINK=gcc -m32 -m elf_i386
# endif
