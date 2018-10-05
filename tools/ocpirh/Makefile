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

# This Makefile creates the executable proxy for wrapping OpenCPI apps as REDHAWK components
# It needs a partial redhawk installation to create this executable, but not the IDE.
# The scripts/install-redhawk.sh script will install the necessary files to build, but
# not a complete installation for actually using redhawk.
# NOTE:  The redhawk installation is not sandboxed unfortunately, it is global.
# The resulting executable and associated files represent the necessary artifacts to later
# export OpenCPI apps as RH components.  This is accomplished by the ocpirh_export command.
# This latter (user) step does NOT require a RH installation.

# This Makefile depends on a source distribution of OpenCPI, not just an installed CDK
# The executable will be referenced by all component packages since it is the generic
# wrapper executable that runs the OpenCPI application as a REDHAWK component.

# This is where we expect the redhawk code generator to be installed.
rhcg=/usr/local/redhawk/core/bin/redhawk-codegen
# This name must match the 2 existing source files $a_base.h and $a_base.cpp
ifeq ($(wildcard $(rhcg)),)
  $(warning RedHawk code generation is not installed.  RedHawk proxy not built.)
  all:
else ifneq ($(OCPI_TARGET_PLATFORM),$(OCPI_TOOL_PLATFORM))
  $(warning RedHawk export can not be built for cross-compiled environments.)
  all:
else
AT:=@
t:=target-$(OCPI_TARGET_DIR)
a:=ocpirh_proxy
all: $t/$a

# Create a copy of the "nothing" app with the right name
gen/$a.xml: $(OCPI_CDK_DIR)/examples/xml/nothing.xml
	$(AT)mkdir -p gen
	$(AT)cp $< $@

# Create a deployment file from the "nothing" app.
gen/$a-depl.xml: gen/$a.xml
	$(AT)echo ============== Generating the '"nothing"' app deployment XML file $a.
	$(AT)ocpirun --deploy-out=$@ --no-execute $<

# Create the SCA XML files for building this generic executable
$t/$a.spd.xml: gen/$a-depl.xml
	$(AT)echo ============== Generating the generic SCA XML files for $a.
	$(AT)mkdir -p $t
	$(AT)$(OCPI_CDK_DIR)/bin/$(OCPI_TOOL_DIR)/ocpisca -x -D $t $<

# Just use the last created file as the target
$t/cpp/$a_base.h: $t/$a.spd.xml $a_base.h $a_base.cpp
	$(AT)echo ============== Generating a RH component skeleton for $a using redhawk-codegen.
	$(AT)sh genproxy.sh $a $t $(rhcg)
	$(AT)# Make our source code available to the build where it wants it
	$(AT)cp $a_base.h $t/cpp
	$(AT)cp $a_base.cpp $t/cpp

LDEPS=$(foreach l,$(olibs),\
        $(firstword $(foreach s,.a .so,\
                      $(foreach f,$(OCPI_CDK_DIR)/lib/$(OCPI_TARGET_DIR)/libocpi_$l$s,\
			$(wildcard $f)))))
$t/cpp/$a: $t/cpp/$a_base.h $(LDEPS)
	$(AT)echo ============== Building the generic component executable in $@
	$(AT)touch $t/cpp/$a_base.h
	$(AT)cd $t && source /etc/profile.d/redhawk.sh && ./build.sh

# Put the program where the build machinery expects to find main program binaries,
# and put a placeholder main source file where it will be found
$t/$a: $t/cpp/$a
	$(AT)echo ============== Linking proxy binary $a for export as $@
	$(AT)rm -f $@
	$(AT)ln -s cpp/$a $@

endif

clean:
	rm -r -f gen target-* lib
