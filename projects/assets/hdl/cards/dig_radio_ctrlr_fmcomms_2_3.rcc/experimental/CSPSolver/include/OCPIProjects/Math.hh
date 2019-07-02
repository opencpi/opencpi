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

#ifndef _OCPI_PROJECTS_MATH_HH
#define _OCPI_PROJECTS_MATH_HH

#include <vector> // std::vector
#include <map> // std::map
#include <sstream> // std::ostringstream

namespace OCPIProjects {

namespace Math {

class SetBase {

  public:

    SetBase();
    const bool& get_is_empty() const;

  protected:

    bool m_is_empty;

    void throw_string_if_is_empty() const;

}; // class SetBase

/// @brief https://en.wikipedia.org/wiki/Interval_(mathematics)
template<class T>
class Interval : public SetBase {

  public:

    Interval();
    Interval(T min, T max);
    /*! @param[in] min               Minimum value for interval.
     *  @param[in] max               Maximum value for interval.
     *  @param[in] fp_comparison_tol Tolerance used for determining equality in
     *                               this event that this interval's min/max
     *                               values are compared against other values.
     *                               Value should be > 0 and only set set for
     *                               floating point template types.
     **************************************************************************/
    Interval(T min, T max, T fp_comparison_tol);
    bool operator==(const Interval<T>& rhs) const;
    const T& get_min() const;
    const T& get_max() const;
    bool get_is_fp() const;
    T    get_fp_comparison_tol() const;
    void set_min(T min);
    void set_max(T max);

    //@{ /// @brief https://en.wikipedia.org/wiki/Subset
    bool is_superset_of(T val) const;
    bool is_superset_of(const Interval<T>& interval) const;
    bool is_proper_superset_of(T val) const;
    bool is_proper_superset_of(const Interval<T>& interval) const;
    //@}

  protected:

    T    m_min;
    T    m_max;
    /// @brief Indicates template type is floating point.
    bool m_is_fp;
    /// @brief Comparison tolerance when using floating point template type.
    T    m_fp_comparison_tol;

    void throw_invalid_argument_if_max_gt_min() const;

}; // class Interval

/*! @brief <B>Exception safety: If min <= max, No-throw guarantee.
 *                              If min > max, Strong guarantee.</B>
 ******************************************************************************/
template<>
Interval<float>::Interval(float min, float max, float fp_comparison_tol) :
    SetBase(), m_min(min), m_max(max), m_is_fp(true),
    m_fp_comparison_tol(fp_comparison_tol) {

  if(m_fp_comparison_tol <= 0) {
    std::ostringstream oss;
    oss << "Interval object constructed with invalid tolerance comparison ";
    oss << "value of " << m_fp_comparison_tol;
    throw std::invalid_argument(oss.str());
  }

  m_is_empty = false;
  throw_invalid_argument_if_max_gt_min();
}

/*! @brief <B>Exception safety: If min <= max, No-throw guarantee.
 *                              If min > max, Strong guarantee.</B>
 ******************************************************************************/
template<>
Interval<double>::Interval(double min, double max, double fp_comparison_tol) :
    SetBase(), m_min(min), m_max(max), m_is_fp(true),
    m_fp_comparison_tol(fp_comparison_tol) {

  if(m_fp_comparison_tol <= 0) {
    std::ostringstream oss;
    oss << "Interval object constructed with invalid tolerance comparison ";
    oss << "value of " << m_fp_comparison_tol;
    throw std::invalid_argument(oss.str());
  }

  m_is_empty = false;
  throw_invalid_argument_if_max_gt_min();
}

/// @brief https://en.wikipedia.org/wiki/Set_(mathematics)
template<class T>
class Set : public SetBase {

  public:

    Set();
    Set(const Interval<T>& interval);
    Set(const std::vector<Interval<T> >& intervals);
    const std::vector<Interval<T> >& get_intervals() const;
    T get_min() const;
    T get_max() const;
    bool operator==(const Set<T>& rhs) const;
    bool operator!=(const Set<T>& rhs) const;

  protected:

    std::vector<Interval<T> > m_intervals;

}; // class Set

/*! @brief Constr Satisfaction Problem Solver.
 *         (https://en.wikipedia.org/wiki/Constr_satisfaction_problem)
 ******************************************************************************/
class CSPSolver {

