
#include "OcpiApi.hh"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <cassert>

#include <string>
#include <vector>
#include <iomanip>
#include <iostream>
#include <algorithm>

namespace
{
  template<typename T>
  class GenRand
  {
    public:
      explicit GenRand ( )
      {
        // Empty
      }

      T operator() ( )
      {
        return static_cast<T> ( rand() );
      }
  };

  template<typename T>
  class GenRamp
  {
    public:
      explicit GenRamp ( T start )
        : ramp ( start )
      {
        // Empty
      }

      T operator() ( )
      {
        return ramp++;
      }

    private:
      T ramp;
  };


  template<typename T>
  void print ( const T& v )
  {
    std::cout << v << " " ;
  }

  void test_worker ( const char* container_name,
                     std::size_t n_buffers_to_process )
  {
      // Find a container to run our worker
     // (it returns a pointer since it might return NULL)
      OCPI::API::Container* c =
                        OCPI::API::ContainerManager::find ( container_name );
      if ( !c )
      {
        throw std::string ( "Failed to find an OCL container" );
      }

      // Create an application context on that container
      // (and delete it when we exit)
      // (it returns a pointer since the caller is allowed to delete it)
      OCPI::API::ContainerApplication& a = *c->createApplication ( "vsadd-app" );

      // Create a worker of type "vsadd", whose name in this
      // app will be "vsadd_instance".
      OCPI::API::Worker& w = a.createWorker ( "vsadd", // vsadd_instance
                                              "ocpi.vsadd");

      // Get a handle (reference) on the "in" port of the worker
      OCPI::API::Port& port_in = w.getPort ( "in" );
      // Get an external port (that this program can use) connected to
      // the "in" port.

      OCPI::API::ExternalPort& xport_in = port_in.connectExternal ( );

      // Get a handle (reference) on the "out" port of the worker
      OCPI::API::Port& port_out = w.getPort ( "out" );

      // Get an external port (that this program can use) connected to
      // the "out" port.
      OCPI::API::ExternalPort& xport_out = port_out.connectExternal ( );

      OCPI::API::Property p ( w, "scalar" );

      float v = p.getFloatValue ( );

      v = p.getFloatValue ( );
      printf ( "scalar %f init\n", v ); fflush(stdout);

      w.start ( ); // Start the worker running

      v = p.getFloatValue ( );
      printf ( "scalar %f start\n", v ); fflush(stdout);

      std::size_t n_buffers_processed ( 0 );

      std::vector<float> recv_data;
      recv_data.reserve ( 4096 );

      p.setFloatValue ( 2.0 );

      printf ("before loop\n" );

      size_t n_generated ( 1 );

      do
      {
        uint8_t* data;
        size_t length;
        uint8_t opcode = 0;
        bool end = false;

        std::vector<float> sent_data;

        OCPI::API::ExternalBuffer* src = xport_in.getBuffer ( data, length );

        p.setFloatValue ( ( float ) n_buffers_processed + 1 );

        if ( src )
        {
          float ramp_start = pow ( 10, n_generated++ );
          printf ( "host src %p data %p length %zu %f\n", src, data, length, ramp_start ); fflush(stdout);
          const std::size_t n_elems ( length / sizeof ( float ) );
          sent_data.reserve ( n_elems );
          std::generate_n ( sent_data.begin ( ), n_elems, GenRamp<float> ( ramp_start ) );
          memcpy ( data, (void*) &sent_data [ 0 ], length );
          src->put ( length, opcode, end );
        }

        data = 0;
        length = 0;
        OCPI::API::ExternalBuffer* dst = xport_out.getBuffer ( data,
                                                               length,
                                                               opcode,
                                                               end );
        if ( dst )
        {
          printf ( "host dst %p data %p length %zu\n", dst, data, length ); fflush(stdout);

          float* pf32 = ( float* ) data;
          for ( size_t n = 0; n < ( length / sizeof ( float) ); n++ )
          {
            if ( n == 16 ) break;
            printf ( "%zu %4zu %f\n", n_buffers_processed, n, pf32 [ n ] );
          }
          pf32 [ 0 ] = -1.0;

          // Check the data
          dst->release ( );
          n_buffers_processed++;

        }
      }
      while ( n_buffers_processed < n_buffers_to_process );

      v = p.getFloatValue ( );
      printf ( "scalar %f post loop\n", v );

      w.stop ( ); // Stop the worker running

      v = p.getFloatValue ( );
      printf ( "scalar %f stop\n", v );

      w.release ( ); // Release the worker running

      v = p.getFloatValue ( );
      printf ( "scalar %f release\n", v );
  }

} // End: namespace<unamed>

int main ( int argc, char* argv [ ] )
{
  std::cout << "\n\n** Start of main() **************************************************\n"
            << std::endl;
  try
  {
      const std::size_t n_buffers_to_process ( 5 );

      test_worker ( "ocl", n_buffers_to_process );
  }
  catch ( const std::string& s )
  {
    std::cerr << "\nException(s): " << s << std::endl;
    return 1;
  }
  catch ( ... )
  {
    std::cerr << "\nException(u): unknown" << std::endl;
    return 1;
  }

  std::cout << "\nDone\n" << std::endl;

  std::cout << "\n\n** End of main() **************************************************\n"
            << std::endl;

  return 0;
}
