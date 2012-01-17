ACLOCAL_AMFLAGS = -I m4

SUBDIRS = system/ossie \
	  system/standardInterfaces \
          system/nodebooter \
	  system/customInterfaces \
	  system/c_wavLoader \
	  system/SigProc \
	  platform/domain \
	  platform/dtd \
	  platform/nodes/default_GPP_node \
	  platform/GPP \
	  waveforms/ossie_demo \
	  components/amplifier \
	  components/ChannelDemo \
	  components/RxDemo \
	  components/TxDemo

if WITHNB
 nbdir="$(prefix)/bin"
else
 nbdir="/usr/local/bin"
endif
dist-hook:
	-rm -rf `find $(distdir) -name .svn`

install-data-hook:
	-cp -rf $(prefix)/lib/* $(libdir)
	-rm -rf $(prefix)/lib
	-cp -rf $(prefix)/dom/xml/dtd/ $(prefix)/dev/xml/
	-mv $(prefix)/bin/nodeBooter $(nbdir)
	-mv $(prefix)/bin/c_wavLoader $(nbdir)
	-chown -R $(SUDO_USER).$(SUDO_USER) $(prefix)

uninstall-hook:
	-rm -rf $(prefix)/*
