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

#include <sstream> // std::ostringstream
#include "UtilValidRanges.hh" // ValidRanges

namespace OCPIProjects {

namespace Util {

template<class T>
LockRConstrConfig<T>::LockRConstrConfig(
    const ValidRanges<T> r) : m_is_locked(false) {

  m_ranges_absolute = r;
  set_ranges_constrained(r);
}

template<class T>
LockRConstrConfig<T>::LockRConstrConfig(const T val) : m_is_locked(false) {

  m_ranges_absolute.add_valid_range(val); // basic guarantee
  set_ranges_constrained(val);
}

template<class T>
LockRConstrConfig<T>::LockRConstrConfig(
    const typename T::value_type min,
    const typename T::value_type max) : m_is_locked(false) {

  ValidRanges<T> r;
  r.add_valid_range(min, max);
  m_ranges_absolute = r;
  set_ranges_constrained(r);
}

/*! @brief <B>Exception safety: Assuming T is Range,
 *            Basic guarantee.</B>
 ******************************************************************************/
template<class T>
bool LockRConstrConfig<T>::set_ranges_constrained(const ValidRanges<T> r) {
  m_ranges_constrained = r;

  // constrained ranges are always a subset of absolute ranges
  // if T is Range, basic guarantee
  bool changed = m_ranges_constrained.overlap(m_ranges_absolute);

  // if T is Range, basic guarantee
  set_ranges_possible(r);

  return changed;
}

/*! @brief <B>Exception safety: Assuming T is Range,
 *            Basic guarantee.</B>
 *  @todo / FIXME - add guard to prevent changing when locked!!!
 ******************************************************************************/
template<class T>
bool LockRConstrConfig<T>::set_ranges_possible(const ValidRanges<T> r) {
  // if T is Range, no-throw guarantee
  m_ranges_possible.clear();

  for(auto it=r.m_ranges.begin(); it != r.m_ranges.end(); it++) {
    // if T is Range, basic guarantee
    m_ranges_possible.add_valid_range(*it);
  }

  // possible ranges are always a subset of constrained ranges
  // if T is Range, basic guarantee
  bool changed = m_ranges_possible.overlap(m_ranges_constrained);

  return changed;
}

/*! @brief <B>Exception safety: No-throw guarantee.</B>
 ******************************************************************************/
template<class T>
const ValidRanges<T>& LockRConstrConfig<T>::get_ranges_constrained() const {
  return m_ranges_constrained;
}

/*! @brief <B>Exception safety: No-throw guarantee.</B>
 ******************************************************************************/
template<class T>
const ValidRanges<T>& LockRConstrConfig<T>::get_ranges_possible() const {
  return m_ranges_possible;
}

/*! @brief <B>Exception safety: Assuming T is Range,
 *            Basic guarantee.</B>
 ******************************************************************************/
template<class T>
void LockRConstrConfig<T>::add_valid_range_constrained(
    const typename T::value_type min,
    const typename T::value_type max) {

  // if T is Range, basic guarantee
  add_valid_range_possible(min, max);

  // if T is Range, basic guarantee
  m_ranges_constrained.add_valid_range(min, max);

  // constrained ranges are always a subset of absolute ranges
  // if T is Range, basic guarantee
  m_ranges_constrained.overlap(m_ranges_absolute);
}

/*! @brief <B>Exception safety: Assuming T is Range,
 *            Basic guarantee.</B>
 ******************************************************************************/
template<class T>
void LockRConstrConfig<T>::add_valid_range_possible(
    const typename T::value_type min,
    const typename T::value_type max) {

  // if T is Range, basic guarantee
  m_ranges_possible.add_valid_range(min, max);

  // possible ranges are always a subset of constrained ranges
  // if T is Range, basic guarantee
  m_ranges_possible.overlap(m_ranges_constrained);
}

/*! @brief <B>Exception safety: Assuming T is Range,
 *            Basic guarantee.</B>
 ******************************************************************************/
template<class T>
bool LockRConstrConfig<T>::add_invalid_range_constrained(
    const typename T::value_type min,
    const typename T::value_type max) {

  // if T is Range, basic guarantee
  add_invalid_range_possible(min, max);

  // if T is Range, basic guarantee
  const bool b = m_ranges_constrained.add_invalid_range(min, max);

  // constrained ranges are always a subset of absolute ranges
  // if T is Range, basic guarantee
  m_ranges_constrained.overlap(m_ranges_absolute);

  return b;
}

/*! @brief <B>Exception safety: Assuming T is Range,
 *            Basic guarantee.</B>
 ******************************************************************************/
template<class T>
bool LockRConstrConfig<T>::add_invalid_range_possible(
    const typename T::value_type min,
    const typename T::value_type max) {

  // if T is Range, basic guarantee
  const bool b = m_ranges_possible.add_invalid_range(min, max);

  // possible ranges are always a subset of constrained ranges
  // if T is Range, basic guarantee
  m_ranges_possible.overlap(m_ranges_constrained);

  return b;
}

/*! @brief <B>Exception safety: Assuming T is Range,
 *            Basic guarantee.</B>
 ******************************************************************************/
template<class T>
bool LockRConstrConfig<T>::impose_min_for_all_ranges_constrained(
    const typename T::value_type min) {

  // if T is Range, basic guarantee
  impose_min_for_all_ranges_possible(min);

  // if T is Range, basic guarantee
  return m_ranges_constrained.impose_min_for_all_ranges(min);
}

/*! @brief <B>Exception safety: Assuming T is Range,
 *            Basic guarantee.</B>
 ******************************************************************************/
template<class T>
bool LockRConstrConfig<T>::impose_min_for_all_ranges_possible(
    const typename T::value_type min) {

  // if T is Range, basic guarantee
  return m_ranges_possible.impose_min_for_all_ranges(min);
}

/*! @brief <B>Exception safety: Assuming T is Range,
 *            Basic guarantee.</B>
 ******************************************************************************/
template<class T>
bool LockRConstrConfig<T>::impose_max_for_all_ranges_constrained(
    const typename value_type::value_type max) {

  // if T is Range, basic guarantee
  impose_max_for_all_ranges_possible(max);

  // if T is Range, basic guarantee
  return m_ranges_constrained.impose_max_for_all_ranges(max);
}

/*! @brief <B>Exception safety: Assuming T is Range,
 *            Basic guarantee.</B>
 ******************************************************************************/
template<class T>
bool LockRConstrConfig<T>::impose_max_for_all_ranges_possible(
    const typename T::value_type max) {

  // if T is Range, basic guarantee
  return m_ranges_possible.impose_max_for_all_ranges(max);
}

/*! @brief <B>Exception safety: Assuming T is Range,
 *            Basic guarantee.</B>
 ******************************************************************************/
template<class T>
bool LockRConstrConfig<T>::overlap_constrained(const ValidRanges<T> r) {

  // if T is Range, basic guarantee
  overlap_possible(r);

  // if T is Range, basic guarantee
  return m_ranges_constrained.overlap(r);
}

/*! @brief <B>Exception safety: Assuming T is Range,
 *            Basic guarantee.</B>
 ******************************************************************************/
template<class T>
bool LockRConstrConfig<T>::overlap_possible(const ValidRanges<T> r) {

  // if T is Range, basic guarantee
  return m_ranges_possible.overlap(r);
}

/*! @brief <B>Exception safety: Assuming T is Range,
 *            Strong guarantee.</B>
 ******************************************************************************/
template<class T>
typename T::value_type
LockRConstrConfig<T>::get_largest_max_constrained() const {

  // if T is Range, strong guarantee
  return m_ranges_constrained.get_largest_max();
}

/*! @brief <B>Exception safety: Assuming T is Range,
 *            Strong guarantee.</B>
 ******************************************************************************/
template<class T>
typename T::value_type LockRConstrConfig<T>::get_largest_max_possible() const{

  // if T is Range, strong guarantee
  return m_ranges_possible.get_largest_max();
}

/*! @brief <B>Exception safety: Assuming T is Range,
 *            Strong guarantee.</B>
 ******************************************************************************/
template<class T>
typename T::value_type
LockRConstrConfig<T>::get_smallest_min_constrained() const {

  // if T is Range, strong guarantee
  return m_ranges_constrained.get_smallest_min();
}

/*! @brief <B>Exception safety: Assuming T is Range,
 *            Strong guarantee.</B>
 ******************************************************************************/
template<class T>
typename T::value_type
LockRConstrConfig<T>::get_smallest_min_possible() const {

  // if T is Range, strong guarantee
  return m_ranges_possible.get_smallest_min();
}

/*! @brief <B>Exception safety: No-throw guarantee.</B>
 ******************************************************************************/
template<class T>
bool LockRConstrConfig<T>::is_locked() const {
  return m_is_locked;
}

/*! @brief Note that an attempted lock will fail if the config is already
 *         locked.
 *         <B>Exception safety: Assuming T is Range,
 *            Basic guarantee.</B>
 ******************************************************************************/
template<class T>
bool LockRConstrConfig<T>::lock(const typename T::value_type val,
          const typename T::value_type tolerance) {
  if(tolerance < 0) {
    std::ostringstream oss;
#ifdef __GNU_C__
    oss << __func__ << "(): ";
#endif
    oss << "tolerance (" << tolerance << ") was < 0";
    throw std::invalid_argument(oss.str().c_str());
  }

  if(!m_is_locked) {
    ValidRanges<T> test_ranges;
    
    test_ranges.add_valid_range(val-tolerance, val+tolerance); // basic guarantee
    test_ranges.overlap(m_ranges_possible); // basic guarantee
    if(test_ranges.size() > 0) {
      typename T::value_type min;

      /// @todo / FIXME - try more than just the min!!
      min = test_ranges.get_smallest_min();
      impose_min_for_all_ranges_possible(min);
      impose_max_for_all_ranges_possible(min);

      m_is_locked = true;
      return true;
    }
    else {
      return false;
    }
  }
  return false; // can't lock if already locked
}

/*! @brief <B>Exception safety: Assuming T is Range,
 *            Basic guarantee.</B>
 ******************************************************************************/
template<class T>
void LockRConstrConfig<T>::set_constrained_to_absolute() {
  // if T is Range, basic guarantee
  set_ranges_constrained(m_ranges_absolute);
}

/*! @brief <B>Exception safety: Assuming T is Range,
 *            Basic guarantee.</B>
 ******************************************************************************/
template<class T>
void LockRConstrConfig<T>::unlock() {
  // if T is Range, basic guarantee
  set_constrained_to_absolute();

  m_is_locked = false;
}

/*! @brief <B>Exception safety: No-throw guarantee.</B>
 ******************************************************************************/
template<class T>
bool LockRConstrConfig<T>::operator==(
    const LockRConstrConfig<T>& rhs) const {

  bool eq = (m_ranges_absolute    == rhs.m_ranges_absolute   );
  eq &=     (m_ranges_constrained == rhs.m_ranges_constrained);
  eq &=     (m_ranges_possible    == rhs.m_ranges_possible   );
  eq &=     (m_is_locked          == rhs.m_is_locked         );
  return eq;
}

} // namespace Util

} // namespace OCPIProjects
