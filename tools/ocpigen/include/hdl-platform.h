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

#ifndef HDL_PLATFORM_H
#define HDL_PLATFORM_H
#include <assert.h>
#include <string>
#include "hdl-device.h"
#include "hdl-slot.h"

#define HDL_PLATFORM_ATTRS "dummy", "control"
#define HDL_PLATFORM_ELEMS \
  "cpmaster", "nocmaster", "device", "metadata", "slot", "timebase"


class HdlConfig;
class HdlPlatform : public HdlDevice, public Board, public Device {
  friend class HdlConfig;
  Slots       m_slots;   // slots of the platform
  bool        m_control; // should the platform be used for control?
			 // FIXME should inherit device type and be a device
public:  
  static HdlPlatform *create(ezxml_t xml, const char *xfile, Worker *parent, const char *&err);
  HdlPlatform(ezxml_t xml, const char *xfile, Worker *parent, const char *&err);
  virtual ~HdlPlatform();
  const char *cname() const { return HdlDevice::m_name.c_str(); }
  const Slots &slots() const { return m_slots; }
  void setControl(bool c) { m_control = c; }
  Slot *findSlot(const char *name, const char *&err) const;
};

#endif
