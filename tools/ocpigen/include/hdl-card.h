#ifndef HDL_CARD_H
#define HDL_CARD_H
#include <string>
#include <map>
#include "hdl-slot.h"

// A card is really a "type of card".  There is no object that represents
// an instance of a type of card.  When a card is plugged into a slot it is
// essentially "the card of this type plugged into a particular slot".
// When we need some configuration information PER CARD we might introduce
// card types vs. cards.
// The "Cards" type is just for interning cards, not for collecting them.
struct Card;
typedef std::map<std::string, Card *> Cards;
typedef Cards::iterator CardsIter;
struct Card : public Board {
  std::string                      m_name;
  SlotType                        &m_type;
  std::map<Signal *, std::string>  m_signals;

  static Cards                     s_cards;        // registry of card types

  static Card *get(const char *file, const char *parent, const char *&err);
  Card(ezxml_t xml, const char *name, SlotType &type, const char *&err);
  virtual ~Card();

  const char *name() const { return m_name.c_str(); }
  const SlotType *type() const { return &m_type; }
  static Card *find(const char *name);
};


#endif
