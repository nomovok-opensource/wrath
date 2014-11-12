/*! 
 * \file WRATHConfig.hpp
 * \brief file WRATHConfig.hpp
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

/*
 * This file must be the first header file in a translation unit that contains
 * any WRATH headers. Since all WRATH headers already make sure WRATHConfig.hpp
 * comes first, the users of WRATH only need to make sure any WRATH headers are
 * included first. The reason for this rule is GNU libstdc++ debugging mode that
 * changes standard container types, causing ABI changes to member functions of
 * classes that have standard container member variables on debug builds.
*/

#ifndef WRATH_HEADER_CONFIG_HPP_
#define WRATH_HEADER_CONFIG_HPP_

// Error out if standard headers already included
#if defined(_GLIBCXX_VECTOR) || defined(_GLIBCXX_LIST) || defined(_GLIBCXX_DEQUE) || defined(_GLIBCXX_STRING) || defined(_GLIBCXX_MAP) || defined(_GLIBCXX_SET)
#error Standard C++ headers included before WRATH headers
#error Make sure WRATH headers are included before any other headers
#endif

#if !defined(NDEBUG) && !defined(WRATHRELEASE)
  #define GL_DEBUG
  #define MUTEX_DEBUG
  #define WRATH_NEW_DEBUG
  #define WRATH_MALLOC_DEBUG
  #define WRATH_VECTOR_BOUND_CHECK
  #define WRATH_ASSERT_ACTIVE
  #define WRATHDEBUG
  #define _GLIBCXX_DEBUG
#endif


/*
  We include WRATHNew in WRAThConfig.hpp directly
  to make sure that in a WRATH application, the
  new operator is defined to the WRATH new operator  
 */
#include "WRATHNew.hpp"

/*
 if used, WRATH_USE_BOOST_LOCALE should be set in makefile
 since it also adds a link dependency
 #define WRATH_USE_BOOST_LOCALE
*/

/*
  the make system or place hear should define:
   - WRATH_GL_VERSION as 2 if building for OpenGL 2.x
   - WRATH_GL_VERSION as 3 if building for OpenGL 3.x
   - WRATH_GL_VERSION as 4 if building for OpenGL 4.x
   - WRATH_GLES_VERSION as 2 if building for OpenGL ES2
   - WRATH_GLES_VERSION as 3 if building for OpenGL ES3

  should we also make a symbol for minor GL version?
 */

#ifdef _WIN32
#include <math.h>
inline 
void 
sincosf(float angle, float *s, float *c)
{
  if(s!=NULL)
    {
      *s=sinf(angle);
    }

  if(c!=NULL)
    {
      *c=cosf(angle);
    }
}
#endif

#endif
