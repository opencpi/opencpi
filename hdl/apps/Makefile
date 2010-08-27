APPS=demoappC demoappE testpsd testwbr
all: $(APPS)

.PHONY: $(APPS) clean
$(APPS):
	make -C $@

clean:
	for a in $(APPS); do \
		make -C $$a clean; \
	done
