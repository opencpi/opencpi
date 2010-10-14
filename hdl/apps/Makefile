APPS=demoappC demoappE testpsd testddc
# testwbr
all: $(APPS)

TARGETS=xc5vlx50t-1-ff1136 xc5vsx95t-2-ff1136 xc6vlx240t-1-ff1156

.PHONY: $(APPS) clean
$(APPS):
	for a in $(APPS); do \
		for t  in $(TARGETS); do \
			make -C $$a Target=$$t; \
		done; \
	done

clean:
	for a in $(APPS); do \
		for t  in $(TARGETS); do \
			make -C $$a Target=$$t clean; \
		done \
	done
