/*! 
 * \file ngl_backend_sdl.cpp
 * \brief file ngl_backend_sdl.cpp
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
#include <SDL_video.h>
#include <sstream>

void*
ngl_loadFunction_default(const char *);

void*
ngl_loadFunction(const char *name)
{
  void *q;

  q=SDL_GL_GetProcAddress(name);
  if(q==NULL)
    {
      q=ngl_loadFunction_default(name);
    }

  return q;
}
