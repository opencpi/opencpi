#!/bin/bash
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

# Choose the filetype for Quartus Tcl/QSF assignments based on the file-extension
# of arg1
file_type () {
  ext="${1##*.}"
  file=_FILE
  case $ext in
    (vhd)    echo VHDL$file;;
    (ip)     echo IP$file;;
    (qip)    echo QIP$file;;
    (hex)    echo HEX$file;;
    (sv)     echo SYSTEMVERILOG$file;;
    (qsf)    echo QSF$file;;
    (*)      echo VERILOG$file;;
  esac
}

library () {
  ext="${1##*.}"
  [ -n "$2" ] || exit 1
  if [ "$3" != "no_verilog_libraries" ]; then
    vlg_lib="-library $2"
  fi
  case $ext in
    (vhd)    echo -library $2;;
    (ip|qip) echo -library $2;;
    (hex)    echo -library $2;;
    (sv)     echo -library $2;;
    (*)      echo $vlg_lib;;
  esac
}

generate_assignment () {
  ext="${1##*.}"
  case $ext in
    (qsf)    echo source $1;;
    (*)      echo set_global_assignment -name `file_type $1` `library $1 $2 $3` \"$1\";;
  esac

}
# For each source file in arg2, create a Quartus Tcl assignment that
# includes the source file and its HDL library (arg1). It also includes
# the filetype which is determined by the file-extension and chosen in
# case statement in file_type()
hdl_files () {
  echo \# Assignments for $1 local sources files
  for f in ${@:2}; do
    echo `generate_assignment $f $1`;
  done;
}

hdl_files_no_vlg_libs () {
  echo \# Assignments for $1 local sources files without library name for VERILOG files
  for f in ${@:2}; do
    echo `generate_assignment $f $1 no_verilog_libraries`;
  done;
}
