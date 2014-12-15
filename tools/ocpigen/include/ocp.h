#ifndef OCP_H
#define OCP_H
#include <string>
#include "port.h"
#define OCP_SIGNALS \
  OCP_SIGNAL_MS(Clk) \
  OCP_SIGNAL_MVR(MAddr, 0) \
  OCP_SIGNAL_MVR(MAddrSpace, 1) \
  OCP_SIGNAL_MVR(MBurstLength, 0) \
  OCP_SIGNAL_MSR(MBurstSingleReq) \
  OCP_SIGNAL_MVR(MByteEn, 0) \
  OCP_SIGNAL_MTR(MCmd, 3) \
  OCP_SIGNAL_MV(MData, 0) \
  OCP_SIGNAL_MV(MDataByteEn, 0) \
  OCP_SIGNAL_MV(MDataInfo, 0) \
  OCP_SIGNAL_MS(MDataLast) \
  OCP_SIGNAL_MS(MDataValid) \
  OCP_SIGNAL_MV(MFlag, 0) \
  OCP_SIGNAL_MSR(MBurstPrecise) \
  OCP_SIGNAL_MVR(MReqInfo, 0)		\
  OCP_SIGNAL_MSR(MReqLast) \
  OCP_SIGNAL_MS(MReset_n) \
  OCP_SIGNAL_MS(MRespAccept) \
  OCP_SIGNAL_SS(SCmdAccept) \
  OCP_SIGNAL_SV(SData, 0) \
  OCP_SIGNAL_SS(SDataAccept) \
  OCP_SIGNAL_SV(SDataInfo, 0) \
  OCP_SIGNAL_SV(SDataThreadBusy, 1) \
  OCP_SIGNAL_SV(SFlag, 0) \
  OCP_SIGNAL_ST(SResp, 2) \
  OCP_SIGNAL_SS(SRespLast) \
  OCP_SIGNAL_SS(SReset_n) \
  OCP_SIGNAL_SV(SThreadBusy, 1) \
  /**/
#define OCP_SIGNAL_MT(n,w) OCP_SIGNAL_MV(n,w)
#define OCP_SIGNAL_ST(n,w) OCP_SIGNAL_SV(n,w)
#define OCP_SIGNAL_MS(n) OCP_SIGNAL(n)
#define OCP_SIGNAL_MV(n, w) OCP_SIGNAL(n)
#define OCP_SIGNAL_MSR(n) OCP_SIGNAL(n)
#define OCP_SIGNAL_MVR(n, w) OCP_SIGNAL(n)
#define OCP_SIGNAL_MTR(n, w) OCP_SIGNAL(n)
#define OCP_SIGNAL_SS(n) OCP_SIGNAL(n)
#define OCP_SIGNAL_SV(n, w) OCP_SIGNAL(n)
#define OCP_SIGNAL(n) OCP_##n,
enum OcpSignalEnum {
OCP_SIGNALS
N_OCP_SIGNALS
};
#undef OCP_SIGNAL

struct OcpSignalDesc {
  const char *name;
  bool vector;
  bool master;
  size_t width;
  bool type;
  bool request;
  OcpSignalEnum number;
};

extern OcpSignalDesc ocpSignals[N_OCP_SIGNALS+1];
struct OcpSignal {
  uint8_t *value;
  size_t width;
  char *signal;
  bool master;
};

union OcpSignals {
  OcpSignal signals[N_OCP_SIGNALS];
  struct {
#define OCP_SIGNAL(n) OcpSignal n;
OCP_SIGNALS
#undef OCP_SIGNAL_MS
#undef OCP_SIGNAL_MV
#undef OCP_SIGNAL_SS
#undef OCP_SIGNAL_SV
#undef OCP_SIGNAL_MSR
#undef OCP_SIGNAL_MVR
#undef OCP_SIGNAL_MTR
#undef OCP_SIGNAL
  };
};

class OcpPort : public Port {
  friend class Worker;
  friend class Assembly;
 protected:
  uint8_t *m_values;
  size_t m_nAlloc;
  bool m_impreciseBurst;
  bool m_preciseBurst;
  size_t m_dataWidth;
  size_t m_byteWidth;
  bool m_continuous;
  OcpSignals ocp;
  OcpPort(Worker &w, ezxml_t x, Port *sp, int ordinal, WIPType type, const char *defaultName,
	  const char *&err);
  OcpPort(const OcpPort &other, Worker &w , std::string &name, size_t count, const char *&err);
  void fixOCP();
 public:
  bool isOCP() const { return true; }
  bool needsControlClock() const;
  void emitPortDescription(FILE *f, Language lang) const;
  void emitRecordSignal(FILE *f, std::string &last, const char *prefix, bool inWorker,
			const char *defaultIn, const char *defaultOut);
  void emitSignals(FILE *f, Language lang, std::string &last, bool inPackage, bool inWorker);
  void emitRecordInterface(FILE *f, const char *implName);
  void emitVerilogSignals(FILE *f);
  void emitVHDLSignalWrapperPortMap(FILE *f, std::string &last);
  void emitVHDLRecordWrapperSignals(FILE *f);
  void emitVHDLRecordWrapperAssignments(FILE *f);
  void emitVHDLRecordWrapperPortMap(FILE *f, std::string &last);
  void emitConnectionSignal(FILE *f, bool output, Language lang, std::string &signal);
  void emitPortSignalsDir(FILE *f, bool output, Language lang, const char *indent,
			  bool &any, std::string &comment, std::string &last,
			  OcpAdapt *adapt, Attachments &atts);
  void emitPortSignals(FILE *f, Attachments &atts, Language lang,
		       const char *indent, bool &any, std::string &comment,
		       std::string &last, const char *myComment, OcpAdapt *adapt);
  void connectOcpSignal(OcpSignalDesc &osd, OcpSignal &os, OcpAdapt &oa,
			std::string &signal, std::string &thisComment, Language lang,
			Attachments &atts);
  const char *doPatterns(unsigned nWip, size_t &maxPortTypeName);
  const char *deriveOCP();
};

struct OcpAdapt {
  const char *expr;
  const char *comment;
  const char *signal;
  OcpSignalEnum other;
  OcpAdapt() : expr(NULL), comment(NULL), signal(NULL), other(N_OCP_SIGNALS) {}
};
#endif
