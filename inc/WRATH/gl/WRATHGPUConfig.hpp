/*! 
 * \file WRATHGPUConfig.hpp
 * \brief file WRATHGPUConfig.hpp
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


#ifndef WRATH_HEADER_GPU_CONFIG_HPP_
#define WRATH_HEADER_GPU_CONFIG_HPP_

#include "WRATHConfig.hpp"
#include "WRATHgl.hpp"

/*!\namespace WRATHGPUConfig
  Namespace to encapusulate information used for
  implementing work arounds in various GL implementation
  bugs or shortcomings.
 */
namespace WRATHGPUConfig
{

  /*!\fn bool fragment_shader_poor_branching
    Returns true if the the target GPU does not support
    or poorly supports branching in the Fragment shader
    [PowerVR SGX for example]
  */
  bool
  fragment_shader_poor_branching(void);

  /*!\fn bool fragment_shader_texture_LOD_supported
    Returns true if the target GPU supports texture2dLod
    from withn the fragment shader [vertex texture fetch
    support can be queried by looking at the value of
    GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS
   */
  bool
  fragment_shader_texture_LOD_supported(void);

  /*!\fn bool dependent_texture_lookup_requires_LOD
    Returns true if the GPU exhibits incorrect 
    dependent texture lookup that is resolved
    by using texture-Lod functions in GLSL
    (i.e. texture2DLod in place of texture2D).
   */
  bool
  dependent_texture_lookup_requires_LOD(void);


  /*!\fn GLenum gl_max_texture_level
    Returns a GLenum value on those GLES2 platforms 
    to be fed to glTexParameter to specify the 
    max mipmap level GL will use when using a
    texture. For GL on desktop this is supported
    always and returns the value GL_TEXTURE_MAX_LEVEL.
    For platforms that support GL_APPLE_texture_max_level,
    returns GL_TEXTURE_MAX_LEVEL_APPLE, another example is 
    NVIDIA's gl2ext_nv.h which has it return the 
    symbol value GL_TEXTURE_MAX_LEVEL_SGIS. Comment:
    the actual value of GL_TEXTURE_MAX_LEVEL,
    GL_TEXTURE_MAX_LEVEL_APPLE and GL_TEXTURE_MAX_LEVEL_SGIS
    are all the same value.
    For platforms that do no support specifying the 
    max mipmap level of a texture, returns GL_INVALID_ENUM.
  */
  GLenum
  gl_max_texture_level(void);

  /*!\fn const char* default_shader_version
    Returns the default value to use for 
    \ref WRATHGLShader::shader_source::m_version
   */
  const char*
  default_shader_version(void);

  /*!\fn bool use_in_out_in_shaders
    Returns true if the GL implementation supports
    using the key words in and out for values between
    shaders inplace of varying.
   */
  bool
  use_in_out_in_shaders(void);

  /*!\fn bool unextended_shader_support_derivatives
    Returns true if and only if the GLSL implementation
    supports the derivative functions
    - dFdx()
    - dFdy() 
    - fwidth()

    in the fragment shaders in the targetted GL
    when GL is unextended. This returns true for
    desktop GL and GLES3. However, GLES2 does not
    support these functions unless the extension
    GL_OES_standard_derivatives is present. As such,
    for GLES2 always returns false.
   */
  bool
  unextended_shader_support_derivatives(void);

  /*!\fn old_glsl_texture_functions_deprecated
    Returns true if the old style texture lookup functions
    are deprecated. As an example, GL3.3 core profile deprecates
    texture1D(), texture2D(), texture3D() and in their place, use
    the same named function texture().
   */
  bool
  old_glsl_texture_functions_deprecated(void);
  
}




#endif
