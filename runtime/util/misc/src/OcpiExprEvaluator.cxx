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

#include <cstring>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <limits>
#include <cfloat>
#include <cerrno>
#include <gmpxx.h>
#include "OcpiUtilMisc.h"
#include "OcpiUtilEzxml.h"
#include "OcpiUtilValue.h"
#include "OcpiExprEvaluator.h"

namespace OU = OCPI::Util;
namespace OX = OCPI::Util::EzXml;
namespace OA = OCPI::API;
namespace {
 #define ALLOPS								\
   OPS(Cond1, "?") OPS(Cond2, ":") OPS(Lor, "||") OPS(Land, "&&")	\
   OP(Bor, "|") OP(Xor, "^") OP(Band, "&")				\
   OPS(Eq, "==") OPS(Neq, "!=") OPS(Lt, "<") OPS(Gt, ">") OPS(Le, "<=") OPS(Ge, ">=") \
   OP(Sl, "<<") OP(Sr, ">>") OPS(Plus, "+") OP(Minus, "-") OP(Mult, "*") OP(Div, "/") OP(Mod, "%") \
   OP(Pow,"**") OP(UMinus, "") OP(UPlus, "") OP(Tilde, "~") OP(Not, "!") OP(Lpar, "(") OP(Rpar, ")") \
   /**/
#define OPS(n,s) true,
#define OP(n,s) false,
  const bool opStringOk[] = { ALLOPS false};
#undef OPS
#define OPS OP
#undef OP
#define OP(n,s) s,
  const char *opNames[] = { ALLOPS 0 };
#undef OP
#define OP(n,s) Op##n,
  typedef enum { ALLOPS OpEnd, OpConstant, OpIdent, OpLimit } OpCode;

pthread_once_t once;
mpz_class *mpz_min, *mpz_max;
const char *mpzString(const mpz_class &z, std::string &s) {
  s = z.get_str(10);
  return s.c_str();
}

const char *mpfString(const mpf_class &f, std::string &s) {
  mp_exp_t exp;
  std::string smpf = f.get_str(exp); 
  if (exp == 0 && smpf.empty())
    s = "0";
  else {
    const char *cp = smpf[0] == '-' ? &smpf[1] : &smpf[0];
    if (cp[1] == '\0' && exp == 1)
      s = smpf;
    else
      OU::format(s, "%s%c%s%se%ld", smpf[0] == '-' ? "-" : "", cp[0], cp[1] ? "." : "",
		 cp + 1, exp-1);
  }
  return s.c_str();
}

// Creat the MPZ equivalent of UINT64_MAX and INT64_MIN etc.
void init() {
  // For some reason the static constructors are not run.  FIXME figure this out?
  mpz_min = new mpz_class[OA::OCPI_scalar_type_limit];
  mpz_max = new mpz_class[OA::OCPI_scalar_type_limit];
  mpz_min[OA::OCPI_Char] = INT8_MIN;
  mpz_max[OA::OCPI_Char] = INT8_MAX;
  mpz_min[OA::OCPI_UChar] = 0;
  mpz_max[OA::OCPI_UChar] = UINT8_MAX;
  mpz_min[OA::OCPI_Short] = INT16_MIN;
  mpz_max[OA::OCPI_Short] = INT16_MAX;
  mpz_min[OA::OCPI_UShort] = 0;
  mpz_max[OA::OCPI_UShort] = UINT16_MAX;
  mpz_min[OA::OCPI_Long] = INT32_MIN;
  mpz_max[OA::OCPI_Long] = INT32_MAX;
  mpz_min[OA::OCPI_ULong] = 0;
  mpz_max[OA::OCPI_ULong] = UINT32_MAX;
  mpz_min[OA::OCPI_Enum] = 0;
  mpz_max[OA::OCPI_Enum] = UINT32_MAX;
  mpz_min[OA::OCPI_ULongLong] = 0;
  // gmpxx doesn't make it easy for 64 bit ints
  mpz_class tmp(2);
  mpz_pow_ui(tmp.get_mpz_t(), tmp.get_mpz_t(), 64);
  tmp -= 1;
  mpz_max[OA::OCPI_ULongLong] = tmp; // 65 precision to 64 precision
  tmp = 2;
  mpz_pow_ui(tmp.get_mpz_t(), tmp.get_mpz_t(), 63);
  mpz_min[OA::OCPI_LongLong] = -tmp;
  mpz_max[OA::OCPI_LongLong] = --tmp;
#ifndef NDEBUG
  for (unsigned i = 0; i <= OA::OCPI_ULongLong; i++) {
    std::string s1, s2;
    ocpiDebug("GMP MPF init: for %s min: %s max: %s",
	      OU::baseTypeNames[i], mpfString(mpz_min[i], s1), mpfString(mpz_max[i], s2));
  }
#endif
}

struct Init {
  Init() {
    pthread_once(&once, init);
  }
  ~Init() {
    delete [] mpz_min;
    delete [] mpz_max;
  }
} staticInit;
// A conversion function for binary operators which are only defined for integer values.
const char *
mpf2mpz(const mpf_class &number, mpz_class &z) {
  pthread_once(&once, init);
  std::string s1, s2;
  mpz_set_f(z.get_mpz_t(), number.get_mpf_t()); // implicit truncation
  if (sgn(number) < 0) {
    if (z < mpz_min[OA::OCPI_LongLong])
      return "negative value too large to fit in int64_t";
  } else if (z > mpz_max[OA::OCPI_ULongLong])
    return OU::esprintf("positive value \"%s\" too large to fit in uint64_t (%s)",
			mpfString(number, s1), mpzString(mpz_max[OA::OCPI_ULongLong], s1));
  return NULL;
}

void
mpz2mpf(const mpz_class &z, mpf_class &f) {
  mpf_set_z(f.get_mpf_t(), z.get_mpz_t());
}
void
int64_2_mpf(int64_t i64, mpf_class &number) {
  mpz_class z = (int32_t)(i64 >> 32);
  z <<= 32;
  z |= (int32_t)(i64 & 0xffffffff);
  mpz2mpf(z, number);
}
void
uint64_2_mpf(uint64_t u64, mpf_class &number) {
  mpz_class z = (uint32_t)(u64 >> 32);
  z <<= 32;
  z |= (uint32_t)(u64 & 0xffffffff);
  mpz2mpf(z, number);
}
inline bool mpf2bool(const mpf_class &number) {
  return number.get_mpf_t()->_mp_size != 0; // don't depend on cxx11
}
} // anonymous namespace


