
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

#include <string>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <OcpiOsAssert.h>
#include "OcpiUtilMisc.h"
#include "OcpiUtilCommandLineConfiguration.h"

OCPI::Util::CommandLineConfiguration::
CommandLineConfiguration (const Option * options)
  throw ()
  : m_numOpts (0),
    m_options (options)
{
  while (options->type != OptionType::END) {
    m_numOpts++;
    options++;
  }
}

OCPI::Util::CommandLineConfiguration::
~CommandLineConfiguration ()
{
}

const OCPI::Util::CommandLineConfiguration::Option *
OCPI::Util::CommandLineConfiguration::
findOption (const std::string & name)
  throw ()
{
  for (const Option * oi = m_options;
       oi->type != OptionType::END;
       oi++) {
    if (oi->type == OptionType::DUMMY) {
      continue;
    }
    if (name == oi->name) {
      return oi;
    }
  }

  return 0;
}

void
OCPI::Util::CommandLineConfiguration::
configure (int & argc, char *argv[],
           bool ignoreUnknown,
           bool removeFromSet)
  throw (std::string)
{
  int newargc;
  int cmdidx;

  for (cmdidx=newargc=1; cmdidx<argc; cmdidx++) {
    try {
      if (argv[cmdidx][0] == '-' && argv[cmdidx][1] == '-') {
        std::string option = argv[cmdidx] + 2;
        std::string::size_type equalSignPos = option.find ('=');

        std::string optionName;
        std::string optionValue;

        if (equalSignPos == std::string::npos) {
          optionName = option;
        }
        else {
          optionName = option.substr (0, equalSignPos);
          optionValue = option.substr (equalSignPos + 1);
        }

        const Option * optSpec = findOption (optionName);

        /*
         * In GNU autoconf style, a boolean option can be prefixed by
         * "no-" to set its value to false.
         */

        if (!optSpec &&
            optionName.length() > 3 &&
            optionName[0] == 'n' && optionName[1] == 'o' &&
            optionName[2] == '-' &&
            equalSignPos == std::string::npos) {
          std::string realOptionName = optionName.substr (3);
          const Option * realOptSpec = findOption (realOptionName);

          if (realOptSpec && realOptSpec->type == OptionType::BOOLEAN) {
            optionName = realOptionName;
            optionValue = "false";
            optSpec = realOptSpec;
          }
        }

        /*
         * In a twist of the above GNU autoconf style for negating
         * boolean options, but more in line with our style of options,
         * we also accept a prefix of "no", followed by the option
         * name, with the first character capitalized.
         */

        if (!optSpec &&
            optionName.length() > 2 &&
            optionName[0] == 'n' && optionName[1] == 'o' &&
            std::isupper (optionName[2])) {
          std::string realOptionName = optionName.substr (2);
          realOptionName[0] = std::tolower (realOptionName[0]);
          const Option * realOptSpec = findOption (realOptionName);

          if (realOptSpec && realOptSpec->type == OptionType::BOOLEAN) {
            optionName = realOptionName;
            optionValue = "false";
            optSpec = realOptSpec;
          }
        }

        if (!optSpec) {
          /*
           * Unknown option. Ignore or complain.
           */

          if (ignoreUnknown) {
            if (removeFromSet) {
              if (cmdidx != newargc) {
                argv[newargc] = argv[cmdidx];
              }
              newargc++;
            }
            continue;
          }

          throw std::string ("unknown option");
        }

        /*
         * For options that expect a parameter (i.e., everything except
         * NONE and BOOLEAN), if the command-line parameter does not
         * contain an equal sign, the value is expected in the next
         * command-line parameter.
         */

        if (optSpec->type != OptionType::NONE &&
            optSpec->type != OptionType::BOOLEAN &&
            equalSignPos == std::string::npos) {
          if (cmdidx+1 >= argc) {
            std::string reason = "Command-line option \"";
            reason += optionName;
            reason += "\" requires a value";
            throw reason;
          }

          optionValue = argv[++cmdidx];

          /*
           * In this case, the option value shall not start with two
           * dashes. This would be too confusing. Our documentation
           * says that in this case, the syntax using the equal sign
           * shall be used, i.e., "--option=--value".
           */

          if (optionValue.length() >= 2 &&
              optionValue[0] == '-' &&
              optionValue[1] == '-') {
            std::string reason = "Confused about the parameter for ";
            reason += "command-line option \"";
            reason += optionName;
            reason += "\"; if the value is intended to start with two dashes, ";
            reason += "use \"--";
            reason += optionName;
            reason += '=';
            reason += optionValue;
            reason += "\"";
            throw reason;
          }
        }

        char CommandLineConfiguration::* cMemberPtr = optSpec->value;
        ocpiAssert (cMemberPtr);

        char & cValuePtr = this->*cMemberPtr;
        void * vValuePtr = &cValuePtr;
        ocpiAssert (vValuePtr);

        configureOption (optSpec->type, vValuePtr, optionValue);

        if (optSpec->sentinel) {
          this->*(optSpec->sentinel) = true;
        }
      }
      else {
        if (removeFromSet) {
          if (cmdidx != newargc) {
            argv[newargc] = argv[cmdidx];
          }
          newargc++;
        }
      }
    }
    catch (const std::string & oops) {
      std::string reason = "Error processing command-line option \"";
      reason += argv[cmdidx];
      reason += "\": ";
      reason += oops;
      throw reason;
    }
  }

  if (removeFromSet) {
    argv[newargc] = 0;
    argc = newargc;
  }
}

