/*! 
 * \file WRATHGradientSource.hpp
 * \brief file WRATHGradientSource.hpp
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




#ifndef __WRATH_GRADIENT_SOURCE_HPP__
#define __WRATH_GRADIENT_SOURCE_HPP__


#include "WRATHConfig.hpp"
#include "WRATHImage.hpp"
#include "WRATHGradientSourceBase.hpp"
#include "WRATHGradient.hpp"

/*! \addtogroup Imaging
 * @{
 */

/*!\class WRATHGradientSource
  A WRATHGradientSource provides a simpler 
  interface to implement the conventions
  of WRATHGradientSourceBase to compute
  a gradient interpolate in GLSL.
  
  WRATHGradientSource implements 
  add_shader_source_code_specify_interpolation_implementation()
  as by essentially added the correct shader state based upon
  the interpolation behavior, i.e.
  - If the interpolation behavior is a \ref WRATHGradientSourceBase::linear_computation, 
    it has the vertex shader absorb the GLSL code from 
    \ref shader_code()
  - If the interpolation behavior is \ref WRATHGradientSourceBase::nonlinear_computation, 
    it has the vertex shader absorb the code from
    \ref pre_compute_shader_code() and the fragment shader
    abosrb the code from \ref shader_code()
  - If the interpolation behavior is \ref WRATHGradientSourceBase::fully_nonlinear_computation, 
    it has the vertex shader absorb the code from \ref pre_compute_shader_code() 
    and the fragment shader abosrb the code from \ref shader_code()
 */
class WRATHGradientSource:public WRATHGradientSourceBase
{
public:

  /*!\fn const WRATHGLShader::shader_source& shader_code
    To be implemented by a dervied class
    to return the GLSL source code
    for the the function:
    - prec return_type wrath_compute_gradient(in prec vec2 p)
   
    where prec is one of mediump, highp or 
    an empty string as indicated by \ref
    precision_t and return_type is float
    if ibt is \ref WRATHBaseSource::linear_computation
    and vec2 otherwise
    \param prec the precision in which to perform the gradient
                interpolate computation
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
    - void wrath_pre_compute_gradient(in prec vec2 p) when the 
      interpolation behavior is \ref nonlinear_computation
    - void wrath_pre_compute_gradient(void) when the 
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
    \param prec the precision in which to perform the gradient
                interpolate computation
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
    \param prec the precision in which to perform the gradient
                interpolate computation
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
