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

.PHONY: all
all: ocs.pdf

clean:
	rm -rf *.tex *.inc *.pdf *.aux *.log *.out

ocs.tex: ocs.xml
	../../docGen.py $<

developer_doc.inc: ocs.tex
component_spec_properties.inc: ocs.tex
component_ports.inc: ocs.tex
worker_properties.inc: ocs.tex
worker_interfaces.inc: ocs.tex

ocs.pdf: ocs.tex developer_doc.inc component_spec_properties.inc component_ports.inc worker_properties.inc worker_interfaces.inc
	rubber -d $<
