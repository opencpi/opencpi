# This file is the make file for a group of component libraries

include $(OCPI_CDK_DIR)/include/util.mk

ifndef Libraries
  Libraries=$(foreach d,$(shell find . -type d -depth 1),\
              $(filter lib library,$(call OcpiGetDirType,$(notdir $d))))
endif

DoLibGoal=$(AT)set -e; for l in $(Libraries); do $(MAKE) -C $$l $$@; done

.PHONY: all test clean $(Models)

all:
	$(call DoLibGoal,,$(MAKE))

$(Models) test clean
	$(call DoLibGoal,$@,$(MAKE))
