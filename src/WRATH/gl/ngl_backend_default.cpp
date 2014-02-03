/*! 
 * \file ngl_backend_default.cpp
 * \brief file ngl_backend_default.cpp
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

#include <dlfcn.h>
#include "WRATHassert.hpp" 
#include "WRATHStaticInit.hpp"
#include "vecN.hpp"

#if defined(WRATH_GLES_VERSION)
#include <EGL/egl.h>
#endif

namespace
{
  class libGL_handle
  {
  public:
    libGL_handle(void)
    {
      m_handle[0]=dlopen(NULL,  RTLD_LAZY);

      #ifdef WRATH_GL_VERSION
      {
        m_handle[1]=dlopen("libGL.so", RTLD_LAZY);
      }
      #else
      {
        m_handle[1]=dlopen("libGLESv2.so", RTLD_LAZY);
      }
      #endif      
    }

    ~libGL_handle()
    {
      if(m_handle[0]!=NULL)
        {
          dlclose(m_handle[0]);
        }

      if(m_handle[1]!=NULL)
        {
          dlclose(m_handle[1]);
        }
    }

    void*
    get_function(const char *function_name)
    {
      void *R(NULL);

      if(m_handle[0]!=NULL)
        {
          R=dlsym(m_handle[0], function_name);
        }

      if(R==NULL and m_handle[1]!=NULL)
        {
          R=dlsym(m_handle[1], function_name);
        }

      return R;
    }

  private:
    vecN<void*, 2> m_handle;
  };

}


/*
  TODO: move this function to somewhere _else_ that
  takes into account what the underlying lib's used
  for creating a windowand GL context.
 */
void*
ngl_loadFunction_default(const char *name)
{
  WRATHStaticInit();
  static libGL_handle R;
  void *return_value(NULL);

  #if !defined(WRATH_GL_VERSION)
  {
    return_value=(void*)eglGetProcAddress(name);
  }
  #endif

  /*
    EGL spec is irritating. Only those functions that extension
    functions will eglGetProcAddress() return. As such we will
    need to rely on dlopen/dlsym to get functions those functions
    that are not extensions (i.e. part of the GLES2 specification)
   */
  if(return_value==NULL)
    {
      return_value=R.get_function(name);
    }

  return return_value;
}
