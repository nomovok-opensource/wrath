/*! 
 * \file WRATHPolynomial.hpp
 * \brief file WRATHPolynomial.hpp
 * 
 * Copyright 2013 by Nomovok Ltd.
 * 
 * Contact: info@nomovok.com
 * 
 * This Source Code Form is subject to the
 * terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with
 * this file, You can obtain one at
 * http://mozilla.org/MPL/2.0/.
 * 
 * \author Kevin Rogovin <kevin.rogovin@nomovok.com>
 * 
 */


#ifndef WRATH_HEADER_POLYNOMIAL_HPP_
#define WRATH_HEADER_POLYNOMIAL_HPP_

#include "WRATHConfig.hpp"
#include <vector>
#include <algorithm>
#include "WRATHUtil.hpp"
#include "vecN.hpp"
#include "c_array.hpp"


/*! \addtogroup Utility
 * @{
 */


namespace WRATHUtil
{
  /*!\enum reverse_control_points_t
    Enumeration specifying to or to not to
    reverse the control point ordering when
    generating a polynomial from control
    points of a Bezier curve.
   */
  enum reverse_control_points_t
    {
      /*!
        Do not reverse the control points.
       */
      dont_reverse_control_points,

      /*!
        Reverse the control points.
       */
      reverse_control_points,
    };

  /*!\fn void generate_polynomial_from_bezier(const_c_array< vecN<T,N> >, vecN< std::vector<T>, N> &,
                                              enum reverse_control_points_t t)
    Generate the polynomail curves from an array of points.
    The return_value std::vector output is packed as follows:
    return_value.x() holds the polynomial for the x-coordiante as follows:\n
    \f$x(t) = \f$ return_value.x()[0] \f$ + t \f$ return_value.x()[1] \f$ +  t^2\f$ return_value.x()[2] \f$ + .. \f$\n 

    Given points \f$ p0, p1, ..., \f$, the polynomial \f$B(p0,p1,..)(t) \f$ is generated as follows:
    \n\n Degree 1: \f$ B(p0, p1, t)         = (1-t)p_0 + t p_1 \f$
    \n\n Degree 2: \f$ B(p0, p1, p2, t)     = (1-t)^2 p_0 + 2t(1-t)p_1 + t^2 p_2 \f$ 
    \n\n Degree 3: \f$ B(p0, p1, p2, p3, t) = (1-t)^3 p_0 + 3t(1-t)^2 p_1 + 3t^2(1-t) p_2 + t^3 p_3\f$
    \n and so on
       
    \tparam T arithmatic type of polynomial (for example int, float, etc)
    \tparam N dimension of geometry space (for example Beizer curves on the plane have N=2)
    \param pts control points of a bezier curve
    \param return_value output, each coordinate is the polynomial of that coordinate.
    \param t enumeration to dictate to reverse the control points or not
   */
  template<typename T, unsigned int N>
  void 
  generate_polynomial_from_bezier(const_c_array< vecN<T,N> > pts, 
                                  vecN<std::vector<T>, N> &return_value,
                                  enum reverse_control_points_t t=dont_reverse_control_points);


