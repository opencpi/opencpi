#!/bin/csh
# Copyright (c) 2009 Mercury Federal Systems.
#
# This file is part of OpenCPI.
#
# OpenCPI is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# OpenCPI is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.

set LOCAL_OMNIDIR=/usr/local
#set OMNIDIR_IDL=/opt/omniORB
set OMNIDIR_IDL=/home/jmiller/corba/omniORB-4.1.3
set echo
set verbose
set l=$argv[$#]
$LOCAL_OMNIDIR/bin/omniidl -bcxx -Wbh=.h -Wbd=.cxx -Wbs=.cxx -Wba -I$OMNIDIR_IDL/idl/ $*
cp $l:r.h $l:r_s.h
