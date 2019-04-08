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

#ifndef _OCPI_PROJECTS_UTIL_VALID_RANGES_HH
#define _OCPI_PROJECTS_UTIL_VALID_RANGES_HH

#include <vector> // std::vector

namespace OCPIProjects {

namespace Util {

/*! @brief Template type-agnostic equality comparison. This is a necessary
 *         workaround for equality comparison for floats:
 *         "warning: comparing floating point with == or != is unsafe"
 *  @todo / FIXME - remove this function and all uses of it
 ******************************************************************************/
template<typename T> bool is_equal(const T x, const T y) {
  const bool eq = (!(x > y)) && (!(x < y));
  return eq;
}

/*! @brief Template class which represents a mathematical range/interval.
 *         Interval is modelled using a min/max pair which represent the values
 *         of the interval endpoints closest to negative and positive infinity,
 *         respectively (max > min will always evaluate to true). A Range object
 *         can be sorted with std::sort.
 ******************************************************************************/
template<class T>
class Range {

public    : typedef T value_type; // mimicking STL

protected : T m_min;
protected : T m_max;

public    : Range(T min, T max);
public    : const T& get_min() const;
public    : const T& get_max() const;
public    : void     set_min(T min);
public    : void     set_max(T max);
/*! @brief Determines whether the passed in range is contained within this
 *         object's bounds.
 ******************************************************************************/
public    : bool     contains(const Range<T>& range) const;
/// @brief Determines whether val is valid, i.e. is within this object's bounds.
public    : bool     is_valid(T val) const;
public    : bool     operator<(const Range<T>& rhs) const;
public    : bool     operator==(const Range<T>& rhs) const;

}; // class Range

/*! @brief This template class represents a collection of inclusive numeric
 *         ranges. Ranges are typically mathematically expressed either
 *           inclusively, e.g. [2 100], or
 *           exclusively, e.g. (2 100).
 *         Any value >= 2 an <= 100 would be within the range [2 100]. Any value
 *         >2 and <= 100 would be within the range (2 100].
 *         Each range in the collection is referred to as an entry
 *         herein. Any value within any of the collection of ranges is
 *         considered to be valid. The ranges are stored in
 *         ValidRanges::m_ranges: a std::vector which is always sorted
 *         low-to-high and devoid of redundant or overlapping ranges. This
 *         class has been reasonably tested with the following template types:
 *           - int16_t
 *           - uint16_t
 *           - int32_t
 *           - uint32_t
 *           - int64_t
 *           - uint64_t
 *           - float
 *           - double
 ******************************************************************************/
template<class T>
class ValidRanges {

public    : typedef T                                  value_type; // mimicking STL
public    : typedef typename std::vector<T>::size_type size_type; // mimicking STL

/// @brief Intended to be a collection of Range objects.
public    : std::vector<T> m_ranges;

/// @brief Adds a new (valid) range inclusively to the vector.
public    : void add_valid_range(T range);
/// @brief Adds a new (valid) min/max range inclusively to the vector.
public    : void add_valid_range(typename T::value_type min,
                                typename T::value_type max);
/*! @brief  Invalidates the specified range in an exclusive fashion by
 *          modifying existing valid ranges that overlap with the specified
 *          range. Because the invalid range is exclusive, the invalid range's
 *          exact min/max values will not be invalidated.
 *  @return Boolean indication of whether any ranges changed.
 ******************************************************************************/
public    : bool add_invalid_range(typename T::value_type min,
                                  typename T::value_type max);
/*! @brief  Invalidates any values below the specified min.
 *  @return Boolean indication of whether any ranges changed.
 ******************************************************************************/
public    : bool impose_min_for_all_ranges(typename T::value_type min);
/*! @brief  Invalidates any values above the specified max.
 *  @return Boolean indication of whether any ranges changed.
 ******************************************************************************/
public    : bool impose_max_for_all_ranges(typename T::value_type max);
/*! @brief  Modifies the collection of existing ranges to only include values
 *          that overlap with the referenced collection of ranges.
 *  @return Boolean indication of whether any ranges changed.
 ******************************************************************************/
public    : bool overlap(const ValidRanges<T>& r); /// @todo / FIXME rename as intersect() for consistency with mathematic nomenclature
/*! @brief  Modifies the collection of existing ranges to only include values
 *          that overlap with the referenced range.
 *  @return Boolean indication of whether any ranges changed.
 ******************************************************************************/
public    : bool overlap(T r); /// @todo / FIXME rename as intersect() for consistency with mathematic nomenclature
/// @brief Returns smallest minimum value that exists in any of the ranges.
public    : typename T::value_type get_smallest_min() const;
/// @brief Returns largest maximum value that exists in any of the ranges.
public    : typename T::value_type get_largest_max() const;
/// @brief Determines whether val is valid (is within any of the valid ranges).
public    : bool is_valid(const typename T::value_type val) const;
//! @brief Clears (removes) all ranges from the collection.
public    : void clear(); // mimicking std::vector::clear()
/*! @brief Returns the quantity of disjoint ranges that exist within the
 *         collection of ranges. Note that the ranges are always disjoint
 *         because overlapping ranges are automatically combined into a single
 *         range.
 ******************************************************************************/
public    : size_type size() const; // mimicking std::vector::size()
public    : bool operator==(const ValidRanges<T>& rhs) const;

}; // class ValidRanges

} // namespace Util

} // namespace OCPIProjects

#include "UtilValidRanges.cc"

#endif // _OCPI_PROJECTS_UTIL_VALID_RANGES_HH
