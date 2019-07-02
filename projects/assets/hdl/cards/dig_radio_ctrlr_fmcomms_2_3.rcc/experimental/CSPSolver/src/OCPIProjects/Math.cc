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
#include <iostream> // std::ostream
#include <stdexcept> // std::exception, std::invalid_argument
#include <vector> // std::vector
#include <map> // std::map
#include <limits> // std::numeric_limits
#include <utility> // std::make_pair
#include <cmath> // std::abs(double)
#include <algorithm> // std::min
#include <string> // std::string
#include "Math.hh"

namespace OCPIProjects {

namespace Math {

SetBase::SetBase() : m_is_empty(true) {
}

void SetBase::throw_string_if_is_empty() const {

  if(m_is_empty) {
    std::ostringstream oss;
    oss << "performed operation on interval min/max of empty interval";
    throw oss.str();
  }
}

/*! @brief <B>Exception safety: No-throw guarantee.</B>
 ******************************************************************************/
const bool& SetBase::get_is_empty() const {

  return m_is_empty;
}

/*! @brief <B>Exception safety: No-throw guarantee.</B>
 ******************************************************************************/
template<class T>
Interval<T>::Interval() : SetBase(), m_is_fp(false) {
}

/*! @brief <B>Exception safety: If min <= max, No-throw guarantee.
 *                              If min > max, Strong guarantee.</B>
 ******************************************************************************/
template<class T>
Interval<T>::Interval(T min, T max) :
    SetBase(), m_min(min), m_max(max), m_is_fp(false) {

  m_is_empty = false;
  throw_invalid_argument_if_max_gt_min();
}

template<>
Interval<float>::Interval(float min, float max) : m_min(min), m_max(max) {

  std::ostringstream oss;
  oss << "Interval object constructed for 'float' template type without ";
  oss << "constructor which sets comparison tolerance";
  throw std::invalid_argument(oss.str());
}

/*! @brief <B>Exception safety: Strong guarantee.</B>
 ******************************************************************************/
template<class T>
Interval<T>::Interval(T min, T max, T fp_comparison_tol) :
    SetBase(), m_min(min), m_max(max), m_is_fp(false),
    m_fp_comparison_tol(fp_comparison_tol) {

  std::ostringstream oss;
  oss << "Interval object constructed for non-floating point type but still ";
  oss << "specified comparison tolerance";
  throw std::invalid_argument(oss.str());
}

/*! @brief <B>Exception safety: If min <= max, No-throw guarantee.
 *                              If min > max, Strong guarantee.</B>
 ******************************************************************************/
template<class T>
bool Interval<T>::operator==(const Interval<T>& rhs) const {

  if(m_is_fp != rhs.get_is_fp()) {
    return false;
  }
  if(m_min != rhs.get_min()) {
    return false;
  }
  if(m_max != rhs.get_max()) {
    return false;
  }
  return true;
}

/*! @brief <B>Exception safety: If Interval(float,float,float) was used,
 *                              No-throw guarantee.
 *                              Otherwise, Strong guarantee.</B>
 ******************************************************************************/
template<>
bool Interval<float>::operator==(const Interval<float>& rhs) const {

  if(m_is_fp != rhs.get_is_fp()) {
    return false;
  }
  // m_is_fp is guaranteed to be true by constructor
  if(std::abs(m_min - rhs.get_min()) > m_fp_comparison_tol) {
    return false;
  }
  if(std::abs(m_max - rhs.get_max()) > m_fp_comparison_tol) {
    return false;
  }
  return true;
}

/*! @brief <B>Exception safety: If Interval(double,double,double) was used,
 *                              No-throw guarantee.
 *                              Otherwise, Strong guarantee.</B>
 ******************************************************************************/
template<>
bool Interval<double>::operator==(const Interval<double>& rhs) const {

  if(m_is_fp != rhs.get_is_fp()) {
    return false;
  }
  // m_is_fp is guaranteed to be true by constructor
  if(std::abs(m_min - rhs.get_min()) > m_fp_comparison_tol) {
    return false;
  }
  if(std::abs(m_max - rhs.get_max()) > m_fp_comparison_tol) {
    return false;
  }
  return true;
}

/*! @brief <B>Exception safety: If get_is_empty(), No-throw guarantee.
 *                              Otherwise, Strong guarantee.</B>
 ******************************************************************************/
template<class T>
const T& Interval<T>::get_min() const {

  throw_string_if_is_empty();
  return m_min;
}

/*! @brief <B>Exception safety: If get_is_empty(), No-throw guarantee.
 *                              Otherwise, Strong guarantee.</B>
 ******************************************************************************/
template<class T>
const T& Interval<T>::get_max() const {

  throw_string_if_is_empty();
  return m_max;
}

/*! @brief <B>Exception safety: Strong guarantee.</B>
 ******************************************************************************/
template<class T>
bool Interval<T>::get_is_fp() const {

  return m_is_fp;
}

/*! @brief <B>Exception safety: Strong guarantee.</B>
 ******************************************************************************/
template<class T>
T Interval<T>::get_fp_comparison_tol() const {

  if(not m_is_fp) {
    std::ostringstream oss;
    oss << "requested floating point comparison tolerance for non-floating ";
    oss << "point type";
    throw std::invalid_argument(oss.str());
  }

  return m_fp_comparison_tol;
}

/*! @brief <B>Exception safety: If min <= get_max(), No-throw guarantee.
 *                              Otherwise, Strong guarantee.</B>
 ******************************************************************************/
template<class T>
void Interval<T>::set_min(const T min) {

  m_min = min;
  throw_invalid_argument_if_max_gt_min();
}

/*! @brief <B>Exception safety: If max >= get_min(), No-throw guarantee.
 *                              Otherwise, Strong guarantee.</B>
 ******************************************************************************/
template<class T>
void Interval<T>::set_max(const T max) {

  m_max = max;
  throw_invalid_argument_if_max_gt_min();
}

/*! @brief <B>Exception safety: No-throw guarantee.</B>
 ******************************************************************************/
template<class T>
bool Interval<T>::is_superset_of(T val) const {

  bool ret;

  if(this->get_is_empty()) {
    ret = false;
  }
  else {
    ret = (m_min <= val) and (m_max >= val);
  }

  return ret;
}

/*! @brief <B>Exception safety: No-throw guarantee.</B>
 ******************************************************************************/
template<class T>
bool Interval<T>::is_superset_of(const Interval& interval) const {

  bool ret;

  if(this->get_is_empty()) {
    ret = interval.get_is_empty();
  }
  else {
    if(interval.get_is_empty()) {
      ret = false;
    }
    else {
      ret = (m_min <= interval.get_min()) and (m_max >= interval.get_max());
    }
  }

  return ret;
}

/*! @brief <B>Exception safety: No-throw guarantee.</B>
 ******************************************************************************/
template<class T>
bool Interval<T>::is_proper_superset_of(T val) const {

  bool ret;

  if(this->get_is_empty()) {
    ret = false;
  }
  else {
    ret = (m_min < val) and (m_max > val);
  }

  return ret;
}

/*! @brief <B>Exception safety: No-throw guarantee.</B>
 ******************************************************************************/
template<class T>
bool Interval<T>::is_proper_superset_of(const Interval& interval) const {

  bool ret;

  if(this->get_is_empty()) {
    ret = interval.get_is_empty();
  }
  else {
    if(interval.get_is_empty()) {
      ret = false;
    }
    else {
      ret = (m_min < interval.get_min()) and (m_max > interval.get_max());
    }
  }

  return ret;
}

template<class T>
void Interval<T>::throw_invalid_argument_if_max_gt_min() const {

  if(m_min > m_max) {
    std::ostringstream oss;
    oss << "min (" << m_min << ") ";
    oss << "was > max (" << m_max << ")";
    throw std::invalid_argument(oss.str());
  }
}

template<typename T>
std::ostream& operator<<(std::ostream& os,
    const Interval<T>& rhs) {

  if(rhs.get_is_empty() or (rhs.get_min() == rhs.get_max())) {
    os << "{"; // start of empty set or single value
  }
  else {
    os << "["; // start of non-empty interval
  }
  if(not rhs.get_is_empty()) {
    os << rhs.get_min();

    if(rhs.get_min() != rhs.get_max()) {
      // see https://en.wikipedia.org/wiki/Interval_(mathematics)#Integer_intervals
      os << "..";
      os << rhs.get_max();
    }
  }
  if(rhs.get_is_empty() or (rhs.get_min() == rhs.get_max())) {
    os << "}"; // end of empty_set or single value
  }
  else {
    os << "]"; // end of non-empty interval
  }

  return os;
}

template<>
std::ostream& operator<<(std::ostream& os,
    const Interval<float>& rhs) {

  float tol = rhs.get_fp_comparison_tol();
  bool min_equals_max = (std::abs(rhs.get_max() - rhs.get_min()) <= tol);

  if(rhs.get_is_empty() or min_equals_max) {
    os << "{"; // start of empty set or single value
  }
  else {
    os << "["; // start of non-empty interval
  }
  if(not rhs.get_is_empty()) {
    os << rhs.get_min();

    float tol = rhs.get_fp_comparison_tol();
    if(std::abs(rhs.get_max() - rhs.get_min()) >= tol) {
      os << ",";
      os << rhs.get_max();
    }
  }
  if(rhs.get_is_empty() or min_equals_max) {
    os << "}"; // end of empty_set or single value
  }
  else {
    os << "]"; // end of non-empty interval
  }

  return os;
}

template<>
std::ostream& operator<<(std::ostream& os,
    const Interval<double>& rhs) {

  double tol = rhs.get_fp_comparison_tol();
  bool min_equals_max = (std::abs(rhs.get_max() - rhs.get_min()) <= tol);

  if(rhs.get_is_empty() or min_equals_max) {
    os << "{"; // start of empty set or end of single value
  }
  else {
    os << "["; // start of non-empty interval
  }
  if(not rhs.get_is_empty()) {
    os << rhs.get_min();

    if(not min_equals_max) {
      os << ",";
      os << rhs.get_max();
    }
  }
  if(rhs.get_is_empty() or min_equals_max) {
    os << "}"; // end of empty_set or end of single value
  }
  else {
    os << "]"; // end of non-empty interval
  }

  return os;
}



/*! @brief <B>Exception safety: No-throw guarantee.</B>
 ******************************************************************************/
template<class T>
Set<T>::Set() : SetBase() {
}

template<class T>
Set<T>::Set(const Interval<T>& interval) : SetBase() {

  m_intervals.push_back(interval);
}

template<class T>
Set<T>::Set(const std::vector<Interval<T> >& intervals) : SetBase() {

  m_intervals = intervals;
}

template<class T>
const std::vector<Interval<T> >& Set<T>::get_intervals() const {

  return m_intervals;
}

template<class T>
T Set<T>::get_min() const {

  T ret;
  if(m_intervals.size() > 0) {
    ret = m_intervals[0].get_min();
  }
  else {
    throw std::string("requested min for empty set");
  }

  return ret;
}
template<class T>
T Set<T>::get_max() const {

  T ret;
  if(m_intervals.size() > 0) {
    ret = m_intervals[0].get_max();
  }
  else {
    throw std::string("requested max for empty set");
  }

  return ret;
}

template<class T>
bool Set<T>::operator==(const Set<T>& rhs) const {

  return m_intervals == rhs.get_intervals();
}

template<class T>
bool Set<T>::operator!=(const Set<T>& rhs) const {

  return not(*this == rhs);
}

template<typename T>
std::ostream& operator<<(std::ostream& os,
    const Set<T>& rhs) {

  if(rhs.get_intervals().size() != 1) {
    os << "{"; // set start
  }
  
  auto it = rhs.get_intervals().begin(); 
  for(; it != rhs.get_intervals().end(); ++it) {
    os << *it;
    if(it+1 != rhs.get_intervals().end()) {
      os << ","; // interval separator
    }
  }

  if(rhs.get_intervals().size() != 1) {
    os << "}"; // set end
  }

  return os;
}


/// @brief https://en.wikipedia.org/wiki/Union_(set_theory)
template<typename T>
Set<T> union_of(const Interval<T>& first_interval,
                const Interval<T>& second_interval) {

  bool do_append = true;
  std::vector<Interval<T> > set_intervals;
  set_intervals.push_back(first_interval);

  for(auto it = set_intervals.begin(); it != set_intervals.end(); ++it) {
    if(second_interval.get_min() > it->get_max()) {
      // second_interval                              *****
      // *it                     ********************
      continue;
    }
    else { // (second_interval.get_min() <= it->get_max())
      if(second_interval.get_max() < it->get_min()) {
        // second_interval******
        // *it                     ********************
        set_intervals.insert(it, second_interval);
        do_append = false;
        break;
      }
      else { // (second_interval.get_max() >= it->get_min())
        if(second_interval.get_max() <= it->get_max()) {
          if(second_interval.get_min() < it->get_min()) {
            // second_interval      ****************
            // *it                            ********************
            // new *it              ******************************
            it->set_min(second_interval.get_min());
            do_append = false;
            break;
          }
          else { // second_interval.get_min() >= it->get_max()
            // second_interval                  ****
            // *it                            ********************
            // new *it                        ********************
            do_append = false;
            break;
          }
        }
        else { // (second_interval.get_max() > it->get_max())
          if(second_interval.get_min() <= it->get_min()) {
            // second_interval           ****************************
            // *it                            ********************
            it->set_min(second_interval.get_min());
          }
          // second_interval                                  *****
          // *it                            ********************
          it->set_max(second_interval.get_max());
          auto to_end = it+1;
          // because we expanded *it, we need to handle cases
          // where *it was expanded to overlap *(it+1)
          while(to_end != set_intervals.end()) {
            if(it->get_max() < to_end->get_min()) {
              break; // all overlapping entries have been handled
            }
            else { // (it->get_max() >= to_end->get_min())
              it->set_max(to_end->get_max());
              set_intervals.erase(to_end);
            }
          }
          do_append = false;
          break;
        }
      }
    }
  }

  if(do_append) {
    // if this point is reached, it means interval is higher than any
    // other existing set_intervals so it must be appended
    set_intervals.push_back(second_interval);
  }

  return Set<T>(set_intervals);
}

/// @brief https://en.wikipedia.org/wiki/Intersection_(set_theory)
template<typename T>
Set<T> intersection_of(const Interval<T>& first_interval,
                       const Interval<T>& second_interval) {

  std::vector<Interval<T> > set_intervals;

  if(second_interval.get_max() >= first_interval.get_min()) {
    T min = first_interval.get_min();
    if(second_interval.get_min() > min) {
      min = second_interval.get_min();
    }
    T max = first_interval.get_max();
    if(second_interval.get_max() < max) {
      max = second_interval.get_max();
    }
    Interval<T> interval(min, max);;
    set_intervals.push_back(interval);
  }

  return Set<T>(set_intervals);
}

/// @brief https://en.wikipedia.org/wiki/Intersection_(set_theory)
template<typename T>
Set<T> intersection_of(const Set<T>& set,
                       const Interval<T>& interval) {

  std::vector<Interval<T> > set_intervals = set.get_intervals();

  auto it = set_intervals.begin();
  while(it != set_intervals.end()) {
    
    T iv_min = interval.get_min();
    T iv_max = interval.get_max();
    if((iv_max < it->get_min()) or (iv_min > it->get_max())) {
      it = set_intervals.erase(it);
    }
    else {
      bool override_min = false;
      bool override_max = false;
      if((iv_min > it->get_min()) && (iv_min < it->get_max())) {
        override_min = true;
      }
      if((iv_max > it->get_min()) && (iv_max < it->get_max())) {
        override_max = true;
      }
      if(override_min) {
        it->set_min(iv_min);
      }
      if(override_max) {
        it->set_max(iv_max);
      }
      it++;
    }
  }

  return Set<T>(set_intervals);
}

CSPSolver::Constr::Constr(const char* var_key,
    const char* constr_key, const int32_t constant,
    CSPSolver::Constr::Cond* p_condition) :
    m_var_key(var_key), m_constr_key(constr_key), 
    m_int32_constant(constant),
    m_double_constant(0),
    m_rhs_var_key(0),
    m_type_is_rhs_int32_const(true),
    m_type_is_rhs_double_const(false),
    m_type_is_rhs_var(true),
    m_p_condition(p_condition) {

  throw_invalid_argument_if_constraint_not_supported();
}

CSPSolver::Constr::Constr(const char* var_key,
    const char* constr_key, const double constant,
    CSPSolver::Constr::Cond* p_condition) :
    m_var_key(var_key), m_constr_key(constr_key), 
    m_int32_constant(0),
    m_double_constant(constant),
    m_rhs_var_key(0),
    m_type_is_rhs_int32_const(false),
    m_type_is_rhs_double_const(true),
    m_type_is_rhs_var(false),
    m_p_condition(p_condition) {

  throw_invalid_argument_if_constraint_not_supported();
}

CSPSolver::Constr::Constr(const char* var_key,
    const char* constr_key, const char* rhs_var_key,
    CSPSolver::Constr::Cond* p_condition) :
    m_var_key(var_key), m_constr_key(constr_key), 
    m_int32_constant(0),
    m_double_constant(0),
    m_rhs_var_key(rhs_var_key),
    m_type_is_rhs_int32_const(false),
    m_type_is_rhs_double_const(false),
    m_type_is_rhs_var(true),
    m_p_condition(p_condition) {

  throw_invalid_argument_if_constraint_not_supported();
}

void CSPSolver::Constr::
    throw_invalid_argument_if_constraint_not_supported() {

  bool constraint_is_supported = false;
  constraint_is_supported |= (std::string(m_constr_key) == ">=");
  constraint_is_supported |= (std::string(m_constr_key) == "<=");
  constraint_is_supported |= (std::string(m_constr_key) == "=");
  if(not constraint_is_supported) {
    std::ostringstream oss;
    oss << "invalid constraint specified: " << m_constr_key;
    throw std::invalid_argument(oss.str());
  }
}

void CSPSolver::FeasibleRegionLimits::set_all_var_limits_to_type_limits() {

  for(auto it = m_vars.begin(); it != m_vars.end(); ++it) {
    if(it->second.m_type_is_int32) {
      int32_t max = std::numeric_limits<int32_t>::max();
      int32_t min = std::numeric_limits<int32_t>::min();
      Set<int32_t> set(Interval<int32_t>(min, max));
      it->second.m_int32_set = set;
    }
    else if(it->second.m_type_is_double) {
      double max = std::numeric_limits<double>::max();
      double min = -max;
      Interval<double> interval(min, max, it->second.m_fp_comparison_tol);
      Set<double> set(interval);
      it->second.m_double_set = set;
    }
  }
}

bool CSPSolver::FeasibleRegionLimits::operator==(
    const CSPSolver::FeasibleRegionLimits& rhs) const {

  bool ret = true;

  for(auto it = m_vars.begin(); it != m_vars.end(); ++it) {
    if(it->second.m_type_is_double) {
      auto itrhs = rhs.m_vars.begin();
      for(; itrhs != rhs.m_vars.end(); ++itrhs) {
        if(it->first == itrhs->first) { // if var key matches
          if(it->second.m_type_is_int32 != itrhs->second.m_type_is_int32) {
            ret = false;
            break;
          }
          if(it->second.m_type_is_double != itrhs->second.m_type_is_double) {
            ret = false;
            break;
          }
          if(it->second.m_type_is_int32) {
            if(it->second.m_int32_set != itrhs->second.m_int32_set ) {
              ret = false;
              break;
            }
          }
          else if(it->second.m_type_is_double) {
            if(it->second.m_double_set != itrhs->second.m_double_set) {
              ret = false;
              break;
            }
          }
        }
      }
    }
    if(ret == false) {
      break;
    }
  }

  return ret;
}

bool CSPSolver::FeasibleRegionLimits::operator!=(
    const CSPSolver::FeasibleRegionLimits& rhs) const {

  return not(*this == rhs);
}

std::ostream& operator<<(std::ostream& os,
    const CSPSolver::FeasibleRegionLimits& rhs) {

  bool first = true;

  os << "<";
  {
    auto it = rhs.m_vars.begin();
    for(; it != rhs.m_vars.end(); ++it) {
      if(not first) {
        os << ",";
      }
      first = false;
      if(it->second.m_type_is_int32) {
        os << it->first << ":" << it->second.m_int32_set;
      }
      else if(it->second.m_type_is_double) {
        os << it->first << ":" << it->second.m_double_set;
      }
    }
  }
  os << ">";

  return os;
}

CSPSolver::CSPSolver(size_t max_num_constr_prop_loop_iter) :
  m_max_num_constr_prop_loop_iter(max_num_constr_prop_loop_iter) {
}

template<>
void CSPSolver::add_var<int32_t> (const char* var_key) {

  int32_t hi = std::numeric_limits<int32_t>::max();
  int32_t lo = std::numeric_limits<int32_t>::min();
  typedef FeasibleRegionLimits::Var Var;
  m_feasible_region_limits.m_vars.insert(std::make_pair(var_key, Var(lo, hi)));
}

template<>
void CSPSolver::add_var<double> (const char* var_key,
    double fp_comparison_tol) {

  double hi = std::numeric_limits<double>::max();
  double lo = -hi;
  typedef FeasibleRegionLimits::Var Var;
  double tol = fp_comparison_tol;
  const char*& key = var_key;
  m_feasible_region_limits.m_vars.insert(std::make_pair(key, Var(lo, hi, tol)));
}

template<>
void CSPSolver::add_constr(const char* var_key, const char* constr_key,
                           const double constant) {
  throw_invalid_argument_if_var_key_has_not_been_added(var_key);
  m_constr.push_back(Constr(var_key, constr_key, constant));
  propagate_constraints();
}

template<>
void CSPSolver::add_constr(const char* var_key, const char* constr_key,
                           const int32_t constant) {

  throw_invalid_argument_if_var_key_has_not_been_added(var_key);
  m_constr.push_back(Constr(var_key, constr_key, constant));
  propagate_constraints();
}

template<>
void CSPSolver::add_constr(const char* var_key, const char* constr_key,
                           const char* rhs_var_key) {

  throw_invalid_argument_if_var_key_has_not_been_added(var_key);
  m_constr.push_back(Constr(var_key, constr_key, rhs_var_key));
  propagate_constraints();
}

const std::vector<CSPSolver::Constr>& CSPSolver::get_constr() const {

  return m_constr;
}

const CSPSolver::FeasibleRegionLimits&
CSPSolver::get_feasible_region_limits() const {

  return m_feasible_region_limits;
}

bool CSPSolver::get_var_has_been_added(const char* var_key) const {

  const FeasibleRegionLimits& limits = m_feasible_region_limits;

  return limits.m_vars.find(var_key) != limits.m_vars.end();
}

void CSPSolver::propagate_constraints() {
  m_feasible_region_limits.set_all_var_limits_to_type_limits();

  bool pending_iter = true;
  for(size_t iter=1; iter <= m_max_num_constr_prop_loop_iter; iter++) {

    //std::cout << "iter=" << iter << "\n";
    FeasibleRegionLimits limits_from_prev_iter = m_feasible_region_limits;

    {
      auto itvs = m_feasible_region_limits.m_vars.begin(); 
      for(; itvs != m_feasible_region_limits.m_vars.end(); ++itvs) {

        auto itcs = m_constr.begin();
        for(; itcs != m_constr.end(); ++itcs) {

          if(itcs->m_var_key == itvs->first) {
            if(itcs->m_type_is_rhs_int32_const) {
              int32_t min, max;
              if(std::string(itcs->m_constr_key) == ">=") {
                min = itcs->m_int32_constant;
                max = std::numeric_limits<int32_t>::max();
              }
              else if(std::string(itcs->m_constr_key) == "<=") {
                min = std::numeric_limits<int32_t>::min();
                max = itcs->m_int32_constant;
              }
              else if(std::string(itcs->m_constr_key) == "=") {
                min = itcs->m_int32_constant;
                max = itcs->m_int32_constant;
              }
              Interval<int32_t> iv(min, max);
              itvs->second.m_int32_set = intersection_of(itvs->second.m_int32_set, iv);
            }
            else if(itcs->m_type_is_rhs_double_const) {
              double min, max;
              if(std::string(itcs->m_constr_key) == ">=") {
                min = itcs->m_double_constant;
                max = std::numeric_limits<double>::max();
              }
              else if(std::string(itcs->m_constr_key) == "<=") {
                min = -std::numeric_limits<double>::max();
                max = itcs->m_double_constant;
              }
              else if(std::string(itcs->m_constr_key) == "=") {
                min = itcs->m_double_constant;
                max = itcs->m_double_constant;
              }
              Interval<double> iv(min, max, itvs->second.m_fp_comparison_tol);
              itvs->second.m_double_set = intersection_of(itvs->second.m_double_set, iv);
            }
            else if(itcs->m_type_is_rhs_var) {
              /// @todo / FIXME - check for double set type, etc
              double min, max;
              auto& vars = m_feasible_region_limits.m_vars;
              auto& var = vars.at(itcs->m_rhs_var_key);
              if(std::string(itcs->m_constr_key) == ">=") {
                min = var.m_double_set.get_min();
                max = std::numeric_limits<double>::max();
              }
              else if(std::string(itcs->m_constr_key) == "<=") {
                min = -std::numeric_limits<double>::max();
                max = var.m_double_set.get_max();
              }
              else if(std::string(itcs->m_constr_key) == "=") {
                min = var.m_double_set.get_min();
                max = var.m_double_set.get_max();
              }
              // forward interval
              Interval<double> ivf(min, max, itvs->second.m_fp_comparison_tol);
              // forward constraint
              itvs->second.m_double_set = intersection_of(itvs->second.m_double_set, ivf);

              if(std::string(itcs->m_constr_key) == ">=") {
                min = itvs->second.m_double_set.get_min();
                max = std::numeric_limits<double>::max();
              }
              else if(std::string(itcs->m_constr_key) == "<=") {
                min = -std::numeric_limits<double>::max();
                max = itvs->second.m_double_set.get_max();
              }
              else if(std::string(itcs->m_constr_key) == "=") {
                min = itvs->second.m_double_set.get_min();
                max = itvs->second.m_double_set.get_max();
              }
              // reverse interval
              Interval<double> ivr(min, max, itvs->second.m_fp_comparison_tol);
              // reverse constraint
              itvs->second.m_double_set = intersection_of(itvs->second.m_double_set, ivf);
              var.m_double_set = intersection_of(var.m_double_set, ivr);
            }
          }
        }
      }
    }

    if(m_feasible_region_limits == limits_from_prev_iter) {
      pending_iter = false;
      break;
    }
  }

  if(pending_iter) {
    std::ostringstream oss;
    oss << "max number of constraint propagation loop iterations exceeded, ";
    oss << "CSPSolver object is now in an erroneous state, recommended ";
    oss << "remedies: a) increase max_num_constr_prop_loop_iter value passed ";
    oss << "to CSPSolver constructor, or b) remove constraints";
    throw oss.str();
  }
}

void CSPSolver::throw_invalid_argument_if_var_key_has_not_been_added(
    const char* var_key) {

  if(not get_var_has_been_added(var_key)) {
    std::ostringstream oss;
    oss << "invalid constraint specified (variable " << var_key;
    oss << " has not been added)";
    throw std::invalid_argument(oss.str());
  }
}

std::ostream& operator<<(std::ostream& os,
    const CSPSolver& rhs) {

  os << "<X,D,C>:=<";

  os << "X/D:";
  {
    bool first_var = true;
    auto it = rhs.get_feasible_region_limits().m_vars.begin();
    for(; it != rhs.get_feasible_region_limits().m_vars.end(); ++it) {
      if(not first_var) {
        os << ",";
      }
      first_var = false;
      if(it->second.m_type_is_int32) {
        os << it->first << "/int32";
      }
      if(it->second.m_type_is_double) {
        os << it->first << "/double";
      }
    }
  }

  os << ",C:";
  {
    bool first_var = true;
    auto it = rhs.get_constr().begin();
    for(; it != rhs.get_constr().end(); ++it) {
      if(not first_var) {
        os << ",";
      }
      first_var = false;

      os << it->m_var_key << it->m_constr_key;

      if(it->m_type_is_rhs_int32_const) {
        os << it->m_int32_constant;
      }
      else if(it->m_type_is_rhs_double_const) {
        os << it->m_double_constant;
      }
      else if(it->m_type_is_rhs_var) {
        os << it->m_rhs_var_key;
      }
    }
  }

  os << ">";
  return os;
}

} // namespace Math

} // namespace OCPIProjects