namespace OCPI {
namespace Util {
// Supply value argument for constants
struct ExprToken;
class ExprValue::Internal {
public:
  mpf_class   m_number;
  std::string m_string;
  bool        m_isString;
  bool        m_usesVariable;
  Internal() : m_number(0, 64), m_isString(false), m_usesVariable(false) {
  }
  static OU::ExprValue::Internal fdummy;
  // Numbers can express base by 0[digit] octal, or 0t, 0b, 0x for decimal, binary, hex
  // If none, then defaultBase is used. Dots are only considered if "dot" is true
  static const char *
  getNumber(const char *&cp,  const char *last, const char *&start, const char *&end,
	    unsigned &base, uint32_t &multiply, bool dot, unsigned defaultBase) {
    base = defaultBase;
    if (*cp == '0' && cp < last - 1) {
      base =
	isdigit(cp[1]) ? (cp++, 8) :
	(cp[1] == 'x' || cp[1] == 'X') ? (cp += 2, 16) :
	(cp[1] == 'b' || cp[1] == 'B') ? (cp += 2, 2) :
	(cp[1] == 't' || cp[1] == 'T') ? (cp += 2, 10) :
	defaultBase;
      if (cp == last)
	return "invalid numeric constant";
    } else if ((dot && *cp == '.') || isdigit(*cp))
      base = defaultBase;
    else
      return "invalid numeric constant";
    start = cp;
    // Find the separation between mantissa and exponent
    while (++cp < last && *cp && (((dot && *cp == '.') || isdigit(*cp) ||
				   (base == 16 && strchr("aAbBcCdDeEfF", *cp)))))
      ;
    // now deal with any binary multiplier (k/m/g)
    multiply = 1;
    if (cp < last && *cp && strchr("kKmMgG", *cp))
      switch(*cp++) {
      case 'k': case 'K': multiply = 1024; break;
      case 'm': case 'M': multiply = 1024*1024; break;
      case 'g': case 'G': multiply = 1024*1024*1024; break;
      default:;
      }
    end = cp;
    return NULL;
  }
  const char *
  lex(const char *&cp, const char *last, const char *&start, const char *&end, OpCode &op) {
    op = OpLimit;
    while (cp != last && isspace(*cp))
      cp++;
    start = cp;
    do {
      if (!*cp || cp == last) {
	op = OpEnd;
	break;
      }
      if (isalpha(*cp) || *cp == '_') {
	do
	  cp++;
	while (*cp && cp != last && (isalpha(*cp) || *cp == '_' || isdigit(*cp)));
	op = OpIdent;
	end = cp;
	break;
      }
      if (*cp == '"') {
	for (start = ++cp; *cp && cp != last && *cp != '\"'; cp++)
	  if (*cp == '\\' && !*++cp && cp == last)
	    break;
	if (!*cp || cp == last)
	  return "missing close quote on string";
	end = cp++;
	op = OpConstant;
	m_string.assign(start, end - start);
	m_isString = true;
	break;
      }
      // numbers start with digits or periods, minus sign is a unary operator
      if (isdigit(*cp) || (*cp == '.' && cp < last-1 && isdigit(cp[1]))) {
	const char *mstart;
	unsigned mbase;
	uint32_t mmult;
	const char *err;
	if ((err = getNumber(cp, last, mstart, end, mbase, mmult, true, 10)))
	  return err;
	std::string mpf;
	mpf.assign(mstart, (end - mstart) - (mmult == 1 ? 0 : 1));
	if (cp < last && (*cp == 'e' || *cp == 'E' || *cp == '@')) {
	  mpf += *cp++;
	  if (cp < last && (*cp == '-' || *cp == '+'))
	    mpf += *cp++;
	  if (cp == last)
	    return "invalid exponent";
	  const char *estart;
	  unsigned ebase;
	  uint32_t emult;
	  // We cannot allow implicit octal since %f and %g in sprintf insert extra zeros
	  if (*cp == '0' && isdigit(cp[1]) && cp <= last)
	    cp++;
	  if ((err = getNumber(cp, last, estart, end, ebase, emult, false, mbase)))
	    return err;
	  // gmp has constraints about exponent base which we don't, so we need to forcible
	  // convert it to decimal
	  errno = 0;
	  char *eend;
	  unsigned long ul = strtoul(estart, &eend, ebase);
	  if (errno || eend != end)
	    return "invalid exponent";
	  ul *= emult;
	  OU::formatAdd(mpf, "%lu", ul);
	}
	if (m_number.set_str(mpf, -mbase))
	  return OU::esprintf("invalid numeric constant: '%s'", mpf.c_str());
	m_number *= mmult;
	op = OpConstant;
	cp = end;
	break;
      }
      op = OpLimit;
      for (unsigned n = 0; n < OpEnd; n++)
	if (*cp == opNames[n][0]) {
	  if (opNames[n][1]) {
	    if (cp < last-1 && opNames[n][1] == cp[1]) {
	      cp++;
	      op = (OpCode)n;
	      break;
	    }
	  } else
	    op = (OpCode)n;
	}
      if (op == OpLimit)
	return "illegal token";
      end = ++cp;
    } while (0);
    while (cp < last && isspace(*cp))
      cp++;
    return NULL;
  }

