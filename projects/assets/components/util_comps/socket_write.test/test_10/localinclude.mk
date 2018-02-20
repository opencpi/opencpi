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

ifeq (,$(IP))
# Ensure it runs a LONG time and that it aborts
ifeq (,$(VG))
SIM_TIME?=120
else
SIM_TIME?=180
endif

# Opening 255 ports, plus rest of framework causes out of file descriptor problems...
# Slick! http://stackoverflow.com/a/12034077
CUR_ULIMIT_N := $(shell ulimit -n)

ifneq "2047" "$(word 1, $(sort 2047 $(CUR_ULIMIT_N)))"
  $(warning ulimit -n is currently $(CUR_ULIMIT_N), but this test requires 2048. Try running "ulimit -n 2048")
  SKIP_TEST := 1
endif

else
# IP not blank
ifneq (,$(filter testrun,$(MAKECMDGOALS)))
$(warning Cannot check remote ulimits. This test might fail. Try running "ulimit -n 2048" on target.)
endif
endif
