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

#include <vector>    // std::vector
#include <string>    // std::string
#include <sstream>   // std::ostringstream
#include <algorithm> // std::sort(), std::equal(), std::max()
#include <limits>    // std::numeric_limits
#include <stdexcept> // std::exception, std::invalid_argument
//#include <math.h>   // nextafter

#include <iomanip>   // std::setprecision
#include "UtilValidRanges.hh"

namespace OCPIProjects {

namespace Util {

/*! @brief <B>Exception safety: If min <= max, No-throw guarantee.
 *                              If min > max, Strong guarantee.</B>
 ******************************************************************************/
template<class T>
Range<T>::Range(const T min, const T max) {

  if(min > max) {
    std::ostringstream oss;
#ifdef __GNU_C__
    oss << __func__ << "(): ";
#endif
    oss << "min (" << min << ") ";
    oss << "was > max (" << max << ")";
    throw std::invalid_argument(oss.str().c_str());
  }

  m_min = min;
  m_max = max;
}

/*! @brief <B>Exception safety: No-throw guarantee.</B>
 ******************************************************************************/
template<class T>
const T& Range<T>::get_min() const {
  return m_min;
}

/*! @brief <B>Exception safety: No-throw guarantee.</B>
 ******************************************************************************/
template<class T>
const T& Range<T>::get_max() const {
  return m_max;
}

/*! @brief <B>Exception safety: If min <= get_max(), No-throw guarantee.
 *                              If min > max, Strong guarantee.</B>
 ******************************************************************************/
template<class T>
void Range<T>::set_min(const T min) {

  if(min > m_max) {
    std::ostringstream oss;
#ifdef __GNU_C__
    oss << __func__ << "(): ";
#endif
    oss << "min argument (" << min << ") ";
    oss << "invalid because it is greater than existing max of " << m_max;
    throw std::invalid_argument(oss.str().c_str());
  }

  m_min = min;
}

/*! @brief <B>Exception safety: If max >= get_min(), No-throw guarantee.
 *                              If max < min, Strong guarantee.</B>
 ******************************************************************************/
template<class T>
void Range<T>::set_max(const T max) {

  if(m_min > max) {
    std::ostringstream oss;
#ifdef __GNU_C__
    oss << __func__ << "(): ";
#endif
    oss << "max argument (" << max << ") ";
    oss << "invalid because it is less than existing min of " << m_min;
    throw std::invalid_argument(oss.str().c_str());
  }

  m_max = max;
}

/*! @brief <B>Exception safety: No-throw guarantee.</B>
 ******************************************************************************/
template<class T>
bool Range<T>::contains(const Range<T>& range) const {
  return (m_min < range.m_min) && (m_max > range.m_max);
}

/*! @brief <B>Exception safety: No-throw guarantee.</B>
 ******************************************************************************/
template<class T>
bool Range<T>::is_valid(const T val) const {
  return (m_min <= val) && (val <= m_max);
}

/*! @brief We define < operator so that std::sort will sort by m_min.
 *         <B>Exception safety: No-throw guarantee.</B>
 ******************************************************************************/
template<class T>
bool Range<T>::operator<(const Range<T>& rhs) const {
  return m_min < rhs.m_min;
}

/*! @brief <B>Exception safety: No-throw guarantee.</B>
 ******************************************************************************/
template<class T>
bool Range<T>::operator==(const Range<T>& rhs) const {
  bool eq = is_equal(m_min, rhs.m_min) && is_equal(m_max, rhs.m_max);
  return eq;
}

/*! @brief <B>Exception safety: Basic guarantee.</B>
 ******************************************************************************/
template<class T>
void ValidRanges<T>::add_valid_range(const ValidRanges<T>::value_type range) {
  /*if(!inclusive)
  {
    min = nextafter(min, min+1);
    max = nextafter(max, max-1);
  }*/

  // remember m_ranges is guaranteed to be sorted
  // when we enter this function AND this function
  // is guaranteed to leave m_ranges correctly sorted!!

  for(auto it = m_ranges.begin(); it != m_ranges.end(); it++) {
    if(range.get_min() > it->get_max()) {
      // range                                        *****
      // *it                     ********************
      continue;
    }
    else { // (range.get_min() <= it->get_max())
      if(range.get_max() < it->get_min()) {
        // range         *******
        // *it                     ********************
        m_ranges.insert(it, range);
        return;
      }
      else { // (range.get_max() >= it->get_min())
        if(range.get_max() <= it->get_max()) {
          if(range.get_min() < it->get_min()) {
            // range         ****************
            // *it                     ********************
            // new *it       ******************************
            it->set_min(range.get_min());
            return;
          }
          else { // range.get_min() >= it->get_max()
            // range                     ****
            // *it                     ********************
            // new *it                 ********************
            return;
          }
        }
        else { // (range.get_max() > it->get_max())
          if(range.get_min() <= it->get_min()) {
            // range              ****************************
            // *it                     ********************
            it->set_min(range.get_min());
          }
          // range                                     *****
          // *it                     ********************
          it->set_max(range.get_max());
          auto to_end = it+1;
          // because we expanded *it, we need to handle cases
          // where *it was expanded to overlap *(it+1)
          while(to_end != m_ranges.end()) {
            if(it->get_max() < to_end->get_min()) {
              break; // all overlapping entries have been handled
            }
            else { // (it->get_max() >= to_end->get_min())
              it->set_max(to_end->get_max());
              m_ranges.erase(to_end);
            }
          }
        }
      }
    }
  }
  // if this point is reached, it means range is higher than any
  // other existing ranges so it must be appended
  m_ranges.push_back(range);
}

/*! @brief function combines overlapping or redundant ranges into a single
 *         range.
 *         <B>Exception safety: Basic guarantee.</B>
 *         Example usage:
 * - CODE:
   @verbatim
    #include "ValidRanges.hh"
    using namespace std;
    int main(int argc, char** argv)
    {
    ValidRanges<double> r;
    cout << "Adding valid range: [1. 3.]\n";
    r.add_valid_range(1.,3.);
    cout << "Contents of ValidRanges object: " << r << "\n";
    cout << "Adding valid range: [10. 20.]\n";
    r.add_valid_range(10.,20.);
    cout << "Contents of ValidRanges object: " << r << "\n";
    cout << "Adding valid range: [9. 30.]\n";
    r.add_valid_range(9.,30.);
    cout << "Contents of ValidRanges object: " << r << "\n";
    }
   @endverbatim
 * - OUTPUT:
   @verbatim
   Adding valid range: [1. 3.]
   Contents of ValidRanges object: [1,3]
   Adding valid range: [10. 20.]
   Contents of ValidRanges object: {[1,3],[10,20]}
   Adding valid range: [9. 30.]
   Contents of ValidRanges object: {[1,3],[9,30]}
   @endverbatim
 ******************************************************************************/
template<class T>
void ValidRanges<T>::add_valid_range(const typename value_type::value_type min,
                                     const typename value_type::value_type max) {
  /*if(!inclusive)
  {
    min = nextafter(min, min+1);
    max = nextafter(max, max-1);
  }*/

  if(min > max) {
    std::ostringstream oss;
#ifdef __GNU_C__
    oss << __func__ << "(): ";
#endif
    oss << "min (" << min << ") ";
    oss << "was > max (" << max << ")";
    throw std::invalid_argument(oss.str().c_str());
  }

  // assuming value_type is Range, no-throw guarantee
  value_type range(min, max);

  add_valid_range(range); // basic guarantee
}

/*! @brief <B>Exception safety: Basic guarantee.</B>
 *         Example usage:
 * - CODE:
   @verbatim
    #include "ValidRanges.hh"
    using namespace std;
    int main(int argc, char** argv)
    {
    ValidRanges<double> r;
    cout << "Adding valid range: [1. 3.]\n";
    r.add_valid_range(1.,3.);
    cout << "Adding valid range: [10. 20.]\n";
    r.add_valid_range(10,20.);
    cout << "Adding valid range: [20. 30.]\n";
    r.add_valid_range(20,30.);
    cout << "Contents of ValidRanges object: " << r << "\n";
    cout << "Adding invalid range: (2. 30.)\n";
    r.add_invalid_range(2,30.);
    cout << "Contents of ValidRanges object: " << r << "\n";
    }
   @endverbatim
 * - OUTPUT:
   @verbatim
   Adding valid range: [1. 3.]
   Adding valid range: [10. 20.]
   Adding valid range: [20. 30.]
   Contents of ValidRanges object: {[1,3],[10,30]}
   Adding invalid range: (2. 30.)
   Contents of ValidRanges object: {[1,2],[30]}
   @endverbatim
 ******************************************************************************/
template<class T>
bool ValidRanges<T>::add_invalid_range(
    const typename value_type::value_type min,
    const typename value_type::value_type max) {

  bool ranges_changed = false;

  if(min > max) {
    std::ostringstream oss;
#ifdef __GNU_C__
    oss << __func__ << "(): ";
#endif
    oss << "min (" << min << ") ";
    oss << "was > max (" << max << ")";
    throw std::invalid_argument(oss.str().c_str());
  }

  ///@TODO replace when adding inclusive functionality
  if(is_equal(min, max)) {
    return ranges_changed;
  }

  auto it=m_ranges.begin();
  while(it != m_ranges.end()) {
    if((min < it->get_min()) && (max > it->get_max())) {
      it = m_ranges.erase(it);
    }
    else if(it->is_valid(min)) {
      typename value_type::value_type old_max = it->get_max();
      // replace current iteration's max with the
      // specified invalid min
      ranges_changed = (!is_equal(min, it->get_max()));
      it->set_max(min);
      /*if(inclusive) {
        // excluding the min value itself
        it->set_max(nextafter(it->get_max(), it->get_max()-1));
      }*/
      if(max <= old_max) {
        // invalid range causes current valid range to split
        // into two entries
        // assuming value_type is Range, no throw guarantee since max <= old_max
        value_type range(max, old_max);

        /*if(inclusive)
        {
          range.set_min(nextafter(range.get_min(),
                                  range.get_min()+1));
        }*/
        //std::cout << "DEBUG: inserting\n";
        it = m_ranges.insert(it+1, range);
      }
      it++;
    }
    else if(it->is_valid(max)) {
      typename value_type::value_type old_min = it->get_min();
      // replace current iteration's min with the
      // specified invalid max, excluding the max value itself
      ranges_changed = !is_equal(max, it->get_min());
      it->set_min(max);
      /*if(inclusive)
      {
        it->set_min(nextafter(it->get_min(), it->get_min()+1));
      }*/
      if(min > old_min) {
        // invalid range causes current valid range to split
        // into two entries
        // assuming value_type is Range, no throw guarantee since old_min <= min
        value_type range(old_min, min);

        /*if(inclusive)
        {
          range.set_min(nextafter(range.get_min(),
                                  range.get_min()-1));
        }*/

        // as of right now, iterator points -> current
        
        //std::cout << "DEBUG: inserting\n";
        it = m_ranges.insert(it, range); // iterator points->inserted
        it++;
      }
      it++;
    }
    else {
      it++;
    }
  }
  /*if(inclusive) {
    ///@TODO handle an excluded hole, i.e. single point on a line
  }*/
  return ranges_changed;
}

/*! @brief <B>Exception safety: Basic guarantee.</B>
 ******************************************************************************/
template<class T>
bool ValidRanges<T>::impose_min_for_all_ranges(
    const typename value_type::value_type min) {

  bool ranges_changed = false;
  auto it = m_ranges.begin();
  while(it != m_ranges.end()) {
    if(min > it->get_max()) { ///@TODO >= when inclusive comes back
      // imposed min causes entry erasure
      m_ranges.erase(it); // basic guarantee

      ranges_changed = true;
      // don't increment it because we called erase()
    }
    else {
      if(min > it->get_min()) {
        // current range's min is moved
        it->set_min(min); // no-throw guarantee because min <= get_max()

        ranges_changed = true;
      }
      it++;
    }
  }
  return ranges_changed;
}

/*! @brief <B>Exception safety: Basic guarantee.</B>
 ******************************************************************************/
template<class T>
bool ValidRanges<T>::impose_max_for_all_ranges(
    const typename value_type::value_type max) {

  bool ranges_changed = false;
  auto it = m_ranges.begin();
  while(it != m_ranges.end()) {
    if(max < it->get_min()) { ///@TODO <= when inclusive comes back
      // imposed max causes entry erasure
      m_ranges.erase(it); // basic guarantee
      ranges_changed = true;
    }
    else {
      if(max < it->get_max()) {
        // current range's max is moved
        it->set_max(max); // no-throw guarantee because max >= get_min()
        ranges_changed = true;
      }
      it++;
    }
  }
  return ranges_changed;
}

/*! @brief <B>Exception safety: Basic guarantee.</B>
 *         Example usage:
 * - CODE:
   @verbatim
    #include "ValidRanges.hh"
    using namespace std;
    int main(int argc, char** argv)
    {
    ValidRanges<double> r;
    r.add_valid_range(1.,3.);
    r.add_valid_range(10.,20.);
    r.add_valid_range(30.,40.);
    ValidRanges<double> r2;
    r2.add_valid_range(2.,4.);
    r2.add_valid_range(10.,10.);
    r2.add_valid_range(29.,41.);
    cout << "      Overlapping " << r;
    cout << " with "       << r2 << "\n";
    r.overlap(r2);
    cout << "      Result of overlap: " << r << "\n";
    }
   @endverbatim
 * - OUTPUT:
   @verbatim
    Overlapping {[1,3],[10,20],[30,40]} with {[2,4],[10],[29,41]}
    Result of overlap: {[2,3],[10],[30,40]}
   @endverbatim
 ******************************************************************************/
template<class T>
bool ValidRanges<T>::overlap(const ValidRanges<value_type>& r) {
  const ValidRanges<value_type>& rc = r;
  ValidRanges<value_type> result;
  auto itr = rc.m_ranges.begin();
  while(itr != rc.m_ranges.end()) {
    auto it = m_ranges.begin();
    while(it != m_ranges.end()) {
      // itr     *******
      // it      *******
      // result  *******
      if(*itr == *it) {
        result.add_valid_range(it->get_min(), it->get_max()); // basic guarantee
        goto next_it;
      }

      // itr           *******
      // it      ********************
      // result        *******
      if(it->contains(*itr)) {
        result.add_valid_range(itr->get_min(),itr->get_max()); // basic guarantee
        goto next_it;
      }

      // itr     ********************
      // it            *******
      // result        *******
      if(itr->contains(*it)) {
        result.add_valid_range(it->get_min(), it->get_max()); // basic guarantee
        goto next_it;
      }
     
      // itr      *******
      // it                *******
      // result     (none)
      if(itr->get_max() < it->get_min()) {
        goto next_it;
      }

      // itr      *******
      // it             *******
      // result         *
      if(is_equal(itr->get_max(), it->get_min())) {
        result.add_valid_range(itr->get_max(),itr->get_max()); // basic guarantee
        goto next_it;
      }

      // itr      *******
      // it          *******
      // result      ****
      // itr      *******
      // it          **
      // result      **
      if(itr->is_valid(it->get_min())) {
        result.add_valid_range(it->get_min(),
            std::min(itr->get_max(), it->get_max())); // basic guarantee
        goto next_it;
      }

      // itr               *******
      // it        ******
      // result     (none)
      if(itr->get_min() > it->get_max()) {
        goto next_it;
      }

      // itr            *******
      // it        ******
      // result         *
      if(is_equal(itr->get_min(), it->get_max())) {
        result.add_valid_range(itr->get_min(),itr->get_min()); // basic guarantee
        goto next_it;
      }

      // itr        ********
      // it       *******
      // result     *****
      // itr        ********
      // it            **
      // result        **
      if(itr->is_valid(it->get_max())) {
        result.add_valid_range(
            std::max(itr->get_min(), it->get_min()), it->get_max()); // basic guarantee
        goto next_it;
      }

      next_it:
        it++;
    }
    itr++;
  }
  bool ranges_changed = !(*this == result);
  if(ranges_changed) {
    m_ranges = result.m_ranges;
  }
  return ranges_changed;
}

/*! @brief <B>Exception safety: Basic guarantee.</B>
 ******************************************************************************/
template<class T>
bool ValidRanges<T>::overlap(value_type r) {
  ValidRanges<value_type> rc;
  rc.add_valid_range(r); // basic guarantee
  return overlap(rc);
}

/*! @brief <B>Exception safety: If size() > 0, No-throw guarantee.
 *                              If size() == 0, Strong guarantee.</B>
 ******************************************************************************/
template<class T>
typename ValidRanges<T>::value_type::value_type
ValidRanges<T>::get_smallest_min() const {
  typename value_type::value_type min;
  if(m_ranges.size() == 0) {
    throw std::string("No min value found");
  }
  else {
    min = m_ranges[0].get_min();
  }
  for(auto it = m_ranges.begin(); it != m_ranges.end(); ++it) {
    min = (it->get_min()< min) ? it->get_min() : min;
  }
  return min;
}

/*! @brief <B>Exception safety: If size() > 0, No-throw guarantee.
 *                              If size() == 0, Strong guarantee.</B>
 ******************************************************************************/
template<class T>
typename ValidRanges<T>::value_type::value_type
ValidRanges<T>::get_largest_max() const {
  typename value_type::value_type max;
  if(m_ranges.size() == 0) {
    throw std::string("No max value found");
  }
  else {
    max = m_ranges[0].get_max();
  }
  for(auto it = m_ranges.begin(); it != m_ranges.end(); ++it) {
    max = (it->get_max() > max) ? it->get_max() : max;
  }
  return max;
}

/*! @brief <B>Exception safety: Assuming value_type is Range,
 *                              No-throw guarantee.</B>
 * - CODE:
   @verbatim
    #include "ValidRanges.hh"
    using namespace std;
    int main(int argc, char** argv)
    {
    ValidRanges<double> r;
    cout << "      Adding valid range: [1. 3.]\n";
    r.add_valid_range(1.,3.);
    cout << "      Contents of ValidRanges object: " << r <<"\n";
    cout << "      2.999 is valid: " << (r.is_valid(2.999) ? "true":"false");
    cout << "\n";
    cout << "      3.    is valid: " << (r.is_valid(3.   ) ? "true":"false");
    cout << "\n";
    cout << "      3.001 is valid: " << (r.is_valid(3.001) ? "true":"false");
    cout << "\n";
    }
   @endverbatim
 * - OUTPUT:
   @verbatim
    Contents of ValidRanges object: [1,3] 
    2.999 is valid: true
    3.    is valid: true
    3.001 is valid: false
   @endverbatim
 ******************************************************************************/
template<class T>
bool ValidRanges<T>::is_valid(const typename value_type::value_type val) const {
  bool valid = false;
  auto it = m_ranges.begin();
  while(it != m_ranges.end()) {
    if(it->is_valid(val)) {
      valid = true;
      break;
    }
    it++;
  }
  return valid;
}

/*! @brief <B>Exception safety: No-throw guarantee.</B>
 * - CODE:
   @verbatim
    #include "ValidRanges.hh"
    using namespace std;
    int main(int argc, char** argv)
    {
    ValidRanges<double> r;
    cout << "      Adding valid range: [1. 3.]\n";
    r.add_valid_range(1.,3.);
    cout << "      Contents of ValidRanges object: " << r <<"\n";
    cout << "      Clearing\n";
    r.clear();
    cout << "      Contents of ValidRanges object: " << r <<"\n";
    }
   @endverbatim
 * - OUTPUT:
   @verbatim
    Adding valid range: [1. 3.]
    Contents of ValidRanges object: [1,3] 
    Clearing
    Contents of ValidRanges object: 
   @endverbatim
 ******************************************************************************/
template<class T>
void ValidRanges<T>::clear() {
  m_ranges.clear(); // no-throw guarantee
}

/// @brief <B>Exception safety: No-throw guarantee.</B>
template<class T>
typename ValidRanges<T>::size_type ValidRanges<T>::size() const {
  return m_ranges.size(); // no-throw guarantee
}

/*! @brief <B>Exception safety: No-throw guarantee.</B>
 ******************************************************************************/
template<class T>
bool ValidRanges<T>::operator==(const ValidRanges<value_type>& rhs) const {
  if(m_ranges.size() != rhs.m_ranges.size()) {
    // note this accounts for case where one of the sizes is 0
    return false;
  }
  auto itthis = m_ranges.begin();
  auto itrhs = rhs.m_ranges.begin();
  while(itthis != m_ranges.end()) {
    if(itrhs == rhs.m_ranges.end()) {
      return false;
    }
    if(!(*itthis == *itrhs)) {
      return false;
    }
    itthis++; itrhs++;
  }
  return true;
}

/*! @brief
 * - CODE:
   @verbatim
    #include "ValidRanges.hh"
    using namespace std;
    int main(int argc, char** argv)
    {
    ValidRanges<double> r;
    cout << "      Adding valid range: [1. 3.]\n";
    r.add_valid_range(1.,3.);
    cout << "      Contents of ValidRanges object: " << r <<"\n";
    }
   @endverbatim
 * - OUTPUT:
   @verbatim
    Adding valid range: [1. 3.]
    Contents of ValidRanges object: [1,3] 
   @endverbatim
 ******************************************************************************/
template<class T>
std::ostream& operator<< (std::ostream& ostream,
    const ValidRanges<T>& obj) {

  const char* mathematics_notation_set_start= "{";
  const char* mathematics_notation_set_separator= ",";
  const char* mathematics_notation_interval_start_inclusive= "[";
  const char* mathematics_notation_interval_separator = ",";
  const char* mathematics_notation_interval_end_inclusive= "]";
  const char* mathematics_notation_set_end= "}";
  bool first = true;
  if(obj.m_ranges.size() > 1) {
    ostream << mathematics_notation_set_start;
  }
  else if(obj.m_ranges.size() == 1) {
    if(is_equal(obj.m_ranges[0].get_max(), obj.m_ranges[0].get_min())) {
      ostream << mathematics_notation_set_start;
    }
  }
  auto it = obj.m_ranges.begin();
  while(it != obj.m_ranges.end()) {
    if(not first) {
      ostream << mathematics_notation_set_separator;
    }
    if(!is_equal(it->get_max(), it->get_min())) {
      ostream << mathematics_notation_interval_start_inclusive;
    }
    ostream << std::setprecision(std::numeric_limits<typename T::value_type>::digits10+1);
    ostream << it->get_min();
    first = false;
    if(!is_equal(it->get_max(), it->get_min()))
    {
      ostream << mathematics_notation_interval_separator;
      ostream << std::setprecision(std::numeric_limits<typename T::value_type>::digits10+1);
      ostream << it->get_max();
    }
    if(!is_equal(it->get_max(), it->get_min())) {
      ostream << mathematics_notation_interval_end_inclusive;
    }
    it++;
  }
  if(obj.m_ranges.size() > 1) {
    ostream << mathematics_notation_set_end;
  }
  else if(obj.m_ranges.size() == 1) {
    if(is_equal(obj.m_ranges[0].get_max(), obj.m_ranges[0].get_min())) {
      ostream << mathematics_notation_set_end;
    }
  }
  return ostream;
}

} // namespace Util

} // namespace OCPIProjects
