ACLOCAL_AMFLAGS = -I m4

LIBTOOL_DEPS = @LIBTOOL_DEPS@
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

docs :
	doxygen

EXTRA_DIST = ossie.pc.in LICENSE

SUBDIRS = idl parser framework include

pkgconfigdir = $(libdir)/pkgconfig
dist_pkgconfig_DATA = ossie.pc

uninstall-hook:
	-rm -rf $(includedir)/ossie
