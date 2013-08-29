/*! 
 * \file WRATHGLExtensionList.cpp
 * \brief file WRATHGLExtensionList.cpp
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


#include "WRATHConfig.hpp"
#include <iostream>
#include <sstream>
#include "WRATHGLExtensionList.hpp"
#include "WRATHgl.hpp"


WRATHGLExtensionList::
WRATHGLExtensionList(void)
{
  const char *str;

  /*
    istringstream wants a const char*, but
    glGetString returns a const GLubyte*
    which is const unsigned char* (usually).
    Sighs.
   */
  str=reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
  std::istringstream istr(str);


  while(!istr)
    {
      std::string v;
      istr >> v;
      if(!istr.fail())
        {
          m_extensions.insert(v);
        }
    }
}
