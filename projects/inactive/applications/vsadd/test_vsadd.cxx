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

#include "OcpiApi.hh"
namespace OA=OCPI::API;
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
      OA::PValue pvs[] = { OA::PVBool("verbose", true), OA::PVEnd };
      OA::Application app("<application>"
			  "  <instance component='ocpi.inactive.vsadd' externals='true'/>"
			  "</application>", pvs);
      app.initialize();
      OA::ExternalPort &xport_in = app.getPort("in");
      OA::ExternalPort &xport_out = app.getPort("out");
      OA::Property p(app, "vsadd.scalar");

      float v = p.getFloatValue ( );

      v = p.getFloatValue ( );
      printf ( "scalar %f init\n", v ); fflush(stdout);

      app.start(); // Start the worker running

      v = p.getFloatValue();
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

        OA::ExternalBuffer* src = xport_in.getBuffer ( data, length );

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
        OA::ExternalBuffer* dst = xport_out.getBuffer ( data,
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

      app.stop(); // Stop the app

      v = p.getFloatValue ( );
      printf ( "scalar %f stop\n", v );
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
