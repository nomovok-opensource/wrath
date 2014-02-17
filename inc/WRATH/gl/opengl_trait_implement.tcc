// -*- C++ -*-

/*! 
 * \file opengl_trait_implement.tcc
 * \brief file opengl_trait_implement.tcc
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


#if !defined(WRATH_OPENGL_TRAIT_H_) || defined(WRATH_OPENGL_TRAIT_IMPLEMENT_TCC_)
#error "Direction inclusion of private header file opengl_trait_implement.tcc" 
#endif

#define WRATH_OPENGL_TRAIT_IMPLEMENT_TCC_


template<>
struct opengl_trait<GLbyte>
{
  enum { type=GL_BYTE };
  enum { count=1 }; 
  enum { stride=sizeof(signed char) };
  typedef GLbyte basic_type; 
  typedef basic_type data_type;
};

template<>
struct opengl_trait<GLubyte>
{
  enum { type=GL_UNSIGNED_BYTE };
  enum { count=1 }; 
  enum { stride=sizeof(char) };
  typedef GLubyte basic_type; 
  typedef basic_type data_type;
};

template<>
struct opengl_trait<GLshort>
{
  enum { type=GL_SHORT };
  enum { count=1 }; 
  enum { stride=sizeof(short) };
  typedef GLshort basic_type; 
  typedef basic_type data_type;
};

template<>
struct opengl_trait<GLushort>
{
  enum { type=GL_UNSIGNED_SHORT };
  enum { count=1 }; 
  enum { stride=sizeof(unsigned short) };
  typedef GLushort basic_type; 
  typedef basic_type data_type;
};

template<>
struct opengl_trait<GLint>
{
  enum { type=GL_INT };
  enum { count=1 }; 
  enum { stride=sizeof(int) };
  typedef GLint basic_type; 
  typedef basic_type data_type;
};

template<>
struct opengl_trait<GLuint>
{
  enum { type=GL_UNSIGNED_INT };
  enum { count=1 }; 
  enum { stride=sizeof(unsigned int) };
  typedef GLuint basic_type; 
  typedef basic_type data_type;
};

template<>
struct opengl_trait<GLfloat>
{
  enum { type=GL_FLOAT };
  enum { count=1 }; 
  enum { stride=sizeof(float) };
  typedef GLfloat basic_type; 
  typedef basic_type data_type;
};

template<typename T, unsigned int N>
struct opengl_trait< vecN<T,N> >
{
  enum { type=opengl_trait<T>::type };
  enum { count=N*opengl_trait<T>::count }; 
  enum { stride=sizeof(vecN<T,N>) };

  typedef typename opengl_trait<T>::basic_type basic_type; 
  typedef vecN<T,N> data_type;
};
