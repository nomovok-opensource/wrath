/*! 
 * \file WRATHWrapper.hpp
 * \brief file WRATHWrapper.hpp
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





#ifndef __WRATH_WRAPPER_HPP__
#define __WRATH_WRAPPER_HPP__

#include "WRATHConfig.hpp"

namespace WRATHUtil
{
  /*!\class wrapper_type_base
    A base type that "wraps" another 
    type T. This class serves as a
    base class to \ref wrapper_type.
    Explicitely contstructable from T,
    provides a cast operator to T,
    also provides operator==() and 
    operator!=(). View this class
    as an implementation class for
    \ref wrapper_type whose methods
    \ref wrapper_type uses.
    \tparam T type to wrap 
   */
  template<typename T>
  class wrapper_type_base
  {
  public:
    /*!\typedef type
      Typedef to type that is wrapped
     */
    typedef T type;
    
    /*!\fn wrapper_type_base(const T&)
      Ctor. 
      \param v value to which to assign to \ref m_value
    */
    explicit
    wrapper_type_base(const T &v):
      m_value(v)
    {}
    
    /*!\fn wrapper_type_base(void)
      Ctor. Does not explicitely
      initialize \ref m_value
    */
    wrapper_type_base(void)
    {}
    
    /*!\fn operator type() const
      Cast operator to \ref type
     */
    operator type() const { return m_value; }
    
    /*!\fn bool operator==(const wrapper_type_base &) const 
      Comparison operator.
      \param rhs value to which to compare
     */
    bool 
    operator==(const wrapper_type_base &rhs) const 
    {
      return m_value==rhs.m_value;
    }
    
    /*!\fn bool operator!=(const wrapper_type_base &) const 
      Comparison operator.
      \param rhs value to which to compare
     */
    bool 
    operator!=(const wrapper_type_base &rhs) const 
    {
      return !operator==(rhs);
    }

    /*!\fn bool operator<(const wrapper_type_base &) const 
      Comparison operator.
      \param rhs value to which to compare
     */
    bool 
    operator<(const wrapper_type_base &rhs) const 
    {
      return m_value < rhs.m_value;
    }

    /*!\fn bool operator<=(const wrapper_type_base &) const 
      Comparison operator.
      \param rhs value to which to compare
     */
    bool 
    operator<=(const wrapper_type_base &rhs) const 
    {
      return m_value <= rhs.m_value;
    }

    /*!\fn bool operator>(const wrapper_type_base &) const 
      Comparison operator.
      \param rhs value to which to compare
     */
    bool 
    operator>(const wrapper_type_base &rhs) const 
    {
      return m_value > rhs.m_value;
    }

    /*!\fn bool operator>=(const wrapper_type_base &) const 
      Comparison operator.
      \param rhs value to which to compare
     */
    bool 
    operator>=(const wrapper_type_base &rhs) const 
    {
      return m_value >= rhs.m_value;
    }
    
    /*!\var m_value
      The actual value, i.e. the present in the wrapper
     */
    type m_value;
  };
  
  /*!\class wrapper_type
    A type that "wraps" another type T.
    Explicitely contstructable from T,
    provides a cast operator to T,
    also provides operator==, operator!=,
    operator<, operator<=, operator>, and
    operator >=. Has template specilization 
    so that if T is a pointer type, provides 
    operator* and operator-> correctly.
    \tparam T type to wrap
   */
  template<typename T>
  class wrapper_type:public wrapper_type_base<T>
  {
  public:
    /*!\fn wrapper_type(const T&)
      Ctor. 
      \param v value to which to assign to \ref m_value
    */
    explicit
    wrapper_type(const T &v):
      wrapper_type_base<T>(v)
    {}
    
    /*!\fn wrapper_type(void)
      Ctor. Does not explicitely
      initialize \ref m_value
    */
    wrapper_type(void)
    {}
  };

}

#include "WRATHWrapperImplement.tcc"

#endif
