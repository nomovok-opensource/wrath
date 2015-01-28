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
#include "WRATHStaticInit.hpp"
#include "ngl_backend_lib.hpp"
#include "ngl_backend_sdl.hpp"
#include <SDL_video.h>
#include <sstream>


namespace
{
  class Loader:boost::noncopyable
  {
  public:
    Loader(void):
      m_libName(NGLBackendLib::default_gl_library()),
      m_p(NULL)
    {}

    ~Loader()
    {
      if(m_p!=NULL)
        {
          WRATHDelete(m_p);
        }
    }

    void
    set_name(const std::string &pname)
    {
      WRATHassert(m_p==NULL);
      m_libName=pname;
    }

    void*
    get_function(const char *f)
    {
      if(m_p==NULL)
        {
          m_p=WRATHNew NGLBackendLibEGL(m_libName);
          SDL_GL_LoadLibrary(m_libName.c_str());
        }
      void *R(NULL);

      R=SDL_GL_GetProcAddress(f);
      if(R==NULL)
        {
          R=m_p->load_function(f);
        }

      return R;
    }

  private:
    std::string m_libName;
    NGLBackendLibEGL *m_p;
  };

  Loader&
  get_loader(void)
  {
    WRATHStaticInit();
    static Loader L;
    return L;
  }
  
}


void
ngl_set_GL_librarySDL(const std::string &pname)
{
  get_loader().set_name(pname);
}

void*
ngl_loadFunction(const char *name)
{
  return get_loader().get_function(name);
}
