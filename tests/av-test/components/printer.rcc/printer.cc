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

/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Thu Mar 14 17:34:03 2019 EDT
 * BASED ON THE FILE: printer.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the printer worker in C++
 */

#include "printer-worker.hh"
#include <iostream>
#include <unistd.h>

using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace PrinterWorkerTypes;


class PrinterWorker : public PrinterWorkerBase {
  RunCondition m_aRunCondition;
  unsigned int m_counter;  
  public: 
  PrinterWorker() : m_aRunCondition(RCC_ALL_PORTS, 1000000, true){
    m_counter =0; 
    // AV-5347 periodic run conditions are broken so cant use them to trotle callign of run using 
    // usleep instead 
    //setRunCondition(&m_aRunCondition);
  } 
  private: 
  RCCResult run(bool /*timedout*/) {
    std::cout << "Value of printer.rcc Property1 is: " << properties().print_value1 << std::endl 
              << "Value of printer.rcc Property2 is: " << properties().print_value2 << std::endl 
              << std::endl; 
    usleep(1000000); 
    
    return m_counter++ < 5 ? RCC_OK : RCC_DONE; 
  }
};

PRINTER_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
PRINTER_END_INFO
