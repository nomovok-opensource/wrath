/*! 
 * \file WRATHUtil.hpp
 * \brief file WRATHUtil.hpp
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



#ifndef __WRATH_UTIL_HPP__
#define __WRATH_UTIL_HPP__

#include "WRATHConfig.hpp"
#include <boost/integer_traits.hpp>
#include <stdint.h>
#include <string>
#include <typeinfo>
#include <list>
#include <limits>
#include <sys/time.h>
#include "c_array.hpp"

/*! \addtogroup Utility
 * @{
 */

namespace WRATHUtil
{
  /*!\class TypeInfoSortable
    A wrapper over reference to a 
    const std::type_info defining
    operator< so that one can sort
    by std::type_info values.
   */
  class TypeInfoSortable
  {
  public:
    /*!\fn TypeInfoSortable(const std::type_info&)
      Ctor. Construct from an std::type_info object,
      internally stores the reference, not a copy.
      \param tp value
     */
    TypeInfoSortable(const std::type_info &tp):
      m_type(tp)
    {}

    /*!\fn bool operator<(const TypeInfoSortable&) const
      Comparison operator for TypeInfoSortable
      \param rhs object to which to compare (right hand side of operator<).
     */
    bool
    operator<(const TypeInfoSortable &rhs) const
    {
      return m_type.before(rhs.m_type);
    }

  private:
    const std::type_info &m_type;
  };

  /*!\enum coordinate_type
    Enumeration to indicate which coordinate is fixed.
   */
  enum coordinate_type
    {
      /*!
        indicates the x-coordinate is fixed
        (and thus the y-coordinate is varying).
       */
      x_fixed=0,

      /*!
        indicates the y-coordinate is fixed
        (and thus the x-coordinate is varying).
       */
      y_fixed=1,

      /*!
        Equivalent to \ref y_fixed
       */
      x_varying=y_fixed,

      /*!
        Equivalent to \ref x_fixed
       */
      y_varying=x_fixed
    };

  /*!\fn int fixed_coordinate(enum coordinate_type)
    Returns the coordinate index that is fixed
    for a given \ref coordinate_type, i.e.
    \ref x_fixed returns 0.
    \param tp coordinate_type to query.
   */
  inline
  int
  fixed_coordinate(enum coordinate_type tp)
  {
    return tp;
  }

  /*!\fn int varying_coordinate(enum coordinate_type)
    Returns the coordinateindex  
    that is varying for a given 
    \ref coordinate_type, i.e.
    \ref x_fixed returns 1.
    \param tp coordinate_type to query.
   */
  inline
  int
  varying_coordinate(enum coordinate_type tp)
  {
    return 1-fixed_coordinate(tp);
  }

  /*!\fn uint32_t ceiling_power_2(uint32_t)
    Returns the smallest power of 2 which
    is atleast as large as the passed value,
    i.e. returns n=2^k where v<=2^k and 
    2^{k-1} < v. If v=0, returns 1. 
    
    \param v value to find the least power of 2
    that is atleast
  */
  inline
  uint32_t
  ceiling_power_2(uint32_t v)
  {
    uint32_t n;
    
    n=(v>0)? 
      (v-1):
      0;
    
    //algorithm:
    //  n = v -1
    // say n = abcdefgh (binary), then:
    // 
    // n|= (n >> 1) makes n= a (a|b) (b|c) (c|d) (d|e) (e|f) (f|g)
    // n|= (n >> 2) makes n= a (a|b) (a|b|c) (a|b|c|d) (b|c|d|e) (c|d|e|f) (d|e|f|g) 
    // n|= (n >> 4) makes n= a (a|b) (a|b|c) (a|b|c|d) (a|d|c|d|e) (a|b|c|d|e|f) (a|b|c|d|e|f|g)
    //
    // thus the bits of n have the property that once a bit is
    // up, all following bits are also up, thus n=-1 + 2^k 
    // incrementing by 1 then makes n= 2^k, which is then 
    // the precise power of 2 which is strictly greater than v-1.
    // ain't bits grand? Dug this algorithm up from somewhere
    // on the internet. Operation count= 5 bit-ors, 5 bit-shits,
    // one increment, one decrement and one conditional move.
    
    n|= (n >> 1);
    n|= (n >> 2);
    n|= (n >> 4);
    n|= (n >> 8);
    n|= (n >> 16);
    ++n;
    
    return n;
  }
  
  /*!\fn uint32_t floor_power_2(uint32_t)
    Returns the smallest power of 2
    for which a given uint32_t is
    greater than or equal to.
    \param v uint32_t to query
  */
  inline
  uint32_t
  floor_power_2(uint32_t v)
  {
    uint32_t n;
    
    n=ceiling_power_2(v);
    return (n==v)?
      v:
      n/2;
  }
  
  /*!\fn is_power_of_2(uint32_t)
    Returns true if a uint32_t is
    an exact non-zero power of 2.
    \param v uint32_t to query
  */
  inline
  bool
  is_power_of_2(uint32_t v)
  {
    return v && !(v & (v - 1));
  }
  
