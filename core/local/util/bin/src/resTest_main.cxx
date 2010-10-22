
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


#include <stdlib.h>
#include <stdio.h>
#include <OcpiRes.h>

int main( int argc, char** argv )
{
  ( void ) argc;
  ( void ) argv;
  OCPI::Util::ResAddrType addr[10];
  OCPI::Util::MemBlockMgr mMgr(0,2048);

  if ( mMgr.alloc( 10, 16, addr[0] ) ) {
    printf("Error: Could not allocate from the resource pool\n");
    return -1;
  }
  printf("Got address %lld from pool\n", (long long)addr[0] );

  if ( mMgr.alloc( 35, 16, addr[1] ) ) {
    printf("Error: Could not allocate from the resource pool\n");
    return -1;
  }
  printf("Got address %lld from pool\n", (long long)addr[1] );

  
  if ( mMgr.free( addr[1] ) ) {
    printf("Error: Could not allocate from the resource pool\n");
    return -1;
  }

  if ( mMgr.alloc( 35, 16, addr[1] ) ) {
    printf("Error: Could not allocate from the resource pool\n");
    return -1;
  }
  printf("Got address %lld from pool\n", (long long)addr[1] );

  if ( mMgr.free( addr[0] ) ) {
    printf("Error: Could not allocate from the resource pool\n");
    return -1;
  }

  if ( mMgr.alloc( 10, 16, addr[0] ) ) {
    printf("Error: Could not allocate from the resource pool\n");
    return -1;
  }
  printf("Got address %lld from pool\n", (long long)addr[0] );


  if ( mMgr.alloc( 1000, 16, addr[2] ) ) {
    printf("Error: Could not allocate from the resource pool\n");
    return -1;
  }
  printf("Got address %lld from pool\n", (long long)addr[2] );

  if ( mMgr.alloc( 100, 16, addr[3] ) ) {
    printf("Error: Could not allocate from the resource pool\n");
    return -1;
  }
  printf("Got address %lld from pool\n", (long long)addr[3] );

}
