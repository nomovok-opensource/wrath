/*! 
 * \file WRATHGradientSourceBase.hpp
 * \brief file WRATHGradientSourceBase.hpp
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




#ifndef WRATH_HEADER_GRADIENT_SOURCE_BASE_HPP_
#define WRATH_HEADER_GRADIENT_SOURCE_BASE_HPP_


#include "WRATHConfig.hpp"
#include "WRATHImage.hpp"
#include "WRATHBaseSource.hpp"
#include "WRATHGradient.hpp"

namespace WRATHGradientSourceBasePrivate
{
  class NonLinearFacade;
}

/*! \addtogroup Imaging
 * @{
 */

/*!\class WRATHGradientSourceBase
  A WRATHGradientSourceBase represents how to
  compute the interpolate value to feed
  to the texture look up for \ref WRATHGradient.
  The class WRATHGradientSourceBase defines
  an interface an convention for computing a gradient
  interpolate value as follows:

  - A gradient interpolate may be computed entirely
    linearly, partially linearly and fully non-linearly.
    This is controlled by the enumeration \ref
    interpolation_behaviour_t

  - A gradient interpolate is implemented in GLSL by
    implementing 2 functions: <B>wrath_pre_compute_gradient()</B>
    and <B>wrath_compute_gradient()</B>. 

  The interpolation behavior can be linear, partially-nonlinear
  and fully non-linear. The caller requests the nature
  of the interpolation behavior. An implementation can
  promote the interpolation behavior, where the ordering is
  linear < partially-nonlinear < fully nonlinear,
  for example if partially-nonlinear is requested,
  then the implementation may choose to implement as 
  partially-nonlinear or fully nonlinear.

  A class derived from WRATHGradientSourceBase must
  abide by the following conventions:
  - When the interpolation behavior is linear,
    implement the function 
    \code
    float wrath_compute_gradient(in vec2 p)
    \endcode
    in the vertex shader. The coordinate p
    is in item local coordinates.
    The return value is the gradient interpolate.

  - When the interpolation behavior is partially
    non-linear, implement the function
    \code   
    void wrath_pre_compute_gradient(in vec2 p)
    \endcode
    in the vertex shader.\n\n 
    Also implement the function
    \code
    vec2 wrath_compute_gradient(in vec2 p)
    \endcode
    in the fragment shader. The coordinate p
    is in item local coordinates.
    The .x of the return value is the
    gradient interpolate, the .y is
    1.0 if the gradient interpolate is 
    well defined and 0.0 if it is not
    well defined [for example a radial
    gradient will return 0.0 in .y if 
    the point is outside of the domain 
    of a radial gradient]

  - When the interpolation behavior is fully
    non-linear, implement the function
    \code   
    void wrath_pre_compute_gradient(void)
    \endcode
    in the vertex shader.\n\n 
    Also implement the function
    \code
    vec2 wrath_compute_gradient(in vec2 p)
    \endcode
    in the fragment shader. The coordinate p
    is in item local coordinates.
    The .x of the return value is the
    gradient interpolate, the .y is
    1.0 if the gradient interpolate is 
    well defined and 0.0 if it is not
    well defined [for example a radial
    gradient will return 0.0 in .y if 
    the point is outside of the domain 
    of a radial gradient]


  The class \ref WRATHShaderBrushSourceHoard, in implementing the 
  GLSL code for brush functions, obey the added macros WRATH_LINEAR_GRADIENT, 
  WRATH_NON_LINEAR_GRADIENT and WRATH_FULLY_NON_LINEAR_GRADIENT by adding \#ifdef's checking 
  for those macros when calling the functions wrath_compute_gradient() 
  and wrath_pre_compute_gradient(). These macros are added
  by \ref add_shader_source_code_specify_interpolation() when the
  passed parameter suffix is empty as follows:
  - WRATHBaseSource::linear_computation: WRATH_LINEAR_GRADIENT in vertex and fragment shader
  - WRATHBaseSource::nonlinear_computation WRATH_NON_LINEAR_GRADIENT in vertex and fragment shader
  - WRATHBaseSource::fully_nonlinear_computation WRATH_NON_LINEAR_GRADIENT and WRATH_FULLY_NON_LINEAR_GRADIENT in vertex and fragment shader
 */
