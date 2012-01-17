IDL_BIN = $(shell which omniidl)

all : 
	$(IDL_BIN) -bpython -Wbpackage=ossie.cf ../cf.idl
	$(IDL_BIN) -bpython -Wbpackage=ossie.cf ../StandardEvent.idl
	$(IDL_BIN) -bpython -Wbpackage=ossie.cf ../PortTypes.idl

EXTRA_DIST = setup.py

install-exec-hook:
	python setup.py install

distclean:
	rm -rf ossie

clean:
	rm -rf ossie

