
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

#include "OcpiUtilAutoMutex.h"
#include "OcpiContainerApplication.h"
#include "OcpiContainerErrorCodes.h"
#include "OcpiWorker.h"
#include "OcpiContainerPort.h"
#include "OcpiContainerInterface.h"
#include "OcpiContainerMisc.h"
#include "OcpiContainerArtifact.h"

namespace OA = OCPI::API;
namespace OP = OCPI::Util::Prop;
namespace OU = OCPI::Util;
namespace OM = OCPI::Metadata;
namespace OCPI {
  namespace Container {
    Controllable::Controllable()
      : m_state(OM::Worker::EXISTS), m_controlMask(0) {
    }
    void Controllable::setControlOperations(const char *ops) {
      if (ops) {
#define CONTROL_OP(x, c, t, s1, s2, s3) \
	if (strstr(ops, #x))				\
	  m_controlMask |= 1 << OM::Worker::Op##c;
	OCPI_CONTROL_OPS
#undef CONTROL_OP
      }
    }

    // Due to class hierarchy issues..
    Worker::Worker(Artifact *art, ezxml_t impl, ezxml_t inst, const OA::PValue *) 
      : OM::Worker::Worker(impl),
	m_artifact(art), m_xml(impl), m_instXml(inst), m_workerMutex(true)
    {
      setControlOperations(ezxml_cattr(impl, "controlOperations"));
      if (impl)
	m_implTag = ezxml_cattr(impl, "name");
      if (inst)
	m_instTag = ezxml_cattr(inst, "name");
    }

    Worker::~Worker()
    {
      if (m_artifact)
	m_artifact->removeWorker(*this);
    }

    OA::Port &Worker::
    getPort(const char *name, const OA::PValue *props ) {
      Port *p = findPort(name);
      if (p)
        return *p;
      OM::Port *metaPort = findMetaPort(name);
      if (!metaPort)
        throw ApiError("no port found with name \"", name, "\"", NULL);
      return createPort(*metaPort, props);
    }
    void Worker::setupProperty(const char *name, OA::Property &apiProp) {
      Metadata::Property &prop = findProperty(name);
      apiProp.m_info = prop;
      apiProp.m_type = prop.members->type;
      prepareProperty(prop, apiProp);
    }
    void Worker::setProperty(const char *name, const char *value) {
      OA::Property prop(*this, name);
      OP::Scalar::Value v; // FIXME storage when not scalar
      if (prop.m_info.m_isStruct)
	throw ApiError("No support yet for setting struct properties", NULL);
      OP::ValueType &vt = prop.m_type;
      const char *err = v.parse(vt, value);
      if (err)
        throw ApiError("Error parsing property value:\"", value, "\"", NULL);
      switch (vt.scalar) {
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)		 \
	case OA::OCPI_##pretty:					         \
	  if (v.m_vector)						 \
	    prop.set##pretty##SequenceValue((const run*)(v.m_p##pretty), \
					    v.m_length);		 \
	  else								 \
	    prop.set##pretty##Value(v.m_##pretty);			 \
          break;
      OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
      case OA::OCPI_none: case OA::OCPI_scalar_type_limit:;
      }
    }
    bool Worker::getProperty(unsigned ordinal, std::string &name, std::string &value) {
      unsigned nProps;
      OM::Property *props = getProperties(nProps);
      if (ordinal >= nProps)
	return false;
      OM::Property &p = props[ordinal];
      if (p.m_isStruct || p.m_isStructSequence)
	throw OU::Error("Struct properties are unsupported");
      OP::Member &m = p.members[0];
      OA::Value v(m.type);
      OA::Property a(*this, p.m_name); // FIXME clumsy because get methods take API props
      switch (m.type.scalar) {
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)                         \
      case OA::OCPI_##pretty:		                                               \
	if (v.m_vector)							               \
	  v.m_length = get##pretty##SequenceProperty(a, v.m_p##pretty, m.type.length); \
	else								               \
	  v.m_##pretty = get##pretty##Property(a);                                     \
	break;
      OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE_S OCPI_DATA_TYPE
      case OA::OCPI_String:
	if (v.m_vector)
	  v.m_length = getStringSequenceProperty(a, v.m_pString, m.type.length,
						 v.m_stringSpace,
						 (m.type.stringLength + 1) * m.type.length);
	else
	  getStringProperty(a, v.m_String, m.type.stringLength + 1);
	break;
      case OA::OCPI_none: case OA::OCPI_scalar_type_limit:;
      }
      v.unparse(value);
      name = p.m_name;
      return true;
    }
#if 0
    void Worker::setProperty(OA::Property &prop, const OA::PValue &val) {
      switch (prop.m_type.scalar) {
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)		\
	case OP::Scalar::OCPI_##pretty:					\
	    prop.set##pretty##Value(val.v##pretty);			\
            break;
      OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
      default:
	ocpiAssert(!"unknown data type");
      }
    }
#endif
    // batch setting with lots of error checking - all or nothing
    void Worker::setProperties(const OA::PValue *props) {
      if (props)
	for (const OA::PValue *p = props; p->name; p++) {
	  OA::Property prop(*this, p->name); // exception goes out
	  prop.checkTypeAlways(p->type, 1, true);
	  switch (prop.m_type.scalar) {
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)		\
	    case OP::Scalar::OCPI_##pretty:				\
	      prop.set##pretty##Value(p->v##pretty);			\
	      break;
            OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
            default:
	      ocpiAssert(!"unknown data type");
	  }
	}
    }
    // batch setting with lots of error checking - all or nothing
    void Worker::setProperties(const char *props[][2]) {
      for (const char *(*p)[2] = props; (*p)[0]; p++)
	setProperty((*p)[0], (*p)[1]);
    }
    // Common top level implementation for control operations
    // Note that the m_controlMask does not apply at this level
    // since the container might want to know anyway, even if the
    // worker doesn't have an implementation
#define CONTROL_OP(x, c, t, s1, s2, s3)	\
    void Worker::x() { controlOp(Op##c); }
    OCPI_CONTROL_OPS
#undef CONTROL_OP

    struct ControlTransition {
      OM::Worker::ControlState valid[3];
      OM::Worker::ControlState next;
    } controlTransitions[] = {
#define CONTROL_OP(x, c, t, s1, s2, s3)  \
      {{OM::Worker::s1, OM::Worker::s2, OM::Worker::s3}, OM::Worker::t},
	OCPI_CONTROL_OPS
#undef CONTROL_OP
    };
    void Worker::controlOp(OM::Worker::ControlOperation op) {
      OU::AutoMutex guard (m_workerMutex, true);
      OM::Worker::ControlState cs = getControlState();
      ControlTransition ct = controlTransitions[op];
      if (cs == ct.valid[0] ||
	  (ct.valid[1] != OM::Worker::NONE && cs == ct.valid[1]) ||
	  (ct.valid[2] != OM::Worker::NONE && cs == ct.valid[2])) {
        controlOperation(op);
	if (ct.next != OM::Worker::NONE)
	  setControlState(ct.next);
      } else
	throw OU::EmbeddedException(cs == OM::Worker::UNUSABLE ?
				    OU::WORKER_UNUSABLE :
				    OU::INVALID_CONTROL_SEQUENCE,
				    "Illegal control state for operation",
				    OU::ApplicationRecoverable);
      Application &a = application();
      Container &c = a.container();
      c.start();
    }

    //      application().container().start(); 
  }
  namespace API {
    Worker::~Worker(){}
  }
}
