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


#if !defined(__WRATH_GL_UNIFORM_HPP__) || defined(__WRATH_GL_UNIFORM_IMPLEMENT_TCC__)
#error "Direction inclusion of private header file "__FILE__
#endif

#define __WRATH_GL_UNIFORM_IMPLEMENT_TCC__


/*
  Implement WRATHglUniform{1,2,3,4}v overloads to correct
  glUniform{1,2,3,4}{d,f,i,ui}v calls and provide overloads
  for vecN types too.

  GLFN one of {d,f,i,ui}
  TYPE GL type, such as GLfloat
  COUNT one of {1,2,3,4}
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

/*
  Use IMPLEMENT_WRATH_GL_UNIFORM_CNT to implement
  all for a given type. In addition array WRATHglUniform
  without vecN.

  GLFN one of {d,f,i,ui}
  TYPE GL type, such as GLfloat
  COUNT one of {1,2,3,4}
  
 */
#define IMPLEMENT_WRATH_GL_UNIFORM(GLFN, TYPE)				\
  IMPLEMENT_WRATH_GL_UNIFORM_CNT(GLFN, TYPE, 1)				\
  IMPLEMENT_WRATH_GL_UNIFORM_CNT(GLFN, TYPE, 2)				\
  IMPLEMENT_WRATH_GL_UNIFORM_CNT(GLFN, TYPE, 3)				\
  IMPLEMENT_WRATH_GL_UNIFORM_CNT(GLFN, TYPE, 4)				\
  inline void WRATHglUniform(int location, TYPE v)			\
  {									\
    glUniform1##GLFN(location, v);					\
  }									\
  inline void WRATHglUniform(int location, GLsizei count, const TYPE *v) \
  {									\
    WRATHglUniform1v(location, count, v);				\
  }


IMPLEMENT_WRATH_GL_UNIFORM(f, GLfloat)
IMPLEMENT_WRATH_GL_UNIFORM(i, GLint)

#if defined(WRATH_GL_VERSION) || WRATH_GLES_VERSION>=3
IMPLEMENT_WRATH_GL_UNIFORM(ui, GLuint)
IMPLEMENT_WRATH_GL_UNIFORM(d, GLdouble)
#endif


#undef IMPLEMENT_WRATH_GL_UNIFORM
#undef IMPLEMENT_WRATH_GL_UNIFORM_CNT

/*
  Implement square matrices uniform setting
 */
#define WRATH_GL_UNIFORM_MATRIX_SQUARE_IMPL(A) \
  inline void WRATHglUniform(int location, GLsizei count, const matrixNxM<A,A> *matrices, bool transposed=false) \
  {									\
    glUniformMatrix##A##fv(location, count, transposed?GL_TRUE:GL_FALSE, reinterpret_cast<const GLfloat*>(matrices)); \
  }									\
  inline void WRATHglUniform(int location, const matrixNxM<A,A> &matrix, bool transposed=false) \
  {									\
    WRATHglUniform(location, 1, &matrix, transposed);			\
  }
WRATH_GL_UNIFORM_MATRIX_SQUARE_IMPL(2)
WRATH_GL_UNIFORM_MATRIX_SQUARE_IMPL(3)
WRATH_GL_UNIFORM_MATRIX_SQUARE_IMPL(4)

#undef WRATH_GL_UNIFORM_MATRIX_SQUARE_IMPL


#if defined(WRATH_GL_VERSION) || WRATH_GLES_VERSION>=3

/*
  Implement non-square matrices uniform setting
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
