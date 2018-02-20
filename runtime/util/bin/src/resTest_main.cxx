/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
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
