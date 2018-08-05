# This file is protected by Copyright. Please refer to the COPYRIGHT file
# distributed with this source distribution.
#
# This file is part of OpenCPI <http://www.opencpi.org>
#
# OpenCPI is free software: you can redistribute it and/or modify it under the
# terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License along
# with this program. If not, see <http://www.gnu.org/licenses/>.

# This file will run when "import opencpi" is done.
# We always load the (single) aci swig module now.
# In the future if we have more subpackages we can use one of the variety of techniques
# for lazy import of subpackages.

# This file is only functional when exported, and with swig libraries collocated.
import sys
old=sys.getdlopenflags()
if sys.platform != 'darwin':
   import ctypes
   sys.setdlopenflags(old|ctypes.RTLD_GLOBAL)
import aci
sys.setdlopenflags(old)

