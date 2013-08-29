/*! 
 * \file WRATHgluniform.hpp
 * \brief file WRATHgluniform.hpp
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



#ifndef __WRATH_GL_UNIFORM_HPP__
#define __WRATH_GL_UNIFORM_HPP__

#include "WRATHConfig.hpp"
#include <vector>
#include "WRATHgl.hpp"
#include "matrixGL.hpp"
#include "vecN.hpp"
#include "WRATHNew.hpp"
#include "c_array.hpp"


/*! \addtogroup GLUtility
 * @{
 */

/*!\fn void WRATHglUniform1v(int, GLsizei, const GLfloat*)
  Equivalent to
  \code
  glUniform1fv(location, count, v);
  \endcode
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param count number of elements at location to set (i.e. if an array)
  \param v pointer to values.
 */
inline
void
WRATHglUniform1v(int location, GLsizei count, const GLfloat *v)
{
  glUniform1fv(location, count, v);
}

/*!\fn void WRATHglUniform2v(int, GLsizei, const GLfloat*)
  Equivalent to
  \code
  glUniform2fv(location, count, v);
  \endcode
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param count number of elements at location to set (i.e. if an array)
  \param v pointer to values.
 */
inline
void
WRATHglUniform2v(int location, GLsizei count, const GLfloat *v)
{
  glUniform2fv(location, count, v);
}

/*!\fn void WRATHglUniform(int, const vecN<GLfloat, 2>&)
  Equivalent to
  \code
  WRATHglUniform2v(location,1 , &v[0]);
  \endcode
  i.e. set a vec2 uniform of GLSL program.
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param v value to set to the uniform at location
 */
inline
void
WRATHglUniform(int location, const vecN<GLfloat, 2> &v)
{
  WRATHglUniform2v(location, 1, &v[0]);
}

/*!\fn void WRATHglUniform3v(int, GLsizei, const GLfloat*)
  Equivalent to
  \code
  glUniform3fv(location, count, v);
  \endcode
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param count number of elements at location to set (i.e. if an array)
  \param v pointer to values.
 */
inline
void
WRATHglUniform3v(int location, GLsizei count, const GLfloat *v)
{
  glUniform3fv(location, count, v);
}

/*!\fn void WRATHglUniform(int, const vecN<GLfloat, 3>&)
  Equivalent to
  \code
  WRATHglUniform3v(location,1 , &v[0]);
  \endcode
  i.e. set a vec3 uniform of GLSL program.
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param v value to set to the uniform at location
 */
inline
void
WRATHglUniform(int location, const vecN<GLfloat, 3> &v)
{
  WRATHglUniform3v(location, 1, &v[0]);
}

/*!\fn void WRATHglUniform4v(int, GLsizei, const GLfloat*)
  Equivalent to
  \code
  glUniform4fv(location, count, v);
  \endcode
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param count number of elements at location to set (i.e. if an array)
  \param v pointer to values.
 */
inline
void
WRATHglUniform4v(int location, GLsizei count, const GLfloat *v)
{
  glUniform4fv(location, count, v);
}

/*!\fn void WRATHglUniform(int, const vecN<GLfloat, 4>&)
  Equivalent to
  \code
  WRATHglUniform4v(location,1 , &v[0]);
  \endcode
  i.e. set a vec4 uniform of GLSL program.
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param v value to set to the uniform at location
 */
inline
void
WRATHglUniform(int location, const vecN<GLfloat, 4> &v)
{
  WRATHglUniform4v(location, 1, &v[0]);
}

/*!\fn void WRATHglUniform1v(int, GLsizei, const GLint*)
  Equivalent to
  \code
  glUniform1iv(location, count, v);
  \endcode
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param count number of elements at location to set (i.e. if an array)
  \param v pointer to values.
 */
inline
void
WRATHglUniform1v(int location, GLsizei count, const GLint *v)
{
  glUniform1iv(location, count, v);
}

/*!\fn void WRATHglUniform2v(int, GLsizei, const GLint*)
  Equivalent to
  \code
  glUniform2iv(location, count, v);
  \endcode
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param count number of elements at location to set (i.e. if an array)
  \param v pointer to values.
 */
inline
void
WRATHglUniform2v(int location, GLsizei count, const GLint *v)
{
  glUniform2iv(location, count, v);
}

/*!\fn void WRATHglUniform(int, const vecN<GLint, 2>&)
  Equivalent to
  \code
  WRATHglUniform2iv(location, 1, &v[0]);
  \endcode
  i.e. set a ivec2 uniform of GLSL program.
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param v value to set to the uniform at location
 */
inline
void
WRATHglUniform(int location, const vecN<GLint, 2> &v)
{
  WRATHglUniform2v(location, 1, &v[0]);
}

