#include "WRATHConfig.hpp"
#include "ngl_backend_lib.hpp"
#include <dlfcn.h>

#if defined(WRATH_GLES_VERISON) || defined(WRATH_USE_EGL)
#include <egl.h>
#endif



#ifdef _WIN32
  #define DefaultOpenGLLibrary "opengl32.dll"
#else
  #ifdef WRATH_GL_VERSION
    #define DefaultOpenGLLibrary "libGL.so"
  #else
    #define DefaultOpenGLLibrary "libGLESv2.so"
  #endif
#endif


/////////////////////////////////////////////
// NGLBackendLib
NGLBackendLib::
NGLBackendLib(const std::string &libName)
{
  m_handle[0]=dlopen(NULL,  RTLD_LAZY);
  m_handle[1]=dlopen(libName.c_str(), RTLD_LAZY);
}

NGLBackendLib::
~NGLBackendLib()
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
NGLBackendLib::
load_function(const char *function_name)
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

std::string
NGLBackendLib::
default_gl_library(void)
{
  return DefaultOpenGLLibrary;
}

///////////////////////////////
// NGLBackendLibEGL
void*
NGLBackendLibEGL::
load_function(const char *function_name)
{
  void *R(NULL);

  #if defined(WRATH_GLES_VERISON) || defined(WRATH_USE_EGL)
  {
    R = eglGetProcAddress(function_name);
  }
  #endif

  if(R==NULL)
    {
      R = m_ngl.load_function(function_name);
    }

  return R;
    
}
