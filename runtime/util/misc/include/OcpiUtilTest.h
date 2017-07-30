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

// -*- c++ -*-

#ifndef OCPI_UTIL_TEST_SUITE_H
#define OCPI_UTIL_TEST_SUITE_H

#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>

/**
  @brief
    Macro called to catch the return code of a call to a test.

  The macro test_call() must be called from a member function of a class
  derived from test. The rc parameter to the macro test_call() should be
  zero if the test call passed and non-zero if the sub-test failed.

  @code
    test_call ( foo_test ( arg0, arg1, arg2 ) );
  @endcode

  @param [ in ] _invocation
                A zero value returned from the function indicates the test
                passed. A non-zero value indicates the test failed.
*/

#define test_call( _invocation ) \
  do \
  { \
    int _rc = ( int ) _invocation; \
    do_test ( _rc, #_invocation, __FILE__, __LINE__ ); \
  } \
  while ( 0 )

/**
  @brief
    Macro called to test the specified condition.

  The macro test() must be called from a member function of a class
  derived from test. The condition is evaluated. If the condition evaluates
  to true then the test "passes" otherwise the test failed.  In the latter
  case, the current test set is aborted.

  @code
    test_cond ( status.error != 0 );
  @endcode

  @param [ in ] _cond
                Condition to evaluate.
*/

#define test( _cond ) \
    do_test ( !( _cond ), #_cond, __FILE__, __LINE__, 1 );


/**
  @brief
    Macro called to test the specified condition.

  The macro test_cond() must be called from a member function of a class
  derived from test. The condition is evaluated. If the condition evaluates
  to true then the test "passes" otherwise the test failed.

  @code
    test_cond ( status.error != 0 );
  @endcode

  @param [ in ] _cond
                Condition to evaluate.
*/

#define test_cond( _cond ) \
    do_test ( !( _cond ), #_cond, __FILE__, __LINE__ );


/**
  @brief
    Macro called to fail a sub-test.

  The macro fail() must be called from a member function of a class
  derived from test. The fail() macro is provided as a convenience to
  force the "failure" of a sub-test. This is good to use as a stub for
  a test that is not implemented.

  @param [ in ] message
                Message associated with a failed test.
*/

#define fail( _message ) \
  do_fail( _message, __FILE__, __LINE__)


/**
  @brief
    Macro called to pas a sub-test.

  The macro pass() must be called from a member function of a class
  derived from test. The pass() macro is provided as a convenience to
  force the "pass" of a sub-test. This is good to use as a stub for
  a test that is not implemented.
*/

#define pass( ) do_pass( )

namespace OCPI {
  namespace Util {
    namespace Test {

      /**
         @brief Abort a test

         Throwing this exception aborts the current test.
      */

      class Abort {
      public:
        Abort ()
        {
          // Empty
        }
      };

      /**
         @brief
         test suite exception class.

         Exceptions of this type are thrown by the test suite for run time
         errors that are not related to test failures. For example, an
         exception is thrown if a NULL test pointer is provided to
         Suite::add_test().
      */

      class Exception : public std::logic_error
        {
        public:

          /**
             @brief
             Construct a test suite exception instance.

             Creates a test suite exception with the specified error
             message.
          */

          Exception ( const std::string& message = "" )
            : std::logic_error ( message )
            {
              // Empty
            }
        };

      /**
         @brief
         test suite test class.

         The class test is an abstract class. A class must derived from test
         and implement the run() function.  The derived class contains the
         tests that are to be executed. The tests should pass their return status
         into the macro called test() (defined above). If the return status
         is 0 the tests "passed". Otherewise, the test failed. The run() function
         in the derived class is invoked from the framework. The run() function
         should call all of the tests in the derived class.

         @code

         class foo_test : public OCPI::Util::Test::Test
         {
         public:

           // Implement the pure virtual run() function from test
           void run ( )
           {
             test_foo_1 ( );
             test_foo_2 ( );
           }

           void test_foo_1 ( )
           {
             // The macro test() records the rc from each sub-test
             test_call( test_foo_1a ( arg0 ) );
             test_call( test_foo_1b ( argA, argB ) );
             test_call( test_foo_1c ( ) );
           }

           void test_foo_2 ( )
           {
             test_call( test_foo_2a ( ) );
             test_call( test_foo_2b ( ) );
             test_call( test_foo_2c ( ) );
           }

         private:
           // Data used by the tests
         };

         int main ( )
         {
           // Instance of the foo_test
           foo_test test;

           // Perform the test
           test.run();

           // Report the results
           std::size_t n_failed = test.report ( );
         }

         @endcode
      */

      class Test
      {
      public:

        /**
           @brief
           Construct an instance of a test.

           The function Test() initializes the test instance. Provide an
           opportunity to use out output stream other than stdout.

           @param [ in ] p_name
           Optional test name.

           @param [ in ] p_stream
           Optional output stream.
        */

        Test ( const char * p_name = 0, std::ostream* p_stream = &std::cout );

        /**
           @brief
           Destroys an instance of a test.

           The function ~test() finalizes the test instance.
        */

        virtual ~Test ( );

        /**
           @brief
           Run the test.

           Calls run() and catches any exceptions.
        */

        void runTest ();

        /**
           @brief
           Returns umber of tests that passed.

           The function n_passed() returns the number of tests that have
           passed.

           @retval
           Number of tests that passed.
        */

        std::size_t n_passed ( ) const;

        /**
           @brief
           Returns number of tests that failed.

           The function n_failed() returns the number of tests that have
           failed.

           @retval
           Number of tests that failed.
        */

        std::size_t n_failed ( ) const;

        /**
           @brief
           Returns a pointer to the output stream.

           The function stream() returns a pointer to the output stream.

           @retval
           Pointer to the output stream.
        */

        const std::ostream* stream ( ) const;

        /**
           @brief
           Sets the output stream.

           The function stream() sets the output stream that will be used
           by the class to write its output.

           @param [ n ] p_stream
           Pointer to the stream the class will use to write
           its output.
        */

        void stream ( std::ostream* p_stream );

        /**
           @brief
           Called when a test passes.

           The function do_pass() increments the number of tests that have
           passed.
        */

        void do_pass ( );

        /**
           @brief
           Reports the status of the tests.

           The function report() reports the number of tests that have
           passed and failed. The function report() should be called after
           run() to see the status of the tests performed by run().

           @retval Number of tests that have failed.
        */

        std::size_t report ( ) const;

        /**
           @brief
           Resets the status of the tests.

           The function reset() resets the number of tests that have
           passed and failed to zero.
        */

        virtual void reset ( );

      protected:
        /**
           @brief
           Function invoked by the framework to run the tests.

           The function run() is implemented in the derived class to perform
           the tests encapsulated by the derived class.
        */

        virtual void run ( ) = 0;

        /**
           @brief
           Records information about a test.

           The function do_test() is called from the macro test(). The function
           do_test() records the pass/fail status of a test.

           @param [ in ] rc
           A zero value indicates the test passed. A non-zero
           value indicates the test failed.

           @param [ in ] message
           Message associated with a failed test.

           @param [ in ] file_name
           Name of the file that contains the test that failed.

           @param [ in ] line_number
           Line number in the file of the failed test.

           @param [ in ] abort
           Whether the current test set shall be aborted if the test failed.
        */

        void do_test ( bool rc,
                       const std::string& message,
                       const char* file_name,
                       std::size_t line_number,
                       bool abort = false );

        /**
           @brief
           Reports a failed test.

           The function do_fail() is called from the do_test() when a test
           fails (rc to do_test() is non-zero). The function do_fail()
           increments the number of test that failed and reports the
           message, file name, and line number of the failed test.

           @param [ in ] message
           Message associated with a failed test.

           @param [ in ] file_name
           Name of the file that contains the test that failed.

           @param [ in ] line_number
           Line number in the file of the failed test.

           @param [ in ] abort
           Whether the current test set shall be aborted.
        */

        void do_fail ( const std::string& message,
                       const char* file_name,
                       std::size_t line_number,
                       bool abort = false );

      private:

        // Not implemented
        Test ( const Test& );
        Test& operator= ( const Test& );

      private:

        const char * d_name;
        //!< Test name

        std::ostream* d_stream;
        //!< Output stream

        std::size_t d_n_passed;
        //!< Number of tests that have passed

        std::size_t d_n_failed;
        // Number of tests that have failed

      }; // End: class test

      /**
         @brief
         Test suite "suite" class.

         The class suite aggregates together multiple test instances. The
         status for a collection of tests is maintained by suite.

         @code
         // Create an instance of a test suite
         OCPI::Util::Test::Suite suite ( "Example suite" );

         // Add tests to to the suite

         suite.add_test ( new foo_test );
         suite.add_test ( new BarTest );
         suite.add_test ( new FuzzTest );

         // Perform all of the tests
         suite.run ( );

         // Report the results
         std::size_t n_failed = suite.report ( );
         @endcode
      */

      class Suite
      {
      public:


        /**
           @brief
           Construct an instance of a test suite.

           The function Suite() initializes the suite instance. Provide an
           opportunity to use out output stream other than stdout.

           @param [ in ] name
           Name of the test suite.

           @param [ in ] p_stream
           Optional output stream.
        */

        Suite ( const std::string& name, std::ostream* d_stream = &std::cout );

        /**
           @brief
           Destroys an instance of a suite.

           The function ~Suite() finalizes the suite instance.
        */

        ~Suite ( );

        /**
           @brief
           Returns the name of the suite instance.

           The function name() returns the name of the suite instance.

           @retval Name of the suite instance.
        */

        std::string name ( ) const;

        /**
           @brief
           Returns the total number of tests that have passed.

           The function n_passed() returns total number of tests that have
           passed.

           @retval Ttotal number of tests that have passed.
        */

        std::size_t n_passed ( ) const;

        /**
           @brief
           Returns the total number of tests that have failed.

           The function n_failed() returns total number of tests that have
           failed.

           @retval Ttotal number of tests that have failed.
        */

        std::size_t n_failed ( ) const;

        /**
           @brief
           Returns a pointer to the output stream.

           The function stream() returns a pointer to the output stream.

           @retval
           Pointer to the output stream.
        */

        const std::ostream* stream ( ) const;

        /**
           @brief
           Sets the output stream.

           The function stream() sets the output stream that will be used
           by the class to write its output.

           @param [ in ] stream
           Pointer to the stream the class will use to write
           its output.
        */

        void stream ( std::ostream* stream );

        /**
           @brief
           Add a test instance to the test suite.

           The function add_test() adds an instance of a test to the test
           suite. The test will be forced to use the output stream of the
           suite instance. The passed/failed status of the test will also
           be reset.

           @param [ in ] p_test
           Pointer to the test instance to add to the suite.
        */

        void add_test ( Test* p_test );

        /**
           @brief
           Invokes the run() function of each test in the suite.

           The function run() invokes the run() function of each test
           instance in the suite. The suite owns the test instances and will
           delete the tests instances when the suite instance is destroyed.
        */

        void run ( );

        /**
           @brief
           Reports the status of all of the tests in the suite.

           The function report() reports the number of tests that have
           passed and failed for all of the tests in the suite. The function
           report() should be called after run() to see the status of the tests
           performed by suite.

           @retval Number of tests that have failed.
        */

        std::size_t report ( ) const;

      private:

        /**
           @brief
           Resets the status of all of the tests in the suite.

           The function reset() calls the reset() function on each test
           in the suite.
        */

        void reset ( );

        /**
           @brief
           Delete each test instance in the suite.

           The function free() is called from the suite destructor to
           delete all of the test instances.
        */

        void free ( );

        // Not implemented
        Suite ( const Suite& );
        Suite& operator= ( const Suite& );

      private:

        std::string d_name;
        //!< Name of the test suite

        std::ostream* d_stream;
        //!< Output stream

        std::vector<Test*> d_p_tests;
        //!< Container that holds the test instances owned by the suite.

      }; // End: class suite

    } // End: namespace Test
  } // End: namespace Util
} // End: namespace OCPI

#endif // End: #ifndef OCPI_UTIL_TEST_SUITE_H
