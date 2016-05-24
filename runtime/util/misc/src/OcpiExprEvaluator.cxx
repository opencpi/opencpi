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

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "OcpiUtilMisc.h"
#include "OcpiExprEvaluator.h"

namespace OU = OCPI::Util;
namespace {
#define ALLOPS								\
  OPS(Cond1, "?") OPS(Cond2, ":") OPS(Lor, "||") OPS(Land, "&&")	\
  OP(Bor, "|") OP(Xor, "^") OP(Band, "&")				\
  OPS(Eq, "==") OPS(Neq, "!=") OPS(Lt, "<") OPS(Gt, ">") OPS(Le, "<=") OPS(Ge, ">=") \
  OP(Sl, "<<") OP(Sr, ">>") OP(Plus, "+") OP(Minus, "-") OP(Mult, "*") OP(Div, "/") OP(Mod, "%") \
  OP(UMinus, "") OP(UPlus, "") OP(Tilde, "~") OP(Not, "!") OP(Lpar, "(") OP(Rpar, ")") \
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
  typedef enum { ALLOPS OpEnd, OpString, OpIdent, OpNumber, OpLimit } OpCode;

  static const char *
  lex(const char *&cp, const char *&start, const char *&end, OpCode &op) {
    op = OpLimit;
    while (isspace(*cp))
      cp++;
    start = cp;
    do {
      if (!*cp) {
	op = OpEnd;
	break;
      }
      if (isalpha(*cp) || *cp == '_') {
	do
	  cp++;
	while (isalpha(*cp) || *cp == '_' || isdigit(*cp));
	op = OpIdent;
	end = cp;
	break;
      }
      if (*cp == '"') {
	start = ++cp;
	while (*cp != '\"') {
	  if (!*cp)
	    return "missing close quote on string";
	  else if (*cp == '\\')
	    cp++;
	  cp++;
	}
	end = cp++;
	op = OpString;
	break;
      }
      if (isdigit(*cp)) {
	cp++;
	while (isdigit(*cp)) // || *cp == '.' || *cp == 'e') no floats yet
	  cp++;
	end = cp;
	op = OpNumber;
	break;
      }
      for (unsigned n = 0; n < OpEnd; n++)
	if (*cp == opNames[n][0])
	  if (opNames[n][1]) {
	    if (opNames[n][1] == cp[1]) {
	      cp++;
	      op = (OpCode)n;
	      break;
	    }
	  } else
	    op = (OpCode)n;
      if (op == OpLimit)
	return "illegal token";
      end = ++cp;
    } while (0);
    while (isspace(*cp))
      cp++;
    return NULL;
  }

  struct ExprToken {
    OpCode op;
    const char *start, *end; 
    std::string string;
    int64_t number;

    void string2Number() {
      if (op == OpString) {
	op = OpNumber;
	number = string.size() ? 1 : 0;
      }
    }
  };

