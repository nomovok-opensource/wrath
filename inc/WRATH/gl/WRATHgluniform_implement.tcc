// -*- C++ -*-

/*! 
 * \file WRATHgluniform_implement.tcc
 * \brief file WRATHgluniform_implement.tcc
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


#if !defined(WRATH_HEADER_GL_UNIFORM_HPP_) || defined(WRATH_HEADER_GL_UNIFORM_IMPLEMENT_TCC_)
#error "Direction inclusion of private header file "__FILE__
#endif

#define WRATH_HEADER_GL_UNIFORM_IMPLEMENT_TCC_

#if defined(WRATH_GL_VERSION) || WRATH_GLES_VERSION>=3
#define WRATH_GL_SUPPORT_NON_SQUARE_MATRICES
#define WRATH_GL_SUPPORT_UINT_UNIFORMS
#endif

#if defined(WRATH_GL_VERSION)
#define WRATH_GL_SUPPORT_DOUBLE_UNIFORMS
#endif

/*
  Implement WRATHglUniform{1,2,3,4}v overloads to correct
  glUniform{1,2,3,4}{d,f,i,ui}v calls and provide overloads
  for vecN types too.

  GLFN one of {d,f,i,ui}
  TYPE GL type, such as GLfloat
  COUNT one of {1,2,3,4}
 */
#define WRATH_GL_UNIFORM_IMPL_CNT(GLFN, TYPE, COUNT)            \
  inline void WRATHglUniform##COUNT##v(int location, GLsizei count, const TYPE *v) \
  {                                                                     \
    glUniform##COUNT##GLFN##v(location, count, v);                      \
  }                                                                     \
  inline void WRATHglUniform(int location, const vecN<TYPE, COUNT> &v)  \
  {                                                                     \
    WRATHglUniform##COUNT##v(location, 1, &v[0]);                       \
  }                                                                     \
  inline void WRATHglUniform(int location, GLsizei count, const vecN<TYPE, COUNT> *v) \
  {                                                                     \
    WRATHglUniform##COUNT##v(location, count, reinterpret_cast<const TYPE*>(v)); \
  }

/*
  Use WRATH_GL_UNIFORM_IMPL_CNT to implement
  all for a given type. In addition array WRATHglUniform
  without vecN.

  GLFN one of {d,f,i,ui}
  TYPE GL type, such as GLfloat
 */
#define WRATH_GL_UNIFORM_IMPL(GLFN, TYPE)                               \
  WRATH_GL_UNIFORM_IMPL_CNT(GLFN, TYPE, 1)                              \
  WRATH_GL_UNIFORM_IMPL_CNT(GLFN, TYPE, 2)                              \
  WRATH_GL_UNIFORM_IMPL_CNT(GLFN, TYPE, 3)                              \
  WRATH_GL_UNIFORM_IMPL_CNT(GLFN, TYPE, 4)                              \
  inline void WRATHglUniform(int location, TYPE v)                      \
  {                                                                     \
    glUniform1##GLFN(location, v);                                      \
  }                                                                     \
  inline void WRATHglUniform(int location, GLsizei count, const TYPE *v) \
  {                                                                     \
    WRATHglUniform1v(location, count, v);                               \
  }

/*
  Implement square matrices uniform setting
  A: dimension of matrix, one of  {2,3,4}
  GLFN: one of {f,d}
  TYPE: one of {GLfloat, GLdouble}
*/
#define WRATH_GL_UNIFORM_SQUARE_MATRIX_IMPL_DIM(GLFN, TYPE, A)          \
  inline void WRATHglUniform(int location, GLsizei count, const matrixNxM<A,A,TYPE> *matrices, bool transposed=false) \
  {                                                                     \
    glUniformMatrix##A##GLFN##v(location, count, transposed?GL_TRUE:GL_FALSE, reinterpret_cast<const TYPE*>(matrices)); \
  }                                                                     \
  inline void WRATHglUniform(int location, const matrixNxM<A,A,TYPE> &matrix, bool transposed=false) \
  {                                                                     \
    WRATHglUniform(location, 1, &matrix, transposed);                   \
  }


#ifdef WRATH_GL_SUPPORT_NON_SQUARE_MATRICES
  /*
    Implement non-square matrices uniform setting
    A: height of matrix, one of  {2,3,4}
    B: width of matrix, one of  {2,3,4}
    GLFN: one of {f,d}
    TYPE: one of {GLfloat, GLdouble}
 */