void
OCPI::Util::CommandLineConfiguration::
configureOption (unsigned int type,
                 void * valuePtr,
                 const std::string & optionValue)
  throw (std::string)
{
  switch (type) {
  case OptionType::NONE:
    {
      bool & value = *reinterpret_cast<bool *> (valuePtr);

      if (!optionValue.empty()) {
        throw std::string ("option does not take a value");
      }

      value = true;
    }
    break;

  case OptionType::BOOLEAN:
    {
      bool & value = *reinterpret_cast<bool *> (valuePtr);

      if (optionValue.empty() || optionValue == "true") {
        value = true;
      }
      else if (optionValue == "false") {
        value = false;
      }
      else {
        std::string reason = "can not parse \"";
        reason += optionValue;
        reason += "\" as a boolean value";
        throw reason;
      }
    }
    break;

  case OptionType::LONG:
    {
      if (optionValue.empty()) {
        throw std::string ("option needs a value");
      }

      long int & value = *reinterpret_cast<long int *> (valuePtr);
      const char * txtPtr = optionValue.c_str();
      char * endPtr;

      value = std::strtol (txtPtr, &endPtr, 10);

      if ((value == 0 && endPtr == txtPtr) || *endPtr) {
        std::string reason = "expected integer, but got \"";
        reason += optionValue;
        reason += "\"";
        throw reason;
      }
    }
    break;

  case OptionType::UNSIGNEDLONG:
    {
      if (optionValue.empty()) {
        throw std::string ("option needs a value");
      }

      unsigned long & value = *reinterpret_cast<unsigned long *> (valuePtr);
      const char * txtPtr = optionValue.c_str();
      char * endPtr;

      value = std::strtoul (txtPtr, &endPtr, 10);

      if ((value == 0 && endPtr == txtPtr) || *endPtr) {
        std::string reason = "expected unsigned integer, but got \"";
        reason += optionValue;
        reason += "\"";
        throw reason;
      }
    }
    break;

  case OptionType::STRING:
    {
      if (optionValue.empty()) {
        throw std::string ("option needs a value");
      }

      std::string & value = *reinterpret_cast<std::string *> (valuePtr);
      value = optionValue;
    }
    break;

  case OptionType::NAMEVALUE:
    {
      if (optionValue.empty()) {
        throw std::string ("option needs a value");
      }

      NameValue & value = *reinterpret_cast<NameValue *> (valuePtr);
      std::string::size_type nvEqualSign = optionValue.find ('=');

      if (nvEqualSign == std::string::npos) {
        value.first = optionValue;
        value.second.clear ();
      }
      else {
        value.first = optionValue.substr (0, nvEqualSign);
        value.second = optionValue.substr (nvEqualSign + 1);
      }
    }
    break;

  case OptionType::MULTISTRING:
    {
      if (optionValue.empty()) {
        throw std::string ("option needs a value");
      }

      MultiString & value = *reinterpret_cast<MultiString *> (valuePtr);
      std::string stringSet = optionValue;

      while (stringSet.length()) {
        std::string::size_type firstCommaPos = stringSet.find (',');
        std::string sv;

        if (firstCommaPos == std::string::npos) {
          sv = stringSet;
          stringSet.clear ();
        }
        else {
          sv = stringSet.substr (0, firstCommaPos);
          stringSet = stringSet.substr (firstCommaPos + 1);
        }

        value.push_back (sv);
      }
    }
    break;

  case OptionType::MULTINAMEVALUE:
    {
      if (optionValue.empty()) {
        throw std::string ("option needs a value");
      }

      MultiNameValue & value = *reinterpret_cast<MultiNameValue *> (valuePtr);
      std::string stringSet = optionValue;

      while (stringSet.length()) {
        std::string::size_type firstCommaPos = stringSet.find (',');
        std::string nameValueString;

        if (firstCommaPos == std::string::npos) {
          nameValueString = stringSet;
          stringSet.clear ();
        }
        else {
          nameValueString = stringSet.substr (0, firstCommaPos);
          stringSet = stringSet.substr (firstCommaPos + 1);
        }

        NameValue nameValue;
        std::string::size_type nvEqualSign = nameValueString.find ('=');

        if (nvEqualSign == std::string::npos) {
          nameValue.first = nameValueString;
          nameValue.second.clear ();
        }
        else {
          nameValue.first = nameValueString.substr (0, nvEqualSign);
          nameValue.second = nameValueString.substr (nvEqualSign + 1);
        }

        value.push_back (nameValue);
      }
    }
    break;

  default:
    {
      ocpiAssert (0);
    }
  }
}

