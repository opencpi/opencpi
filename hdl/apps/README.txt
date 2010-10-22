
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

This is where the HDL/FPGA apps go.
All apps result in a synthesized core called ocpi_app, with a precompiled library with the stub for ocpi_app, and an ngc file.
The precompoiled stub library enables the container that calls ocpi_app to find it during compilation.
The ngc file enables the container to actually include the ocpi_app core during synthesis.

To use these app in the xst/ngc build process: (this is preliminary and a bit messy right now).

For xst in a shep drop:

1. setenv OCPI_BASE_DIR to top of opencpi tree above hdl/apps etc. (and remove it from any .cshrc)
2. to make "board" (like ml555) until it fails or just interrupt it
3. go to build/tmp-<board>
4. add "app" to the end of the fpgaTop.lso file
5. add "app=$OCPI_BASE_DIR/hdl/apps/<theapp>/lib/hdl/virtex[5|6]" to the .ini file
e.g.:
app=$OCPI_BASE_DIR/hdl/apps/testpsd/lib/hdl/virtex5

5a. make sure "bsv" and "ocpi" are also there as in:
e.g.:
bsv=$OCPI_BASE_DIR/ocpi/lib/hdl/bsv/virtex5
ocpi=$OCPI_BASE_DIR/ocpi/lib/hdl/ocpi/virtex5

6. add -sd $OCPI_BASE_DIR/hdl/apps/<theapp>/lib/hdl/xc5vsx95t-2-ff1136 to .xst file for the board
e.g.:
-sd $OCPI_BASE_DIR/hdl/apps/testpsd/lib/hdl/xc5vlx50t-1-ff1136

7. add -sd $OCPI_BASE_DIR/hdl/apps/<theapp>/lib/hdl/xc5vsx95t-2-ff1136 as an argument to ngdbuild in the build fpgaTop script.
e.g.:
  -sd $OCPI_BASE_DIR/hdl/apps/testpsd/lib/hdl/$PART

8. edit the project file to remove stuff and reference the correct mkOCApp.v file in apps
e.g.:
  remove any existing OCApp, or app workers
  remote any existing library primitives
9. run build_fpgaTop <board>




