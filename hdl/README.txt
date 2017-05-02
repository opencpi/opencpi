
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

This tree holds code and tools specific to the HDL model and associated hardware (FPGAs).

The "primitives" directory supplies low level/primitive utility libraries
and cores used to build application workers, device workers and other infrastructure modules.

The "assemblies" directory holds assembly configurations built into bitstreams, consisting of
workers, which will build/synthesize for particular platforms.  The workers in such an assembly
are essentially a pre-connected, pre-instanced subset of an application.

The "devices" directory holds device workers that may use primitives, and are optionally
used in platforms (i.e. they are included when an app needs them, but not otherwise).
Examples are dram (with memory controller), gbe, adc, dac, flash memory, etc.

The "cards" directory holds specs and device workers for "plug-in" cards that plug into
slots in a platform.  E.g. FMC cards.

The "platforms" directory holds platform IP and parameters used to build "platform cores" that
then get combined with applications to make bitstreams.  A "platform" is a particular chip on
a board with various devices attached.  Since most currently supported boards have one FPGA,
normally a platform is a chip on a board, but when there are multiple supported FPGAs on a
board, a "platform" is not a board, but one of the chips on the board.

HDL OpenCPI components/workers are not built here since they are built under the
top level "components" directory where multiple model implementations
of the same components are built (a heterogeneous component library).

Device workers are build in either the "devices" directory or the "cards" directory.
The platform workers are build in the directory under "platforms" for that platform.