/*!\fn void WRATHglUniform3v(int, GLsizei, const GLint*)
  Equivalent to
  \code
  glUniform3iv(location, count, v);
  \endcode
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param count number of elements at location to set (i.e. if an array)
  \param v pointer to values.
 */
inline
void
WRATHglUniform3v(int location, GLsizei count, const GLint *v)
{
  glUniform3iv(location, count, v);
}

/*!\fn void WRATHglUniform(int, const vecN<GLint, 3> &)
  Equivalent to
  \code
  WRATHglUniform3iv(location, 1, &v[0]);
  \endcode
  i.e. set a ivec3 uniform of GLSL program.
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param v value to set to the uniform at location
 */
inline
void
WRATHglUniform(int location, const vecN<GLint, 3> &v)
{
  WRATHglUniform3v(location, 1, &v[0]);
}

/*!\fn void WRATHglUniform4v(int, GLsizei, const GLint*)
  Equivalent to
  \code
  glUniform4iv(location, count, v);
  \endcode
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param count number of elements at location to set (i.e. if an array)
  \param v pointer to values.
 */
inline
void
WRATHglUniform4v(int location, GLsizei count, const GLint *v)
{
  glUniform4iv(location, count, v);
}

/*!\fn void WRATHglUniform(int, const vecN<GLint, 4>&)
  Equivalent to
  \code
  WRATHglUniform4iv(location, 1, &v[0]);
  \endcode
  i.e. set a ivec4 uniform of GLSL program.
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param v value to set to the uniform at location
 */
inline
void
WRATHglUniform(int location, const vecN<GLint, 4> &v)
{
  WRATHglUniform4v(location, 1, &v[0]);
}

/*!\fn void WRATHglUniform(int, GLsizei, const float2x2*, bool)
  Equivalent to
  \code
  glUniformMatrix2fv(location, count, transposed?GL_TRUE:GL_FALSE, reinterpret_cast<const GLfloat*>(matrices));
  \endcode
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param count number of elements at location to set (i.e. if an array)
  \param matrices pointer to values
  \param transposed flag if matrices are to be interpreted as transposed by GL
 */
inline
void
WRATHglUniform(int location, GLsizei count, const float2x2 *matrices, bool transposed=false)
{
  glUniformMatrix2fv(location,
                     count,
                     transposed?GL_TRUE:GL_FALSE,
                     reinterpret_cast<const GLfloat*>(matrices));
}

/*!\fn void WRATHglUniform(int, const float2x2&, bool)
  Equivalent to
  \code
  glUniformMatrix2fv(location, 1, transposed?GL_TRUE:GL_FALSE, reinterpret_cast<const GLfloat*>(&matrices));
  \endcode
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param matrices pointer to values
  \param transposed flag if matrices are to be interpreted as transposed by GL
 */
inline
void
WRATHglUniform(int location, const float2x2 &matrices, bool transposed=false)
{
  glUniformMatrix2fv(location,
                     1,
                     transposed?GL_TRUE:GL_FALSE,
                     reinterpret_cast<const GLfloat*>(&matrices));
}

/*!\fn void WRATHglUniform(int, GLsizei, const float3x3*, bool)
  Equivalent to
  \code
  glUniformMatrix3fv(location, count, transposed?GL_TRUE:GL_FALSE, reinterpret_cast<const GLfloat*>(matrices));
  \endcode
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param count number of elements at location to set (i.e. if an array)
  \param matrices pointer to values
  \param transposed flag if matrices are to be interpreted as transposed by GL
 */
inline
void
WRATHglUniform(int location, GLsizei count, const float3x3 *matrices, bool transposed=false)
{
  glUniformMatrix3fv(location,
                     count,
                     transposed?GL_TRUE:GL_FALSE,
                     reinterpret_cast<const GLfloat*>(matrices));
}

/*!\fn void WRATHglUniform(int, const float3x3&, bool)
  Equivalent to
  \code
  glUniformMatrix3fv(location, 1, transposed?GL_TRUE:GL_FALSE, reinterpret_cast<const GLfloat*>(&matrices));
  \endcode
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param matrices pointer to values
  \param transposed flag if matrices are to be interpreted as transposed by GL
 */
inline
void
WRATHglUniform(int location, const float3x3 &matrices, bool transposed=false)
{
  glUniformMatrix3fv(location,
                     1,
                     transposed?GL_TRUE:GL_FALSE,
                     reinterpret_cast<const GLfloat*>(&matrices));
}

