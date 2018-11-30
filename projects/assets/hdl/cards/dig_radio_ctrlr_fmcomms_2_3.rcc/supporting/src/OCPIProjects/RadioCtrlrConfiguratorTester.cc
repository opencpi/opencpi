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

#include <iostream> // std::cout
#include "RadioCtrlrConfiguratorTester.hh"

namespace OCPIProjects {

namespace RadioCtrlr {

template<class T>
ConfiguratorTester<T>::ConfiguratorTester(T& UUT) : m_UUT(UUT) {
}

template<class T>
void ConfiguratorTester<T>::test_config_lock(const data_stream_ID_t& ds_ID,
    const config_key_t& cfg_key, const config_value_t val, const bool expect,
    const config_value_t tol) {
  std::cout << "TEST: attempting to lock ";
  std::cout << ds_ID << " data stream's ";
  std::cout << cfg_key << " config to " << val << "\n";
  bool did_lock = m_UUT.lock_config(ds_ID, cfg_key, val, tol);
  expected_value_did_occur(did_lock, expect);
  std::cout << "PASSED\n";
}

template<class T>
void ConfiguratorTester<T>::test_config_lock(const config_key_t cfg_key,
    const config_value_t val, const bool expect,
    const config_value_t tol) {
  std::cout << "TEST: attempting to lock ";
  std::cout << cfg_key << " config to " << val << "\n";
  bool did_lock = m_UUT.lock_config(cfg_key, val, tol);
  expected_value_did_occur(did_lock, expect);
  std::cout << "PASSED\n";
}

template<class T>
void ConfiguratorTester<T>::expected_value_did_occur(const bool actual,
    const bool expect) {
  std::cout << "actual: " << (actual ? "true" : "false") << "\t";
  std::cout << "expected: " << (expect ? "true" : "false") << "\t";
  std::cout << ((actual == expect) ? "EXPECTED\n" : "UNEXPECTED\n");
  if(actual != expect) {
    std::cout << "FAILED\n";
    throw std::string("failed");
  }
}

} // namespace RadioCtrlr

} // namespace OCPIProjects
