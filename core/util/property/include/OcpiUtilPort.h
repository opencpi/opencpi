
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

#ifndef OCPI_UTIL_PORT_H
#define OCPI_UTIL_PORT_H

#include <string>
#include <cstdio>
#include "OcpiUtilProtocol.h"
#include "OcpiPValue.h"
#include "ezxml.h"

namespace OCPI {
  namespace Util {
    typedef uint32_t PortOrdinal; // this must be fixed size across achitectures
    const size_t DEFAULT_BUFFER_SIZE = 2*1024;
    const unsigned BUFFER_ALIGNMENT = 16;
    // FIXME:  use a pointer to a protocol, and share protocols in the artifact xml
    class Worker;
    class Port : public Protocol {
    public:
      struct Scaling {
	size_t
	  m_min,     // zero means no minimum
	  m_max,     // zero means no maximum.  1 essentially means not scalable
	  m_modulo,  // initialized to 1, can't be zero
	  m_default; // suggested value.  can't be zero
	Scaling();
	bool operator==(const Scaling &s) {
	  return m_min == s.m_min && m_max == s.m_max && m_modulo == s.m_modulo &&
	  m_default == s.m_default;
	}
	const char *check(size_t scale);
	const char *parse(ezxml_t x, Worker &w);
	void emit(std::string &out, const Scaling *def) const;
      };
#define SCALING_ATTRS "min", "max", "modulo", "default"
#define OCPI_PADDINGS	      \
      OCPI_PADDING(None)      \
      OCPI_PADDING(Replicate) \
      OCPI_PADDING(Zero)      \
      OCPI_PADDING(Wrap)

#define OCPI_PADDING(x) x,
      enum Padding { OCPI_PADDINGS };
#undef OCPI_PADDING

      struct Overlap {
	size_t m_left, m_right;
	Padding m_padding;
	static const char *s_oNames[];
	Overlap();
	const char *parse(ezxml_t x);
	void emit(std::string &out, const Overlap *def) const;
      };
#define OVERLAP_ATTRS "left", "right", "padding"
      // This structure is per-dimension
      struct Partitioning {
	Scaling  m_scaling;
	Overlap  m_overlap;
	size_t   m_sourceDimension;
	Partitioning();
        const char *parse(ezxml_t x, Worker &w);
	void emit(std::string &out, const Partitioning *def) const;
      };

#define PARTITION_ATTRS SCALING_ATTRS, OVERLAP_ATTRS, "source"
#define OCPI_DISTRIBUTIONS			\
        OCPI_DISTRIBUTION(All)			\
	OCPI_DISTRIBUTION(Cyclic)		\
	OCPI_DISTRIBUTION(First)		\
	OCPI_DISTRIBUTION(Balanced)		\
	OCPI_DISTRIBUTION(Directed)		\
	OCPI_DISTRIBUTION(Random)		\
	OCPI_DISTRIBUTION(Hashed)		\

#define OCPI_DISTRIBUTION(x) x,
      enum Distribution { OCPI_DISTRIBUTIONS DistributionLimit };
      static const char *s_dNames[];
#undef OCPI_DISTRIBUTION

#define DISTRIBUTION_ATTRS "distribution", "hashfield", "indistribution", "outdistribution"
      struct OpScaling {
	Distribution                m_distribution;
	Member                     *m_hashField;
	Partitioning                m_defaultPartitioning; // default for all args
	bool                        m_multiple;
	bool                        m_allSeeOne;
	bool                        m_allSeeEnd;
	std::vector<Partitioning *> m_partitioning; // tricky: these pointers are arrays for dims
	bool                        m_isPartitioned;
	OpScaling(size_t nArgs);
	const char *parse(Port &dp, const Operation &op, ezxml_t x);
	void emit(std::string &out, const Port &port, const Operation &op) const;
      };
    private:
      Worker     *m_worker;
    public:
      typedef uint32_t Mask;
      static const Mask c_maxPorts = sizeof(Mask)*8;
      std::string m_name;
      PortOrdinal m_ordinal;
      // These two values are redundant and opposite, but lots of code likes them that way.
      // In general runtime code uses "provider" and tool-time code uses "producer".
      bool        m_provider;
      bool        m_isProducer;
      bool        m_isOptional;
      bool        m_isBidirectional; // implementation-defined value
      bool        m_isInternal;
      size_t      m_minBufferCount;  // implementation-defined value
      size_t      m_defaultBufferCount; // specify default when none is specified.
      size_t      m_bufferSize;      // metadata protocol override, if non-zero
      ezxml_t     m_xml;
      ssize_t     m_bufferSizePort;  // The ordinal of port we copy our buffer size from or -1
      size_t      m_nOpcodes;
      bool        m_clone;
      bool        m_parsed;          // for assertions
      bool        m_seenSummary;     // ugly - for inheritors, but must be here for construction
      // Scalability
      bool                    m_isScalable;
      std::string             m_scaleExpr;
      bool                    m_isPartitioned;
      std::vector<OpScaling*> m_opScaling;
      Distribution m_defaultDistribution;
      Partitioning m_defaultPartitioning;
      std::string m_defaultHashField;

      Port(ezxml_t x = NULL);
      // constructor from tools, with new xml (to turn a spec port into an impl port)
      // If oldP != NULL, its cloning/taking.
      // nameOrdinal (when not == -1), and defaultName are used when xml has no name
      Port(Port *oldP, Worker &w, ezxml_t xml, size_t ordinal, int nameOrdinal,
	   const char *defaultName, const char *&err);
      Port(const Port &other, Worker &w, std::string &name, size_t ordinal,
	   const char *&err);
      virtual ~Port();
      void init();
      const char *preParse(Worker &w, ezxml_t x, size_t ordinal, int nameOrdinal = -1,
			   const char *defaultName = NULL);
      const char *parse();
      const char *postParse();
      virtual const char *parseProtocol();
      const char *parseDistribution(ezxml_t x, Distribution &d, std::string &hash);
      void emitDistribution(std::string &out, const Distribution &d) const;
      const char *parseOperations();
      const char *parseScaling();
      void emitXml(std::string &out) const;
      void emitScalingAttrs(std::string &out) const;
      void emitScaling(std::string &out) const;
      // Get the buffer size to use on this port given meta info and params and defaults
      size_t getBufferSize(const PValue *portParams, const PValue *connParams);
      Distribution getDistribution(unsigned op) const;
      // Determine the buffer size for a connection, where "in" or "out" could be NULL when
      // they are "external" and not specified by any port metadata.
      static size_t determineBufferSize(Port *in, const PValue *paramsIn,
					Port *out, const PValue *paramsOut,
					const PValue *connParams);
    };

  }
}
#endif
