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

#ifndef DATA_H
#define DATA_H

#include "OcpiUtilPort.h"
#include "OcpiUtilAssembly.h"
#include "ocp.h"

#define SPEC_DATA_PORT_ATTRS \
  "Name", "Producer", "Count", "Optional", "Protocol", "buffersize", \
    OCPI_PROTOCOL_SUMMARY_ATTRS, "numberofopcodes"

#define  AP(name,type,...) \
  do {\
    const char *_aperr = addProperty(#name,OA::OCPI_##type,__VA_ARGS__); if (_aperr) return _aperr; \
  } while(0)

// FIXME:  This class should have a base class for data that is not HDL
class DataPort : public OcpPort, public OCPI::Util::Port {
 protected:
  // This constructor is used when data port is inherited
  DataPort(Worker &w, ezxml_t x, DataPort *sp, int ordinal, WIPType type, const char *&err);
  DataPort(const DataPort &other, Worker &w , std::string &name, size_t count,
	   OCPI::Util::Assembly::Role *role, const char *&err);
 public:
  // Virtual constructor - every derived class must have one
  ::Port &clone(Worker &w, std::string &name, size_t count, OCPI::Util::Assembly::Role *role,
	      const char *&err) const;
  inline const char *typeName() const { return "WDI"; }
  inline const char *prefix() const { return "data"; }
  bool matchesDataProducer(bool isProducer) const { return m_isProducer == isProducer; }
  // This constructor is used when data port *is* the derived class (WDIPort)
  DataPort(Worker &w, ezxml_t x, int ordinal, const char *&err);
  bool isData() const { return true; }
  DataPort *dataPort() { return this; }
  bool isDataProducer() const { return m_isProducer; } // call isData first
  bool isDataOptional() const { return m_isOptional; } // call isData first
  bool isDataBidirectional() const { return m_isBidirectional; } // call isData first
  bool isOptional() const { return m_isOptional; }
  virtual const char *finalize();
  const char
    *parse(),
    *parseProtocolChild(ezxml_t op),
    *parseProtocol(),
    *addProperty(const char *name, OCPI::API::BaseType type, bool isDebug, bool isParameter, bool isInitial,
		 bool isVolatile, bool isImpl, bool isBuiltin = false, size_t value = 0, const char *enums = NULL),
    *addProperty(),
    *fixDataConnectionRole(OCPI::Util::Assembly::Role &role),
    *resolveExpressions(OCPI::Util::IdentResolver &ir);
  void initRole(OCPI::Util::Assembly::Role &role);
  void emitOpcodes(FILE *f, const char *pName, Language lang);
  void emitPortDescription(FILE *f, Language lang) const;
  void emitRecordDataTypes(FILE *f);
  void emitRecordInputs(FILE *f);
  void emitRecordOutputs(FILE *f);
  virtual void emitVHDLShellPortMap(FILE *f, std::string &last);
  virtual void emitImplSignals(FILE *f);
  void emitXML(std::string &out);
  const char *emitRccCppImpl(FILE *f);
  void emitRccCImpl(FILE *f);
  void emitRccCImpl1(FILE *f);
  void emitRccArgTypes(FILE *f, bool &first);
  void emitRecordInterface(FILE *f, const char *implName);
  void emitRecordInterfaceConstants(FILE *f);
  void emitVerilogPortParameters(FILE *f);
  void emitInterfaceConstants(FILE *f, Language lang);
  const char *adjustConnection(Connection &c, bool isProducer, OcpAdapt *myAdapt, bool &myHasExpr,
			       ::Port &otherPort, OcpAdapt *otherAdapt, bool &otherHasExpr,
			       Language lang, size_t &unused);
  virtual const char *adjustConnection(::Port &consumer, const char *masterName, Language lang,
				       OcpAdapt *prodAdapt, OcpAdapt *consAdapt, size_t &unused);
  virtual unsigned extraDataInfo() const;
  const char *finalizeHdlDataPort();
  const char *finalizeRccDataPort();
  const char *finalizeOclDataPort();
  const char *finalizeExternal(Worker &aw, Worker &iw, InstancePort &ip,
			       bool &cantDataResetWhileSuspended);
};

// Factory function template for data port types
template <typename ptype> DataPort *createDataPort(Worker &w, ezxml_t x, DataPort *sp,
						   int ordinal, const char *&err) {
  err = NULL;
  DataPort *p = new ptype(w, x, sp, ordinal, err);
  if (err) {
    delete p;
    return NULL;
  }
  return p;
}
class WsiPort : public DataPort {
  ~WsiPort();
  bool m_abortable;
  bool m_earlyRequest;
  bool m_regRequest; // request is registered
  bool m_insertEOM;
  WsiPort(const WsiPort &other, Worker &w , std::string &name, size_t count,
	  OCPI::Util::Assembly::Role *role, const char *&err);
 public:
  WsiPort(Worker &w, ezxml_t x, DataPort *sp, int ordinal, const char *&err);
  ::Port &clone(Worker &w, std::string &name, size_t count, OCPI::Util::Assembly::Role *role,
	      const char *&err) const;
  bool masterIn() const;
  inline const char *prefix() const { return "wsi"; }
  inline const char *typeName() const { return "WSI"; }
  bool haveWorkerOutputs() const { return true; }
  bool haveWorkerInputs() const { return true; }
  void emitPortDescription(FILE *f, Language lang) const;
  void emitRecordSignal(FILE *f, std::string &last, const char *prefix, bool inRecord,
			bool inPackage, bool inWorker, const char *defaultIn,
			const char *defaultOut);
  const char *deriveOCP();
  const char *finalize(); // virtual override
  void emitVhdlShell(FILE *f, ::Port *wci);
  const char *adjustConnection(::Port &consumer, const char *masterName, Language lang,
			       OcpAdapt *prodAdapt, OcpAdapt *consAdapt, size_t &unused);
  void emitImplAliases(FILE *f, unsigned n, Language lang);
  void emitImplSignals(FILE *f);
  void emitSkelSignals(FILE *f);
  void emitRecordInputs(FILE *f);
  void emitRecordOutputs(FILE *f);
  void emitVHDLShellPortMap(FILE *f, std::string &last);
  void emitVHDLShellPortClock(FILE *f, std::string &last);
};
class WmiPort : public DataPort {
  bool m_talkBack;
  size_t m_mflagWidth; // kludge for shep - FIXME
  WmiPort(const WmiPort &other, Worker &w , std::string &name, size_t count,
	  OCPI::Util::Assembly::Role *role, const char *&err);
 public:
  WmiPort(Worker &w, ezxml_t x, DataPort *sp, int ordinal, const char *&err);
  ::Port &clone(Worker &w, std::string &name, size_t count, OCPI::Util::Assembly::Role *role,
	      const char *&err) const;
  inline const char *prefix() const { return "mem"; }
  inline const char *typeName() const { return "WMI"; }
  const char *deriveOCP();
  void emitPortDescription(FILE *f, Language lang) const;
  const char *adjustConnection(::Port &consumer, const char *masterName, Language lang,
			       OcpAdapt *prodAdapt, OcpAdapt *consAdapt, size_t &unused);
  void emitImplAliases(FILE *f, unsigned n, Language lang);
  void emitRecordInputs(FILE *f);
  void emitRecordOutputs(FILE *f);
  void emitImplSignals(FILE *f);
};
#endif
