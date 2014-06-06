# The default definitions for native RCC compilation
Gc_$(OCPI_TOOL_HOST)=gcc -std=c99
Gc++_$(OCPI_TOOL_HOST)=g++
Gc_LINK_$(OCPI_TOOL_HOST)=gcc
Gc++_LINK_$(OCPI_TOOL_HOST)=g++
# ifeq ($(RccTarget),Linux-MCS_864x)
# GCC=/opt/timesys/toolchains/ppc86xx-linux/bin/ppc86xx-linux-gcc
# GCCLINK=$(GCC)
# else ifeq ($(RccTarget),Linux-x86_32)
# GCC=gcc -m32
# GCCLINK=gcc -m32 -m elf_i386
# endif
