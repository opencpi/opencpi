
/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCPI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <OcpiRDTInterface.h>
#include <OcpiPortMetaData.h>
#include <OcpiContainerDataTypes.h>
#include <stdlib.h>
#include <stdio.h>


void OCPI::RDT::printDesc( Desc_t& desc )
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
