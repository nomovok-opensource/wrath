/*! 
 * \file WRATHBufferBindingPoint.cpp
 * \brief file WRATHBufferBindingPoint.cpp
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


#include "WRATHConfig.hpp"
#include "WRATHBufferBindingPoint.hpp"

bool
WRATHBufferBindingPoint::
operator<(const WRATHBufferBindingPoint &rhs) const
{
  if(m_binding_point!=rhs.m_binding_point)
    {
      return m_binding_point<rhs.m_binding_point;
    }

  if(m_is_index_binding!=rhs.m_is_index_binding)
    {
      return m_is_index_binding<rhs.m_is_index_binding;
    }

  /*
    at this point m_is_index_binding==rhs.m_is_index_binding,
    however the value of m_index is ignored if m_is_index_binding
    is false
   */
  return m_is_index_binding?
    m_index<rhs.m_is_index_binding:
    false;
}


bool
WRATHBufferBindingPoint::
operator==(const WRATHBufferBindingPoint &rhs) const
{
  return m_binding_point==rhs.m_binding_point
    and m_is_index_binding==rhs.m_is_index_binding
    and (!m_is_index_binding or m_index==rhs.m_index); //m_index only matter if m_is_index_binding is true
}