  /*!\fn std::string filename_extension(const std::string&)
    Simple utility function to fetch the 
    file extension of a filename.
    \param S filename 
  */
  std::string
  filename_extension(const std::string &S);
  
  /*!\fn std::string filename_fullpath(const std::string&)
    Simple utility function to return the
    absolute file path from a filename, returns the
    absoulte file path with relative pathing collapsed
    as a string with directories  delimited by '/', 
    for example "./some/path/../file.txt"
    will essentially return get_current_working() + "/some/file.txt".
    \param S filename 
  */
  std::string
  filename_fullpath(const std::string &S);
  
  /*!\fn void convert_to_halfp_from_float_raw(void*, const void*, int)
    Converts from 32bit-floats to 16bit-floats.

    \param dest destination to which to write 16bit-floats
    \param src source from which to read 32bit-floats
    \param number_elements number of conversions to perform, i.e.
                           source points to number_elements floats
                           and target points to 2*number_elements bytes.
   */
  void
  convert_to_halfp_from_float_raw(void *dest, const void *src, int number_elements);

  /*!\fn void convert_to_halfp_from_float(c_array<T>, const_c_array<S>)
    "Safer" version of convert_to_halfp_from_float_raw() taking as
    arguments c_array and const_c_array objects.
    Will WRATHassert if any of the following are true:
    - dest does not point to an even number of _bytes_,
      i.e. dest.size()*sizeof(T) must be even
    - src does not point to a number of bytes divisible by 4,
      i.e. src.size()*sizeof(S) must be divisible by 4
    - if the number of bytes pointed to by src is not precisely
      double the number of bytes pointed to by dest,
      i.e. src.size()*sizeof(S) must equal 2*dest.size()*sizeof(T) 
    \tparam T destination type, typically uint16_t or \ref vecN<uint16_t, N> for some N
    \tparam S source type, typically float or \ref vecN<float, N> for some N
    \param dest destination to which to write 16bit-floats
    \param src source from which to read 32bit-floats
   */
  template<typename T, typename S>
  void
  convert_to_halfp_from_float(c_array<T> dest, const_c_array<S> src)
  {
    int number_bytes_src, number_bytes_dest;

    number_bytes_dest=(dest.size()*sizeof(T));
    number_bytes_src=(src.size()*sizeof(S));

    WRATHassert(number_bytes_dest%2==0);
    WRATHassert(number_bytes_src%4==0);
    WRATHassert(2*number_bytes_dest==number_bytes_src);

    convert_to_halfp_from_float_raw(dest.c_ptr(), src.c_ptr(), 
                                    std::min(number_bytes_src/4,
                                             number_bytes_dest/2) );
  }

  /*!\fn convert_to_halfp_from_float(T&, const S&)
    Template friendly version, the types S and T
    need to be container class that can be used
    to constuct a c_array of types S::value_type
    and T::value_type.
    \tparam T destination type, typically uint16_t or \ref vecN<uint16_t, N> for some N
    \tparam S source type, typically float or \ref vecN<float, N> for some N
    \param dest destination to which to write 16bit-floats
    \param src source from which to read 32bit-floats
   */
  template<typename T, typename S>
  void
  convert_to_halfp_from_float(T &dest, const S &src)
  {
    convert_to_halfp_from_float(c_array<typename T::value_type>(dest), 
                                const_c_array<typename S::value_type>(src));
  }

  
  
  /*!\fn convert_to_float_from_halfp_raw(void*, const void*, int)
    Converts from 16bit-floats to 32bit-floats.
    \param dest  destination to which to write 32bit-floats
    \param src source from which to read 16bit-floats
    \param number_elements number of conversions to perform, i.e.
                           source points to 2*number_elements bytes
                           and target points to number_elements floats.
   */
  void
  convert_to_float_from_halfp_raw(void *dest, const void *src, int number_elements);


  /*!\fn convert_to_float_from_halfp(c_array<T>, const_c_array<S>)
    "Safer" version of convert_to_float_from_halfp_raw() taking as
    arguments c_array and const_c_array objects.
    Will WRATHassert if any of the following are true:
    - src does not point to an even number of _bytes_,
      i.e. src.size()*sizeof(S) must be even
    - dest does not point to a number of bytes divisible by 4,
      i.e. dest.size()*sizeof(T) must be divisible by 4
    - if the number of bytes pointed to by dest is not precisely
      double the number of bytes pointed to by src,
      i.e. dest.size()*sizeof(T) must equal 2*src.size()*sizeof(S) 
    \tparam T destination type, typically float or \ref vecN<float, N> for some N
    \tparam S source type, typically uint16_t or \ref vecN<uint16_t, N> for some N
    \param dest destination to which to write 32bit-floats
    \param src source from which to read 16bit-floats
   */
  template<typename T, typename S>
  void
  convert_to_float_from_halfp(c_array<T> dest, const_c_array<S> src)
  {
    int number_bytes_src, number_bytes_dest;

    number_bytes_dest=(dest.size()*sizeof(T));
    number_bytes_src=(src.size()*sizeof(S));

    WRATHassert(number_bytes_dest%4==0);
    WRATHassert(number_bytes_src%2==0);
    WRATHassert(number_bytes_dest==2*number_bytes_src);

    convert_to_float_from_halfp_raw(dest.c_ptr(), src.c_ptr(), 
                                    std::min(number_bytes_src/2,
                                             number_bytes_dest/4) );
  }

