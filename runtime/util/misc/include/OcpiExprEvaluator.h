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

/*
 * A small expression parser that deals in strings or (int64_t) numbers.
 * There is a callback to provide the value of "variables", whose value can be numeric or string.
 * The syntax is that of C/C++ "constant expression", without enums, sizeof, casting or functions.
 * Per C/C++, no comma operator or assignment operators or pointers or references (unary & or *).
 *
 * Strings in the context of logical operators are considered true if non-empty, false if empty.
 * There is no NULL value for strings.
 * String errors are returned (no exceptions thrown).
 * Unary operators are: + - ~ !
 * Binary operators are: || && | ^ & == != < >> <= >= << >> + - * / %
 * Conditional operator
 *
 * Simple usage is:
 *    const char *err;
 *    OU::ExprValue v;
 *    if ((err = evalExpression("4 + 75 < 55 ? \"hello\" : 78", v)))
 *      printf("Error parsing expression: %s\n", err);
 *    else if (v.isNumber)
 *      printf("Got a number value: %lld\n", (long long)v.number);
 *    else
 *      printf("Got a string value: %s\n", v.string.c_ctr());
 *
 *  Example with indentifier resolver is:
 *  
 *    class MyResolver : public OU::IndentResolver {
 *       const char *getValue(const std::string &name, ExprValue &v) {
 *         if (name == "myvarname") {
 *           v.isNumber = false;
 *           v.string = "myvalue";
 *           return NULL;
 *         }
 *         return "identifier not defined";
 *       }
 *    } resolver;
 *    const char *err;
 *    OU::ExprValue v;
 *    if ((err = evalExpression("4 + 75 < 55 ? \"hello\" : 78", v, resolver)))
 *      printf("Error parsing expression: %s\n", err);
 */

#ifndef OCPI_EXPREVALUATOR_H
#define OCPI_EXPREVALUATOR_H
#include <stdint.h>
#include <string>
#include "ezxml.h"

namespace OCPI {
  namespace Util {
    // The value returned from parsing an expression and also returned as the value of an identifier
    struct ExprValue {
      std::string string;
      int64_t number;
      bool isNumber;
      bool isVariable; // a variable was used to compute the result
    };

    // The class provideed by the caller that can provide the value of identifiers
    // in the expression
    struct IdentResolver {
      virtual const char *getValue(const char *sym, ExprValue &val) const = 0;
      // Look up the variable reference, but we're not looking for values
      // virtual const char *isVariable(const char *) = 0;
      virtual ~IdentResolver(){};
    };
    
    // The core function that evaluates expressions
    const char *evalExpression(const char *string, ExprValue &val, const
			       IdentResolver *resolve = NULL);
    // Evaluate the expression, using the resolver, and if the expression was variable,
    // save the expression so it can be reevaluated again later when the values of
    // variables are different.
    const char *parseExprNumber(const char *a, size_t &np, std::string *expr,
				const IdentResolver *resolver);
    // Parse an integer (size_t) attribute that might be an expression
    // Only consider if we have an identifier resolver
    // The string value of the expression is returned in expr.
    const char *getExprNumber(ezxml_t x, const char *attr, size_t &np, bool *found,
			      std::string *expr, const IdentResolver *resolver);
    // Convert an expression to C/C++
    const char *makeCexpression(const char *expr, const char *prefix, const char *suffix,
				bool toUpper, std::string &out);
  }
}

#endif