  /*!\fn void generate_polynomial_from_bezier(const_c_array< vecN<T,N> >, vecN< std::vector<T>, N> &,
                                              const F &filter,
                                              enum reverse_control_points_t t)
    Generate the polynomail curves from a filter applied to an array of points.
    return_value.x() holds the polynomial for the x-coordiante as follows:\n
    \f$x(t) = \f$ return_value.x()[0] \f$ + t \f$ return_value.x()[1] \f$ +  t^2\f$ return_value.x()[2] \f$ + .. \f$\n 

    Given points \f$ p0, p1, ..., \f$, the polynomial \f$B(p0,p1,..)(t) \f$ is generated as follows:
    \n\n Degree 1: \f$ B(p0, p1, t)         = (1-t)p_0 + t p_1 \f$
    \n\n Degree 2: \f$ B(p0, p1, p2, t)     = (1-t)^2 p_0 + 2t(1-t)p_1 + t^2 p_2 \f$
    \n\n Degree 3: \f$ B(p0, p1, p2, p3, t) = (1-t)^3 p_0 + 3t(1-t)^2 p_1 + 3t^2(1-t) p_2 + t^3 p_3\f$
    \n and so on

    \tparam T arithmatic type of polynomial (for example int, float, etc)
    \tparam N dimension of geometry space (for example Beizer curves on the plane have N=2)
    \tparam F must implement T& operator()(const vecN<T,N>&, int coord) or 
              T& operator()(const vecN<T,N>&, int coord) of
              "filtering" a point from an array of control points,
              i.e. the type F provides an inplace means to change what point
              values are used to generate the polynomial
    \param pts control points of a bezier curve
    \param return_value output, each coordinate is the polynomial of that coordinate.
    \param filter functor applied to pts to modify their values.
    \param t enumeration to dictate to reverse the control points or not
    
   */
  template<typename T, unsigned int N, typename F>
  void 
  generate_polynomial_from_bezier(const_c_array< vecN<T,N> > pts, 
                                  vecN<std::vector<T>, N> &return_value,
                                  const F &filter,
                                  enum reverse_control_points_t t=dont_reverse_control_points);



  /*!\fn void generate_polynomial_from_bezier(const_c_array<T>, std::vector<T>&,
                                              enum reverse_control_points_t t)
    Generate the polynomail curves from an array of points.
    return_value holds the polynomial as follows:\n
    \f$x(t) = \f$ return_value[0] \f$ + t \f$ return_value[1] \f$ +  t^2\f$ return_value[2] \f$ + .. \f$\n 

    Given points \f$ p0, p1, ..., \f$, the polynomial \f$B(p0,p1,..)(t) \f$ is generated as follows:
    \n\n Degree 1: \f[ B(p0, p1, t)         = (1-t)p_0 + t p_1 \f]
    \n\n Degree 2: \f[ B(p0, p1, p2, t)     = (1-t)^2 p_0 + 2t(1-t)p_1 + t^2 p_2 \f] 
    \n\n Degree 3: \f[ B(p0, p1, p2, p3, t) = (1-t)^3 p_0 + 3t(1-t)^2 p_1 + 3t^2(1-t) p_2 + t^3 p_3\f]
    \n and so on
    \tparam T arithmatic type of polynomial (for example int, float, vecN<float,2>, etc)
    \param pts control points of a bezier curve
    \param return_value output polynomial from the input control points
    \param t enumeration to dictate to reverse the control points or not
   */
  template<typename T>
  void 
  generate_polynomial_from_bezier(const_c_array<T> pts, 
                                  std::vector<T> &return_value,
                                  enum reverse_control_points_t t=dont_reverse_control_points);

  /*!\fn void generate_polynomial_from_bezier(const_c_array<T>, std::vector<T>&, const F &,
                                              enum reverse_control_points_t t)
    Generate the polynomail curves from a filter applied to an array of control points.
    return_value holds the polynomial as follows:\n
    \f$x(t) = \f$ return_value[0] \f$ + t \f$ return_value[1] \f$ + t^2\f$ return_value[2] \f$ + .. \f$\n 

    Given points \f$ p0, p1, ..., \f$, the polynomial \f$B(p0,p1,..)(t) \f$ is generated as follows:
    \n\n Degree 1: \f[ B(p0, p1, t)         = (1-t)p_0 + t p_1 \f]
    \n\n Degree 2: \f[ B(p0, p1, p2, t)     = (1-t)^2 p_0 + 2t(1-t)p_1 + t^2 p_2 \f] 
    \n\n Degree 3: \f[ B(p0, p1, p2, p3, t) = (1-t)^3 p_0 + 3t(1-t)^2 p_1 + 3t^2(1-t) p_2 + t^3 p_3\f]
    \n and so on
    \tparam T arithmatic type of polynomial (for example int, float, vecN<float,2>, etc)
    \tparam F must implement T& operator()(const vecN<T,N>&, int coord) or 
              T& operator()(const vecN<T,N>&, int coord) of
              "filtering" a point from an array of control points,
              i.e. the type F provides an inplace means to change what point
              values are used to generate the polynomial
    \param pts control points of a bezier curve
    \param return_value output polynomial from the input control points with filter applied to them
    \param filter functor applied to pts to modify their values.
    \param t enumeration to dictate to reverse the control points or not
   */
  template<typename T, typename F>
  void 
  generate_polynomial_from_bezier(const_c_array<T> pts, 
                                  std::vector<T> &return_value,
                                  const F &filter,
                                  enum reverse_control_points_t t=dont_reverse_control_points);


  

