#ifndef WRATH_NGL_BACKEND_SDL_HPP_
#define WRATH_NGL_BACKEND_SDL_HPP_

#include "WRATHConfig.hpp"
#include <string>

/*!\fn ngl_set_GL_librarySDL
  For the implementation of ngl_loadFunction
  backed by SDL: set the name of the GL library
  to use. The default for GL is libGL.so
  and the default value for GLES is libGLESv2.so.
  Fucntion may not be called once any GL function 
  is called.

  \param pname name of library from which to fetch GL/GLES function
 */
void
ngl_set_GL_librarySDL(const std::string &pname);

#endif
