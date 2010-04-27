
#include <stdlib.h>
#include <stdio.h>
#include <CpiRes.h>

int main( int argc, char** argv )
{
  CPI::Util::ResAddrType addr[10];
  CPI::Util::MemBlockMgr mMgr(0,2048);

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
