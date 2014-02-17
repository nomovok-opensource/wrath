/*! 
 * \file opengl_trait.hpp
 * \brief file opengl_trait.hpp
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


#ifndef WRATH_OPENGL_TRAIT_H_
#define WRATH_OPENGL_TRAIT_H_

#include "WRATHConfig.hpp"
#include "WRATHgl.hpp"
#include "vectorGL.hpp"


/*! \addtogroup GLUtility
 * @{
 */


/*!\struct opengl_trait
  Type trait struct that provides type information to feed GL commands.
  \tparam T type from which to extract values. The struct is specicialized for
            each of the GL types: GLubyte, GLbyte, GLuint, GLint, GLushort, GLshort, GLfloat
            and recursively for vecN 
 */
template<typename T>
struct opengl_trait
{
  /*!\typedef data_type
    Typedef to template parameter
   */
  typedef T data_type;

  /*!\typedef basic_type
    For an array type, such as vecN,
    element type of the array, otherwise
    is same as \ref data_type. Note,
    for vecN types of vecN's reports
    the element type of inner array type,
    for example 
    \code vecN<vecN<something,N>, M> \endcode
    gives something for \ref basic_type
   */
  typedef T basic_type;

  enum
    {
      /*!
        GL type label, for example if \ref basic_type
        is GLuint, then \ref type is GL_UNSIGNED_INT
       */
      type=GL_INVALID_ENUM
    };

  enum
    {
      /*!
        The number of \ref basic_type
        elements packed into one \ref data_type
       */
      count=1
    };

  enum
    {
      /*!
        The space between adjacent \ref
        data_type elements in an array
       */
      stride=sizeof(T)
    };
};

#include "opengl_trait_implement.tcc"



/*!\class opengl_trait_value
  An opengl_trait_value represents the parameters
  to feed to glVertexAttribPointer.
 */
class opengl_trait_value
{
public:
  /*!\var m_type  
    The data type, i.e. for example GL_FLOAT, GL_UNSIGNED_BYTE,
    etc. The value to use for a type T is provided by the template
    enumeration opengl_trait<T>::type, this is the 3'rd (type)
    argument to glVertexAttribPointer(GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* ptr).
   */
  GLenum m_type;

  /*!\var m_count
    The number of coordinates (for example for vec3's this is 3)
    for the data type, the value to use for a type T is provided
    by the template enumeration opengl_trait<T>::count, this is the 2'nd (size)
    argument to glVertexAttribPointer(GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* ptr).
   */
  int m_count;

  /*!\var m_stride
    The distance in bytes between consecutive elements, the 
    use case in mind is for interleaved attributes, this is the 5'th (stride)
    argument to glVertexAttribPointer(GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* ptr).
   */
  int m_stride;

  /*!\var m_normalized
    For when m_type indicates an integer type (i.e. for examples
    GL_UNSIGNED_INT), this flag indicates if the values are to
    be normalized by GL, for example for GL_UNSIGNED_BYTE, GL
    will interpret the byte value 255 as 1.0f, this is the 4'th
    argument (normalized) to glVertexAttribPointer(GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* ptr).
   */
  GLboolean m_normalized;

  /*!\var m_offset
    An offset to apply to reach the first byte, this is the essentially
    the 6'th argument (ptr) to glVertexAttribPointer(GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* ptr).
   */
  int m_offset;
  
  /*!\fn opengl_trait_value(void)
    Ctor, initialize all enumeration as GL_INVALID_ENUM,
    normalization as GL_FALSE, \ref m_count and \ref
    m_stride as -1 and \ref m_offset as 0.
   */
  opengl_trait_value(void):
    m_type(GL_INVALID_ENUM),
    m_count(-1),
    m_stride(-1),
    m_normalized(GL_FALSE),
    m_offset(0)
  {}

  /*!\fn opengl_trait_value(type_tag<T>, int)
    Ctor, initialize enumerations, stride and count
    via using opengl_trait<T> enumerations. Additionally
    sets \ref m_offset.
    \param loc value to which to set \ref m_offset
   */
  template<typename T>
  opengl_trait_value(type_tag<T>, int loc=0):
    m_type(opengl_trait<T>::type),
    m_count(opengl_trait<T>::count),
    m_stride(opengl_trait<T>::stride),
    m_normalized(GL_FALSE),
    m_offset(loc)
  {}

  /*!\fn opengl_trait_value(GLenum, int, int, int)
    Ctor, initialize enumerations, stride and count
    explicity
    \param ptype value to which to set \ref m_type
    \param pcount value to which to set \ref m_count
    \param pstride value to which to set \ref m_stride
    \param loc value to which to set \ref m_offset
   */
  opengl_trait_value(GLenum ptype, int pcount, int pstride, int loc=0):
    m_type(ptype),
    m_count(pcount),
    m_stride(pstride),
    m_normalized(GL_FALSE),
    m_offset(loc)
  {}

