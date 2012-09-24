
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

# This primitive library contains VHDL primitive modules that are used
# by all workers
# These need to be in order if tools aren't very smart
LibName=ocpi
SourceFiles=ocpi.vhd ocpi_impl.vhd gen/ocpi_types.vhd ocpi_types_body.vhd gen/ocpi_types_impl.vhd
include $(OCPI_CDK_DIR)/include/hdl/hdl-lib.mk # we are a primitive library

CPPFiles=$(wildcard *.cpp_vhd)
GenFiles=$(CPPFiles:%.cpp_vhd=gen/%.vhd)
$(GenFiles): gen/%.vhd : %.cpp_vhd | gen
	$(AT)echo Preprocessing $< into $@:
	$(AT)gcc -E -P -std=c99 -xc \
        -imacros $$OCPI_BASE_DIR/core/local/util/property/include/OcpiUtilDataTypesApi.h \
	-o foo $< ;  tr '$$' '\n' < foo | tr '@' ' ' | tr '~' "'" | sed '/^ *$$/d' > $@

#gen:
#	@mkdir -p gen

