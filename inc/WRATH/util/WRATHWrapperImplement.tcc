// -*- C++ -*-

/*! 
 * \file WRATHWrapperImplement.tcc
 * \brief file WRATHWrapperImplement.tcc
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


#if !defined(__WRATH_WRAPPER_HPP__) || defined(__WRATH_WRAPPER_IMPLEMENT_TCC__)
#error "Direction inclusion of private header file WRATHWrapperImplement.tcc" 
#endif

#define __WRATH_WRAPPER_IMPLEMENT_TCC__
namespace WRATHUtil
{
  template<typename T>
  class wrapper_type<T*>:public wrapper_type_base<T*>
  {
  public:
    explicit
    wrapper_type(T *v):
      wrapper_type_base<T*>(v)
    {}  

    wrapper_type(void)
    {}

    T*
    operator->(void) const
    {
      return this->m_value;
    }

    T&
    operator*(void) const
    {
      return *this->m_value;
    }
  };

  template<typename T>
  class wrapper_type<const T*>:public wrapper_type_base<const T*>
  {
  public:

    explicit
    wrapper_type(const T *v):
      wrapper_type_base<const T*>(v)
    {}

    wrapper_type(void)
    {}
    
    const T* 
    operator->(void) const
    {
      return this->m_value;
    }

    const T&
    operator*(void) const
    {
      return *this->m_value;
    }
  };
}
