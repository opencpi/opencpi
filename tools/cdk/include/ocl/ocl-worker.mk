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
ArtifactXmlFile=$(call WkrTargetDir,$1,$2)/$(word 1,$(Workers))_assy-art.xml
ToolSeparateObjects:=yes
OcpiLibDir=$(OCPI_CDK_DIR)/lib/$(OclTarget)

# OpenCL does not allow for individual objects to be assembled into one binary
# So the link binary command becomes the "compile" command for OpenCL workers
# The -I$(CURDIR)/gen below works when -Igen does not, in some cases.  I do not know why
# The -I. below is needed by nvidia
LinkBinary= $(ToolsDir)/ocpiocl $(ExtraCompilerOptions) \
  -I. \
  -I$(CURDIR)/gen \
  -I$(OCPI_CDK_DIR)/include/ocl \
  $(foreach i,$(IncludeDirs),-I$i) \
  -o $@ -t $(OclTarget) compile $1
Compile_cl=echo '\#include "'$$<'"'>$$@
#Compile_cl=echo '\#include "../'$$<'"'>$$@

include $(OCPI_CDK_DIR)/include/xxx-worker.mk
# Even though the entry point files are generated, they must be compiled last,
# so they are not added to generated source files.
OclAssemblyFile=$(call WkrTargetDir,$1,$2)/$(word 1,$(Workers))_assy.xml

define DoOclArtifactFile

$(call OclAssemblyFile,$1,$2): | $(call WkrTargetDir,$1,$2)
	$(AT)(echo "<OclAssembly>"; \
	  for w in $$(Workers); do \
            echo "<instance Worker=\"$$$$w.xml\" paramconfig=\"$2\"/>"; \
          done; \
	  echo "</OclAssembly>") > $$@

# Different since it is in the targetdir
$(call ArtifactXmlFile,$1,$2): $(call OclAssemblyFile,$1,$2)
	$(AT)echo Generating artifact/runtime xml file $$@ for all workers in one binary
	$(AT)$(DYN_PREFIX) $(ToolsDir)/ocpigen $(call OcpiFixPathArgs,-M $(call WkrTargetDir,$1,$2)/$$(@F).deps \
	     -O $(call OclOs,$1) \
             -V $(call OclOsVersion,$1) \
             -H $(call OclArch,$1) \
	     -P $3 \
	     -D $(call WkrTargetDir,$1,$2) \
             $(XmlIncludeDirsInternal:%=-I%) \
             -A $(call OclAssemblyFile,$1,$2))

endef

$(foreach p,$(OclPlatforms),$(foreach c,$(ParamConfigurations),$(eval $(call DoOclArtifactFile,$(OclTarget_$p),$c,$p))))

endif
