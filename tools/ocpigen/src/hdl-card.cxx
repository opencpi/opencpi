#include "cdkutils.h"
#include "hdl-device.h"
#include "hdl-card.h"

namespace OE=OCPI::Util::EzXml;
namespace OU=OCPI::Util;

Cards Card::s_cards;        // registry of card types

// A slot may have a default mapping to the external platform's signals,
// ie. <slot-name>_signal.
Card::
Card(ezxml_t xml, const char *name, SlotType &type, const char *&err)
  : m_name(name), m_type(type)
{
  // process non-default signals: slot=pfsig, platform=dddd
  for (ezxml_t xs = ezxml_cchild(xml, "Signal"); xs; xs = ezxml_next(xs)) {
    std::string slot, card;
    if ((err = OE::getRequiredString(xs, slot, "slot")) ||
	(err = OE::getRequiredString(xs, card, "card")))
      break;
    Signal *s = Signal::find(m_type.m_signals, slot.c_str());
    if (!s) {
      err = OU::esprintf("Slot signal '%s' does not exist for slot type '%s'",
			 slot.c_str(), m_type.m_name.c_str());
      break;
    } else if (m_signals.find(s) != m_signals.end()) {
      err = OU::esprintf("Duplicate slot signal: %s", slot.c_str());
      break;
    } else
      m_signals[s] = card;
  }
  if (!err)
    err = Device::addDevices(*this, xml, m_devices);
  if (err)
    err = OU::esprintf("Error for slot '%s': %s", m_name.c_str(), err);
}

Card::
~Card() {
}

// Cards are interned, and we want the type to be a reference.
// Hence we check the type first.
Card *Card::
get(const char *file, const char *parent, const char *&err) {
  ezxml_t xml;
  std::string xfile;
  if ((err = parseFile(file, parent, NULL, &xml, xfile)))
    return NULL;
  std::string name;
  OE::getOptionalString(xml, name, "name");
  if (name.empty())
    name = xfile;
  else if (name != xfile)
    err = OU::esprintf("File name (%s) does not match name attribute in XML (%s)",
		       xfile.c_str(), name.c_str());
  Card *c = Card::find(name.c_str());
  if (c)
    return c;
  std::string type;
  if ((err = OE::getRequiredString(xml, type, "type")))
    return NULL;
  SlotType *st = SlotType::get(type.c_str(), parent, err);
  if (!st)
    return NULL;
  c = new Card(xml, name.c_str(), *st, err);
  if (err) {
    delete c;
    return NULL;
  }
  return s_cards[c->m_name] = c;
}

Card *Card::
find(const char *name) {
  CardsIter si = s_cards.find(name);
  return si == s_cards.end() ? NULL : si->second;
}


