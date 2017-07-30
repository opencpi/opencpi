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

#ifndef OCPIUTILCOMMANDLINECONFIGURATION_H__
#define OCPIUTILCOMMANDLINECONFIGURATION_H__

/**
 * \file
 * \brief Parse command-line options.
 *
 * Revision History:
 *
 *     11/22/2005 - Frank Pilhofer
 *                  Initial version.
 */

#include <string>
#include <vector>
#include <utility>
#include <ostream>

#if defined(__vxworks) && defined(NONE)
// This interferes with our use of "NONE" as an enumeration
#undef NONE
#endif

/*
 * Helpers: these are defined as macros, as they are intended to be used
 * at compile-time.
 */

#define OCPI_CLC_OPT(x) reinterpret_cast<char ::OCPI::Util::CommandLineConfiguration::*> (x)
#define OCPI_CLC_SENT(x) static_cast<bool ::OCPI::Util::CommandLineConfiguration::*> (x)

namespace OCPI {
  namespace Util {

    /**
     * \brief Parse command-line options.
     *
     * Class to parse options from the command line (or from a similar
     * array of strings).  To use this class, one must implement a derived
     * class, directly or indirectly, and supply a list of supported
     * command-line options using an array of type
     * OCPI::Util::CommandLineConfiguration::Option.
     *
     * Due to compiler limitations, derived classes are restricted to
     * single inheritance only.  (MSVC++ 7 gets confused by pointers to
     * members in classes that use multiple inheritance. It works fine
     * with gcc 3.)
     *
     * Example usage:
     *
     * \code
     * class Configurable : public CommandLineConfiguration {
     * private:
     *   static CommandLineConfiguration::Option g_options[];
     * public:
     *   Configurable ()
     *     : CommandLineConfiguration (g_options)
     *   {};
     *   bool fooOpt;
     * };
     *
     * CommandLineConfiguration::Option
     * Configurable::g_options[] = {
     *   { CommandLineConfiguration::OptionType::BOOLEAN,
     *     "foo", "Whether to foo or not",
     *     OCPI_CLC_OPT(&Configurable::fooOpt) },
     *   { OptionType::END }
     * };
     * \endcode
     */

    class CommandLineConfiguration {
    public:
      /**
       * Name-value pair, for command-line options of the form
       * <tt>--&lt;name&gt;=&lt;optname&gt;=&lt;optvalue&gt;</tt>.
       */

      typedef std::pair<std::string, std::string> NameValue;

      /**
       * Multiple string type, for command-line options of the form
       * <tt>--&lt;name&gt;=&lt;value1&gt;[,&lt;value2&gt;...]</tt>.
       */

      typedef std::vector<std::string> MultiString;

      /**
       * Multiple name-value pairs, for command-line options of the form
       * <tt>--&lt;name&gt;=&lt;optname1&gt;=&lt;optvalue1&gt;[,&lt;optname2&gt;=&lt;optvalue2&gt;...]</tt>.
       */

      typedef std::vector<NameValue> MultiNameValue;

      /**
       * \brief Enumerator for supported command-line data types.
       */

      struct OptionType {
        /**
         * Enumerator for supported command-line data types.
         *
         * See OCPI::Util::CommandLineConfiguration and
         * OCPI::Util::CommandLineConfiguration::Option.
         *
         * Derived classes may extend this set.
         */

        enum OptionTypes {
          END,
          DUMMY,
          NONE,
          BOOLEAN,
          LONG,
          UNSIGNEDLONG,
          STRING,
          NAMEVALUE,
          MULTISTRING,
          MULTINAMEVALUE,
          MAXOPTIONTYPE
        };
      };

      /**
       * \brief Structure for describing a command-line option.
       *
       * Data structure describing a command-line option.  A derived
       * class supplies an array of this class to the constructor.
       * The end of the array is indicated by an element which has the
       * <em>type</em> field set to OptionType::END.
       */

