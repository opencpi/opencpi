IDL_BIN = $(shell which omniidl)

all:
	$(IDL_BIN) -bpython -I../../ossie/idl -Wbpackage=ossie.custominterfaces -Wbinline ../charArray.idl
	$(IDL_BIN) -bpython -I../../ossie/idl -Wbpackage=ossie.custominterfaces -Wbinline ../customInterfaces.idl
	$(IDL_BIN) -bpython -I../../ossie/idl -Wbpackage=ossie.custominterfaces -Wbinline ../timingStatus.idl

EXTRA_DIST = setup.py

install-exec-hook:
	python setup.py install

distclean:
	rm -rf ossie

clean:
	rm -rf ossie