  /*!\fn valid
    Simple, not complete check if this opengl_trait_value
    is valid-ish. An alias for m_type!=GL_INVALID_ENUM.
   */
  bool
  valid(void) const
  {
    return m_type!=GL_INVALID_ENUM;
  }
 
  /*!\fn bool operator==(const opengl_trait_value &) const
    Equality operator.                          
    \param obj object to which to test against
   */
  bool
  operator==(const opengl_trait_value &obj) const
  {
    return (!valid() and !obj.valid())?
      true:
      (m_type==obj.m_type
       and m_count==obj.m_count
       and m_stride==obj.m_stride
       and m_normalized==obj.m_normalized
       and m_offset==obj.m_offset);
  }

 
  /*!\fn bool operator!=(const opengl_trait_value &) const
    Inequality operator.                          
    \param obj object to which to test against
   */
  bool
  operator!=(const opengl_trait_value &obj) const
  {
    return !operator==(obj);
  }

  /*!\fn bool operator<(const opengl_trait_value &) const
    Comparison operator.                          
    \param obj object to which to test against
   */
  bool
  operator<(const opengl_trait_value &obj) const
  {
    if(!valid() and !obj.valid())
      {
        return false;
      }

    if(m_type!=obj.m_type)
      {
        return m_type<obj.m_type;
      }

    if(m_count!=obj.m_count)
      {
        return m_count<obj.m_count;
      }

    if(m_stride!=obj.m_stride)
      {
        return m_stride<obj.m_stride;
      }

    if(m_offset!=obj.m_offset)
      {
        return m_offset<obj.m_offset;
      }

    if(m_normalized!=obj.m_normalized)
      {
        return m_normalized<obj.m_normalized;
      }

    return false;
  }

  /*!\fn normalized
    Set \ref m_normalized, returns a reference to this.
    \param v value to which to set \ref m_normalized
   */
  opengl_trait_value&
  normalized(GLboolean v)
  {
    m_normalized=v; 
    return *this;
  }

  /*!\fn count
    Set m_count, returns a reference to this.
    \param pcount value to which to set \ref m_count
   */
  opengl_trait_value&
  count(int pcount)
  {
    m_count=pcount; 
    return *this;
  }

  /*!\fn stride
    Set m_stride, returns a reference to this.
    \param pstride value to which to set \ref m_stride
   */
  opengl_trait_value&
  stride(int pstride)
  {
    m_stride=pstride; 
    return *this;
  }

  /*!\fn type
    Set m_type, returns a reference to this.
    \param ptype value to which to set \ref m_type
   */
  opengl_trait_value&
  type(GLenum ptype)
  {
    m_type=ptype; 
    return *this;
  }

  /*!\fn offset
    Set m_offset, returns a reference to this.
    \param v value to which to set \ref m_offset
   */
  opengl_trait_value&
  offset(int v) 
  {
    m_offset=v;
    return *this;
  }

  /*!\fn traits(type_tag<T>, GLboolean)
    Sets \ref m_type, \ref m_count and \ref  m_stride as
    opengl_trait<T>::type, opengl_trait<T>::count
    and opengl_trait<T>::stride repsectively.
    \tparam type to pass to \ref opengl_trait
    \param pnormalize value to which to set \ref m_normalized
   */
  template<typename T>
  opengl_trait_value&
  traits(type_tag<T>, GLboolean pnormalize=GL_FALSE)
  {
    m_type=opengl_trait<T>::type;
    m_count=opengl_trait<T>::count;
    m_stride=opengl_trait<T>::stride;
    m_normalized=pnormalize;
    return *this;
  }

  /*!\fn traits(type_tag<T>, type_tag<S>, GLboolean)
    Sets \ref m_type as opengl_trait<T>::type, 
    \ref m_count as opengl_trait<T>::count,
    \ref m_stride as sizeof(S) and also
    sets the value to \ref m_normalized
    The use case is that the type T is the type
    of a member variable of the type S.
    \tparam T specify member type
    \tparam S specify structure type
    \param pnormalize value to which to set \ref m_normalized    
   */
  template<typename T, typename S>
  opengl_trait_value&
  traits(type_tag<T>, type_tag<S>, GLboolean pnormalize=GL_FALSE)
  { 
    m_type=opengl_trait<T>::type;
    m_count=opengl_trait<T>::count;
    m_stride=sizeof(S);
    m_normalized=pnormalize;
    return *this;
  }

   

};

/*! @} */




#endif