class WRATHGradientSourceBase:public WRATHBaseSource
{
public:

  /*!\fn WRATHGradientSourceBase(void)
    Public ctor.
   */ 
  WRATHGradientSourceBase(void);

  virtual
  ~WRATHGradientSourceBase();

  /*!\fn enum interpolation_behaviour_t adjust_interpolation_behavior
    To be implemented by a derived class to adjust the
    intepolation behavior to what the implementation
    can accept. The return value must be greater
    than or equal to the input value, where:
    \code
    WRATHBaseSource::linear_computation <= WRATHBaseSource::nonlinear_computation <= WRATHBaseSource::fully_nonlinear_computation
    \endcode
    \param ibt interpolation behavior to potentially promote
   */
  virtual
  enum interpolation_behaviour_t
  adjust_interpolation_behavior(enum interpolation_behaviour_t ibt) const=0;

  /*!\fn bool gradient_always_valid
    To be implemented by a derived class to indicate if the 
    the domain of the gradient interpolate computation is
    the entire plane; for example radial gradients have that
    their domain is NOT the entire plane and should return
    false and linear gradients have that their domain is
    the entire plane and should return true.
   */
  virtual
  bool
  gradient_always_valid(void) const=0;


  /*!\fn void add_shader_source_code_specify_interpolation
    Adds the GLSL code to compute a gradient interpolate
    with the specified the interpolation behavior. 
    \param ibt interpolation behavior of the computation
               of the gradient interpolate. It is an error
               if adjust_interpolation_behavior(ibt)!=ibt.
    \param src an std::map keyed by shader type with values
               of shader source code to which to add source code
    \param prec precision qaulifier to use
    \param suffix suffix to which to append to all function, macros, etc
                  added to the GLSL code, including the functions
                  <B>wrath_compute_gradient()</B> and <B>wrath_pre_compute_gradient()</B>. 
                  A non-empty suffix indicates that the functions
                  are being chained from another function, in this
                  case none of the macros WRATH_LINEAR_GRADIENT, WRATH_NON_LINEAR_GRADIENT
                  and WRATH_FULLY_NON_LINEAR_GRADIENT will be added
   */
  void
  add_shader_source_code_specify_interpolation(enum interpolation_behaviour_t ibt,
                                               std::map<GLenum, WRATHGLShader::shader_source> &src,
                                               enum precision_t prec,
                                               const std::string &suffix) const;

  /*!\fn const WRATHGradientSourceBase* non_linear_facade
    Returns a WRATHGradientSourceBase using the same
    underlying code of this WRATHGradientSourceBase,
    but forces the interpolation mode to be 
    \ref WRATHBaseSource::fully_nonlinear_computation
   */
  const WRATHGradientSourceBase*
  non_linear_facade(void) const
  {
    return m_fully_non_linear_facade;
  }

protected:

  virtual
  void
  add_shader_source_code_implement(std::map<GLenum, WRATHGLShader::shader_source> &src,
                                   enum precision_t prec,
                                   const std::string &suffix) const;

  /*!\fn add_shader_source_code_specify_interpolation_implementation
    To be implemented by a derived class to add the 
    GLSL code to compute a gradient interpolate
    with the specified the interpolation behavior. 
    \param ibt interpolation behavior of the computation
               of the gradient interpolate. It is an guaranteed 
               that adjust_interpolation_behavior(ibt)==ibt.
    \param src an std::map keyed by shader type with values
               of shader source code to which to add source code
    \param prec precision qaulifier to use
    \param suffix suffix to which to append to all symbols of GLSL shaders
                  that are at global scope, including the functions
                  <B>wrath_compute_gradient()</B> and <B>wrath_pre_compute_gradient()</B>
   */
  virtual
  void
  add_shader_source_code_specify_interpolation_implementation(enum interpolation_behaviour_t ibt,
                                                              std::map<GLenum, WRATHGLShader::shader_source> &src,
                                                              enum precision_t prec,
                                                              const std::string &suffix) const=0;

private:
  friend class WRATHGradientSourceBasePrivate::NonLinearFacade;

  enum is_facade_t
    {
      is_facade
    };

  //different ctor to ctop creation of facade.
  WRATHGradientSourceBase(enum is_facade_t);


  WRATHGradientSourceBase *m_fully_non_linear_facade;

};
/*! @} */

#endif
