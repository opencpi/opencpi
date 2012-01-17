/****************************************************************************

Copyright 2006, Virginia Polytechnic Institute and State University

This file is part of the OSSIE Core Framework.

OSSIE Core Framework is free software; you can redistribute it and/or modify
it under the terms of the Lesser GNU General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

OSSIE Core Framework is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
Lesser GNU General Public License for more details.

You should have received a copy of the Lesser GNU General Public License
along with OSSIE Core Framework; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

****************************************************************************/

#include <iostream>

#include <ossie/ossieSupport.h>

#include "standardinterfaces-metadata/RadioMetaData_impl.h"
#include "ossie/debug.h"

namespace standardInterfacesMD
{

void InitializeMetaData( MetaData &m )
{
// initialize meta data
    m.modulation_scheme.scheme = standardInterfacesMD::ModulationScheme::UNKNOWN;
    m.modulation_scheme.M = 0;
    m.packet_id = 0;
    m.sampling_frequency = 0;
    m.carrier_frequency = 0;
    m.signal_bandwidth = 0;
    m.signal_strength = 0;
    m.eom = false;
    m.src_id = 0;
    m.dst_id = 0;
    m.app_id = 0;
    m.port_id = 0;
    m.packet_type = 0;
}

void PrintMetaData( MetaData &m )
{
    switch (m.modulation_scheme.scheme) {
    case standardInterfacesMD::ModulationScheme::UNKNOWN:
        std::cout << "  Modulation scheme: UNKNOWN" << std::endl;
        break;
    case standardInterfacesMD::ModulationScheme::PSK:
        std::cout << "  Modulation scheme: PSK" << std::endl;
        break;
    case standardInterfacesMD::ModulationScheme::DPSK:
        std::cout << "  Modulation scheme: Differential PSK" << std::endl;
        break;
    case standardInterfacesMD::ModulationScheme::FSK:
        std::cout << "  Modulation scheme: FSK" << std::endl;
        break;
    case standardInterfacesMD::ModulationScheme::QAM:
        std::cout << "  Modulation scheme: QAM" << std::endl;
        break;
    case standardInterfacesMD::ModulationScheme::PAM:
        std::cout << "  Modulation scheme: PAM" << std::endl;
        break;
    }

    std::cout << "  Modulation depth: " << m.modulation_scheme.M << std::endl;
    std::cout << "  packet id: " << m.packet_id << std::endl;
    std::cout << "  sampling frequency: " << m.sampling_frequency << std::endl;
    std::cout << "  carrier frequency: " << m.carrier_frequency << std::endl;
    std::cout << "  signal bandwidth: " << m.signal_bandwidth << std::endl;
    std::cout << "  signal strength: " << m.signal_strength << std::endl;
    std::cout << "  end-of-message flag: " << m.eom << std::endl;
    std::cout << "  source id: " << m.src_id << std::endl;
    std::cout << "  destination id: " << m.dst_id << std::endl;
    std::cout << "  application id: " << m.app_id << std::endl;
    std::cout << "  port id: " << m.port_id << std::endl;
    std::cout << "  packet type: " << m.packet_type << std::endl;

}

}
