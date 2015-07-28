#include "hdl.h"
#include "assembly.h"

SdpPort::
SdpPort(Worker &w, ezxml_t x, Port *sp, int ordinal, const char *&err)
  : Port(w, x, sp, ordinal, SDPPort, "sdp", err) {
}

// Our special copy constructor
SdpPort::
SdpPort(const SdpPort &other, Worker &w , std::string &name, size_t count,
		const char *&err)
  : Port(other, w, name, count, err) {
  if (err)
    return;
}

// Virtual constructor: the concrete instantiated classes must have a clone method,
// which calls the corresponding specialized copy constructor
Port &SdpPort::
clone(Worker &w, std::string &name, size_t count, OCPI::Util::Assembly::Role */*role*/,
      const char *&err) const {
  return *new SdpPort(*this, w, name, count, err);
}

void SdpPort::
emitRecordInterface(FILE *f, const char *implName) {
  std::string in, out;
  OU::format(in, typeNameIn.c_str(), "");
  OU::format(out, typeNameOut.c_str(), "");
  fprintf(f,
	  "\n"
	  "  -- Record for the %s input signals for port \"%s\" of worker \"%s\"\n"
	  "  alias %s_t is sdp.sdp.%s_t;\n"
	  "  -- Record for the %s output signals for port \"%s\" of worker \"%s\"\n"
	  "  alias %s_t is sdp.sdp.%s_t;\n",
	  typeName(), name(), implName, in.c_str(), master ? "s2m" : "m2s",
	  typeName(), name(), implName, out.c_str(), master ? "m2s" : "s2m");
}

void SdpPort::
emitRecordTypes(FILE */*f*/) {
}

void SdpPort::
emitConnectionSignal(FILE *f, bool output, Language /*lang*/, std::string &signal) {
  fprintf(f,
	  "  signal %s : sdp.sdp.%s_t;\n"
	  "  signal %s_data : dword_array_t(0 to to_integer(sdp_width)-1);\n",
	  signal.c_str(), master == output ? "m2s" : "s2m", signal.c_str());
}

void SdpPort::
emitRecordSignal(FILE *f, std::string &last, const char *prefix, bool inRecord, bool inPackage,
		 bool inWorker, const char *defaultIn, const char *defaultOut) {
  Port::emitRecordSignal(f, last, prefix, inRecord, inPackage, inWorker, defaultIn, defaultOut);
  fprintf(f, last.c_str(), ";\n");
  std::string in, out;
  OU::format(in, "%s_in_data", name());
  OU::format(out, "%s_out_data", name());
  OU::format(last,
	     "  %-*s : in  dword_array_t(0 to to_integer(%s)-1);\n"
	     "  %-*s : out dword_array_t(0 to to_integer(%s)-1)%%s",
	     (int)m_worker->m_maxPortTypeName, in.c_str(),
	     inRecord ? "sdp_width" : "unsigned(sdp_width)",
	     (int)m_worker->m_maxPortTypeName, out.c_str(),
	     inRecord ? "sdp_width" : "unsigned(sdp_width)");
}

void SdpPort::
emitVHDLShellPortMap(FILE *f, std::string &last) {
  Port::emitVHDLShellPortMap(f, last);
  std::string in;
  OU::format(in, typeNameIn.c_str(), "");
  fprintf(f,
	  "%s"
	  "    %s_in_data => %s_in_data,\n"
	  "    %s_out_data => %s_out_data\n",
	  last.c_str(), name(), name(), name(), name());
}

void SdpPort::
emitPortSignal(FILE *f, bool any, const char *indent, std::string &sName,
	       const char *name, std::string &index) {
  Port::emitPortSignal(f, any, indent, sName, name, index);
  any = true;
  std::string dsName = sName, dName = name;
  dsName += "_data";
  dName += "_data";
  fprintf(f, ",\n");
  Port::emitPortSignal(f, any, indent, dsName, dName.c_str(), index);
}
