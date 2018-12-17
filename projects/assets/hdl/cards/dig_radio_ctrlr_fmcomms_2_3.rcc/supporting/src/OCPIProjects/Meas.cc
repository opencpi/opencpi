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

#include <string>  // std::string
#include <ostream> // std::ostream
#include <iomanip> // std::setprecision

namespace OCPIProjects {

namespace RadioCtrlr {

template<class T>
Meas<T>::Meas(meas_type_t type) : m_type(type) {
}

template<class T>
void Meas<T>::append_to_ostream(std::ostream& os) const {
  os << std::setprecision(std::numeric_limits<T>::digits10+1);
  os << m_value << m_unit;
  if(m_type == meas_type_t::NOMINAL) {
    os << " (nominal)";
  }
  if(m_type == meas_type_t::THEORETICAL) {
    os << " (theoretical)";
  }
  if(m_type == meas_type_t::MEASURED) {
    os << " (measured)";
  }
  if(m_type == meas_type_t::EXACT) {
    os << " (exact)";
  }
}

template<class T>
std::ostream& operator<< (std::ostream& ostream,
                          const Meas<T>& meas) {
  meas.append_to_ostream(ostream);
  return ostream;
}

} // namespace RadioCtrlr

} // namespace OCPIProjects
