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

/*
 * Abstact:
 *   This file contains the Implementation for the block partition class.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 7/2004
 *    Revision Detail: Created
 *
 */

#include <CpiIntIndivisiblePartition.h>

using namespace CPI::DataTransport;

IndivisiblePartition::IndivisiblePartition(){}
IndivisiblePartition::~IndivisiblePartition(){}


/**********************************
 * Given the inherit distribution and partition information
 * calculate the offsets into the requested buffers for distribution.
 *
 * returns 0 on success.
 **********************************/
CPI::OS::int32_t IndivisiblePartition::calculateBufferOffsets( 
                                                              CPI::OS::uint32_t                ,                                
                                                              Buffer     *,                           
                                                              Buffer     *,                        
                                                              BufferInfo **)       
{

  return -1;
}

