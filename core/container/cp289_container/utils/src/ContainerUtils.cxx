// Copyright (c) 2009 Mercury Federal Systems.
// 
// This file is part of OpenCPI.
// 
// OpenCPI is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// OpenCPI is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.

#include <CpiRDTInterface.h>
#include <CpiPortMetaData.h>
#include <CpiContainerDataTypes.h>
#include <stdlib.h>
#include <stdio.h>


void CPI::RDT::printDesc( Desc_t& desc )
{
  printf("Descriptor:\n");
  printf("  Nbuffers = %d\n", desc.nBuffers );
  printf("  DataBufferBaseAddr = 0x%llx\n", (long long)desc.dataBufferBaseAddr );
  printf("  DataBufferPitch = %d\n", desc.dataBufferPitch );
  printf("  MetaDataBaseAddr = 0x%llx\n", (long long)desc.metaDataBaseAddr );
  printf("  MetaDataPitch = %d\n", desc.metaDataPitch );
  printf("  FullFlagBaseAddr = 0x%llx\n", (long long)desc.fullFlagBaseAddr );
  printf("  FullFlagSize = %d\n", desc.fullFlagSize );
  printf("  FullFlagPitch = %d\n", desc.fullFlagPitch );
  printf("  FullFlagValue = 0x%llx\n", (long long)desc.fullFlagValue );
}
