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

#define IMPLEMENT_WRATH_GL_UNIFORM_CNT(GLFN, TYPE, COUNT)		\
  inline void WRATHglUniform##COUNT##v(int location, GLsizei count, const TYPE *v) \
  {									\
    glUniform##COUNT##GLFN##v(location, count, v);			\
  }									\
  inline void WRATHglUniform(int location, const vecN<TYPE, COUNT> &v)	\
  {									\
    WRATHglUniform##COUNT##v(location, 1, &v[0]);			\
  }									\
  inline void WRATHglUniform(int location, GLsizei count, const vecN<TYPE, COUNT> *v) \
  {									\
    WRATHglUniform##COUNT##v(location, count, reinterpret_cast<const TYPE*>(v)); \
  }

#define IMPLEMENT_WRATH_GL_UNIFORM(GLFN, TYPE)				\
  IMPLEMENT_WRATH_GL_UNIFORM_CNT(GLFN, TYPE, 1)				\
  IMPLEMENT_WRATH_GL_UNIFORM_CNT(GLFN, TYPE, 2)				\
  IMPLEMENT_WRATH_GL_UNIFORM_CNT(GLFN, TYPE, 3)				\
  IMPLEMENT_WRATH_GL_UNIFORM_CNT(GLFN, TYPE, 4)				\
  inline void WRATHglUniform(int location, TYPE v)			\
  {									\
    glUniform1##GLFN(location, v);					\
  }


IMPLEMENT_WRATH_GL_UNIFORM(f, GLfloat)
IMPLEMENT_WRATH_GL_UNIFORM(i, GLint)

#if defined(WRATH_GL_VERSION) || WRATH_GLES_VERSION>=3
IMPLEMENT_WRATH_GL_UNIFORM(ui, GLuint)
IMPLEMENT_WRATH_GL_UNIFORM(d, GLdouble)
#endif


#undef IMPLEMENT_WRATH_GL_UNIFORM
#undef IMPLEMENT_WRATH_GL_UNIFORM_CNT



/*!\fn void WRATHglUniform(int, GLsizei, const matrixNxM<2,2>*, bool)
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
WRATHglUniform(int location, GLsizei count, const matrixNxM<2,2> *matrices, bool transposed=false)
{
  glUniformMatrix2fv(location,
                     count,
                     transposed?GL_TRUE:GL_FALSE,
                     reinterpret_cast<const GLfloat*>(matrices));
}

/*!\fn void WRATHglUniform(int, const matrixNxM<2,2>&, bool)
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
WRATHglUniform(int location, const matrixNxM<2,2> &matrices, bool transposed=false)
{
  glUniformMatrix2fv(location,
                     1,
                     transposed?GL_TRUE:GL_FALSE,
                     reinterpret_cast<const GLfloat*>(&matrices));
}

/*!\fn void WRATHglUniform(int, GLsizei, const matrixNxM<3,3>*, bool)
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
WRATHglUniform(int location, GLsizei count, const matrixNxM<3,3> *matrices, bool transposed=false)
{
  glUniformMatrix3fv(location,
                     count,
                     transposed?GL_TRUE:GL_FALSE,
                     reinterpret_cast<const GLfloat*>(matrices));
}

/*!\fn void WRATHglUniform(int, const matrixNxM<3,3>&, bool)
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
WRATHglUniform(int location, const matrixNxM<3,3> &matrices, bool transposed=false)
{
  glUniformMatrix3fv(location,
                     1,
                     transposed?GL_TRUE:GL_FALSE,
                     reinterpret_cast<const GLfloat*>(&matrices));
}

/*!\fn void WRATHglUniform(int, GLsizei, const matrixNxM<4,4>*, bool)
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
WRATHglUniform(int location, GLsizei count, const matrixNxM<4,4> *matrices, bool transposed=false)
{
  glUniformMatrix4fv(location,
                     count,
                     transposed?GL_TRUE:GL_FALSE,
                     reinterpret_cast<const GLfloat*>(matrices));
}

/*!\fn void WRATHglUniform(int, const matrixNxM<4,4>&, bool)
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
WRATHglUniform(int location, const matrixNxM<4,4> &matrices, bool transposed=false)
{
  glUniformMatrix4fv(location,
                     1,
                     transposed?GL_TRUE:GL_FALSE,
                     reinterpret_cast<const GLfloat*>(&matrices));
}

#if defined(WRATH_GL_VERSION) || WRATH_GLES_VERSION>=3

/*
  support for non-square matrices, via macro.
 */
#define WRATH_GL_UNIFORM_MATRIX_IMPL(A,B) \
  inline void WRATHglUniform(int location, GLsizei count, const matrixNxM<A,B> *matrices, bool transposed=false) \
  {									\
    glUniformMatrix##A##x##B##fv(location, count, transposed?GL_TRUE:GL_FALSE, reinterpret_cast<const GLfloat*>(matrices)); \
  }									\
  inline void WRATHglUniform(int location, const matrixNxM<A,B> &matrix, bool transposed=false) \
  {									\
    WRATHglUniform(location, 1, &matrix, transposed);			\
  }

WRATH_GL_UNIFORM_MATRIX_IMPL(2,3)
WRATH_GL_UNIFORM_MATRIX_IMPL(2,4)
WRATH_GL_UNIFORM_MATRIX_IMPL(3,2)
WRATH_GL_UNIFORM_MATRIX_IMPL(3,4)
WRATH_GL_UNIFORM_MATRIX_IMPL(4,2)
WRATH_GL_UNIFORM_MATRIX_IMPL(4,3)

#undef WRATH_GL_UNIFORM_MATRIX_IMPL

#endif


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




/*! @} */





#endif