#define WRATH_GL_UNIFORM_NON_SQUARE_MATRIX_IMPL_DIM(GLFN, TYPE, A,B)    \
  inline void WRATHglUniform(int location, GLsizei count, const matrixNxM<A,B,TYPE> *matrices, bool transposed=false) \
  {                                                                     \
    glUniformMatrix##A##x##B##GLFN##v(location, count, transposed?GL_TRUE:GL_FALSE, reinterpret_cast<const TYPE*>(matrices)); \
  }                                                                     \
  inline void WRATHglUniform(int location, const matrixNxM<A,B,TYPE> &matrix, bool transposed=false) \
  {                                                                     \
    WRATHglUniform(location, 1, &matrix, transposed);                   \
  }


#else
  #define WRATH_GL_UNIFORM_NON_SQUARE_MATRIX_IMPL_DIM(GLFN, TYPE, A, B) 
#endif



/*
  Implement square matrices uniform setting
  GLFN: one of {f,d}
  TYPE: one of {GLfloat, GLdouble}
*/
#define WRATH_GL_UNIFORM_SQUARE_MATRIX_IMPL(GLFN, TYPE)  \
  WRATH_GL_UNIFORM_SQUARE_MATRIX_IMPL_DIM(GLFN, TYPE, 2) \
  WRATH_GL_UNIFORM_SQUARE_MATRIX_IMPL_DIM(GLFN, TYPE, 3) \
  WRATH_GL_UNIFORM_SQUARE_MATRIX_IMPL_DIM(GLFN, TYPE, 4) 


/*
  Implement non-square matrices uniform setting
  GLFN: one of {f,d}
  TYPE: one of {GLfloat, GLdouble}
*/
#define WRATH_GL_UNIFORM_NON_SQUARE_MATRIX_IMPL(GLFN, TYPE)             \
  WRATH_GL_UNIFORM_NON_SQUARE_MATRIX_IMPL_DIM(GLFN,TYPE,2,3)            \
  WRATH_GL_UNIFORM_NON_SQUARE_MATRIX_IMPL_DIM(GLFN,TYPE,2,4)            \
  WRATH_GL_UNIFORM_NON_SQUARE_MATRIX_IMPL_DIM(GLFN,TYPE,3,2)            \
  WRATH_GL_UNIFORM_NON_SQUARE_MATRIX_IMPL_DIM(GLFN,TYPE,3,4)            \
  WRATH_GL_UNIFORM_NON_SQUARE_MATRIX_IMPL_DIM(GLFN,TYPE,4,2)            \
  WRATH_GL_UNIFORM_NON_SQUARE_MATRIX_IMPL_DIM(GLFN,TYPE,4,3)            
  

/*
  Implement all matrix uniform setting
  GLFN: one of {f,d}
  TYPE: one of {GLfloat, GLdouble}
*/
#define WRATH_GL_UNIFORM_MATRIX_IMPL(GLFN, TYPE)        \
  WRATH_GL_UNIFORM_SQUARE_MATRIX_IMPL(GLFN, TYPE)       \
  WRATH_GL_UNIFORM_NON_SQUARE_MATRIX_IMPL(GLFN, TYPE)   


#ifdef WRATH_GL_SUPPORT_UINT_UNIFORMS
  WRATH_GL_UNIFORM_IMPL(ui, GLuint)
#endif

#ifdef WRATH_GL_SUPPORT_DOUBLE_UNIFORMS
  WRATH_GL_UNIFORM_IMPL(d, GLdouble)
  WRATH_GL_UNIFORM_MATRIX_IMPL(d, GLdouble)
#endif


WRATH_GL_UNIFORM_IMPL(f, GLfloat)
WRATH_GL_UNIFORM_IMPL(i, GLint)
WRATH_GL_UNIFORM_MATRIX_IMPL(f, GLfloat)

#undef WRATH_GL_UNIFORM_IMPL
#undef WRATH_GL_UNIFORM_IMPL_CNT
#undef WRATH_GL_UNIFORM_MATRIX_IMPL
#undef WRATH_GL_UNIFORM_NON_SQUARE_MATRIX_IMPL
#undef WRATH_GL_UNIFORM_NON_SQUARE_MATRIX_IMPL_DIM
#undef WRATH_GL_UNIFORM_SQUARE_MATRIX_IMPL
#undef WRATH_GL_UNIFORM_SQUARE_MATRIX_IMPL_DIM