  // The expression can now be evaluated backwards, with the RHS value at t[-1].
  // Parens means eliminate start and end
  // Otherwise "end" needs to be preserved.
  // "end" is set to point to the preserved token at the end
  static const char *
  reduce(ExprToken *start, ExprToken *&end, bool parens = false) {
    if (end != start + 1) {
      ExprToken *t;
      for (t = end; --t > start;) {
	if (t->op == OpString && !opStringOk[t[-1].op])
	  return "operator illegal for string";
	if (t->op <= OpEnd)
	  return "operator syntax";
	OpCode op = t[-1].op;
	switch (op) {
	case OpTilde:
	  t[-1].op = OpNumber;
	  t[-1].number = ~t->number;
	  break;
	case OpNot:
	  t[-1].op = OpNumber;
	  t[-1].number = t->op == OpString ? t->string.size() != 0 : t->number != 0;
	  break;
	case OpUPlus:
	case OpUMinus:
	  t[-1].op = OpNumber;
	  t[-1].number = op == OpUMinus ? - t->number : t->number;
	  break;
#define NumOp(tokenOp, op) case tokenOp: t[-2].number op t[0].number; t--; break
	  NumOp(OpMinus, -=);
	  NumOp(OpPlus, +=);
	  NumOp(OpMult, *=);
	  NumOp(OpDiv, /=);
	  NumOp(OpMod, %=);
	  NumOp(OpXor, ^=);
	  NumOp(OpBor, |=);
	  NumOp(OpBand, &=);
	  NumOp(OpSl, <<=);
	  NumOp(OpSr, >>=);
#define CmpOp(tokenOp, cop)						\
	  case tokenOp:							\
	    if (t[-2].op != t[0].op) return "string and non-string with binary operator"; \
	    t[-2].op = OpNumber;					\
	    t[-2].number = t[0].op == OpString ?			\
	      strcasecmp(t[-2].string.c_str(), t[0].string.c_str()) cop 0 : \
	      t[-2].number cop t[0].number;				\
	    t--; break
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
	  t[-2].number = op == OpLor ? t[-2].number || t[0].number : t[-2].number && t[0].number;
	  t--;
	  break;
	case OpCond2:
	  if (t < start + 4 || t[-3].op != OpCond1 || t[-2].op <= OpEnd)
	    return "bad conditional operator syntax";
	  t[-4].string2Number();
	  t[-4] = t[-4].number ? t[-2] : t[0];
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

  static const char *
  parse(const char *buf, OU::ExprValue &val, ExprToken *&tokens,
	const OU::IdentResolver *resolver) {
    OpCode op;
    unsigned nTokens = 0, nParens = 0;
    const char *cp, *dummy, *end;
    const char *err;
  
    for (const char *cp = buf; !(err = lex(cp, dummy, dummy, op)) && op != OpEnd; nTokens++)
      ;
    if (err)
      return err;
    val.isVariable = false;
    ExprToken *lpar = 0, *t = tokens = new ExprToken[++nTokens];
    cp = buf;
    do {
      lex(cp, t->start, t->end, t->op);
      switch ((op = t->op)) {
      case OpString:
	// FIXME: remove escaped quotes???
	t->string.assign(t->start, t->end - t->start);
	break;
      case OpNumber:
	t->number = strtoll(t->start, (char **)&end, 0);
	if (end != t->end)
	  return "bad number syntax";
	switch (*t->end) {
	case 'k': case 'K': t->number *= 1024; t->end++; cp++; break;
	case 'm': case 'M': t->number *= 1024*1024; t->end++; cp++; break;
	case 'g': case 'G': t->number *= 1024ul*1024ul*1024ul; t->end++; cp++; break;
	}
	break;
      case OpIdent:
	{
	  std::string sym(t->start, t->end - t->start);
	  OU::ExprValue v;
	  if (!resolver)
	    return "no symbols are available for this expression";
	  if ((err = resolver->getValue(sym.c_str(), v)))
	    return err;
	  val.isVariable = true;
	  if (v.isNumber) {
	    t->op = OpNumber;
	    t->number = v.number;
	  } else {
	    t->op = OpString;
	    t->string = v.string;
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
	  return "binary operator with no value on left side";
	if (t > (lpar ? lpar : tokens) + 2 && t->op < t[-2].op)
	  if ((err = reduce(lpar ? lpar + 1 : tokens, t)))
	    return err;
      }
      t++;
    } while (op != OpEnd);
    if (tokens[0].op == OpNumber) {
      val.isNumber = true;
      val.number = tokens[0].number;
    } else {
      val.isNumber = false;
      val.string = tokens[0].string;
    }
    return NULL;
  }
}
namespace OCPI {
  namespace Util {
    const char *evalExpression(const char *string, ExprValue &val,
			       const IdentResolver *resolver) {
      ExprToken *tokens = 0;
      const char *err = parse(string, val, tokens, resolver);
      delete [] tokens;
      return err ? esprintf("when parsing expression \"%s\": %s", string, err) : NULL;
    }
    IdentResolver::~IdentResolver() {}
    // Evaluate the expression, using the resolver, and if the expression was variable,
    // save the expression so it can be reevaluated again later when the values of
    // variables are different.
    const char *
    parseExprNumber(const char *a, size_t &np, std::string *expr,
		    const IdentResolver *resolver) {
      ExprValue v;
      const char *err = evalExpression(a, v, resolver);
      if (!err) {
	if (!v.isNumber)
	  err = esprintf("the expression \"%s\" does not evaluate to a number", a);
	np = v.number;
	if (expr && v.isVariable)
	  *expr = a; // provide the expression to the caller in a string
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
    const char *makeCexpression(const char *cp, const char *prefix, const char *suffix,
				bool toUpper, std::string &out) {
      ExprToken t;
      const char *err;
      while (!(err = lex(cp, t.start, t.end, t.op)) && t.op != OpEnd) {
	std::string before, after;
	const char *end;
	switch (t.op) {
	case OpNumber:
	  t.number = strtoll(t.start, (char **)&end, 0);
	  if (end != t.end)
	    return "bad number syntax";
	  before = "(";
	  switch (*t.end) {
	  case 'k': case 'K': after = "*1024)"; cp++; break;
	  case 'm': case 'M': after = "*1024*1024)"; cp++; break;
	  case 'g': case 'G': after = "*1024ul*1024ul*1024ul)"; cp++; break;
	  default:
	    before = "";
	  }
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
  }
}
#if TEST_EXPR_EVALUATOR
namespace OU = OCPI::Util;
int main(int argc, char **argv) {
  const char *err;
  const char *start, *end;
  OpCode op;
  for (const char *cp = argv[1]; !(err = lex(cp, start, end, op)) && op != OpEnd; )
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
