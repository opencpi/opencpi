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
class Worker;
typedef std::vector<Port*> Ports;
typedef Ports::const_iterator PortsIter;
typedef std::list<Worker *> Workers;
typedef Workers::iterator WorkersIter;

struct Signal;
// This container provides lookup by name
typedef std::map<const char *, Signal *, OCPI::Util::ConstCharCaseComp> SigMap_;
class SigMap : public SigMap_ {
 public:
  Signal *findSignal(const std::string &name, std::string *suffixed = NULL) const {
    return findSignal(name.c_str(), suffixed);
  }
  Signal *findSignal(const char *name, std::string *suffixed = NULL) const;
  bool findSignal(const char *name, Signal *&sig) const {
    SigMap_::const_iterator i = find(name);
    return i == end() ? false : (sig = i->second, true);
  }
  const char *findSignal(Signal *);
};

// This container provides processing in the original order (user friendly)
typedef std::list<Signal *> Signals;
typedef Signals::const_iterator SignalsIter;
namespace OCPI { namespace Util { struct IdentResolver; }}
struct Signal {
  std::string m_name;
  // The NONE is used in contexts where you are saying do not deal with direction
  // The OUTIN direction is for emulators on the opposite side of an INOUT,
  // which, inside the FPGA is a triple of in/out/oe
  enum Direction { IN, OUT, INOUT, BIDIRECTIONAL, OUTIN, UNUSED, NONE } m_direction;
#define DIRECTIONS "in", "out", "inout", "bidirectional", "outin", "unused"
  std::string m_directionExpr;
  size_t m_width;
  std::string m_widthExpr;
  bool m_differential;
  bool m_pin;        // this signal is at a pin, outside of any IO block/pad.
  std::string m_pos; // pattern for positive if not %sp
  std::string m_neg; // pattern for negative if not %sn
  std::string m_in;  // pattern for in of tristate if not %s_i
  std::string m_out; // pattern for out of tristate if not %s_o
  std::string m_oe;  // pattern for output enable of tristate if not %s_oe
  const char *m_type;
  Signal();
  const char *parseDirection(const char *direction, std::string *expr,
			     OCPI::Util::IdentResolver &ir);
  const char *parseWidth(const char *width, std::string *expr,
   			     OCPI::Util::IdentResolver &ir);
  const char *parse(ezxml_t, Worker *w);
  const char *cname() const { return m_name.c_str(); }
  Signal *reverse();
  void emitConnectionSignal(FILE *f, const char *iname, const char *pattern, bool single,
			    Language lang);
  //  static void emitConnectionSignals(FILE *f, const char *iname, Signals &signals);
  static const char *parseSignals(ezxml_t x, const std::string &parent, Signals &signals,
				  SigMap &sigmap, Worker *w);
  static void deleteSignals(Signals &signals);
  static const Signal *find(const SigMap &signals, const char *name);
  static const char *directions[];
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
	   const char *type = "std_logic", const char *value = NULL,
	   const char *widthExpr = NULL);

#endif
