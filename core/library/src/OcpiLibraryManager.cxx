#include <string.h>
#include <dirent.h>
#include "OcpiOsFileIterator.h"
#include "OcpiUtilException.h"
#include "OcpiContainerErrorCodes.h"
#include "OcpiLibraryManager.h"
#include <OcpiOsAssert.h>
#include <OcpiExpParser.h>

// This file contains code common to all library drivers

namespace OA = OCPI::API;
namespace OU = OCPI::Util;
namespace OC = OCPI::Container; // only for error codes...
namespace OD = OCPI::Driver;

namespace OCPI {
  namespace Library {
    const char *library = "library";

    // The Library Driver Manager class
    Manager::Manager() {
    }
    void Manager::setPath(const char *path) {
      parent().configureOnce();
      m_libraryPath = path;
    }
    std::string &Manager::getPath() {
      parent().configureOnce();
      return m_libraryPath;
    }
    // The high level entry point to search all libraries in search of
    // an artifact that will support the worker and the offered capabilities
    Artifact &Manager::
    findArtifact(const Capabilities &caps, const char *specName,
		 const OCPI::API::PValue *params,
		 const OCPI::API::PValue *selectCriteria,
		 const OCPI::API::Connection *conns,
		 const char *&artInst) {
      return getSingleton().findArtifactX(caps, specName, params, selectCriteria, conns, artInst);
    }
    Artifact &Manager::
    findArtifactX(const Capabilities &caps, const char *specName,
		  const OCPI::API::PValue *params,
		  const OCPI::API::PValue *selectCriteria,
		  const OCPI::API::Connection *conns,
		  const char *&artInst) {
      parent().configureOnce();
      // If some driver already has it in one of its libraries, return it.
      Artifact *a;
      for (Driver *d = firstDriver(); d; d = d->nextDriver())
	if ((a = d->findArtifact(caps, specName, params, selectCriteria, conns, artInst)))
	  return *a;
      throw OU::Error("No usable artifact found in any library in OCPI_LIBRARY_PATH, "
		      "for worker implementing \"%s\"", specName);
    }

    Artifact &Manager::getArtifact(const char *url, const OA::PValue *params) {
      return getSingleton().getArtifactX(url, params);
    }

    Artifact &Manager::getArtifactX(const char *url, const OA::PValue *params) {
      parent().configureOnce();
      OCPI::Library::Driver *d;
      Artifact *a;
      // If some driver already has it in one of its libraries, return it.
      for (d = firstDriver(); d; d = d->nextDriver())
	if ((a = d->findArtifact(url)))
	  return *a;
      // The artifact was not found in any driver's libraries
      // Now we need to find a library driver that can deal with this
      // artifact. In this case a driver returning NULL means the driver is
      // passing on the artifact.  If there is an error dealing with the
      // artifact, that will throw an exception.
      for (Driver *d = firstChild(); d; d = d->nextChild())
	if ((a = d->addArtifact(url, params)))
	  return *a;
      throw OU::EmbeddedException(OU::ARTIFACT_UNSUPPORTED,
				  "No library driver supports this file",
				  OU::ApplicationRecoverable);
    }


    // Libraries can be specified in the environment
    // We will be given "librarydrivers"
#if 0
    void Manager::configure(ezxml_t x) {
      ezxml_t lib = NULL;
      if (x) {
	for (lib = ezxml_cchild(x, "library"); lib; lib = ezxml_next(lib)) {
	  const char *name = ezxml_cattr(lib, "name");
	  if (!name)
	    throw ApiError("Missing 'name' attribute for 'library' in system configuration", NULL);
	  (new Library(name))->configure(lib);
	}
      }
    }
#endif
    Driver::
    Driver(const char *name)
      : OD::DriverType<Manager,Driver>(name) {
    }
    Artifact *Driver::
    findArtifact(const Capabilities &caps,
		 const char *specName,
		 const OCPI::API::PValue *params,
		 const OCPI::API::PValue *selectCriteria,
		 const OCPI::API::Connection *conns,
		 const char *&artInst) {
      for (Library *l = firstLibrary(); l; l = l->nextLibrary())
	return l->findArtifact(caps, specName, params, selectCriteria, conns, artInst);
      return NULL;
    }

    Library::~Library(){}
    Artifact * Library::
    findArtifact(const Capabilities &caps,
		 const char *specName,
		 const OCPI::API::PValue *params,
		 const OCPI::API::PValue *selectCriteria,
		 const OCPI::API::Connection *conns,
		 const char *&artInst) {

      Artifact * best=NULL;
      int score, best_score=0;
      for (Artifact *a = firstArtifact(); a; a = a->nextArtifact()) {
	if (a->meetsRequirements(caps, specName, params, selectCriteria, conns, artInst, score)) {
	  if ( selectCriteria ) {
	    if ( score >= best_score ) {
	      best = a;
	      best_score = score;
	    }
	  }
	  else {
	    return a;
	  }
	}
      }
      return best;
      }

