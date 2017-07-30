This file is protected by Copyright. Please refer to the COPYRIGHT file
distributed with this source distribution.

This file is part of OpenCPI <http://www.opencpi.org>

OpenCPI is free software: you can redistribute it and/or modify it under the
terms of the GNU Lesser General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program. If not, see <http://www.gnu.org/licenses/>.

A component library, consisting of different models built for
different targets.

The expectation is that a library has spec XML in ./specs/ and
subdirectories for each implementation.  So for component abc, there
should be a ./specs/abc_specs.xml file, and if there is an rcc
implementation it will be in the directory abc.rcc.  If there is an
HDL implementation, it should should be in the subdirectory abc.hdl.

Active implementations (those are are built) are listed in the Make
variables for each model: RccImplementations, HdlImplementations,
XmImplementations, etc.  Thus, this makefile just names this library
and lists implementations to be built.

The Makefile also lists the targets per model.
