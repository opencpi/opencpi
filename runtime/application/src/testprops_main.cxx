#include "OcpiApplication.h"
#include <cstdio>
#include <limits>
#pragma GCC diagnostic ignored "-Wfloat-equal"
namespace OA = OCPI::API;
namespace OU = OCPI::Util;
#define OCPI_OPTIONS_HELP \
  "Usage syntax is: ocpiexpr [options] expression\n" \
  "This command evaluates the expression, with the provided variables and result type.\n"

#define OCPI_OPTIONS \
  CMD_OPTION(loglevel,      l,   UChar,  "0",    "The logging level to be used during operation")

#include "CmdOption.h"

template <typename VT> void
printType(const VT val) {
  if (std::numeric_limits<VT>::is_signed)
    printf("%lld(%llx) ", (long long)val, (long long)val);
  else
    printf("%llu(%llx) ", (unsigned long long)val, (unsigned long long)val);
}
template <> void
printType<std::string>(std::string val) {
  printf("%s", val.c_str());
}
template <> void
printType<float>(float val) {
  printf("float: %g", val);
}
template <> void
printType<double>(double val) {
  printf("double: %g", val);
}

template <typename VT> void
doVType(OA::Application &app, OA::BaseType t, const char *name, const char *desc, VT val,
	OA::AccessList &list) {
  printf("=========== Setting %s %s value(", desc, OU::baseTypeNames[t]);
  printType(val);  
  printf(")\n");
  try {
    VT value = val;
    app.setPropertyValue("ptest", name, static_cast<VT>(value), list);
    VT rval;
    rval = app.getPropertyValue<VT>("ptest", name, list);
    assert(value == rval);
    app.getPropertyValue("ptest", name, rval, list);
    assert(value == rval);
  } catch (std::string &e) {
    printf("String exception: %s\n", e.c_str());
  } catch (...) {
    printf("Unknown exception\n");
  }
}

template <typename T> void
doPType(OA::Application &app, const char *name, OA::AccessList list = OA::emptyList) {
    printf("======== Setting %s property\n", name);

#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)		\
    doVType(app, OA::OCPI_##pretty, name, "min",			\
	    static_cast<run>(std::numeric_limits<run>::is_integer ?	\
			     std::numeric_limits<run>::min() :		\
			     -std::numeric_limits<run>::max()), list);	\
    doVType(app, OA::OCPI_##pretty, name, "max", std::numeric_limits<run>::max(), list);
    OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE_S OCPI_DATA_TYPE
      doVType(app, OA::OCPI_String, name, "string", std::string("abc123"), list);
  // try the others
  app.setPropertyValue("ptest", "pString", "abc");
  std::string a;
  app.getPropertyValue("ptest", "pString", a);
  assert(a == "abc");
}

static int mymain(const char **) {
  //  unsigned long x = (char)-1;
  //  printf("UL: %lx\n", x);
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)		\
  {									\
    union {								\
      run m_run;							\
      uint64_t m_store;							\
    } umin, umax, ulow;							\
    umin.m_store = umax.m_store = ulow.m_store = 0;			\
    umin.m_run = std::numeric_limits<run>::min();			\
    umax.m_run = std::numeric_limits<run>::max();			\
    printf("%s: special %u signed %u min %" PRIx64 " max %" PRIx64 " low %" PRIx64 " max %g\n", #pretty, \
	   std::numeric_limits<run>::is_specialized,			\
	   std::numeric_limits<run>::is_signed,				\
	   umin.m_store,						\
	   umax.m_store,						\
	   ulow.m_store,						\
	   (double)umax.m_run						\
	   );								\
  }
  OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE_S OCPI_DATA_TYPE

    OA::PValue params[] =
    {OA::PVBool("dump", true), OA::PVBool("verbose", true), OA::PVString("model","=rcc"), OA::PVEnd};
  OA::Application app("<application>"
		      "  <instance component='ocpi.ptest'/>"
		      "</application>", params);
  app.initialize();
  app.start();

#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)		\
  doPType<OA::pretty>(app, "p" #pretty);
  OCPI_PROPERTY_DATA_TYPES

// This is only necessary for GCC 4.4 that doesn't fully implement initialize_lists
#ifdef __APPLE__
#define A(...) {__VA_ARGS__}
#else
#define A(...) OA::AccessList({__VA_ARGS__})
#endif
  doPType<OA::ULong>(app, "ap4", {4});
  app.setPropertyValue("ptest", "ap4", 7, A(3));
  app.setPropertyValue("ptest", "stest", 3.4f, A("flt"));
  app.setPropertyValue("ptest", "stest", true, A("boola", 2));
  app.setPropertyValue("ptest", "stest", "hello", A("str"));
  app.setPropertyValue("ptest", "stestp", true, A(1, "boola", 2));

  app.wait();
  app.finish();
  return 0;
}
