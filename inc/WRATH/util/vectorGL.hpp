/*! 
 * \file vectorGL.hpp
 * \brief file vectorGL.hpp
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



#ifndef __VECTOR_GL_HPP__
#define __VECTOR_GL_HPP__


#include "WRATHConfig.hpp"
#include <cmath>
#include "WRATHgl.hpp"
#include "vecN.hpp"

/*! \addtogroup Utility
 * @{
 */

/*!\typedef vec1
  Convenience typedef to GLfloat.
 */
typedef GLfloat vec1;

/*!\typedef vec2
  Convenience typedef to \ref vecN\<GLfloat,2\>
 */
typedef vecN<GLfloat,2> vec2;

/*!\typedef vec3
  Convenience typedef to \ref vecN\<GLfloat,3\>
 */
typedef vecN<GLfloat,3> vec3;

/*!\typedef vec4
  Convenience typedef to \ref vecN\<GLfloat,4\>
 */
typedef vecN<GLfloat,4> vec4;

/*!\typedef ivec1
  Conveniance typedef to GLint
 */
typedef GLint ivec1;

/*!\typedef ivec2
  Conveniance typedef to \ref vecN\<GLint,2\>
 */
typedef vecN<GLint,2> ivec2;

/*!\typedef ivec3
  Conveniance typedef to \ref vecN\<GLint,3\>
 */
typedef vecN<GLint,3> ivec3;

/*!\typedef ivec4
  Conveniance typedef to \ref vecN\<GLint,4\>
 */
typedef vecN<GLint,4> ivec4;


/*!\typedef uvec1
  Conveniance typedef to GLuint.
 */
typedef GLuint uvec1;

/*!\typedef uvec2
  Conveniance typedef to \ref vecN\<GLuint,2\>
 */
typedef vecN<GLuint,2> uvec2;

/*!\typedef uvec3
  Conveniance typedef to \ref vecN\<GLuint,3\>
 */
typedef vecN<GLuint,3> uvec3;

/*!\typedef uvec4
  Conveniance typedef to \ref vecN\<GLuint,4\>
 */
typedef vecN<GLuint,4> uvec4;


/*!\fn vecN<GLfloat,N> add_quarter(const vecN< GLint, N >&)
  Given an integer valued vecN,
  return a float value vecN where
  each component of the return value
  of 0.25f larger than the input.
  \tparam N size of vector to return and accept
  \param in value to evaluate
 */
template<unsigned int N>
vecN<GLfloat,N>
add_quarter(const vecN<GLint,N> &in)
{
  vecN<float,N> return_value(0.25f);
  for(unsigned int i=0;i<N;++i)
    {
      return_value[i]+=static_cast<float>(in[i]);
    }
  return return_value;
}

/*! @} */


                  

#endif