void
OCPI::Util::CommandLineConfiguration::
printOptions (std::ostream & out)
  throw ()
{
  for (const Option * oi = m_options;
       oi->type != OptionType::END;
       oi++) {
    if (oi->type == OptionType::DUMMY) {
      if (oi->desc) {
        out << "  " << oi->desc;
      }
      out << std::endl;
      continue;
    }

    std::size_t length = std::strlen (oi->name);

    out << "    --" << oi->name;

    switch (oi->type) {
    case OptionType::NONE:
      break;

    case OptionType::BOOLEAN:
      out << "[=<b>]";
      length += 6;
      break;

    case OptionType::LONG:
      out << "=<i>";
      length += 4;
      break;

    case OptionType::UNSIGNEDLONG:
      out << "=<n>";
      length += 4;
      break;

    case OptionType::STRING:
      out << "=<s>";
      length += 4;
      break;

    case OptionType::NAMEVALUE:
      out << "=n[=v]";
      length += 6;
      break;

    case OptionType::MULTISTRING:
      out << "=s[,s]";
      length += 6;
      break;

    case OptionType::MULTINAMEVALUE:
      out << "=n[=v]";
      length += 6;
      break;

    default:
      std::string os = printOptionSpec (oi->type);
      out << os;
      length += os.length();
      break;
    }

    if (oi->desc) {
      if (length < 29) {
        std::string emptySpace (29-length, ' ');
        out << emptySpace;
      }

      out << ' ' << oi->desc;
    }

    out << std::endl;
  }
}

std::string
OCPI::Util::CommandLineConfiguration::
printOptionSpec (unsigned int) const
  throw ()
{
  ocpiAssert (0);
  // silence compiler warning
  return std::string();
}
