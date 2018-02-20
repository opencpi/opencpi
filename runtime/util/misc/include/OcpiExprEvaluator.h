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
    class Value;
    class ExprValue {
      mutable int64_t m_number;
      mutable bool m_numberSet;
    public:
      class Internal;
      friend class Internal;
      ExprValue();
      ~ExprValue();
    protected:
      Internal *m_internal;
    public:
      void setString(const char *s);
      void setString(const std::string &s) { setString(s.c_str()); }
      void setNumber(int64_t);
      // Set this expression value based on an OU::Value, which already has a data type.
      const char *getTypedValue(Value &v, size_t index = 0) const; // value ref has a type
      // Convert this expression value in to an OU::Value, which already has a data type.
      const char *setFromTypedValue(const Value &v);
      bool isNumber() const;
      bool isVariable() const;
      int64_t getNumber() const;
      const char *getString(std::string &) const;
    };
    // The class provideed by the caller that can provide the value of identifiers
    // in the expression
    struct IdentResolver {
      virtual ~IdentResolver();
      virtual const char *getValue(const char *sym, ExprValue &val) const = 0;
    };
    
    const char
      // The core function that evaluates expressions
      *evalExpression(const char *string, ExprValue &val, const IdentResolver *resolve = NULL,
		      const char *end = NULL),
      // Evaluate the expression, using the resolver, and if the expression was variable,
      // save the expression so it can be reevaluated again later when the values of
      // variables are different.
      *parseExprNumber(const char *a, size_t &np, std::string *expr,
		       const IdentResolver *resolver),
      *parseExprString(const char *a, std::string &s, std::string *expr,
		       const IdentResolver *resolver),
      // Convert an expression to C/C++
      *makeCexpression(const char *expr, const char *prefix, const char *suffix,
		       bool toUpper, std::string &out);
  }
}

#endif
