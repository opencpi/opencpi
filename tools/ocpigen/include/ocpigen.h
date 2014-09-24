// This file contains basic definitions used all over ocpigen
#ifndef OCPIGEN_H
#define OCPIGEN_H

#include <cstdio>
#include <list>

enum Language {
  NoLanguage,
  Verilog,
  VHDL,
  C,
  CC
};

static inline const char *hdlComment(Language lang) { return lang == VHDL ? "--" : "//"; }

class Port;
struct Clock {
  const char *name;
  const char *signal;
  std::string reset;
  Port *port;
  bool assembly; // This clock is at the assembly level
  size_t ordinal; // within the worker
  Clock();
};

struct Signal;
typedef std::list<Signal *> Signals;
typedef Signals::iterator SignalsIter;
struct Signal {
  std::string m_name;
  enum Direction { IN, OUT, INOUT, BIDIRECTIONAL } m_direction;
  size_t m_width;
  bool m_differential;
  std::string m_pos; // pattern for positive if not %sp
  std::string m_neg; // pattern for negative if not %sn
  const char *m_type;
  Signal();
  const char * parse(ezxml_t);
  static const char *parseSignals(ezxml_t x, Signals &signals);
  static void deleteSignals(Signals &signals);
  static Signal *find(Signals &signals, const char *name); // poor man's map
};

extern void
emitSignal(const char *signal, FILE *f, Language lang, Signal::Direction dir,
	   std::string &last, int width, unsigned n, const char *pref = "",
	   const char *type = "std_logic", const char *value = NULL);

#endif
