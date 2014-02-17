/*! 
 * \file WRATHBufferBindingPoint.hpp
 * \brief file WRATHBufferBindingPoint.hpp
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




#ifndef WRATH_HEADER_BUFFER_BINDING_POINT_HPP_
#define WRATH_HEADER_BUFFER_BINDING_POINT_HPP_

#include "WRATHConfig.hpp"
#include "WRATHgl.hpp"

/*! \addtogroup GLUtility
 * @{
 */

/*!\class WRATHBufferBindingPoint
  A WRATHBufferBindingPoint specifies a binding point
  for a buffer object.
*/
class WRATHBufferBindingPoint
{
public:
  /*!\fn WRATHBufferBindingPoint(GLenum)
    Ctor to set the WRATHBufferBindingPoint as
    a non-indexed buffer binding point, i.e.
    is_index_binding() returns false 
    \param bp value for binding_point() to return 
   */
  WRATHBufferBindingPoint(GLenum bp=GL_INVALID_ENUM):
    m_binding_point(bp),
    m_is_index_binding(false),
    m_index(0)
  {}

  /*!\fn WRATHBufferBindingPoint(GLenum, int)
    Ctor to set the WRATHBufferBindingPoint as
    an indexed buffer binding point, i.e.
    is_index_binding() returns true 
    \param bp value for binding_point() to return 
    \param idx value index() to return 
   */
  WRATHBufferBindingPoint(GLenum bp, int idx):
    m_binding_point(bp),
    m_is_index_binding(true),
    m_index(idx)
  {}
  
  /*!\fn GLenum binding_point
    Names the binding point, i.e. the GL enumeration
    to pass to glBindBuffer, glBindBufferRange or
    glBinderBufferBase. 
   */ 
  GLenum
  binding_point(void) const
  {
    return m_binding_point; 
  }

  /*!\fn bool is_index_binding
    If true, specifies that the binding is an indexed
    binding and that index() is used in the
    binding command which is then one of
    glBindBufferRange or glBinderBufferBase
   */ 
  bool
  is_index_binding(void) const
  {
    return m_is_index_binding;
  }

  /*!\fn GLint index
    Only has effect if  is_index_binding() is true.
    Specifies the index parameter of glBindBufferRange
    and glBinderBufferBase
   */ 
  GLint
  index(void) const
  {
    return m_index;
  }

  /*!\fn bool operator<()
    Comparison operator
    \param rhs value to which to compate
   */
  bool
  operator<(const WRATHBufferBindingPoint &rhs) const;

  /*!\fn bool operator==()
    Comparison operator
    \param rhs value to which to compate
   */
  bool
  operator==(const WRATHBufferBindingPoint &rhs) const;

private:

  GLenum m_binding_point;
  bool m_is_index_binding;
  GLint m_index;
};


/*! @} */

#endif