  public:

    /// @brief Constraint.
    struct Constr {
      /*! @brief A constraint's condition is formulated the same as a
       *         condition-less constraint.
       ************************************************************************/
      typedef Constr Cond;

      const char*   m_var_key;
      const char*   m_constr_key;
      const int32_t m_int32_constant;
      const double  m_double_constant;
      const char*   m_rhs_var_key;
      const bool    m_type_is_rhs_int32_const;
      const bool    m_type_is_rhs_double_const;
      const bool    m_type_is_rhs_var;
      /// @brief Will be null if constraint has no condition.
      const Cond*   m_p_condition;

      Constr(const char* var_key, const char* constr_key,
                 const int32_t constant, Cond* p_condition = 0);
      Constr(const char* var_key, const char* constr_key,
                 const double constant, Cond* p_condition = 0);
      Constr(const char* var_key, const char* constr_key,
                 const char* rhs_var_key, Cond* p_condition = 0);

      void throw_invalid_argument_if_constraint_not_supported();
    }; // struct Constr

    /// @brief https://en.wikipedia.org/wiki/Feasible_region
    struct FeasibleRegionLimits {

      struct Var {
        Set<int32_t> m_int32_set;
        Set<double>  m_double_set;
        double       m_fp_comparison_tol;
        const bool   m_type_is_int32;
        const bool   m_type_is_double;

        Var(int32_t min, int32_t max) :
            m_int32_set(Interval<int32_t>(min, max)),
            m_double_set(),
            m_fp_comparison_tol(0.),
            m_type_is_int32(true), m_type_is_double(false) {
        }

        Var(double min, double max, double fp_comparison_tol) :
            m_int32_set(),
            m_double_set(Interval<double>(min, max, fp_comparison_tol)),
            m_fp_comparison_tol(fp_comparison_tol),
            m_type_is_int32(false), m_type_is_double(true) {
        }
      }; // struct Var

      std::map<const char*, Var> m_vars;
      void set_all_var_limits_to_type_limits();
      bool operator==(const FeasibleRegionLimits& rhs) const;
      bool operator!=(const FeasibleRegionLimits& rhs) const;
    }; // struct FeasibleRegionLimits

    /// @brief Available variable domains (D in CSP <X,D,C>)
    /*enum class domain_t {int32};*/

    CSPSolver(size_t max_num_constr_prop_loop_iter = 1024);
    /// @brief Add integer variable in given domain (X in given D for CSP <X,D,C>)
    template<typename T> void add_var(const char* var_key);
    /*! @brief Add floating point variable in given domain (X in given D for CSP <X,D,C>)
     *  @param[in] var_key           C-string key which will be used to refer to
     *  @param[in] fp_comparison_tol Tolerance used for determining equality in
     *                               this event that this variable's
     *                               values are compared against other values.
     *                               Value should be > 0 and only set set for
     *                               floating point template types.
     **************************************************************************/
    template<typename T> void add_var(const char* var_key, T fp_comparison_tol);

    /*! @brief Constrain variable. Left-hand side of equation constains only
     *         the variable being constrained, e.g. x1 >= 5 would be implemented
     *         as add_constr("x1", ">=", 5).
     *  @param[in] constr_key  One of: ">="
     *                                 "<="
     *                                 "="
     **************************************************************************/
    template<typename T> void add_constr(const char* var_key,
                                         const char* constr_key,
                                         const T rhs);

    const std::vector<Constr>& get_constr() const;
    /// @brief https://en.wikipedia.org/wiki/Feasible_region
    const FeasibleRegionLimits& get_feasible_region_limits() const;
    bool get_var_has_been_added(const char* var_key) const;

  protected:

    size_t               m_max_num_constr_prop_loop_iter;
    std::vector<Constr>  m_constr;
    FeasibleRegionLimits m_feasible_region_limits;

    /// @brief (https://en.wikipedia.org/wiki/Constr_propagation)
    void propagate_constraints();
    void throw_invalid_argument_if_var_key_has_not_been_added(
        const char* var_key);

}; // class CSPSolver

} // namespace Math

} // namespace OCPIProjects

#include "Math.cc"

#endif // _OCPI_PROJECTS_MATH_HH
