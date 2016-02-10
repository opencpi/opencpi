include $(OCPI_CDK_DIR)/include/util.mk

$(call OcpiIncludeProject)

ifeq ($(origin Applications),undefined)
  Applications:=$(call OcpiFindSubdirs,application) $(wildcard *.xml)
  $(call OcpiDbgVar,Applications)
endif

.PHONY: all test clean run

DOALL=$(AT)\
  set -e;\
  for i in $(filter-out %.xml,$(Applications)); do\
    echo ========$1 $$i: ; $(MAKE) --no-print-directory -C $$i $2;\
  done

OcpiUse=$(if $(filter undefined,$(origin OcpiRun$1_$2)),$(OcpiRun$1),$(OcpiRun$1_$2))
ifndef OcpiRunXML

  OcpiRunXML=$(strip \
    $(call OcpiUse,Before,$1) \
    $(if $2,ocpirun,$(OCPI_BIN_DIR)/ocpirun) \
    $(call OcpiUse,Args,$1) \
    $1 \
    $(call OcpiUse,After,$1))

endif

all:
	$(call DOALL,Building apps,)

clean:
	$(call DOALL,Cleaning,clean)

test:
	$(call DOALL,Building tests for,test)

run:
	$(call DOALL,Running,run)
	$(AT)$(and $(filter %.xml,$(Applications)),\
	       echo ======== Running local XML applications: $(filter %.xml,$(Applications)) \
               $(foreach a,$(basename $(filter %.xml,$(Applications))),\
                && echo "========= $(call OcpiRunXML,$a,x)" && $(call OcpiRunXML,$a)))
