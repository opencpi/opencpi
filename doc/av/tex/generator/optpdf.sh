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
#
# optpdf file.pdf
#   This script will attempt to optimize the given pdf
#
# Based on
# http://tex.stackexchange.com/a/199150
#
# Modified to return exit code, allowing:
# while ./optpdf.sh Picoflexor_T6A_Notes.pdf; do :; done
#
if [ -z "${MYLOC}" ]; then
  export MYLOC=.
fi
if [ ! -e ${MYLOC}/pdfmarks ]; then
  echo "Cannot find ${MYLOC}/pdfmarks file. Use makepdf.sh."
  exit 1
fi

file=$1
filebase=$(basename $file .pdf)
optfile=/tmp/$$-${filebase}_opt.pdf
gs -sDEVICE=pdfwrite -dPDFSETTINGS=/prepress -dCompatibilityLevel=1.4 -dNOPAUSE -dQUIET -dBATCH \
        -sOutputFile=${optfile} ${file} <(sed -e "s/@TITLE@/$(echo ${filebase} | tr _ ' ')/" ${MYLOC}/pdfmarks)
if [ $? == '0' ]; then
    optsize=$(stat -c "%s" ${optfile})
    orgsize=$(stat -c "%s" ${file})
    if [ "${optsize}" -eq 0 ]; then
        echo "No output!  Keeping original"
        rm -f ${optfile}
        exit 1
    fi
    if [ ${optsize} -ge ${orgsize} ]; then
        echo "Didn't make it smaller! Keeping original"
        rm -f ${optfile}
        exit 1
    fi
    bytesSaved=$(expr $orgsize - $optsize)
    percent=$(expr $optsize '*' 100 / $orgsize)
    echo Saving $bytesSaved bytes \(now ${percent}% of old\)
    rm ${file}
    mv ${optfile} ${file}
fi
exit 0
