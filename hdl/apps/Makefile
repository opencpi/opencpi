APPS=demoappC demoappE testpsd testddc testwbr
all: $(APPS)

.PHONY: $(APPS) clean
$(APPS):
	make -C $@

clean:
	for a in $(APPS); do \
		make -C $$a clean; \
	done
