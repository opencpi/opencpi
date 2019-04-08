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

#ifndef _OCPI_PROJECTS_MEAS_HH
#define _OCPI_PROJECTS_MEAS_HH

#include <string>  // std::string
#include <ostream> // std::ostream

namespace OCPIProjects {

namespace RadioCtrlr {

/// @brief A measurement type.
enum class meas_type_t {
    /*! @brief approximate or no guarantee of precision, e.g. 2 in 2"x4" */
    NOMINAL,

    /*! @brief highly accurate and calculated based on theory of operation,
     *         e.g. PLL frequency calculation based upon precise theoretical
     *         values of discrete parts */
    THEORETICAL,

    /*! @brief digital representation of analog, i.e. real-world, value, e.g.
     *  a value read from an ADC */
    MEASURED,

    /*! @brief no discrepancy between real-world and theoretical value, e.g. a
      *        boolean value */
    EXACT};

template<class T>
class Meas {

public    : T           m_value;
public    : std::string m_unit;
private   : meas_type_t m_type;

public    : Meas(meas_type_t type);
public    : meas_type_t get_type() const;

}; // class Meas

template<class T>
std::ostream& operator<< (std::ostream& os,
                          const Meas<T>& meas) {

  os << std::setprecision(std::numeric_limits<T>::digits10+1);
  os << meas.m_value << meas.m_unit;
  if(meas.get_type() == meas_type_t::NOMINAL) {
    os << " (nominal)";
  }
  if(meas.get_type() == meas_type_t::THEORETICAL) {
    os << " (theoretical)";
  }
  if(meas.get_type() == meas_type_t::MEASURED) {
    os << " (measured)";
  }
  if(meas.get_type() == meas_type_t::EXACT) {
    os << " (exact)";
  }

  return os;
}

} // namespace RadioCtrlr

} // namespace OCPIProjects

#include "Meas.cc"

#endif // _OCPI_PROJECTS_MEAS_HH
