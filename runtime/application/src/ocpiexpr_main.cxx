#include <ctype.h>
#include <inttypes.h>
#include <string.h>
#include <strings.h>
#include <map>
#include <string>
#include "OcpiOsDebug.h"
#include "OcpiExprEvaluator.h"
#include "OcpiUtilValue.h"
#include "OcpiUtilMisc.h"

namespace OU = OCPI::Util;
namespace OA = OCPI::API;

#define OCPI_OPTIONS_HELP \
  "Usage syntax is: ocpiexpr [options] expression\n" \
  "This command evaluates the expression, with the provided variables and result type.\n"

#define OCPI_OPTIONS \
  CMD_OPTION(type,          t,   String, "Long", "The type of result") \
  CMD_OPTION(as_value,      a,   Bool,   0,      "Treat the expr as a value for the type") \
  CMD_OPTION_S(variable,    v,   String, NULL,   "Define a variable, e.g. a[:type]=4.5") \
  CMD_OPTION(hex,           x,   Bool,   0,      "print numeric values in hex, not decimal")\
  CMD_OPTION(c_expression,  c,   Bool,   0,      "generate a C expression")\
  CMD_OPTION(loglevel,      l,   UChar,  "0",    "The logging level to be used during operation")

#include "CmdOption.h"

static int mymain(const char **ap) {
  if (options.loglevel())
    OCPI::OS::logSetLevel(options.loglevel());
  size_t nvars;
  struct Vars : OU::IdentResolver {
    typedef std::map<std::string,OU::Value *> Map;
    typedef Map::const_iterator MapIter;
    Map map;
    Vars(const char **argv) {
      for (const char **vars = argv; vars && *vars; vars++) {
	const char
	  *eq = strchr(*vars, '='),
	  *colon = strchr(*vars, ':');
	if (!eq)
	  options.bad("No equal sign in variable assignment: %s", *vars);
	std::string var;
	OA::BaseType type;
	if (colon && colon < eq) {
	  var.assign(*vars, colon - *vars);
	  std::string typeName;
	  typeName.assign(colon + 1, eq - (colon + 1));
	  const char **tp;
	  for (tp = OU::baseTypeNames; *tp; tp++)
	    if (!strcasecmp(typeName.c_str(), *tp))
	      break;
	  if (!*tp)
	    options.bad("Unknown variable type: \"%s\"", typeName.c_str());
	  type = (OA::BaseType)(tp - OU::baseTypeNames);
	  if (type == OA::OCPI_Enum || type == OA::OCPI_Struct || type == OA::OCPI_Type)
	    options.bad("Ivalid variable type: \"%s\"", typeName.c_str());
	} else {
	  var.assign(*vars, eq - *vars);
	  type = OA::OCPI_Long;
	}
	OU::ValueType *vt = new OU::ValueType(type);
	OU::Value *v = new OU::Value(*vt);
	const char *err;
	if ((err = v->parse(eq + 1)))
	  options.bad(err);
	map[var] = v;
      }
      for (MapIter i = map.begin(); i != map.end(); i++) {
	OU::Value *v = i->second;
	std::string pretty;
	v->unparse(pretty);
	printf("%s: type: %s value: %s\n", i->first.c_str(),
	       OU::baseTypeNames[v->m_vt->m_baseType], pretty.c_str());
      }
    }
    const char *getValue(const char *sym, OU::ExprValue &val) const {
      printf("getValue: %s\n", sym);
      std::string s(sym);
      MapIter mi = map.find(s);
      if (mi == map.end())
	return OU::esprintf("Unknown identifier: %s", sym);
      val.setFromTypedValue(*mi->second);
      return NULL;
    }
  } vars(options.variable(nvars));
  OU::ExprValue val;
  const char *expr = ap[0];
  if (options.c_expression()) {
    std::string out;
    OU::makeCexpression(expr+1, "PREF_", NULL, false, out);
    printf("c expr is: %s\n", out.c_str());
  } else {
    const char *err;
    std::string s;
    if (options.type()) {
      const char **tp;
      for (tp = OU::baseTypeNames; *tp; tp++)
	if (!strcasecmp(options.type(), *tp))
	  break;
      if (!*tp)
	options.bad("Unknown variable type: \"%s\"", options.type());
      OA::BaseType type = (OA::BaseType)(tp - OU::baseTypeNames);
      OU::ValueType vt(type);
      OU::Value v(vt);
      if (options.as_value()) {
	if ((err = v.parse(expr, NULL, false, &vars)))
	  options.bad("error: %s", err);
	v.unparse(s, NULL, false, options.hex());
	printf("Final unparsed typed value: '%s'\n", s.c_str());
      } else if ((err = OU::evalExpression(expr, val, &vars)))
	options.bad("error: %s", err);
      else {
	printf("Expression value is %s:  %s\n", val.isNumber() ? "number" : "string",
	       val.getString(s));
	if ((err = val.getTypedValue(v)))
	  options.bad("When converting to %s: %s", options.type(), err);
#if 0
	// Print to show mismatched LSB due to rounding mode issue with gmp
	static double pp=0.1;
	printf("PP: %" PRIx64 " %.17e expr: %" PRIx64 " %.17e\n",
	       *(uint64_t*)&pp, pp, *(uint64_t*)&v.m_Double, v.m_Double);
#endif
	v.unparse(s, NULL, false, options.hex());
	printf("Converted value for type %s: %s\n", options.type(), s.c_str());
      }
    } else if (options.as_value())
      options.bad("must supply type option with as-value option");
    else if ((err = OU::evalExpression(expr, val, &vars)))
      options.bad("error: %s", err);
    else
      printf("Expression value is %s:  %s\n", val.isNumber() ? "number" : "string",
	     val.getString(s));
  }
  return 0;
}

int
main(int /*argc*/, const char **argv) {
  return options.main(argv, mymain);
}