      struct Option {
        /**
         * An enumeration value from the
         * OCPI::Util::CommandLineConfiguration::OptionType::OptionTypes
         * set above.
         *
         * If the #type is <tt>DUMMY</tt>, then this element does not
         * identify an option, but only adds some description in the
         * #desc field which is printed among the other options by the
         * OCPI::Util::CommandLineConfiguration::printOptions()
         * operation.  The #name, #value and #sentinel fields are
         * ignored.
         *
         * If the #type is <tt>NONE</tt>, then the option is of boolean type,
         * but does not accept a value.  The <em>value</em> member is set
         * to true if the option occurs on the command line.  The
         * command-line parameter must have the form
         * <tt>--&lt;name&gt;</tt>.
         *
         * If the #type is <tt>BOOLEAN</tt>, the command-line parameter
         * must have the form
         * <tt>--&lt;name&gt;[=&lt;value&gt;]</tt>,
         * <tt>--no-&lt;name&gt;</tt> or
         * <tt>--no&lt;Name&gt;</tt> (with the first letter capitalized).
         * In the first case, <tt>value</tt> must be <tt>true</tt> or
         * <tt>false</tt>, if omitted, it defaults to true.  In the
         * second and third case, the value is set to false.
         *
         * For all of the following option types, the name and value can
         * be separated by an equal sign, or supplied as separate
         * command-line parameters.  The equal sign shall be used if
         * the value starts with two dashes, i.e.,
         * <tt>--option=--value</tt> instead of
         * <tt>--option --value</tt>, which might be misinterpreted.
         *
         * If the #type is <tt>LONG</tt>, <tt>UNSIGNEDLONG</tt>,
         * or <tt>STRING</tt>, then the command-line parameter must be
         * of the form
         * <tt>--&lt;name&gt;=&lt;value&gt;</tt>.
         *
         * If the #type is <tt>NAMEVALUE</tt>, then the command-line
         * parameter must be of the form
         * <tt>--&lt;name&gt;=&lt;optname&gt;=&lt;optvalue&gt;</tt>.
         *
         * If the #type is <tt>MULTISTRING</tt>, then the command-line
         * parameter must be of the form
         * <tt>--&lt;name&gt;=&lt;value1&gt;[,&lt;value2&gt;...]</tt>.
         * The name can occur multiple times on the command line; the
         * values will be concatenated.
         *
         * If the #type is <tt>MULTINAMEVALUE</tt>, then the command-line
         * parameter must be of the form
         * <tt>--&lt;name&gt;=&lt;optname1&gt;=&lt;optvalue1&gt;[,&lt;optname2&gt;=&lt;optvalue2&gt;...]</tt>.
         * The name can occur multiple times on the command line;
         * the values will be concatenated.
         *
         * A #type of END indicates the end of an array of
         * command-line option descriptions.  All other fields are
         * ignored.
         *
         * Derived classes may add further values for #type.
         */

        unsigned int type;

        /**
         * The name of the command-line option.  On the command
         * line, users need to prefix this name with two dashes,
         * i.e., <tt>--foo</tt> is used to configure a property named
         * <tt>foo</tt>.
         */

        const char * name;

        /**
         * A brief human-readable description of the option.  This
         * text is printed by the printOptions() operation below.
         * To fit on an 80-character wide screen, the description
         * should be less than 44 characters long.
         */

        const char * desc;

        /**
         * A pointer to the data member where the value is to
         * be stored.  This must be a non-static data member of
         * a class derived from CommandLineConfiguration.
         * This pointer-to-member must then be cast to type
         * <tt>char CommandLineConfiguration::*</tt>,
         * using <em>reinterpret_cast&lt;&gt;</em>.
         * The <tt>OCPI_CLC_OPT(x)</tt> macro is provided for this
         * purpose.
         *
         * If #type == <tt>NONE</tt>, then #value must be of type bool.
         *
         * If #type == <tt>BOOLEAN</tt>, then #value must be of type bool.
         *
         * If #type == <tt>LONG</tt>, then #value must be of type long.
         *
         * If #type == <tt>UNSIGNEDLONG</tt>, then #value must be of type unsigned long.
         *
         * If #type == <tt>STRING</tt>, then #value must be of type std::string.
         *
         * If #type == <tt>NAMEVALUE</tt>, then #value must be of type
         * OCPI::Util::CommandLineConfiguration::NameValue.
         *
         * If #type == <tt>MULTISTRING</tt>, then #value must be of type
         * OCPI::Util::CommandLineConfiguration::MultiString.
         *
         * If #type == <tt>MULTINAMEVALUE</tt>, then #value must be of type
         * OCPI::Util::CommandLineConfiguration::MultiNameValue.
         *
         * Note that a mismatch between the type indicated by
         * #type and the actual type of the data member pointed to
         * by #value of the data member yields undefined behavior.
         */

        char CommandLineConfiguration::* value;

        /**
         * If non-zero, this is a pointer to a data member of type
         * bool.  It is set to true if the option is being configured
         * on the command line.  This information can be used to
         * determine if the option occured on the command line,
         * even if the value remains indistinguishable from its
         * default setting.
         *
         * This must be a pointer to a non-static data member of a
         * class derived from this class.  This pointer-to-member
         * must then be cast to the
         * <tt>bool CommandLineConfiguration::*</tt> type (from
         * its original <tt>bool &lt;Derived&gt;::*</tt> type), using
         * <em>static_cast&lt;&gt;</em>.
         * The <tt>OCPI_CLC_SENT(x)</tt> macro is provided
         * for this purpose.
         *
         * Note that this field can be omitted from an initializer
         * list to default-initialize it to zero.
         */

        bool CommandLineConfiguration::* sentinel;
      };

