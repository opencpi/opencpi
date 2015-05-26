// This file contains basic definitions used all over ocpigen
#ifndef OCPIGEN_H
#define OCPIGEN_H

#include <cstdio>
#include <list>
#include <string>
#include <vector>
#include <map>
#include <ezxml.h>
#include <OcpiUtilMisc.h>

enum Language {
  NoLanguage,
  Verilog,
  VHDL,
  C,
  CC
};

class Port;
typedef std::vector<Port*> Ports;
typedef Ports::const_iterator PortsIter;
struct Clock {
  std::string m_name, m_signal, m_reset;
  Port *port;
  bool assembly; // This clock is at the assembly level
  size_t ordinal; // within the worker
  Clock();
  const char *name() const { return m_name.c_str(); }
  const char *signal() const { return m_signal.c_str(); }
  const char *reset() const { return m_reset.c_str(); }
};

struct Signal;
// This container provides lookup by name
typedef std::map<const char *, Signal *, OCPI::Util::ConstCharCaseComp> SigMap_;
class SigMap : public SigMap_ {
 public:
  Signal *findSignal(const std::string &name) const { return findSignal(name.c_str()); }
  Signal *findSignal(const char *name) const {
    SigMap_::const_iterator i = find(name);
    return i == end() ? NULL : i->second;
  }
  bool findSignal(const char *name, Signal *&sig) const {
    SigMap_::const_iterator i = find(name);
    return i == end() ? false : (sig = i->second, true);
  }
  const char*findSignal(Signal *);
};

// This container provides processing in the original order (user friendly)
typedef std::list<Signal *> Signals;
typedef Signals::const_iterator SignalsIter;
struct Signal {
  std::string m_name;
  // The NONE is used in contexts where you are saying do not deal with direction
  enum Direction { IN, OUT, INOUT, BIDIRECTIONAL, NONE } m_direction;
  size_t m_width;
  bool m_differential;
  std::string m_pos; // pattern for positive if not %sp
  std::string m_neg; // pattern for negative if not %sn
  std::string m_in;  // pattern for in of tristate if not %s_i
  std::string m_out; // pattern for out of tristate if not %s_o
  std::string m_oe;  // pattern for output enable of tristate if not %s_oe
  const char *m_type;
  Signal();
  const char * parse(ezxml_t);
  const char *name() const { return m_name.c_str(); }
  Signal *reverse();
  void emitConnectionSignal(FILE *f, const char *iname, const char *pattern, bool single);
  static void emitConnectionSignals(FILE *f, const char *iname, Signals &signals);
  static const char *parseSignals(ezxml_t x, const std::string &parent, Signals &signals,
				  SigMap &sigmap);
  static void deleteSignals(Signals &signals);
  static const Signal *find(const SigMap &signals, const char *name);
};
// This container provides lookup by name
typedef std::map<const char *,
		 std::pair<Signal *,size_t>,
		 OCPI::Util::ConstCharCaseComp> SigMapIdx_;
typedef SigMapIdx_::const_iterator SigMapIdxIter;
class SigMapIdx : public SigMapIdx_ {
 public:
  Signal *findSignal(const std::string &name) const { return findSignal(name.c_str()); }
  Signal *findSignal(const char *name) const {
    SigMapIdx_::const_iterator i = find(name);
    return i == end() ? NULL : i->second.first;
  }
  Signal *findSignal(const char *name, size_t &idx) const {
    SigMapIdx_::const_iterator i = find(name);
    if (i == end())
      return NULL;
    idx = i->second.second;
    return i->second.first;
  }
  const char*findSignal(Signal *sig, size_t idx) const;
  void insert(const char *name, Signal *s, size_t idx) {
    (*this)[name] = std::pair<Signal *,size_t>(s, idx);
  }
};

extern void
emitSignal(const char *signal, FILE *f, Language lang, Signal::Direction dir,
	   std::string &last, int width, unsigned n, const char *pref = "",
	   const char *type = "std_logic", const char *value = NULL);

#endif
