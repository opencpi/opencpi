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

# This mess is to NOT expand ${LIB} or $LIB if found in current LD_PRELOAD
ifneq (,$(LD_PRELOAD))
  ifneq (,$(L))
    $(error L is defined but it cannot be. Sorry.)
  endif
  ifneq (,$(LIB))
    $(error LIB is defined but it cannot be. Sorry.)
  endif
  export L=_X___LIB___X_
  export LIB=_X___LIB___X_
  export OLD_LD_PRELOAD1=$(subst _X___LIB___X_IB,\$$LIB,$(LD_PRELOAD))
  export OLD_LD_PRELOAD=$(subst _X___LIB___X_,\$$LIB,$(OLD_LD_PRELOAD1))
  unexport L
  unexport LIB
endif
SPOOF_HOSTNAME:=LD_PRELOAD="$(OCPI_PREREQUISITES_DIR)/myhostname/$(OCPI_TOOL_DIR)/lib/libmyhostname.so:$(OLD_LD_PRELOAD)"
ifdef NODE_NAME
MYHOSTNAME=$(NODE_NAME)_$(MYHOSTNAME_MNAME).jenkins
else
MYHOSTNAME=buildhost_$(MYHOSTNAME_MNAME)
endif
MYHOSTNAME:=$(MYHOSTNAME).fakehostname
export MYHOSTNAME