    // The artifact base class
    Artifact::Artifact() : m_xml(NULL), m_nImplementations(0) {}
    Artifact::~Artifact() {
      delete [] m_implementations;
    }

    class MyVarDefiner : public OCPI::DefineExpVarInterface {
    public:

      struct UndefinedVar : public std::string { UndefinedVar(const char* s):std::string(s){}};

      MyVarDefiner(  const OCPI::API::PValue *p, ezxml_t xml ) 
	:m_props(p),m_xml(xml)
      {

      }

      virtual double defineVariable( const char* var )
	throw (std::string) {
#ifndef NDEBUG
	std::cout << "Defining var = " << var << std::endl;
#endif
	// First see if it is a worker property
	ezxml_t x;
	x = ezxml_cchild(m_xml, "worker");
	for (x = ezxml_cchild(x, "property"); x; x = ezxml_next(x) )
	  {
	    const char* pname = ezxml_attr ( x, "name" );
	    if ( !pname ) {
	      throw std::string("Invalid Worker xml property");
	    }
	    if (strcmp(var,pname)!=0) continue;
	    const char* value = ezxml_attr ( x, "default" );
	    if ( !value ) {
	      std::cerr << "Invalid Worker xml property (must have a default value)" << std::endl;
	      throw UndefinedVar("No Default value");
	      return 0;
	    }
	    double v = atof(value);
#ifndef NDEBUG
	    std::cout << "value s = " << value << " Value = " << v << std::endl;
#endif
	    return v;
	  }

	// Must be an application property
	unsigned int n=0;
	const OCPI::API::PValue *p = m_props;
	while (n<p->length() ) {
	  if ( strcmp( p->name, var ) != 0) {
	    n++;p++;
	    continue;
	  }
	  switch ( p->type ) {
	  case OCPI::API::OCPI_Struct:
	  case OCPI::API::OCPI_Enum:
	  case OCPI::API::OCPI_Type:
	  case OCPI::API::OCPI_none:
	  case OCPI::API::OCPI_scalar_type_limit:
	    {
	      std::cout << "Unsuporrted evaluation property type - Non Fatal " << std::endl;
	    }
	  case OCPI::API::OCPI_Long:
	    {
	      return (double)p->vLong;
	    }
	    break;
	  case OCPI::API::OCPI_Short:
	    {
	      return (double) p->vShort;
	    }
	    break;
	  case OCPI::API::OCPI_Bool:
	    {
	      return (double)p->vBool;
	    }
	    break;
	  case OCPI::API::OCPI_Char:
	    {
	      return (double)p->vChar;
	    }
	    break;
	  case OCPI::API::OCPI_Double:
	    {
	      return (double)p->vDouble;
	    }
	    break;
	  case OCPI::API::OCPI_Float:
	    {
	      return (double) p->vFloat;
	    }
	    break;
	  case OCPI::API::OCPI_UChar:
	    {
	      return (double)p->vUChar;
	    }
	    break;
	  case OCPI::API::OCPI_ULong:
	    {
	      return (double)p->vULong;
	    }
	    break;
	  case OCPI::API::OCPI_UShort:
	    {
	      return (double)p->vUShort;
	    }
	    break;
	  case OCPI::API::OCPI_LongLong:
	    {
	      return (double) p->vLongLong;
	    }
	    break;
	  case OCPI::API::OCPI_ULongLong:
	    {
	      return (double)p->vULongLong;
	    }
	    break;
	  case OCPI::API::OCPI_String:
	    {
	      return (double)0;
	    }
	    break;
	  }
	}
	throw std::string("Invalid property used in evaluation string");
	return 0;
      }

      virtual double defineArrayElement( const char* var, int index )
	throw (std::string) {
	( void ) var;
	return (double)index;
      }

    private:
      const OCPI::API::PValue *m_props;
      ezxml_t m_xml;

    };



    bool 
    Artifact::
    evaluateWorkerSuitability( const OCPI::API::PValue *p, int & score )
    {
      score = 0;
      MyVarDefiner vd(p,m_xml);
      OCPI::Parser exp_parser( &vd );
      OCPI::BooleanEvaluator * be;
      unsigned int n=0;
      const OCPI::API::PValue *prop = p;
      while (n<p->length() ) {
	std::string s = prop->name;
	if ( s.find("__ocpi__") != std::string::npos ) {
	  const char* ex = prop->vString;
	  be = exp_parser.parseEvaluation( ex );
	  bool result;
	  try {
	    result = be->evaluate();
	  }
	  catch ( MyVarDefiner::UndefinedVar & e ) {
	    result = false;
	  }
	  catch ( ... ) {
	    throw;
	  }
#ifndef NDEBUG
	  std::cout << "Result for " << ex << " = " << result << std::endl;
#endif
	  delete be;
	  if ( ! result ) {
	    if ( s.find("-required") != std::string::npos ) {
	      return false;
	    }
	  }
	  else 	if ( s.find("-scored") != std::string::npos ) {
	    int sval=0;
	    sscanf(s.c_str(),"__ocpi__exp-scored %d",&sval);
	    score += sval;
	  }
	}
	n++;
	prop++;
      }
      return true;
    }

