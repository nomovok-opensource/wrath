/*! 
 * \file WRATHTextureCoordinateSourceBase.hpp
 * \brief file WRATHTextureCoordinateSourceBase.hpp
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




#ifndef __WRATH_TEXTURE_COORDINATE_SOURCE_BASE_HPP__
#define __WRATH_TEXTURE_COORDINATE_SOURCE_BASE_HPP__


#include "WRATHConfig.hpp"
#include "WRATHImage.hpp"
#include "WRATHBaseSource.hpp"
#include "WRATHImage.hpp"

namespace WRATHTextureCoordinateSourceBasePrivate
{
  class NonLinearFacade;
}

/*! \addtogroup Imaging
 * @{
 */

/*!\class WRATHTextureCoordinateSourceBase
  A WRATHTextureCoordinateSourceBase represents how to
  compute a texture coordinate. The class 
  WRATHTextureCoordinateSourceBase defines
  an interface and convention for computing 
  the textue value as follows:

  - A texture coordinate may be computed entirely
    linearly, partially linearly and fully non-linearly.
    This is controlled by the enumeration \ref
    interpolation_behaviour_t

  - The calculation is implemented in GLSL by
    implementing 2 functions: wrath_pre_compute_texture_coordinate()
    and wrath_compute_texture_coordinate(). 

  A class derived from WRATHTextureCoordinateSourceBase must
  abide by the following conventions:
  - If the interpolation behavior is linear, implement the function 
    \code
    vec2 wrath_compute_texture_coordinate(in vec2 p)
    \endcode
    in the vertex shader. The coordinate p
    is the coordinate in item coordinates
    divided by the size of the texture 
    from which to sample. 

  - If the interpolation behavior is partially 
    non-linear, implement the function
    \code   
    void wrath_pre_compute_texture_coordinate(in vec2 p)
    \endcode
    in the vertex shader.\n\n 
    Also implement the function
    \code
    vec2 wrath_compute_texture_coordinate(in vec2 p)
    \endcode
    in the fragment shader. The coordinate p
    is the coordinate in item coordinates
    divided by the size of the texture 
    from which to sample. 

  - If the interpolation behavior is fully-nonlinear,
    implement the function
    \code   
    void wrath_pre_compute_texture_coordinate(void)
    \endcode
    in the vertex shader.\n\n 
    Also implement the function
    \code
    vec2 wrath_compute_texture_coordinate(in vec2 p)
    \endcode
    in the fragment shader. The coordinate p
    is the coordinate in item coordinates
    divided by the size of the texture 
    from which to sample. 

  The class \ref WRATHShaderBrushSourceHoard, in implementing the 
  GLSL code for brush functions, obeys the added macros 
  WRATH_LINEAR_TEXTURE_COORDINATE, WRATH_NON_LINEAR_TEXTURE_COORDINATE
  and WRATH_FULLY_NON_LINEAR_TEXTURE_COORDINATE by adding \#ifdef's checking 
  for those macros when calling the functions. These macros are added
  by \ref add_shader_source_code_specify_interpolation() as necessary as follows:
  - WRATHBaseSource::linear_computation: WRATH_LINEAR_TEXTURE_COORDINATE in vertex and fragment shader
  - WRATHBaseSource::nonlinear_computation WRATH_NON_LINEAR_TEXTURE_COORDINATE in vertex and fragment shader
  - WRATHBaseSource::fully_nonlinear_computation WRATH_NON_LINEAR_TEXTURE_COORDINATE and 
    WRATH_FULLY_NON_LINEAR_TEXTURE_COORDINATE in vertex and fragment shader
 */
class WRATHTextureCoordinateSourceBase:public WRATHBaseSource
{
public:

  /*!\fn WRATHTextureCoordinateSourceBase(void)
    Public (empty) ctor.
   */ 
  WRATHTextureCoordinateSourceBase(void);

  virtual
  ~WRATHTextureCoordinateSourceBase();
  
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

  /*!\fn add_shader_source_code_specify_interpolation
    Adds the GLSL code to compute a texture_coordinate 
    with the specified the interpolation behavior. 
    \param ibt interpolation behavior of the computation
               of the texture_coordinate. It is an error
               if adjust_interpolation_behavior(ibt)!=ibt.  
    \param src an std::map keyed by shader type with values
               of shader source code to which to add source code
    \param prec precision qaulifier to use
    \param suffix suffix to which to append to all function, macros, etc
                  added to the GLSL code, including the functions
                  <B>wrath_compute_texture_coordinate()</B> and 
                  <B>wrath_pre_compute_texture_coordinate()</B>.
                  A non-empty suffix indicates that the functions
                  are being chained from another function, in this
                  case none of the macros WRATH_LINEAR_TEXTURE_COORDINATE, 
                  WRATH_NON_LINEAR_TEXTURE_COORDINATE and WRATH_FULLY_NON_LINEAR_TEXTURE_COORDINATE
                  will be added
   */
  void
  add_shader_source_code_specify_interpolation(enum interpolation_behaviour_t ibt,
                                               std::map<GLenum, WRATHGLShader::shader_source> &src,
                                               enum precision_t prec,
                                               const std::string &suffix) const;

  /*!\fn const WRATHTextureCoordinateSourceBase* non_linear_facade
    Returns a WRATHTextureCoordinateSourceBase using the same
    underlying code of this WRATHTextureCoordinateSourceBase,
    but forces the interpolation mode to be 
    \ref WRATHBaseSource::fully_nonlinear_computation
   */
  const WRATHTextureCoordinateSourceBase*
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
    GLSL code to compute a texture_coordinate 
    with the specified the interpolation behavior. 
    \param ibt interpolation behavior of the computation
               of the texture_coordinate. It is an guaranteed 
               that adjust_interpolation_behavior(ibt)==ibt.
    \param src an std::map keyed by shader type with values
               of shader source code to which to add source code
               Any macro's defined within the source need to 
               be undefined after the source.
    \param prec precision qaulifier to use
    \param suffix suffix to which to append to all symbols of GLSL shaders
                  that are at global scope, including the functions
                  <B>wrath_compute_texture_coordinate()</B> and 
                  <B>wrath_pre_compute_texture_coordinate()</B>.
   */
  virtual
  void
  add_shader_source_code_specify_interpolation_implementation(enum interpolation_behaviour_t ibt,
                                                              std::map<GLenum, WRATHGLShader::shader_source> &src,
                                                              enum precision_t prec,
                                                              const std::string &suffix) const=0;

private:
  friend class WRATHTextureCoordinateSourceBasePrivate::NonLinearFacade;
  enum is_facade_t
    {
      is_facade
    };

  //differnt ctor to ctop creation of facade.
  WRATHTextureCoordinateSourceBase(enum is_facade_t);


  WRATHTextureCoordinateSourceBase *m_fully_non_linear_facade;

};
/*! @} */

#endif
