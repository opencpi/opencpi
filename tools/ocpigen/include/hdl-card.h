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
  static Cards                     s_cards;        // registry of card types
  static Card *get(const char *file, const char *parentFile, Worker *parent, const char *&err);
  Card(ezxml_t xml, const char *name, SlotType &type, const char *parentFile, Worker *parent,
       const char *&err);
  virtual ~Card();

  const char *cname() const { return m_name.c_str(); }
  const SlotType *type() const { return &m_type; }
  static Card *find(const char *name);
};


#endif