/*!\fn void WRATHglUniform(int, GLsizei, const float4x4*, bool)
  Equivalent to
  \code
  glUniformMatrix4fv(location, 1, transposed?GL_TRUE:GL_FALSE, reinterpret_cast<const GLfloat*>(matrices));
  \endcode
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param matrices pointer to values
  \param count number of matrices that pointer matrices point to
  \param transposed flag if matrices are to be interpreted as transposed by GL
 */
inline
void
WRATHglUniform(int location, GLsizei count, const float4x4 *matrices, bool transposed=false)
{
  glUniformMatrix4fv(location,
                     count,
                     transposed?GL_TRUE:GL_FALSE,
                     reinterpret_cast<const GLfloat*>(matrices));
}

/*!\fn void WRATHglUniform(int, const float4x4&, bool)
  Equivalent to
  \code
  glUniformMatrix4fv(location, 1, transposed?GL_TRUE:GL_FALSE, reinterpret_cast<const GLfloat*>(&matrices));
  \endcode
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param matrices pointer to values
  \param transposed flag if matrices are to be interpreted as transposed by GL
 */
inline
void
WRATHglUniform(int location, const float4x4 &matrices, bool transposed=false)
{
  glUniformMatrix4fv(location,
                     1,
                     transposed?GL_TRUE:GL_FALSE,
                     reinterpret_cast<const GLfloat*>(&matrices));
}

/*!\fn void WRATHglUniform(int, GLsizei, const vecN<GLint,2>*)
  Equivalent to
  \code
  WRATHglUniform2v(location, count, &(v[0].x()) );
  \endcode
  i.e. set an array of ivec2 GLSL uniforms.
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param v pointer to values
  \param count length of array point to by v.
 */
inline
void
WRATHglUniform(int location, GLsizei count, const vecN<GLint,2> *v)
{
  WRATHglUniform2v(location, count, &(v[0].x()) );
}

/*!\fn void WRATHglUniform(int, GLsizei, const vecN<GLint,3>*)
  Equivalent to
  \code
  WRATHglUniform2v(location, count, &(v[0].x()) );
  \endcode
  i.e. set an array of ivec3 GLSL uniforms.
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param v pointer to values
  \param count length of array point to by v.
 */
inline
void
WRATHglUniform(int location, GLsizei count, const vecN<GLint,3> *v)
{
  WRATHglUniform3v(location, count, &(v[0].x()) );
}

/*!\fn void WRATHglUniform(int, GLsizei, const vecN<GLint,4>*)
  Equivalent to
  \code
  WRATHglUniform3v(location, count, &(v[0].x()) );
  \endcode
  i.e. set an array of ivec4 GLSL uniforms.
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param v pointer to values
  \param count length of array point to by v.
 */
inline
void
WRATHglUniform(int location, GLsizei count, const vecN<GLint,4> *v)
{
  WRATHglUniform4v(location, count, &(v[0].x()) );
}


/*!\fn void WRATHglUniform(int, GLsizei, const vecN<GLfloat,2>*)
  Equivalent to
  \code
  WRATHglUniform2v(location, count, &(v[0].x()) );
  \endcode
  i.e. set an array of vec2 GLSL uniforms.
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param v pointer to values
  \param count length of array point to by v.
 */
inline
void
WRATHglUniform(int location, GLsizei count, const vecN<GLfloat,2> *v)
{
  WRATHglUniform2v(location, count, &(v[0].x()) );
}

/*!\fn void WRATHglUniform(int, GLsizei, const vecN<GLfloat,3>*)
  Equivalent to
  \code
  WRATHglUniform2v(location, count, &(v[0].x()) );
  \endcode
  i.e. set an array of vec3 GLSL uniforms.
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param v pointer to values
  \param count length of array point to by v.
 */
inline
void
WRATHglUniform(int location, GLsizei count, const vecN<GLfloat,3> *v)
{
  WRATHglUniform3v(location, count, &(v[0].x()) );
}

/*!\fn void WRATHglUniform(int, GLsizei, const vecN<GLfloat,4>*)
  Equivalent to
  \code
  WRATHglUniform2v(location, count, &(v[0].x()) );
  \endcode
  i.e. set an array of vec4 GLSL uniforms.
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param v pointer to values
  \param count length of array pointed to by v.
 */
inline
void
WRATHglUniform(int location, GLsizei count, const vecN<GLfloat,4> *v)
{
  WRATHglUniform4v(location, count, &(v[0].x()) );
}




/*!\fn void WRATHglUniform(int, GLsizei, const vecN<T,N> &)
  Template version for setting arrays of uniforms,
  equivalent to
  \code
  WRATHglUniform(location, count, v.c_ptr());
  \endcode
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param v array of values
  \param count number of elements from the array v to use.
 */
