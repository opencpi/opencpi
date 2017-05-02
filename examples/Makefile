ifneq ($(filter $(MAKECMDGOALS),test),)
  ALL:=bias2_xml bias_xml cat_xml copy ext2file_xml fsk_modem_xml hello hello_xml ptest vsadd xml
else
  ALL:=$(wildcard *[^~])
endif

DOALL=$(AT)\
  set -e;\
  set -o pipefail;\
  for i in $(ALL); do\
    if test -d $$i; then\
      echo ========$1 $$i: ;\
      export TGT=$2;\
      $(MAKE) --no-print-directory -C $$i $2 2>&1 | tee $${i}_$${TGT:-build}.log;\
    fi;\
  done

all:
	$(call DOALL,Building,)
clean:
	$(call DOALL,Cleaning,clean)
	$(AT)rm -r -f *.log
test run:
	$(call DOALL,Running,run)