    public:
      /**
       * Constructor.
       *
       * \param[in] options An array of command-line option descriptions.
       *               The end of the array must be indicated by an
       *               Option structure that has its <em>type</em> field set
       *               to OCPI::Util::CommandLineConfiguration::OptionType::END.
       */

      CommandLineConfiguration (const Option * options)
        throw ();

      /**
       * Destructor.
       *
       * A destructor isn't really necessary, but gcc 3.2 complains
       * if it doesn't exist: "class has virtual functions but
       * non-virtual destructor".  That's all right, as this class is
       * not intended for polymorphic use, but let's just avoid the
       * warning.
       *
       * Ordinarily, the exception specification "throw ()" should be,
       * added, but since the default destructor, if a derived class did
       * not define a destructor, does not have an exception specification,
       * it would conflict with the more strict "throw ()" in the base
       * class.
       */

      virtual ~CommandLineConfiguration ();

      /**
       * Process command-line options.
       *
       * Non-options, i.e., command-line parameters that do not begin
       * with two dashes <em>--</em>, are always ignored.  A user can test
       * for non-options by setting the \a removeFromSet parameter to
       * true, and then checking whether \a argc==1.
       *
       * \param[in,out] argc The number of elements in the argv array.
       * \param[in,out] argv The command-line options. By convention,
       *               \a argv[0] is ignored. \a argv[\a argc] shall be 0.
       * \param[in] ignoreUnknown Whether to ignore unknown command-line
       *               options.  If this is set to false, an exception
       *               is raised if an unknown option is encountered
       *               on the command line.
       * \param[in] removeFromSet Whether to remove processed options
       *               from the command line.  If this is set to true,
       *               processed options are removed from the \a argv
       *               array, and \a argc is adjusted likewise before
       *               returning.
       *
       * \throw std::string If an option is recognized, but an error
       * occurs parsing its value.
       * \throw std::string If an option is not recognised and
       * \a ignoreUnknown is false.
       */

      void configure (int & argc, char * argv[],
                      bool ignoreUnknown = false,
                      bool removeFromSet = true)
        throw (std::string);

      /**
       * Print a human-readable description of available command-line
       * options to the given output stream.
       *
       * Users of this class frequently have a separate <tt>help</tt>
       * option that is part of the <em>options</em> array (i.e., an
       * option whose name is <tt>help</tt> and whose type is
       * <tt>NONE</tt> or <tt>BOOLEAN</tt>), so that a user can use
       * the <tt>--help</tt> command-line parameter to see this
       * information.
       *
       * Only the options themselves are printed without decoration.
       * Derived classes may want to print some introduction or footer
       * along with the options.
       *
       * \param[in] out Output stream to write the command-line options
       * to.
       */

      void printOptions (std::ostream & out)
        throw ();

    protected:
      /**
       * Helper to assign a configurable value from the command-line
       * value string.  #configure() calls this function for every
       * command-line parameter, after locating the command-line
       * parameter's name in the options array.  This function can
       * be overloaded by a derived class to support more option
       * types.
       *
       * E.g., the implementation for
       * OCPI::Util::CommandLineConfiguration::OptionType::STRING
       * looks like:
       *
       * \code
       *   std::string & value = *reinterpret_cast<std::string *> (valuePtr);
       *   value = optionValue;
       * \endcode
       *
       * \param[in] type The type field from the
       *                 OCPI::Util::CommandLineConfiguration::Option
       *                 structure.
       * \param[in] valuePtr A generic pointer to the value that
       *                 should be configured.
       * \param[in] optionValue The value string that was extracted
       *                 from the command line.
       *
       * \throw std::string If \a type is not recognized.
       * \throw std::string If the format of the \a optionValue does
       * not match the expected pattern implied by \a type.
       */

      virtual void configureOption (unsigned int type,
                                    void * valuePtr,
                                    const std::string & optionValue)
        throw (std::string);

      /**
       * Returns a string indicating how to options of a certain type
       * can be configured on the command line.  Called by #printOptions()
       * if it encounters an option type beyond the basic set implemented
       * in this base class.
       *
       * If a derived class extends the set of option types, then
       * it shall also overload this function, and return a string
       * matching the parsing applied to values of this option type
       * in the derived #configureOption() function.
       *
       * E.g., the string for the <tt>STRING</tt> option type is
       * <tt>=&lt;s&gt;</tt>.
       *
       * \param[in] type The type field from the
       *                 OCPI::Util::CommandLineConfiguration::Option
       *                 structure.
       * \return A string describing how to use this type on the
       *         command line.
       */

      virtual std::string printOptionSpec (unsigned int type) const
        throw ();

    protected:
      const Option * findOption (const std::string & name)
        throw ();

    protected:
      unsigned long m_numOpts;
      const Option * m_options;

    private:
      // not implemented

    };

  }
}

#endif