  /*!\class polynomial_solution_solve
    A polynomial_solution_solve represents the properties
    of solving a polynomial for 0, i.e. a solution to
    \f$ f(t)=0 \f$ where f is a scaler polynomial.
   */
  class polynomial_solution_solve
  {
  public:
    /*!\fn polynomial_solution_solve& t
      Sets \ref m_t
      \param v value to use
     */
    polynomial_solution_solve&
    t(float v)
    {
      m_t=v;
      return *this;
    }

    /*!\fn polynomial_solution_solve& multiplicity
      Sets \ref m_multiplicity
      \param v value to use
     */
    polynomial_solution_solve&
    multiplicity(int v)
    {
      m_multiplicity=v;
      return *this;
    }

    /*!\var m_t
      The solution point
     */
    float m_t;

    /*!\var m_multiplicity
      Absolute value gives the multiplicity of the solution. 
      Negative values indicate that the solution is outside 
      of the interval [0,1].
     */
    int m_multiplicity;

    /*!\fn bool operator<(const polynomial_solution_solve &) const
      comparison operator, first by \ref m_t than by \ref m_multiplicity
      \param obj value to which to compare
     */
    bool
    operator<(const polynomial_solution_solve &obj) const;
  };

  /*!\fn void solve_linear(c_array<T>, std::vector<polynomial_solution_solve>&, bool)
    Solves a linear equation. Note that the polynomial
    coefficients are passed by reference to value. The
    coefficients might be changed by solve_linear().
    \tparam T coefficient type of polynomial must be implicitely castable to float
    \param poly polynomial p(t) to solve for p(t)=0.
                Must be of degree one, thus must have that poly.size() == 2. 
                poly will be modified by the computation.
    \param out_sols solution output, solutions are appended and
                    out_sols is NOT cleared
    \param record_all if true, record all solutions. If false,
                      only record those solutions for which
                      0<=t<=1.
   */
  template<typename T>
  void
  solve_linear(c_array<T> poly,
               std::vector<polynomial_solution_solve> &out_sols,
               bool record_all=false);

  /*!\fn void solve_quadratic(c_array<T>, std::vector<polynomial_solution_solve>&, bool)
    Solves a quadratic equation. Note that the polynomial
    coefficients are passed by reference to value. The
    coefficients might be changed by solve_quadratic().
    \tparam T coefficient type of polynomial must be implicitely castable to float
    \param poly polynomial p(t) to solve for p(t)=0.
                Must be of degree two, thus must have that poly.size() == 3. 
                poly will be modified by the computation.
    \param out_sols solution output, solutions are appended and
                    out_sols is NOT cleared
    \param record_all if true, record all solutions. If false,
                      only record those solutions for which
                      0<=t<=1.
   */
  template<typename T>
  void
  solve_quadratic(c_array<T> poly,
                  std::vector<polynomial_solution_solve> &out_sols,
                  bool record_all=false);

  /*!\fn void solve_cubic(c_array<T>, std::vector<polynomial_solution_solve>&, bool) 
    Solves a cubic equation. Note that the polynomial
    coefficients are passed by reference to value. The
    coefficients might be changed by solve_cubic().
    \tparam T coefficient type of polynomial must be implicitely castable to float
    \param poly polynomial p(t) to solve for p(t)=0.
                Must be of degree three, thus must have that poly.size() == 4. 
                poly will be modified by the computation.
    \param out_sols solution output, solutions are appended and
                    out_sols is NOT cleared
    \param record_all if true, record all solutions. If false,
                      only record those solutions for which
                      0<=t<=1.
   */
  template<typename T>
  void
  solve_cubic(c_array<T> poly,
              std::vector<polynomial_solution_solve> &out_sols,
              bool record_all=false);

