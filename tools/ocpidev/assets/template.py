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
"""
This module holds the templates for assset creation.  these in theroy could be moved out to seperate
files in the future if that makes sence.  This file is intended to be easily editable by people who
know nothing about ocpidev internals but want to change templates for outputs of ocpidev create
"""

PROJ_EXPORTS="""
# This file specifies aspects of this project that are made available to users,
# by adding or subtracting from what is automatically exported based on the
# documented rules.
# Lines starting with + add to the exports
# Lines starting with - subtract from the exports
all

\n\n"""
PROJ_GIT_IGNORE = """
# Lines starting with '#' are considered comments.
# Ignore (generated) html files,
#*.html
# except foo.html which is maintained by hand.
#!foo.html
# Ignore objects and archives.
*.rpm
*.obj
*.so
*~
*.o
target-*/
*.deps
gen/
*.old
*.hold
*.orig
*.log
lib/
#Texmaker artifacts
*.aux
*.synctex.gz
*.out
**/doc*/*.pdf
**/doc*/*.toc
**/doc*/*.lof
**/doc*/*.lot
run/
exports/
imports
*.pyc
\n\n"""

PROJ_GIT_ATTR = """
*.ngc -diff
*.edf -diff
*.bit -diff
\n\n"""

PROJ_PROJECT_MK ="""
# This Makefile fragment is for the {{name}} project

# Package identifier is used in a hierarchical fashion from Project to Libraries....
# The PackageName, PackagePrefix and Package variables can optionally be set here:
# PackageName defaults to the name of the directory
# PackagePrefix defaults to local
# Package defaults to PackagePrefix.PackageName
#
# ***************** WARNING ********************
# When changing the PackageName or PackagePrefix of an existing project the
# project needs to be both unregistered and re-registered then cleaned and
# rebuilt. This also includes cleaning and rebuilding any projects that
# depend on this project.
# ***************** WARNING ********************
#
{%if package_name: %}
PackageName={{package_name}}
{% endif %}
{%if package_prefix: %}
PackagePrefix={{package_prefix}}
{% endif %}
{%if package_id: %}
Package={{package_id}}
{% endif %}
ProjectDependencies={{depend}}
{%if prim_lib: %}
Libraries={{prim_lib}}
{% endif %}
{%if include_dir: %}
IncludeDirs={{include_dir}}
{% endif %}
{%if xml_include: %}
XmlIncludeDirs={{xml_include}}
{% endif %}
{%if comp_lib: %}
ComponentLibraries={{comp_lib}}
{% endif %}
\n\n"""

PROJ_MAKEFILE= ("""
$(if $(realpath $(OCPI_CDK_DIR)),,\\
  $(error The OCPI_CDK_DIR environment variable is not set correctly.))
# This is the Makefile for the {{name}} project.
include $(OCPI_CDK_DIR)/include/project.mk
\n\n""")

PROJ_GUI_PROJECT = ("""
<?xml version="1.0" encoding="UTF-8"?>
<projectDescription>
  <name>{{determined_package_id}}</name>
  <comment></comment>
  <projects></projects>
  <buildSpec></buildSpec>
  <natures></natures>
</projectDescription>
\n\n""")
