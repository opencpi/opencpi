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

#include <iomanip>
#include <iostream>
#include <string>
#include <typeinfo>
#include "OcpiUtilTest.h"


/* ---- Implementation of the test class --------------------------------- */

OCPI::Util::Test::Test::Test ( const char * name, std::ostream* a_stream )
  : d_name ( name ) ,
    d_stream ( a_stream ),
    d_n_passed ( 0 ),
    d_n_failed ( 0 )
{
  // Empty
}

OCPI::Util::Test::Test::~Test ( )
{
  // Empty
}

void OCPI::Util::Test::Test::runTest ( )
{
  try {
    run ();
  }
  catch (const OCPI::Util::Test::Abort &) {
    // This has already been reported.
  }
  catch (const std::string & oops) {
    ++d_n_failed;

    if (d_stream) {
      *d_stream << (d_name ? d_name : typeid ( *this ).name ( ))
                << " failure: ("
                << oops
                << ")"
                << std::endl;
    }
  }
  catch (...) {
    ++d_n_failed;
    if (d_stream) {
      *d_stream << (d_name ? d_name : typeid ( *this ).name ( ))
                << " failure: ("
                << "unknown exception"
                << ")"
                << std::endl;
    }
  }
}

std::size_t OCPI::Util::Test::Test::n_passed ( ) const
{
  return d_n_passed;
}

std::size_t OCPI::Util::Test::Test::n_failed ( ) const
{
  return d_n_failed;
}

const std::ostream* OCPI::Util::Test::Test::stream ( ) const
{
  return d_stream;
}

void OCPI::Util::Test::Test::stream ( std::ostream* a_stream )
{
  d_stream = a_stream;
}

void OCPI::Util::Test::Test::do_pass ( )
{
  ++d_n_passed;
}

std::size_t OCPI::Util::Test::Test::report ( ) const
{

  if ( d_stream )
  {
    *d_stream << "test \"" << (d_name ? d_name : typeid(*this).name())
              << "\":\n\tPassed: "
              << std::dec
              << std::setw ( 8 )
              << d_n_passed
              << "\tFailed: "
              << std::dec
              << std::setw ( 8 )
              << d_n_failed
              << std::endl;
  }
  return d_n_failed;
}

void OCPI::Util::Test::Test::reset ( )
{
  d_n_passed = 0;
  d_n_failed = 0;
}

void OCPI::Util::Test::Test::do_test ( bool rc,
                                      const std::string& message,
                                      const char* file_name,
                                      std::size_t line_number,
                                      bool abort )
{
  if ( rc )
  {
    do_fail ( message, file_name, line_number, abort );
  }
  else
  {
    do_pass ( );
  }
}

void OCPI::Util::Test::Test::do_fail ( const std::string& message,
                                      const char* file_name,
                                      std::size_t line_number,
                                      bool abort )
{
  ++d_n_failed;

  if ( d_stream )
  {
    *d_stream << (d_name ? d_name : typeid ( *this ).name ( ))
              << " failure: ("
              << message
              << ") , "
              << file_name
              << " (line "
              << std::dec
              << std::setw ( 6 )
              << line_number << ")"
              << std::endl;
  }

  if (abort) {
    if (d_stream) {
      *d_stream << (d_name ? d_name : typeid (*this).name ())
                << " aborted."
                << std::endl;
    }

    throw OCPI::Util::Test::Abort ();
  }
}

/* ---- Implementation of the suite class -------------------------------- */


OCPI::Util::Test::Suite::Suite ( const std::string& a_name,
                                std::ostream* a_stream )
  : d_name ( a_name ),
    d_stream ( a_stream )
{
  // Empty
}

OCPI::Util::Test::Suite::~Suite ( )
{
  free ( );
}

std::string OCPI::Util::Test::Suite::name ( ) const
{
  return d_name;
}

std::size_t OCPI::Util::Test::Suite::n_passed ( ) const
{
  std::size_t total_n_passed = 0;

  for ( std::size_t n = 0; n != d_p_tests.size ( ); ++n )
  {
    total_n_passed += d_p_tests [ n ]->n_passed ( );
  }

  return total_n_passed;
}


std::size_t OCPI::Util::Test::Suite::n_failed ( ) const
{
  std::size_t total_n_failed = 0;

  for ( std::size_t n = 0; n != d_p_tests.size ( ); ++n )
  {
    total_n_failed += d_p_tests [ n ]->n_failed ( );
  }

  return total_n_failed;
}


const std::ostream* OCPI::Util::Test::Suite::stream ( ) const
{
  return d_stream;
}


void OCPI::Util::Test::Suite::stream ( std::ostream* a_stream )
{
  d_stream = a_stream;
}


void OCPI::Util::Test::Suite::add_test ( Test* p_test )
{
  if( p_test == 0 )
  {
    throw Exception ( "Null test in Suite::add_test" );
  }
  else if ( d_stream && ( ! p_test->stream ( ) ) )
  {
    p_test->stream ( d_stream );
  }

  d_p_tests.push_back ( p_test );

  p_test->reset ( );
}

void OCPI::Util::Test::Suite::run ( )
{
  reset ( );

  for ( std::size_t n = 0; n != d_p_tests.size ( ); ++n )
  {
    d_p_tests [ n ]->runTest();
  }
}


std::size_t OCPI::Util::Test::Suite::report ( ) const
{
  if ( d_stream )
  {
    std::size_t total_n_failed = 0;

    *d_stream << "Suite \""
              << d_name
              << "\"\n=======";

    for ( std::size_t n = 0; n != d_name.size ( ); ++n )
    {
      *d_stream << '=';
    }
    *d_stream << "=" << std::endl;

    for ( std::size_t n = 0; n != d_p_tests.size ( ); ++n )
    {
      total_n_failed += d_p_tests [ n ]->report( );
    }
    *d_stream << "=======";

    for ( std::size_t n = 0; n != d_name.size ( ); ++n )
    {
      *d_stream << '=';
    }

    *d_stream << "=" << std::endl;

    return total_n_failed;
  }
  else
  {
    return n_failed();
  }
}


void OCPI::Util::Test::Suite::reset ( )
{
  for ( std::size_t n = 0; n != d_p_tests.size ( ); ++n )
  {
    d_p_tests [ n ]->reset ( );
  }
}


void OCPI::Util::Test::Suite::free ( )
{
  for ( std::size_t n = 0; n != d_p_tests.size ( ); ++n )
  {
    delete d_p_tests [ n ];
    d_p_tests [ n ] = 0;
  }
}

