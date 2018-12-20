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

#ifndef _OCPI_PROJECTS_UTIL_LOCK_R_CONSTR_CONFIG_HH
#define _OCPI_PROJECTS_UTIL_LOCK_R_CONSTR_CONFIG_HH

#include "UtilValidRanges.hh" // ValidRanges

namespace OCPIProjects {

namespace Util {

/*! @brief "Lock(able) R(ange)-Constr(ained) Config(uration)"
 *         - Absolute ranges are the absolute allowable values for a config
 *           which never change and are imposed at all times.
 *         - Constrained ranges are allowable values for a config which may
 *           be updated periodically, usually as a result of a config being
 *           constrained based on another config which has been updated.
 *         - Possible ranges are the currently allowable values for a config.
 *           This is the same as valid ranges, except that possible ranges are
 *           restricted to the locked value when the config is locked.
 ******************************************************************************/
template<typename T>
class LockRConstrConfig {

public    : typedef T value_type; // mimicking STL

protected : bool m_is_locked;

protected : ValidRanges<T> m_ranges_possible;
protected : ValidRanges<T> m_ranges_constrained;
private   : ValidRanges<T> m_ranges_absolute;

public    : LockRConstrConfig(ValidRanges<T> r);
public    : LockRConstrConfig(T val);
public    : LockRConstrConfig(typename T::value_type min,
                              typename T::value_type max);
public    : bool set_ranges_constrained(ValidRanges<T> r);
public    : bool set_ranges_possible(ValidRanges<T> r);
public    : const ValidRanges<T>& get_ranges_constrained() const;
public    : const ValidRanges<T>& get_ranges_possible() const;
public    : void add_valid_range_constrained(
                const typename T::value_type min,
                const typename T::value_type max);
public    : void add_valid_range_possible(
                const typename T::value_type min,
                const typename T::value_type max);
public    : bool add_invalid_range_constrained(
                const typename T::value_type min,
                const typename T::value_type max);
public    : bool add_invalid_range_possible(
                const typename T::value_type min,
                const typename T::value_type max);
public    : bool impose_min_for_all_ranges_constrained(
                const typename T::value_type min);
public    : bool impose_min_for_all_ranges_possible(
                const typename T::value_type min);
public    : bool impose_max_for_all_ranges_constrained(
                const typename T::value_type max);
public    : bool impose_max_for_all_ranges_possible(
                const typename T::value_type max);
public    : bool overlap_constrained(ValidRanges<T> r);
public    : bool overlap_possible(ValidRanges<T> r);
public    : typename T::value_type get_largest_max_constrained() const;
public    : typename T::value_type get_largest_max_possible() const;
public    : typename T::value_type get_smallest_min_constrained() const;
public    : typename T::value_type get_smallest_min_possible() const;
public    : bool is_locked() const;
public    : bool lock(const typename T::value_type val,
                      const typename T::value_type tolerance = 0);
public    : void set_constrained_to_absolute();
public    : void unlock();

public    : bool operator==(const LockRConstrConfig<T>& rhs) const;

}; // class LockRConstrConfig

} // namespace Util

} // namespace OCPIProjects

#include "UtilLockRConstrConfig.cc"

#endif // _OCPI_PROJECTS_UTIL_LOCK_R_CONSTR_CONFIG_HH
