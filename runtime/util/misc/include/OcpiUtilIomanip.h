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

/**
  @file

  @brief
    Custom stream manipulators for printing formatted decimal, hexadecimal,
    and floating-point numbers.

  Revision History:

    10/13/2008 - Michael Pepe
                 Initial version.

************************************************************************** */

#ifndef INCLUDED_OCPI_UTIL_IOMANIP_H
#define INCLUDED_OCPI_UTIL_IOMANIP_H

#include <ios>
#include <iomanip>
#include <iostream>

namespace OCPI
{
  namespace Util
  {
    namespace Iomanip
    {
        namespace impl
        {
          /**
            @class CustomIoFormatter

            @brief
              Internal implementation class that provides the ability to
              supply custom stream manipulators with one or more arguments.

          **************************************************************** */

          template<typename T, typename C>
          class CustomIoFormatter
          {
            public:
              CustomIoFormatter ( std::basic_ostream<C>& ( *p_fn )
                                  ( std::basic_ostream<C>&, T ),
                                  T arg )
              throw ()
                : d_p_format_fn ( p_fn ),
                  d_arg1 ( arg ),
                  d_arg2 ( 0 )
              {
                // Empty
              }

              CustomIoFormatter ( std::basic_ostream<C>& ( *p_fn )
                                  ( std::basic_ostream<C>&, T, T ),
                                  T arg1,
                                  T arg2 )
              throw ()
                : d_p_format_fn ( p_fn ),
                  d_arg1 ( arg1 ),
                  d_arg2 ( arg2 )
              {
                // Empty
              }

              void operator( ) ( std::basic_ostream<C>& os ) const
              throw ()
              {
                d_p_format_fn ( os, d_arg1, d_arg2 );
              }

            private:

              std::basic_ostream<C>& ( *d_p_format_fn )
                                            ( std::basic_ostream<C>&, T, T );
              //!< Function pointer to the custom stream manipulator.
              T d_arg1;
              //!< First argument to the stream manipulator.
              T d_arg2;
              //!< Second argument to the stream manipulator.

          }; // End: CustomIoFormatter

          template<typename T, typename C>
          std::basic_ostream<C>& operator<< ( std::basic_ostream<C>& os,
                                      const CustomIoFormatter<T, C>& fmt )
          throw ()
          {
             fmt ( os );

             return os;
          }

          /**
            @brief
              Internal helper function to print an integer in decimal
              using the specified width.

              @param [ in ] os
                            Output stream to format.

              @param [ in ] width
                            Number of spaces the output will occupy.

          **************************************************************** */

          inline
          std::ostream& dec_impl ( std::ostream& os, int width, int pad )
          throw ()
          {
             return os << std::dec
                       << std::right
                       << std::setw ( width )
                       << std::setfill ( ( char ) pad );
          }

          /**
            @brief
              Internal helper function to print an integer in hexadecimal
              using the specified width.

              @param [ in ] os
                            Output stream to format.

              @param [ in ] width
                            Number of spaces the output will occupy.

          **************************************************************** */

          inline
          std::ostream& hex_impl ( std::ostream& os, int width, int )
          throw ()
          {
            return os << "0x"
                      << std::hex
                      << std::right
                      << std::setw ( width )
                      << std::setfill ( '0' );
          }

          /**
            @brief
              Internal helper function to print a floating-point number
              using the specified width and precision.

              @param [ in ] os
                            Output stream to format.

              @param [ in ] width
                            Number of spaces the output will occupy.

              @param [ in ] precision
                            Number of digits after the decimal point.

          **************************************************************** */

          inline
          std::ostream& flt_impl ( std::ostream& os,
                                   int width,
                                   int precision )
          throw ()
          {
            return os << std::setw ( width )
                      << std::right
                      << std::setprecision ( precision )
                      << std::setfill ( ' ' )
                      << std::setiosflags ( std::ios::fixed )
                      << std::setiosflags ( std::ios::showpoint );
          }

        } // End: namespace impl

      /**
        @brief
          Print an integer in decimal using the specified width.

          @param [ in ] width
                        Number of spaces the output will occupy.

      ******************************************************************** */

      inline
      impl::CustomIoFormatter<int, char> dec ( int width = 10,
                                                 int pad = ' ' )
      throw ()
      {
        return impl::CustomIoFormatter<int, char> ( impl::dec_impl,
                                                      width,
                                                      pad );
      }

      /**
        @brief
          Print an integer in hexadecimal using the specified width.

          @param [ in ] width
                        Number of spaces the output will occupy.

      ******************************************************************** */

      inline
      impl::CustomIoFormatter<int, char> hex ( int width = 8 )
      throw ()
      {
        return impl::CustomIoFormatter<int, char> ( impl::hex_impl,
                                                      width,
                                                      0 );
      }

      /**
        @brief
          Print a floating-point number using the specified width and
          precision.

          @param [ in ] width
                        Number of spaces the output will occupy.

          @param [ in ] precision
                        Number of digits after the decimal point.

      ******************************************************************** */

      inline
      impl::CustomIoFormatter<int, char> flt ( int width = 16,
                                               int precision = 8 )
      throw ()
      {
        return impl::CustomIoFormatter<int, char> ( impl::flt_impl,
                                                      width,
                                                      precision );
      }

    } // End: namespace Iomanip

  } // End: namespace Util

} // End: namespace OCPI

#endif // End: #ifndef INCLUDED_OCPI_UTIL_IOMANIP_H