  void setInternal(ExprValue &val) {
    assert(!val.m_internal);
    val.m_internal = this;
  }
  const char *getString(std::string &s) const {
    if (m_isString)
      s = m_string;
    else
      mpfString(m_number, s);
    return s.c_str();
  }
  const char
  *reduce(ExprToken *start, ExprToken *&end, bool parens = false),
    *parse(const char *buf, const char *end, ExprToken *&tokens, const IdentResolver *resolve);
};

struct ExprToken {
  OpCode op;
  const char *start, *end; 
  ExprValue::Internal value;
  void string2Number() {
    if (op == OpConstant && value.m_isString) {
      value.m_number = value.m_string.size() ? 1 : 0;
      value.m_isString = false;
    }
  }
};

// The expression can now be evaluated backwards, with the RHS value at t[-1].
// Parens means eliminate start and end
// Otherwise "end" needs to be preserved.
// "end" is set to point to the preserved token at the end
const char * ExprValue::Internal::
reduce(ExprToken *start, ExprToken *&end, bool parens) {
  if (end != start + 1) {
    ExprToken *t;
    for (t = end; --t > start;) {
      if (t->op != OpConstant)
	return "expression syntax";
      if (t->value.m_isString && !opStringOk[t[-1].op])
	return "operator illegal for string";
      OpCode op = t[-1].op;
      t[-1].op = OpConstant;
      bool b;
      switch (op) {
      case OpTilde:
	// If it is an integer, then complement it
	if (ceil(t->value.m_number) != t->value.m_number)
	  return "tilde operator on non-integer value";
	{
	  mpz_class i(t->value.m_number);
	  i = ~i;
	  t[-1].value.m_number = i;
	}
	break;
      case OpNot:
	b = t->value.m_isString ? t->value.m_string.size() != 0 : t->value.m_number != 0;
	t[-1].value.m_number = b ? 1 : 0;
	t[-1].value.m_isString = false;
	break;
      case OpUPlus: // not useful for anything
	t[-1].value = t->value;
	break;
      case OpUMinus:
	t[-1].value.m_number = - t->value.m_number;
	break;
      case OpPlus:
	if (t[-2].op != OpConstant)
	  return "expression syntax";
	if (t[-2].value.m_isString != t->value.m_isString)
	  return "mixing strings and numbers";
	if (t->value.m_isString)
	  t[-2].value.m_string += t->value.m_string;
	else
	  t[-2].value.m_number += t->value.m_number;
	t--;
	break;
      case OpPow:
	if (t[-2].op != OpConstant)
	  return "expression syntax";
	mpf_pow_ui(t[-2].value.m_number.get_mpf_t(), t[-2].value.m_number.get_mpf_t(),
		   t->value.m_number.get_ui());
	t--;
	break;

#define NumOp(tokenOp, opn)				\
	case tokenOp:					\
	  if (t[-2].op != OpConstant)			\
	    return "expression syntax";			\
	  t[-2].value.m_number opn t->value.m_number;	\
	  t--;						\
	  break
	NumOp(OpMinus, -=);
	NumOp(OpMult, *=);
	NumOp(OpDiv, /=);
	// These are operations that must be converted to integers
#define BinOp(tokenOp, op)						\
	case tokenOp: {							\
	  mpz_class op1, op2;						\
	  const char *err;						\
	  if ((err = mpf2mpz(t[-2].value.m_number, op1)) ||		\
	      (err = mpf2mpz(t[0].value.m_number, op2)))		\
	    return err;							\
	  op1 op op2;							\
	  mpf_set_z(t[-2].value.m_number.get_mpf_t(), op1.get_mpz_t()); \
	  t--;								\
	  break;							\
	}
	BinOp(OpMod, %=);
	BinOp(OpXor, ^=);
	BinOp(OpBor, |=);
	BinOp(OpBand, &=);
#define ShiftOp(tokenOp, op)						\
	case tokenOp: {							\
	  mpz_class op1, op2;						\
	  const char *err;						\
	  if ((err = mpf2mpz(t[-2].value.m_number, op1)) ||		\
	      (err = mpf2mpz(t[0].value.m_number, op2)))		\
	    return err;							\
	  unsigned long int ui = op2.get_ui();				\
	  op1 op ui;							\
	  mpf_set_z(t[-2].value.m_number.get_mpf_t(), op1.get_mpz_t());	\
	  t--;								\
	}								\
	break
	ShiftOp(OpSl, <<=);
	ShiftOp(OpSr, >>=);
#define CmpOp(tokenOp, cop)					\
	case tokenOp:						\
	  if (t[-2].op != OpConstant)				\
	    return "expression syntax";				\
	  if (t[-2].value.m_isString != t->value.m_isString)	\
	    return "cannot compare strings and numbers";	\
	  b = t[-2].value.m_isString ?				\
	    t[-2].value.m_string cop t->value.m_string :		\
	    t[-2].value.m_number cop t->value.m_number;		\
	  t[-2].value.m_number = b ? 1 : 0;			\
	  t[-2].value.m_isString = false;			\
	  t--;							\
	  break;
	CmpOp(OpEq, ==);
	CmpOp(OpNeq, !=);
	CmpOp(OpLt, <);
	CmpOp(OpGt, >);
	CmpOp(OpLe, <=);
	CmpOp(OpGe, >=);
      case OpLor:
      case OpLand:
	t[-2].string2Number();
	t[0].string2Number();
	t[-2].value.m_number = op == OpLor ?
	  mpf2bool(t[-2].value.m_number) || mpf2bool(t[0].value.m_number) :
	  mpf2bool(t[-2].value.m_number) && mpf2bool(t[0].value.m_number);
	t--;
	break;
      case OpCond2:
	if (t < start + 4 || t[-3].op != OpCond1 || t[-2].op <= OpEnd)
	  return "bad conditional operator syntax";
	t[-4].string2Number();
	t[-4] = mpf2bool(t[-4].value.m_number) ? t[-2] : t[0];
	t -= 3;
	break;
      default:
	return "bad operator syntax";
      }
    }
    if (t != start)
      return "bad expression syntax";
  }
  if (parens) {
    start[-1] = start[0]; // clobber lparen with computed value
    start--;
  } else {
    start[1] = end[0];    // move new token that triggered us after computed value
    start++;
  }
  end = start;
  return NULL;
}

const char *ExprValue::Internal::
parse(const char *buf, const char *end, ExprToken *&tokens, const IdentResolver *resolver) {
  OpCode op;
  unsigned nTokens = 0, nParens = 0;
  const char *cp, *dummy;
  const char *err;
  Internal dumval;
  
  pthread_once(&once, init);
  for (cp = buf; !(err = dumval.lex(cp, end, dummy, dummy, op)) && op != OpEnd; nTokens++)
    ;
  if (err)
    return err;
  bool usesVariable = false;
  ExprToken *lpar = 0, *t = tokens = new ExprToken[++nTokens];
  cp = buf;
  do {
    t->value.lex(cp, end, t->start, t->end, t->op); // can ignore errors in second pass
    switch ((op = t->op)) {
    case OpConstant:
      // These values are set in lex()
      break;
    case OpIdent:
      {
	std::string sym(t->start, t->end - t->start);
	if (!strcasecmp(sym.c_str(), "false")) {
	  t->op = OpConstant;
	  t->value.m_isString = false;
	  t->value.m_number = 0;
	} else if (!strcasecmp(sym.c_str(), "true")) {
	  t->op = OpConstant;
	  t->value.m_isString = false;
	  t->value.m_number = 1;
	} else if (!resolver)
	  return "no symbols are available for this expression";
	else {
	  OU::ExprValue v;
	  if ((err = resolver->getValue(sym.c_str(), v)))
	    return err;
	  t->value = *v.m_internal;
	  t->op = OpConstant;
	  usesVariable = true;
	  std::string s;
	  //ocpiDebug("Expression variable %s has value %s", sym.c_str(), t->value.getString(s));
	}
      }
      // convert to string value or number value
      break;
    case OpLpar:
      nParens++;
      lpar = t;
      break;
    case OpRpar:
      if (!nParens)
	return "unmatched parens";
      nParens--;
      if ((err = reduce(lpar+1, t, true))) // return value points to resolved value
	return err;
      if (nParens)
	for (lpar--; lpar->op != OpLpar; lpar--)
	  ;
      else
	lpar = 0;
      break;
    case OpEnd:
      // These MUST cause an evaluation
      if ((err = reduce(tokens, t)))
	return err;
      break;
      // Unary operators that are only unary operators, nothing to do
    case OpTilde:
    case OpNot:
      break;
      // Unary operators that are also binary operators
    case OpPlus:
    case OpMinus:
      // if it is unary, skip it
      if (t == tokens || t[-1].op < OpRpar) {
	t->op = op == OpPlus ? OpUPlus : OpUMinus;
	break;
      }
      // fall through to all binary operators
    default:
      // we have a binary or conditional operator
      if (t == tokens || t[-1].op < OpEnd)
	return esprintf("binary operator \"%s\" with no value on left side",
			opNames[t->op]);
      if (t > (lpar ? lpar : tokens) + 2 && t->op < t[-2].op)
	if ((err = reduce(lpar ? lpar + 1 : tokens, t)))
	  return err;
    }
    t++;
  } while (op != OpEnd);
  *this = tokens[0].value;
  m_usesVariable = usesVariable;
  return NULL;
}

const char *evalExpression(const char *start, ExprValue &val, const IdentResolver *resolver,
			   const char *end) {
  if (!end)
    end = start + strlen(start);
  ExprValue::Internal *v = new ExprValue::Internal();
  ExprToken *tokens = 0;
  const char *err = v->parse(start, end, tokens, resolver);
  delete [] tokens;
  v->setInternal(val);
  std::string s;
  //ocpiDebug("Evaluating expression: %.*s err: \"%s\" value: \"%s\"",
  //	    (int)(end - start), start, err ? err : "", val.getString(s));
  return
    err ? esprintf("when parsing expression \"%.*s\": %s", (int)(end-start), start, err) :
    NULL;
}

IdentResolver::~IdentResolver() {}

// Evaluate the expression, using the resolver, and if the expression was variable,
// save the expression so it can be reevaluated again later when the values of
// variables are different.
const char *
parseExprNumber(const char *a, size_t &np, std::string *expr, const IdentResolver *resolver) {
  ExprValue v;
  const char *err = evalExpression(a, v, resolver);
  if (!err) {
    if (!v.isNumber())
      err = esprintf("the expression \"%s\" does not evaluate to a number", a);
    else {
      np = OCPI_UTRUNCATE(size_t, v.getNumber());
      if (expr) {
	expr->clear();
	if (v.isVariable()) {
	  //ocpiDebug("Found an expression: '%s' that was based on variables. prev:'%s'",
	  //		    a, expr->c_str());
	  *expr = a; // provide the expression to the caller in a string
	}
      }
    }
  }
  return err;
}

// Parse an integer (size_t) attribute that might be an expression
// Only consider if we have an identifier resolver
// The string value of the expression is returned in expr.
const char *
getExprNumber(ezxml_t x, const char *attr, size_t &np, bool *found, std::string *expr,
	      const IdentResolver *resolver) {
  const char *a = ezxml_cattr(x, attr);
  if (a) {
    if (found)
      *found = true;
    return parseExprNumber(a, np, expr, resolver);
  }
  if (found)
    *found = false;
  return NULL;
}

const char *
makeCexpression(const char *cp, const char *prefix, const char *suffix, bool toUpper, std::string &out) {
  ExprToken t;
  const char *err;
  ExprValue::Internal dumval;
  while (!(err = dumval.lex(cp, cp + strlen(cp), t.start, t.end, t.op)) && t.op != OpEnd) {
    std::string before, after;
    //	const char *end;
    switch (t.op) {
    case OpConstant:
      assert(!dumval.m_isString);
      break;
    case OpIdent:
      before = prefix ? prefix : "";
      for (; t.start != t.end; t.start++)
	after += toUpper ? (char)toupper(*t.start) : *t.start;
      if (suffix)
	after += suffix;
      break;
    default:;
    }
    out += before;
    if (t.start != t.end)
      out.append(t.start, t.end - t.start);
    out += after;
  }
  return err;
}

ExprValue::ExprValue() : m_numberSet(false), m_internal(NULL) {}
ExprValue::~ExprValue() { delete m_internal; }
void ExprValue::setString(const char *s) {
  if (!m_internal)
    m_internal = new Internal;
  m_internal->m_string = s;
  m_internal->m_isString = true;
}
void ExprValue::setNumber(int64_t i) {
  if (!m_internal)
    m_internal = new Internal;
  m_internal->m_isString = false;
  int64_2_mpf(i, m_internal->m_number);
}

// Extract and convert the almost-untyped ExprValue into the typed OU::Value
const char *ExprValue::
getTypedValue(Value &v, size_t index) const {
  const char *err;
  bool items = v.m_vt->m_isSequence || v.m_vt->m_arrayRank;
  bool isSigned = false; 
  std::string s;
  switch (v.m_vt->m_baseType) {
  case OA::OCPI_Char: case OA::OCPI_Short: case OA::OCPI_Long: case OA::OCPI_LongLong:
    isSigned = true;
  case OA::OCPI_UChar: case OA::OCPI_ULong: case OA::OCPI_UShort: case OA::OCPI_ULongLong:
  case OA::OCPI_Enum:
    // Handle integral types
    if (!isNumber())
      return "Non-numeric expression value invalid for numeric type";
    if (!isSigned && sgn(m_internal->m_number) < 0)
      return "Negative expression value assigned to unsigned type";
    {
      mpz_class z;
      if ((err = mpf2mpz(m_internal->m_number, z)))
	return err;
      if (z < mpz_min[v.m_vt->m_baseType] ||
	  z > mpz_max[v.m_vt->m_baseType] ||
	  (v.m_vt->m_baseType == OA::OCPI_Enum && z >= v.m_vt->m_nEnums))
	return esprintf("Expression value (%s) is out of range for %s type properties",
			mpfString(m_internal->m_number, s), baseTypeNames[v.m_vt->m_baseType]);
      mpz_class tmp = z & (uint32_t)-1;
      uint32_t low32 = (uint32_t)tmp.get_ui();
      tmp = z >>= 32;
      tmp &= (uint32_t)-1;
      int32_t high32 = (int32_t)tmp.get_ui();
      uint64_t val = ((uint64_t)high32 << 32) | low32;
      //ocpiDebug("gettypedvalue from '%s' %" PRIu64, mpfString(m_internal->m_number, s), val);
      switch (v.m_vt->m_baseType) {
#define DO_INT(pretty,x)							\
	case OA::OCPI_##pretty:						   \
	  (items ? v.m_p##x[index] : v.m_##x) = (OA::x)val; \
	    break
	DO_INT(Char,Char);
	DO_INT(UChar,UChar);
	DO_INT(Short,Short);
	DO_INT(UShort,UShort);
	DO_INT(Long,Long);
	DO_INT(ULong,ULong);
	DO_INT(LongLong,LongLong);
	DO_INT(ULongLong,ULongLong);
	DO_INT(Enum,ULong);
      default:;
      }
    }
    break; // done with integral types
  case OA::OCPI_Bool:
    (items ? v.m_pBool[index] : v.m_Bool) = 
      isNumber() ? m_internal->m_number != 0 : !m_internal->m_string.empty();
    break;
  case OA::OCPI_String:
    if (isNumber())
      return "A numeric expression cannot be assigned to a string property";
    v.reserveStringSpace(m_internal->m_string.length(), items); 
    (items ? v.m_pString[index] : v.m_String) = v.m_stringNext;
    for (const char *cp = m_internal->m_string.c_str(); v.setNextStringChar(*cp); ++cp)
      ;
    break;
  case OA::OCPI_Float:
    if (!isNumber())
      return "A string value cannot be assigned to a float property";
    if (m_internal->m_number < -std::numeric_limits<float>::max() ||
	m_internal->m_number > std::numeric_limits<float>::max())
      return esprintf("Value %s out of range for type: float",
		      mpfString(m_internal->m_number, s));
    (items ? v.m_pFloat[index] : v.m_Float) = (float)m_internal->m_number.get_d();
    break;
  case OA::OCPI_Double:
    if (!isNumber())
      return "A string value cannot be assigned to a double property";
    if (m_internal->m_number < -std::numeric_limits<double>::max() ||
	m_internal->m_number > std::numeric_limits<double>::max())
      return esprintf("Value %s out of range for type: double",
		      mpfString(m_internal->m_number, s));
    (items ? v.m_pDouble[index] : v.m_Double) = m_internal->m_number.get_d();
    break;
  default:;
  }
  return NULL;
}

// Set the ExprValue from the typed OU::Value
const char *ExprValue::setFromTypedValue(const Value &v) {
  if (!m_internal)
    m_internal = new Internal;
  m_internal->m_isString = false;
  m_internal->m_usesVariable = false;
  switch (v.m_vt->m_baseType) {
  case OA::OCPI_Bool: m_internal->m_number = v.m_Bool ? 1 : 0; break;
  case OA::OCPI_UChar: m_internal->m_number = v.m_UChar; break;
  case OA::OCPI_UShort: m_internal->m_number = v.m_UShort; break;
  case OA::OCPI_ULong: m_internal->m_number = v.m_ULong; break;
  case OA::OCPI_Char: m_internal->m_number = v.m_Char; break;
  case OA::OCPI_Short: m_internal->m_number = v.m_Short; break;
  case OA::OCPI_Long: m_internal->m_number = v.m_Long; break;
  case OA::OCPI_LongLong: int64_2_mpf(v.m_LongLong, m_internal->m_number); break;
  case OA::OCPI_ULongLong: uint64_2_mpf(v.m_ULongLong, m_internal->m_number); break;
  case OA::OCPI_Float: m_internal->m_number = v.m_Float; break;
  case OA::OCPI_Double: m_internal->m_number = v.m_Double; break;
  case OA::OCPI_String:
    m_internal->m_isString = true;
    m_internal->m_string = v.m_String;
    break;
  default:
    return "value is of an unsupported type";
  }
  return NULL;
}

bool ExprValue::isNumber() const {
  assert(m_internal);
  return !m_internal->m_isString;
}

bool ExprValue::isVariable() const {
  assert(m_internal);
  return m_internal->m_usesVariable;
}

int64_t ExprValue::getNumber() const {
  if (!m_numberSet) {
    assert(!m_internal->m_isString);
    mpz_class z;
    ocpiCheck(mpf2mpz(m_internal->m_number, z) == NULL);
    m_numberSet = true;
    m_number = z.get_si();
  }
  return m_number;
}

const char *ExprValue::getString(std::string &s) const {
  if (!m_internal)
    s = "";
  else
    m_internal->getString(s);
  return s.c_str();
}

}
}

#if TEST_EXPR_EVALUATOR
namespace OU = OCPI::Util;
int main(int argc, char **argv) {
  const char *err;
  const char *start, *end;
  OpCode op;
  for (const char *cp = argv[1];
       !(err = lex(cp, cp + strlen(cp), start, end, op)) && op != OpEnd; )
    switch (op) {
    case OpString:
      printf("String is: \"%.*s\"\n", (int)(end - start), start);
      break;
    case OpNumber:
      printf("Number is: \"%.*s\"\n", (int)(end - start), start);
      break;
    case OpIdent:
      printf("Ident is: \"%.*s\"\n", (int)(end - start), start);
      break;
    default:
      printf("Op is: \"%s\"\n", opNames[op]);
    }
  if (err) {
    printf("Error lexing: %s\n", err);
    return 1;
  }
  class mine : public OU::IdentResolver {
    const char *getValue(const char *symbol, OU::ExprValue &val) {
      if (!strcasecmp(symbol, "fred")) {
	val.string = "barar";
	val.isNumber = false;
	return NULL;
      }
      return "identifier not defined";
    }
  } me;
  OU::ExprValue v;
  if ((err = OU::evalExpression(argv[1], v, &me)))
    printf("Error eval: %s\n", err);
  else if (v.isNumber)
    printf("Value is number: %lld 0(x%llx)\n", (long long)v.number, (long long)v.number);
  else
    printf("Value is string: \"%s\"\n", v.string.c_str());
  return 0;
}
#endif

