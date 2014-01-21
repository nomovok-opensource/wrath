/*! 
 * \file WRATHGPUConfig.cpp
 * \brief file WRATHGPUConfig.cpp
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


#include "WRATHGPUConfig.hpp"

/*
  TODO: some macro and or query GL magic to make this 
  file auto-magic on different platforms
 */

bool
WRATHGPUConfig::
fragment_shader_poor_branching(void)
{
  /*
    Making this true might hurt performance,
    but a number of GPU's seem to require this:
     - N9
     - Tegra (Beta drivers)
   */
  #ifdef HARMATTAN
  {
    return true;
  }
  #endif

  return false;
}

bool
WRATHGPUConfig::
fragment_shader_texture_LOD_supported(void)
{
  /*
    Some devices support texture*Lod functions
    in fragment shader:
     - All GL3 capable desktop GPU's
     - SGX (most anyways)
   */
  return true;
}

bool
WRATHGPUConfig::
dependent_texture_lookup_requires_LOD(void)
{
  /*
    Some GPU's have serious issues with dependent
    texture lookup even when texture filtering
    (both minification and magnification) are
    set to GL_NEAREST. Currently, only
    ATI GPU's (desktop) require this to be true
   */
  return false;
}


const char*
WRATHGPUConfig::
default_shader_version(void)
{
  #if defined(WRATH_GLES_VERSION) && WRATH_GLES_VERSION>=3
    return "300 es";
  #elif defined(WRATH_GL_VERSION) && WRATH_GL_VERSION>=3
    return "330 core";
  #endif

  return "";
}

bool
WRATHGPUConfig::
use_in_out_in_shaders(void)
{
  /*
    NVIDIA's GLSL compiler does not appreciate
    using in and out unless the the GLSL
    version has been specified sufficiently high.
   */
  #if defined(WRATH_GLES_VERSION) && WRATH_GLES_VERSION>=3
    return true;
  #elif defined(WRATH_GL_VERSION) && WRATH_GL_VERSION>=3
    return true;
  #endif

  return false;
}


GLenum
WRATHGPUConfig::
gl_max_texture_level(void)
{
  /*
    Some GPU's support specifying the max
    mipmap LOD via glTexParameter. 
    - all desktop GL's
    - any GLES2 implementation with the extension GL_APPLE_texture_max_level
    - Tegra seems to support this as well
   */
#if defined(WRATH_GLES_VERSION) && WRATH_GLES_VERSION==2
  return GL_INVALID_ENUM;
#else
  return GL_TEXTURE_MAX_LEVEL;
#endif
}

bool
WRATHGPUConfig::
unextended_shader_support_derivatives(void)
{

#if defined(WRATH_GLES_VERSION) && WRATH_GLES_VERSION==2
  return false;
#else
  return true;
#endif  
}

bool
WRATHGPUConfig::
old_glsl_texture_functions_deprecated(void)
{
  
  #if defined(WRATH_GLES_VERSION) && WRATH_GLES_VERSION>=3
    return true;
  #elif defined(WRATH_GL_VERSION) && WRATH_GL_VERSION>=3
    return true;
  #endif

  return false;
}