template<typename T, unsigned int N>
void
WRATHglUniform(int location, GLsizei count, const vecN<T,N> &v)
{
  WRATHglUniform(location, count, v.c_ptr());
}

/*!\fn void WRATHglUniform(int, GLsizei, const vecN<T,N>&, bool)
  Template version for setting arrays of uniforms,
  equivalent to
  \code
  WRATHglUniform(location, count, v.c_ptr(), transposed);
  \endcode
  This call is for when v is an array of matrices.
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param v array of values
  \param count number of elements from the array v to use
  \param transposed flag to indicate if the matrices are to be transposed
 */
template<typename T, unsigned int N>
void
WRATHglUniform(int location, GLsizei count, const vecN<T,N> &v, bool transposed)
{
  WRATHglUniform(location, count, v.c_ptr(), transposed);
}


/*!\fn void WRATHglUniform(int, GLsizei, const_c_array<T>)
  Template version for setting arrays of uniforms,
  equivalent to
  \code
  WRATHglUniform(location, count, &v[0]);
  \endcode
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param v array of values
  \param count number of elements from the array v to use.
 */
template<typename T>
void
WRATHglUniform(int location, GLsizei count, const_c_array<T> v)
{
  if(!v.empty())
    {
      WRATHglUniform(location, count, &v[0]);
    }
}


/*!\fn void WRATHglUniform(int, GLsizei, const std::vector<T> &)
  Template version for setting arrays of uniforms,
  equivalent to
  \code
  WRATHglUniform(location, count, &v[0]);
  \endcode
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param v array of values
  \param count number of elements from the array v to use.
 */
template<typename T>
void
WRATHglUniform(int location, GLsizei count, const std::vector<T> &v)
{
  if(!v.empty())
    {
      WRATHglUniform(location, count, &v[0]);
    }
}


/*!\fn void WRATHglUniform(int, GLsizei, const_c_array<T>, bool)
  Template version for setting arrays of uniforms,
  equivalent to
  \code
  WRATHglUniform(location, count, &v[0], transposed);
  \endcode
  This call is for when v is an array of matrices.
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param v array of values
  \param count number of elements from the array v to use
  \param transposed flag to indicate if the matrices are to be transposed
 */
template<typename T>
void
WRATHglUniform(int location, GLsizei count, const_c_array<T> v, bool transposed)
{
  if(!v.empty())
    {
      WRATHglUniform(location, count, &v[0], transposed);
    }
}


/*!\fn void WRATHglUniform(int, GLsizei, const std::vector<T>&, bool)
  Template version for setting arrays of uniforms,
  equivalent to
  \code
  WRATHglUniform(location, count, &v[0], transposed);
  \endcode
  This call is for when v is an array of matrices.
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param v array of values
  \param count number of elements from the array v to use
  \param transposed flag to indicate if the matrices are to be transposed
 */
template<typename T>
void
WRATHglUniform(int location, GLsizei count, const std::vector<T> &v, bool transposed)
{
  if(!v.empty())
    {
      WRATHglUniform(location, count, &v[0], transposed);
    }
}



/*!\fn void WRATHglUniform(int, const_c_array<T>)
  Template version for setting arrays of uniforms,
  equivalent to
  \code
  WRATHglUniform(location, v.size(), &v[0]);
  \endcode
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param v array of values
 */
template<typename T>
void
WRATHglUniform(int location, const_c_array<T> v)
{
  if(!v.empty())
    {
      WRATHglUniform(location, v.size(), &v[0]);
    }
}


/*!\fn void WRATHglUniform(int, const std::vector<T>&)
  Template version for setting arrays of uniforms,
  equivalent to
  \code
  WRATHglUniform(location, v.size(), &v[0]);
  \endcode
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param v array of values
 */
template<typename T>
void
WRATHglUniform(int location, const std::vector<T> &v)
{
  if(!v.empty())
    {
      WRATHglUniform(location, v.size(), &v[0]);
    }
}

/*!\fn void WRATHglUniform(int, GLfloat)
  Equivalent to
  \code
  glUniform1f(location, v);
  \endcode
  i.e. set one float GLSL uniform.
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param v value to set the uniform as
 */
inline
void
WRATHglUniform(int location, GLfloat v)
{
  glUniform1f(location, v);
}

/*!\fn void WRATHglUniform(int, GLint)
  Equivalent to
  \code
  glUniform1i(location, v);
  \endcode
  i.e. set one int GLSL uniform.
  \param location location of uniform, i.e. return value
                  of glGetUniformLocation
  \param v value to set the uniform as
 */
inline
void
WRATHglUniform(int location, GLint v)
{
  glUniform1i(location, v);
}



/*! @} */





#endif
