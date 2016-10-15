#include "OcpiUtilUUID.h"
#include "OcpiApplication.h"
#include "cdkutils.h"
#include "utils.h"

namespace OCPI {
  namespace API {
    namespace OU = OCPI::Util;
    namespace SCA = OCPI::SCA;
    static void
    emitProperty(ezxml_t root, unsigned level, const char *sname, const OU::Member &pr,
		 const char *mode, const char *value, bool config, bool allocation,
		 bool external, bool uuid) {
      ocpiDebug("emitting property %s", pr.name().c_str());
      const char *name = sname ? sname : pr.m_name.c_str();
      const char *element = pr.m_isSequence || pr.m_arrayRank ?
	(pr.m_baseType == OCPI_Struct ? "structsequence" : "simplesequence") :
	(pr.m_baseType == OCPI_Struct ? "struct" : "simple");
      std::string id;
      if (uuid) {
	OU::UUID::BinaryUUID uuid = OU::UUID::produceRandomUUID();
	id = "DCE:";
	id += OU::UUID::binaryToHex(uuid);
      } else
	id = name;
      std::string prf;
      ezxml_t px = SCA::addChild(root, element, level, NULL, "id", id.c_str());
      if (uuid)
	ezxml_set_attr_d(px, "name", name);
      if (mode)
	ezxml_set_attr(px, "mode", mode);
      const char *type = NULL;
      switch (pr.m_baseType) {
      case OCPI_Bool: type = "boolean"; break;
      case OCPI_Char: type = "char"; break;
      case OCPI_Double: type = "double"; break;
      case OCPI_Float: type = "float"; break;
      case OCPI_Short: type = "short"; break;
      case OCPI_Long: type = "long"; break;
      case OCPI_UChar: type = "octet"; break;
      case OCPI_ULong: type = "ulong"; break;
      case OCPI_UShort: type = "ushort"; break;
      case OCPI_LongLong:
	type = "longlong";
	//	ocpiBad("Warning: OpenCPI longlong property \"%s\" truncated to long for REDHAWK", name);
	break;
      case OCPI_ULongLong:
	type = "ulonglong";
	//	ocpiBad("Warning: OpenCPI ulonglong property \"%s\" truncated to ulong for REDHAWK", name);
	break;
      case OCPI_String: type = "string"; break;
	
      case OCPI_Enum:
	type = "ulong";  // SCA says Long, but RH can do ULong like ocpi
	{
	  ezxml_t esx = SCA::addChild(px, "enumerations", level + 1);
	  for (size_t n = 0; n < pr.m_nEnums; n++) {
	    ezxml_t ex = SCA::addChild(esx, "enumeration", level + 2);
	    ezxml_set_attr(ex, "label", pr.m_enums[n]);
	  }
	}
	break;
      case OCPI_Struct:
	for (unsigned n = 0; n < pr.m_nMembers; n++) {
	  OU::Member &m = pr.m_members[n];
	  if (m.m_baseType == OCPI_Struct || m.m_baseType == OCPI_Type || m.m_isSequence ||
	      m.m_arrayRank)
	    throw OU::Error("Unsupported struct member \"%s\" of property \"%s\" with data type: %s \"%s\"",
			    m.name().c_str(), pr.name().c_str(), 
			    m.m_isSequence ? "sequence of" : (m.m_arrayRank ? "array of" : ""), 
			    OU::baseTypeNames[m.m_baseType]);
	  emitProperty(px, level + 1, NULL, pr.m_members[n], mode, NULL, config, false, true,
		       false);
	}
	break;
      case OCPI_Type:
      default:
	throw OU::Error("Unsupported property data type \"%s\" (%u)",
			OU::baseTypeNames[pr.m_baseType], pr.m_baseType);
      }
      if (type)
	ezxml_set_attr(px, "type", type);
      if (allocation)
	SCA::addChild(px, "kind", level + 1, NULL, "kindtype", "allocation");
      if (config && allocation)
	SCA::addChild(px, "kind", level + 1, NULL, "kindtype", "configure");
      if (pr.m_description.length())
	SCA::addChild(px, "description", level + 1, pr.m_description.c_str());
      if (value)
	SCA::addChild(px, "value", level + 1, value);
      if (!external)
	SCA::addChild(px, "action", level + 1, NULL, "type", "eq");
    }


void ApplicationI::
genScaPrf(const char *outDir) {
  FILE *f;
  std::string path;
  const char *err = openOutput(m_assembly.name().c_str(), outDir, "", ".prf", ".xml", NULL, f,
			       &path);
  if (err)
    throw OU::Error("Error opening PRF output file: %s", err);
  const Property *p = m_properties;
  ezxml_t root = ezxml_new("properties");
  for (size_t n = m_nProperties; n; n--, p++) {
    const OU::Property &pr =
      m_instances[p->m_instance].m_impl->m_metadataImpl.property(p->m_property);
    const char *mode =
      pr.m_isInitial || pr.m_isWritable ?
      (pr.m_isReadable ? NULL : "writeonly") : "readonly";
    emitProperty(root, 1, p->m_name.c_str(), pr, mode, NULL, true, false, true, false);
  }
  const char *xml = ezxml_toxml(root);
  if (fputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	    "<!DOCTYPE properties PUBLIC \"-//JTRS//DTD SCA V2.2.2 PRF//EN\" \"properties.dtd\">\n",
	    f) == EOF ||
      fputs(xml, f) == EOF || fclose(f))
    throw OU::Error("Error closing PRF output file: \"%s\"  No space?", path.c_str());
}
  }
}
