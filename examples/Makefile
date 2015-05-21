ifneq ($(filter $(MAKECMDGOALS),test),)
  ALL:=bias2_xml bias_xml cat_xml copy ext2file_xml fsk_modem_xml hello hello_xml ptest vsadd xml
else
  ALL:=$(filter-out %.mk,$(wildcard *[^~]))
endif

DOALL=$(AT)\
  set -e;\
  for i in $(ALL); do\
    if test $$i != Makefile; then\
      echo ========$1 $$i: ;\
      $(MAKE) --no-print-directory -C $$i $2;\
    fi;\
  done

all:
	$(call DOALL,Building,)
clean:
	$(call DOALL,Cleaning,clean)
test run:
	$(call DOALL,Running,run)





