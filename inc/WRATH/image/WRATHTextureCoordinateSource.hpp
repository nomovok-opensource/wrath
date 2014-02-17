/*! 
 * \file WRATHTextureCoordinateSource.hpp
 * \brief file WRATHTextureCoordinateSource.hpp
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




#ifndef WRATH_HEADER_TEXTURE_COORDINATE_SOURCE_HPP_
#define WRATH_HEADER_TEXTURE_COORDINATE_SOURCE_HPP_


#include "WRATHConfig.hpp"
#include "WRATHImage.hpp"
#include "WRATHTextureCoordinateSourceBase.hpp"
#include "WRATHImage.hpp"

/*! \addtogroup Imaging
 * @{
 */

/*!\class WRATHTextureCoordinateSource

  A WRATHTextureCoordinateSource provides a simpler 
  interface to implement the conventions
  of WRATHTextureCoordinateSourceBase to compute
  a texture coordinate in GLSL.
  
  WRATHTextureCoordinateSource implements 
  add_shader_source_code_specify_interpolation_implementation()
  as follows:
  - If the interpolation behavior is a linear_computation, 
    it has the vertex shader absorb the GLSL code from \ref shader_code()
  - If the interpolation behavior is nonlinear_computation, 
    it has the vertex shader absorb the code from \ref 
    pre_compute_shader_code() and the fragment shader
    absorb the code from \ref shader_code()
  - If the interpolation behavior is fully_nonlinear_computation, 
    it has the vertex shader absorb the code from 
    \ref pre_compute_shader_code() and the fragment shader 
    absorb the code from \ref shader_code()
 */
class WRATHTextureCoordinateSource:public WRATHTextureCoordinateSourceBase
{
public:
  /*!\fn const WRATHGLShader::shader_source& shader_code
    To be implemented by a dervied class
    to return the GLSL source code
    for the the function:
    - prec float wrath_compute_texture_coordinate(in prec vec2 p)
   
    where prec is one of mediump, highp or 
    an empty string as indicated by \ref
    precision_t.

    \param prec the precision in which to perform the texture_coordinate
                 computation
    \param ibt indicates the interpolation behavior at which
               the computation is to take place. 
   */
  virtual
  const WRATHGLShader::shader_source& 
  shader_code(enum precision_t prec, enum interpolation_behaviour_t ibt) const=0;

  /*!\fn const WRATHGLShader::shader_source& pre_compute_shader_code
    To be optionally implemented by a 
    dervied classe to return the GLSL 
    source code for the the function:
    - void wrath_pre_compute_texture_coordinate(in prec vec2 p) when the 
      interpolation behavior is \ref nonlinear_computation
    - void wrath_pre_compute_texture_coordinate(void) when the 
      interpolation behavior is \ref fully_nonlinear_computation

    where prec is one of mediump, highp or 
    an empty string as indicated by \ref
    precision_t. This function is to
    be called from within the vertex shader
    for when the computation is to take place
    in the fragment shader (i.e. the interpolation
    behavior is \ref nonlinear_computation or
    \ref fully_nonlinear_computation). 
    The purpose of the function is to transmit
    any linear data values from the vertex
    shader stage to the fragment shader
    stage. The default implementation is
    to have a function that does nothing.

    \param prec the precision in which to perform the texture_coordinate
                 computation
    \param ibt indicates the interpolation behavior at which
               the computation is to take place. 
   */
  virtual
  const WRATHGLShader::shader_source&
  pre_compute_shader_code(enum precision_t prec, enum interpolation_behaviour_t ibt) const;

  /*!\fn const_c_array<std::string> global_scoped_symbols
    To be optionally implemented by a derived class
    to list all symbols of all shader stages that
    the shader source code defines that are at global
    scope.
    \param prec the precision in which to perform the texture_coordinate
                computation
    \param ibt indicates the interpolation behavior at which
               the computation is to take place. 
   */
  virtual
  const_c_array<std::string>
  global_scoped_symbols(enum precision_t prec, enum interpolation_behaviour_t ibt) const;

protected:

  void
  add_shader_source_code_specify_interpolation_implementation(enum interpolation_behaviour_t ibt,
                                                              std::map<GLenum, WRATHGLShader::shader_source> &src,
                                                              enum precision_t prec,
                                                              const std::string &suffix) const;


};
/*! @} */

#endif
