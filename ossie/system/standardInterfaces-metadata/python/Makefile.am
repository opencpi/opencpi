IDL_BIN = $(shell which omniidl)

all:
	$(IDL_BIN) -bpython -I/usr/local/include -Wbpackage=ossie.standardinterfacesMD -Wbinline ../realCharMD.idl
	$(IDL_BIN) -bpython -I/usr/local/include -Wbpackage=ossie.standardinterfacesMD -Wbinline ../complexShortMD.idl

install-data-local:
	rm -rf $(DESTDIR)$(pythondir)/ossie/standardinterfacesMD
	mkdir -p $(DESTDIR)$(pythondir)/ossie/standardinterfacesMD
	cp -R ossie/standardinterfacesMD $(DESTDIR)$(pythondir)/ossie

distclean:
	rm -rf ossie

clean:
	rm -rf ossie