    bool Artifact::
    meetsRequirements (const Capabilities &caps,
		       const char *specName,
		       const OCPI::API::PValue * /*props*/,
		       const OCPI::API::PValue *selectCriteria,
		       const OCPI::API::Connection * /*conns*/,
		       const char *& /* artInst */,
		       int & score ) {
      if (m_os == caps.m_os && m_platform == caps.m_platform) {
	WorkerRange range = m_workers.equal_range(specName);

	for (WorkerIter wi = range.first; wi != range.second; wi++) {
	  //	  const Implementation &i = (*wi).second;
	  // FIXME: more complex comparison for FPGAs with connectivity
	  const char *model = ezxml_cattr(wi->second->m_worker, "model");
	  if (caps.m_model != model)
	    continue;
	  // Now we will test the selection criteria 
	  if ( selectCriteria ) {
	    const OCPI::API::PValue *p = selectCriteria;
	    bool eval = evaluateWorkerSuitability( p, score );
	    return eval;
	  }
	  else {
	    return true;
	  }
	  
	}
      }
      return false;
    }
    // Note this XML argument is from the system config file, not the
    // XML attached to this artifact file.
    // But in any case we process the attached metadata here, and not
    // in the constructor, for consistency with the Manager/Driver/Device model
    // Someday we might AUGMENT/OVERRIDE the attached metadata with the
    // system configuration metadata
    void Artifact::configure(ezxml_t /* x */) {
      // Retrieve attributes from artifact xml
      Attributes::parse(m_xml);
      // First insert workers
      for (ezxml_t w = ezxml_cchild(m_xml, "worker"); w; w = ezxml_next(w))
	m_nImplementations++;

      OU::Implementation *impl = m_implementations = new OU::Implementation[m_nImplementations];
      for (ezxml_t w = ezxml_cchild(m_xml, "worker"); w; w = ezxml_next(w), impl++) {
	const char *name = ezxml_cattr(w, "specName");
	if (!name)
	  name = ezxml_cattr(w, "name");
	const char *err = impl->parse(w);
	if (err)
	  throw OU::Error("Error processing implementation metadata: %s", err);
	bool instances = false;
	for (ezxml_t i = ezxml_cchild(m_xml, "instance"); i; i = ezxml_next(i)) {
	  if (!strcasecmp(name, ezxml_cattr(i, "worker"))) {
	    instances = true;
	    m_workers.insert(WorkerMapPair(name, new Implementation(impl, i)));
	  }
	}
	if (!instances)
	  m_workers.insert(WorkerMapPair(name, new Implementation(impl)));
      }
    }
    void parse3(char *s, std::string &s1, std::string &s2,
		std::string &s3) {
      char *temp = strdup(s);
      if ((s = strsep(&temp, "-"))) {
	s1 = s;
	if ((s = strsep(&temp, "-"))) {
	  s2 = s;
	  if ((s = strsep(&temp, "-")))
	    s3 = s;
	}
      }
      free(temp);
    }
    void Attributes::parse(ezxml_t x) {
      const char *cp;
      if ((cp = ezxml_cattr(x, "os"))) m_os = cp;
      if ((cp = ezxml_cattr(x, "osVersion"))) m_osVersion = cp;
      if ((cp = ezxml_cattr(x, "model"))) m_model = cp;
      if ((cp = ezxml_cattr(x, "platform"))) m_platform = cp;
      if ((cp = ezxml_cattr(x, "runtime"))) m_runtime = cp;
      if ((cp = ezxml_cattr(x, "runtimeVersion"))) m_runtimeVersion = cp;
      if ((cp = ezxml_cattr(x, "tool"))) m_tool = cp;
      if ((cp = ezxml_cattr(x, "toolVersion"))) m_toolVersion = cp;
      if ((cp = ezxml_cattr(x, "uuid"))) m_uuid = cp;
      validate();
    }
    void Attributes::parse(const char *pString) {
      std::string junk;
      char *p = strdup(pString), *temp = p, *val;
      
      if ((val = strsep(&temp, "="))) {
	parse3(val, m_os, m_osVersion, junk);
	if ((val = strsep(&temp, "="))) {
	  parse3(val, m_platform, junk, junk);
	  if ((val = strsep(&temp, "="))) {
	    parse3(val, m_tool, m_toolVersion, junk);
	    if ((val = strsep(&temp, "=")))
	      parse3(val, m_runtime, m_runtimeVersion, junk);
	  }
	}
      }
      free(p);
      validate();
    }
    void Attributes::validate() { }
  }
  namespace API {
    void LibraryManager::
    setPath(const char *path) {
      OCPI::Library::Manager::getSingleton().setPath(path);
    }
    std::string LibraryManager::
    getPath() {
      return OCPI::Library::Manager::getSingleton().getPath();
    }
    
  }
}
