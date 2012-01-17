ACLOCAL_AMFLAGS = -I m4
%.pp.idl : %.idl
	@CC@ -E -x c++ -I../ossie/idl $< > $@

BUILT_SOURCES = customInterfaces.pp.idl

dist_pkginclude_DATA = customInterfaces.idl \
timingStatus.idl charArray.idl

pkgconfigdir = $(libdir)/pkgconfig
dist_pkgconfig_DATA = customInterfaces.pc

CLEANFILES = customInterfaces.pp.idl

SUBDIRS = 

uninstall-hook:
	-rm -rf $(includedir)/customInterfaces
