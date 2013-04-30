DOALL=set -e; for i in $(filter-out %.mk,$(wildcard *[^~])); do if test $$i != Makefile; then echo ========$1 $$i: ; make -C $$i $2; fi; done
#DOALL=set -e; for i in hello; do if test $$i != Makefile; then echo ========$1 $$i: ; make -C $$i $2; fi; done
all:
	$(call DOALL,Building,)
clean:
	$(call DOALL,Cleaning,clean)
run:
	$(call DOALL,Running,run)





