// This file contains basic definitions used all over ocpigen
#ifndef OCPIGEN_H
#define OCPIGEN_H

#include <cstdio>
#include <list>
#include <string>
#include <vector>
#include <ezxml.h>

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
typedef Signals::const_iterator SignalsIter;
struct Signal {
  std::string m_name;
  // The NONE is used in contexts where you are saying do not deal with direction
  enum Direction { IN, OUT, INOUT, BIDIRECTIONAL, NONE } m_direction;
  size_t m_width;
  bool m_differential;
  std::string m_pos; // pattern for positive if not %sp
  std::string m_neg; // pattern for negative if not %sn
  const char *m_type;
  Signal();
  const char * parse(ezxml_t);
  const char *name() const { return m_name.c_str(); }
  static const char *parseSignals(ezxml_t x, const std::string &parent, Signals &signals);
  static void deleteSignals(Signals &signals);
  static Signal *find(const Signals &signals, const char *name); // poor man's map
};

extern void
emitSignal(const char *signal, FILE *f, Language lang, Signal::Direction dir,
	   std::string &last, int width, unsigned n, const char *pref = "",
	   const char *type = "std_logic", const char *value = NULL);

#endif