  /*!\fn convert_to_float_from_halfp(T&, const S&)
    Template friendly version, the types S and T
    need to be container class that can be used
    to constuct a c_array of types S::value_type
    and T::value_type.
    \tparam T destination type, typically float or \ref vecN<float, N> for some N
    \tparam S source type, typically uint16_t or \ref vecN<uint16_t, N> for some N
    \param dest destination to which to write 32bit-floats
    \param src source from which to read 16bit-floats
   */
  template<typename T, typename S>
  void
  convert_to_float_from_halfp(T &dest, const S &src)
  {
    convert_to_float_from_halfp(c_array<typename T::value_type>(dest), 
                                const_c_array<typename S::value_type>(src));
  }

  /*!\fn const_c_array<int> BinomialCoefficients(int)
    Returns the specified binomial coefficients,
    i.e. BinomialCoefficients(n) will return an
    array n+1 elements long with BinomialCoefficients(n)[k]
    equal to "n choose k", which is \f$ \frac{n!}{k!(n-k)!} \f$.
    The returned array is guaranteed to be valid
    even after additional calls to BinomialCoefficients.
    Moreover, BinomialCoefficients is thread safe and
    maybe called from multiple threads simultaneously.
    \param n binomial power
   */
  const_c_array<int>
  BinomialCoefficients(int n);

  /*!\fn int BinomialCoefficient(int, int)
    Returns the specified binomial coefficient,
    i.e. "n choose k", which is \f$ \frac{n!}{k!(n-k)!} \f$.
    Equivalent to
    \code
     BinomialCoefficients(n)[k]
    \endcode
    \param n binomial power
    \param k "choose" parameter
   */
  inline
  int
  BinomialCoefficient(int n, int k)
  {
    return BinomialCoefficients(n)[k];
  }
 
  /*!\fn int32_t time_difference(const struct timeval&, const struct timeval&)
    Returns the difference in time between two timevals in milliseconds.
    \param end end time
    \param begin begin time
   */
  inline
  int32_t
  time_difference(const struct timeval &end, const struct timeval &begin)
  {
    return (end.tv_sec-begin.tv_sec)*1000+
        (end.tv_usec-begin.tv_usec)/1000;
  }

  /*!\fn uint32_t apply_bit_flag(uint32_t, bool, uint32_t)
    Given if a bit should be up or down returns
    an input value with that bit made to be up 
    or down.
    \param input_value value to return with the named bit(s) changed
    \param to_apply if true, return value has bits made up, otherwise has bits down
    \param bitfield_value bits to make up or down as according to to_apply
   */
  inline
  uint32_t
  apply_bit_flag(uint32_t input_value, bool to_apply, 
		 uint32_t bitfield_value)
  {
    return to_apply?
      input_value|bitfield_value:
      input_value&(~bitfield_value);
  }  

  /*!\class normalizer
    Functor class to provide normalization
    of integer type from an integer range.
    \tparam T integer type from which to normalize
    \tparam Tmin minimum value, value is mapped to 0.0f
                 for unsigned normalization and -1.0f for
                 signed normalization
    \tparam Tmax maximum value, value is mapped to 1.0f
                 for unsigned normalization and 1.0f for
                 signed normalization
  */
  template<typename T, 
           T Tmin=boost::integer_traits<T>::const_min, 
           T Tmax=boost::integer_traits<T>::const_max>
  class normalizer
  {
  public:
    /*!\typedef type
      The integer type on which the normalize acts
     */
    typedef T type;

    /*!\var min_value
      The minimum value clamp, values are clamped
      to be atleast \ref min_value before normaliztion
     */
    static const T min_value=Tmin;

    /*!\var max_value
      The maximum value clamp, values are clamped
      to be no more than \ref max_value before normaliztion
     */
    static const T max_value=Tmax;

    /*!\fn float unsigned_normalize
      Returns a value normalized to [0,1].
      Values outside of \ref min_value and
      \ref max_value are clamped.
      \param v value from which to normalize
    */
    static
    float
    unsigned_normalize(T v)
    {
      float pmin(Tmin), pmax(Tmax);
      float pdelta(pmax-pmin);
      float pv(v);
      
      pv=(pv-pmin)/pdelta;
      return std::max(0.0f, std::min(1.0f, pv));
    }
    
    /*!\fn float signed_normalize
      Returns a value normalized to [0,1].
      Values outside of \ref min_value and
      \ref max_value are clamped.
      \param v value from which to normalize
    */
    static
    float
    signed_normalize(T v)
    {
      return 2.0f*unsigned_normalize(v) - 1.0f;
    }
  };
}

/*! @} */

#endif
