# This file is the make file for a group of component libraries

include $(OCPI_CDK_DIR)/include/util.mk

ifndef Libraries
  Libraries=$(call OcpiFindSubdirs,library)
endif

DoLibGoal=$(AT)set -e; for l in $(Libraries); do echo ====== Entering library $$l for goal: $(@); $(MAKE) -C $$l $(@); done

Goals=all test clean $(Models) $(Models:%=clean%)

.PHONY: $(Goals)

$(Goals):
	$(call DoLibGoal,$(MAKE))

