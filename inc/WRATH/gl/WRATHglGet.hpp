/*! 
 * \file WRATHglGet.hpp
 * \brief file WRATHglGet.hpp
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


#ifndef WRATH_HEADER_GL_GET_HPP_
#define WRATH_HEADER_GL_GET_HPP_


#include "WRATHConfig.hpp"
#include "WRATHgl.hpp"
#include "vecN.hpp"


/*! \addtogroup GLUtility
 * @{
 */

/*!\fn void WRATHglGet(GLenum, GLint*)
  Overloaded C++ version of glGet* family
  of functions in GL. Equivalent to
  glGetInteger(v, ptr).

  \param v GL enumeration to fetch
  \param ptr address to write values to.
 */
inline
void
WRATHglGet(GLenum v, GLint *ptr)
{
  glGetIntegerv(v, ptr);
}

/*!\fn void WRATHglGet(GLenum, GLboolean*)
  Overloaded C++ version of glGet* family
  of functions in GL. Equivalent to
  glGetBooleanv(v, ptr).

  \param v GL enumeration to fetch
  \param ptr address to write values to.
 */
inline
void
WRATHglGet(GLenum v, GLboolean *ptr)
{
  glGetBooleanv(v, ptr);
}

/*!\fn void WRATHglGet(GLenum, bool*)
  Overloaded C++ version of glGet* family
  of functions in GL. Equivalent to
  glGetBooleanv(v, ptr).

  \param v GL enumeration to fetch
  \param ptr address to write values to.
 */
inline
void
WRATHglGet(GLenum v, bool *ptr)
{
  GLboolean bptr( *ptr?GL_TRUE:GL_FALSE);
  glGetBooleanv(v, &bptr);
  *ptr=(bptr==GL_FALSE)?
    false:
    true;
}

/*!\fn void WRATHglGet(GLenum, GLfloat*)
  Overloaded C++ version of glGet* family
  of functions in GL. Equivalent to
  glGetFloatv(v, ptr).

  \param v GL enumeration to fetch
  \param ptr address to write values to.
 */
inline
void
WRATHglGet(GLenum v, GLfloat *ptr)
{
  
  glGetFloatv(v, ptr);
}

/*!\fn void WRATHglGet(GLenum, vecN<T,N>*)
  Overloaded C++ version of glGet* family
  of functions in GL, accepting the address
  of a vecN, by rules of template recursion,
  can take vecN's of other types.

  \param v GL enumeration to fetch
  \param p address to write values to.
 */
template<typename T, unsigned int N>
void
WRATHglGet(GLenum v, vecN<T,N> *p)
{
  WRATHglGet(v, p->c_ptr());
}


/*!\fn void WRATHglGet(GLenum)
  Overloaded C++ version of glGet* family
  of functions in GL. The template parameter
  determines what glGet functions is called.

  \param value GL enumeration to fetch
 */
template<typename T>
T
WRATHglGet(GLenum value)
{
  T return_value(0);
  WRATHglGet(value, &return_value);
  return return_value;
}

/*! @} */


#endif
