DOALL=for i in *; do if test $$i != Makefile; then echo ========$1 $$i: ; make -C $$i $2; fi; done
all:
	$(call DOALL,Building,)
clean:
	$(call DOALL,Cleaning,clean)
run:
	$(call DOALL,Running,run)




