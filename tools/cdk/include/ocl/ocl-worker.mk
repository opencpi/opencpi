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

# Makefile for an OCL worker
ifndef OCL_WORKER_MK
OCL_WORKER_MK:=xxx
include $(OCPI_CDK_DIR)/include/ocl/ocl-make.mk
Model=ocl
OBJ:=.clo
# Default is that you are building in a subdirectory of all implementations
OclImplSuffix=-worker.h
OclSkelSuffix=_skel.cl
OclSourceSuffix=.cl
ifeq ($(shell uname),Linux)
BF=.so
SOEXT=.so
AREXT=.a
else
ifeq ($(shell uname),Darwin)
BF=.dylib
SOEXT=.dylib
AREXT=.a
endif
endif

CompiledSourceFiles+=$(foreach w,$(Workers),\
                        $(GeneratedDir)/$(word 1,$(w))_entry_point$(OclSourceSuffix))
ArtifactFile=$(BinaryFile)
# Artifacts are target-specific since they contain things about the binary
ArtifactXmlFile = $(TargetDir)/$(word 1,$(Workers))_assy-art.xml
ToolSeparateObjects:=yes
OcpiLibDir=$(OCPI_CDK_DIR)/lib/$(OclTarget)

# OpenCL does not allow for individual objects to be assembled into one binary
# So the link binary command becomes the "compile" command for OpenCL workers
LinkBinary= $(ToolsDir)/ocpiocl $(ExtraCompilerOptions) $(foreach i,$(IncludeDirs),-I$(abspath $i)) -o $$@ -t $(OclTarget) compile 
Compile_cl=echo '\#include "../'$$<'"'>$$@

include $(OCPI_CDK_DIR)/include/xxx-worker.mk
# Even though the entry point files are generated, they must be compiled last,
# so they are not added to generated source files.
OclAssemblyFile=$(GeneratedDir)/$(word 1,$(Workers))_assy.xml
$(OclAssemblyFile): | $(GeneratedDir)
	$(AT)(echo "<OclAssembly>"; \
	  for w in $(Workers); do echo "<instance Worker=\"$$w.xml\"/>"; done; \
	  echo "</OclAssembly>") > $@

# Different since it is in the targetdir
$(ArtifactXmlFile): $(OclAssemblyFile)
	@echo Generating artifact/runtime xml file \($(ArtifactXmlFile)\) for all workers in one binary
	$(AT)$(DYN_PREFIX) $(ToolsDir)/ocpigen -M $(TargetDir)/$(@F).deps \
	     -O $(call OclOs,$(OclTarget)) \
             -V $(call OclOsVersion,$(OclTarget)) \
             -H $(call OclArch,$(OclTarget)) \
	     -D $(TargetDir) \
	     $(XmlIncludeDirsInternal:%=-I%) \
	     -A $(OclAssemblyFile)

endif
