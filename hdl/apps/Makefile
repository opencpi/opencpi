APPS=demoappC demoappE testpsd testddc
all: $(APPS)

.PHONY: $(APPS) clean
$(APPS):
	make -C $@

clean:
	for a in $(APPS); do \
		make -C $$a clean; \
	done
