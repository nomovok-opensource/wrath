/*! 
 * \file ngl_backend_lib.hpp
 * \brief file ngl_backend_lib.hpp
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



#ifndef WRATH_NGL_BACKEND_LIB_HPP_
#define WRATH_NGL_BACKEND_LIB_HPP_

#include "WRATHConfig.hpp"
#include <string>
#include <boost/utility.hpp>
#include "vecN.hpp"


/*!\class NGLBackendLib
  NGLBackendLib is a conveniance class to load
  a function. It first checks the main program
  for the symbol, and if not found there, checks
  the library specified in the ctor.
 */
class NGLBackendLib:boost::noncopyable
{
public:
  /*!\fn NGLBackendLib(const std::string &)
    Ctor.
    \param libName library name on which to check function symbols
                   when unable to find a function symbol from 
                   the main program
   */
  explicit
  NGLBackendLib(const std::string &libName);

  ~NGLBackendLib();

  /*!\fn load_function(const char *)
    Load a function, first checking the main program
    for the symbol, then checking the libary (specified)
    in the ctor for the symbol. Returns the function
    pointer for the function.
    \param function_name name of function to load and return
   */
  void*
  load_function(const char *function_name);

  /*!\fn default_gl_library(void)
    Returns the name of the default name for the 
    GL library:
     - Windows it is opengl32.dll
     - GL Unix: libGL.so
     - GLES unix: libGLESv2.so
   */
  static 
  std::string
  default_gl_library(void);

private:
    vecN<void*, 2> m_handle;
};


/*!\class NGLBackendLibEGL
  NGLBackendLibEGL inherits from NGLBackendLib
  but first tries using eglGetProcAddress(),
  if not found via eglGetProcAddress(), then
  uses \ref NGLBackendLib::load_function(const char *name)
 */
class NGLBackendLibEGL:boost::noncopyable
{
public:
  /*!\fn NGLBackendLib(const std::string &)
    Ctor.
    \param libName library name on which to check function symbols
                   when unable to find a function symbol from 
                   the main program or eglGetProcAddress().
   */
  explicit
  NGLBackendLibEGL(const std::string &libName=NGLBackendLib::default_gl_library()):
    m_ngl(libName)
  {}

  ~NGLBackendLibEGL() {}

  /*!\fn load_function(const char *)
    Load a function, first checking with eglGetProcAddress(),
    then the main program for the symbol, then checking 
    the libary (specified) in the ctor for the symbol. 
    Returns the function pointer for the function.
    \param function_name name of function to load and return
   */
  void*
  load_function(const char *function_name);

private:
  NGLBackendLib m_ngl;
};

#endif