  /*!\fn void solve_polynomial(c_array<T>, std::vector<polynomial_solution_solve>&, bool) 
    Solves a polynomial equation up to degree 3. Note that the polynomial
    coefficients are passed by reference to value. The
    coefficients might be changed by solve_cubic().
    \tparam T coefficient type of polynomial must be implicitely castable to float
    \param poly polynomial p(t) to solve for p(t)=0.
                Must be a degree no more than three, thus must have 
                that poly.size() <= 4. poly will be modified by the computation.
    \param out_sols solution output, solutions are appended and
                    out_sols is NOT cleared
    \param record_all if true, record all solutions. If false,
                      only record those solutions for which
                      0<=t<=1.
   */
  template<typename T>
  void
  solve_polynomial(c_array<T> poly,
                   std::vector<polynomial_solution_solve> &out_sols,
                   bool record_all=false);


  /*!\class BernsteinPolynomial
    A BernsteinPolynomial is a polynomial stored
    with respect to the Bernstein basis functions:

    \f[ b(n,k)(t) := C(n,k) t^k (1-t)^{n-k},  0<=k<=n \f]

    where \f$ C(n,k) = \frac{n!}{k!(n-k)!} \f$ [binomial coefficient]

    Thus the coefficient-vector \f$(f0,f1,...,fN)\f$ gives a degree N polynomial
    \f[ f(t)= \sum_{k=0}^{N} f_k b(n,k)(t) = \sum_{k=0}^{N} f_k C(n,k) t^k (1-t)^{n-k} \f]
    Note that given a sequence of control points \f$(p0, p1, .., pN)\f$ the
    Bezier Curve of that sequence is given by the BernsteinPolynomial
    with coefficient-vector \f$(p0, p1, .., pN)\f$.
   */
  template<typename T>
  class BernsteinPolynomial
  {
  public:
    /*!\fn BernsteinPolynomial(unsigned int)
      Construct a BernsteinPolynomial that has the given
      number of control points.
      \param sz number of control points of the BernsteinPolynomial
     */
    explicit
    BernsteinPolynomial(unsigned int sz):
      m_coefficients(sz),
      m_bi_coeffs(WRATHUtil::BinomialCoefficients(sz))
    {}
    
    /*!\fn BernsteinPolynomial(const_c_array<T>)
      Construct a BernsteinPolynomial from a given set of control points.
      \param coeffs control points in a const_c_array<T>
     */
    explicit
    BernsteinPolynomial(const_c_array<T> coeffs):
      m_coefficients(coeffs.begin(), coeffs.end()),
      m_bi_coeffs(WRATHUtil::BinomialCoefficients(coeffs.size()))
    {}

    /*!\fn BernsteinPolynomial(iterator, iterator)
      Construct a BernsteinPolynomial from a given set of control points.
      \param begin iterator to 1st control point
      \param end iterator to one past the last control point
     */
    template<typename iterator>
    BernsteinPolynomial(iterator begin, iterator end):
      m_coefficients(begin, end),
      m_bi_coeffs(WRATHUtil::BinomialCoefficients(m_coefficients.size()))
    {}

    /*!\fn void split_curve
      Split the BernsteinPolynomial, viewing it as a Bezier curve
      storing control points.
      \param out0 location to place the BernsteinPolynomial from 0.0 to the
                  splitting time
      \param out1 location to place the BernsteinPolynomial from
                  the splitting time to 1.0
      \param split_time time in t of the split
     */
    template<typename F>
    void
    split_curve(BernsteinPolynomial &out0,
                BernsteinPolynomial &out1,
                F split_time);

    /*!\fn void reverse
      Reverse the Bernstein polynomial, i.e. replace \f$f(t)\f$ with
      \f$ f(1-t) \f$. Equivalent to reversing the order of the
      control points.
     */
    void
    reverse(void)
    {
      std::reverse(m_coefficients.begin(), m_coefficients.end());
    }

