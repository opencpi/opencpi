
# #####
#
#  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
#
#    Mercury Federal Systems, Incorporated
#    1901 South Bell Street
#    Suite 402
#    Arlington, Virginia 22202
#    United States of America
#    Telephone 703-413-0781
#    FAX 703-413-0784
#
#  This file is part of OpenCPI (www.opencpi.org).
#     ____                   __________   ____
#    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
#   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
#  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
#  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
#      /_/                                             /____/
#
#  OpenCPI is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Lesser General Public License as published
#  by the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  OpenCPI is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public License
#  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
#
########################################################################### #

include $(OCPI_CDK_DIR)/include/hdl/hdl-make.mk

# We make this sort of like building a component library because
# for each application we are indeed making a "worker", and then we go
# on to make bitstreams for platforms.
LibDir=$(OutDir)lib
GenDir=$(OutDir)gen
ifndef OutDir
ifdef OCPI_OUTPUT_DIR 
PassOutDir=OutDir=$(call AdjustRelative,$(OutDir:%/=%))/$@
endif
endif

Applications=testbias testpsd testddc 
# These require adc/dac: demoappC demoappE
# testwbr

all: $(Applications)

.PHONY: $(Applications) $(Platforms) $(Targets) clean

$(Applications):
	$(AT)echo =============Building application $@ for all targets/platforms
	$(AT)$(MAKE) -C $@ HdlPlatforms="$(HdlPlatforms)" \
	       LibDir=$(call AdjustRelative,$(LibDir)) \
	       GenDir=$(call AdjustRelative,$(GenDir)) \
	       $(PassOutDir)


ifneq (,)
	$(AT)set -e;for t in $(Targets); do \
		 echo Making $@ for target $$t. ; \
		 make -C $@ Target=$$t; \
	       done
	$(AT)set -e;for p in $(Platforms); do \
		 echo Making $@ for platform $$p. ; \
		 make -C $@ Platform=$$p; \
	       done

$(Targets):
	$(AT)echo Building all apps for target $@ \(not platforms\)
	$(AT)set -e; for a in $(APPS); do \
		 echo Making $$a for target $@. ; \
		 make -C $$a Target=$@; \
	       done; \
	done

$(Platforms):
	$(AT)echo Building all apps for platform $@
	$(AT)set -e; for a in $(Applications); do \
		 echo Making $$a for platform $@. ; \
		 make -C $$a Platform=$@; \
	     done
endif

clean:
	$(AT)set -e;for a in $(Applications); do \
		echo Cleaning $$a ; \
		$(MAKE) -C $$a clean; \
		done
	$(AT)rm -r -f $(LibDir) $(GenDir)