    /*!\fn BernsteinPolynomial compute_derivative
      compute and return the derivative of this
      BernsteinPolynomial as a BernsteinPolynomial
     */ 
    BernsteinPolynomial
    compute_derivative(void) const;

    /*!\fn T evaluate(F, std::vector<F>&) const
      Evaluate the BernsteinPolynomial 
      \tparam F type suitable as argument type for the BernsteinPolynomial
      \param t value at which to evaluate the BernsteinPolynomial
      \param work_room work room used to perform computation.
     */
    template<typename F>
    T
    evaluate(F t, std::vector<F> &work_room) const;

    /*!\fn T evaluate(F) const
      Evaluate the BernsteinPolynomial 
      \tparam F type suitable as argument type for the BernsteinPolynomial
      \param t value at which to evaluate the BernsteinPolynomial
     */
    template<typename F>
    T
    evaluate(F t) const
    {
      std::vector<F> work_room;
      return evaluate(t, work_room);
    }

    /*!\fn void multiple_evaluate(F, const_c_array<BernsteinPolynomial>, c_array<T>, std::vector<F> &)
      Evalulate multiple BernsteinPolynomial (serially) using the same work
      room for each evaluation.
      Evaluate the BernsteinPolynomial 
      \tparam F type suitable as argument type for the BernsteinPolynomial
      \param t value at which to evaluate the BernsteinPolynomial
      \param polys polynomials to evaluate
      \param results location to place results, size must be the same as poly.
                     The value results[i] is polynomial polys[i] evaluated at t
      \param work_room work room used to perform computation.
     */
    template<typename F>
    static
    void
    multiple_evaluate(F t,
                      const_c_array<BernsteinPolynomial> polys,
                      c_array<T> results,
                      std::vector<F> &work_room);

    /*!\fn void multiple_evaluate(F, const_c_array<BernsteinPolynomial>, c_array<T>)
      Evalulate multiple BernsteinPolynomial (serially) using the same work
      room for each evaluation.
      Evaluate the BernsteinPolynomial 
      \tparam F type suitable as argument type for the BernsteinPolynomial
      \param t value at which to evaluate the BernsteinPolynomial
      \param polys polynomials to evaluate
      \param results location to place results, size must be the same as poly.
                     The value results[i] is polynomial polys[i] evaluated at t
     */
    template<typename F>
    static
    void
    multiple_evaluate(F t,
                      const_c_array<BernsteinPolynomial> polys,
                      c_array<T> results)
    {
      std::vector<F> work_room;
      multiple_evaluate(t, polys, results, work_room);
    }

    /*!\fn void generate_polynomial
      Return this BernsteinPolynomial as a polynomial in the usual 
      basis (i.e. the basis \f$ B={1, t, t^2, t^3, ... } \f$ 
      \param return_value locaiton to which to write the values
     */
    void
    generate_polynomial(std::vector<T> &return_value) const;

    /*!\fn int degree
      Return the degree of the BernsteinPolynomial which
      is defined as the number of control points minus one.
     */
    int
    degree(void) const
    {
      return m_coefficients.size()-1;
    }

    /*!\fn const_c_array<T> control_points(void) const
      Return the control points of the BernsteinPolynomial
     */
    const_c_array<T>
    control_points(void) const { return m_coefficients; }

    /*!\fn c_array<T> control_points(void)
      Return the control points of the BernsteinPolynomial
     */
    c_array<T>
    control_points(void) { return m_coefficients; }

    /*!\fn const T& control_point(int) const
      Return the named control point of the BernsteinPolynomial
      \param I index of control point
     */
    const T&
    control_point(int I) const { return m_coefficients[I]; }

    /*!\fn T& control_point(int)
      Return the named control point of the BernsteinPolynomial
      \param I index of control point
     */
    T&
    control_point(int I) { return m_coefficients[I]; }

  private:
    template<typename F>
    static
    void
    prepare_workroom(std::vector<F> &work_room, int sz, F t);

    std::vector<T> m_coefficients;
    const_c_array<int> m_bi_coeffs;
  };
  
}
/*! @} */

#include "WRATHPolynomialImplement.tcc"

#endif

