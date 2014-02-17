/*! 
 * \file WRATHBaseSource.hpp
 * \brief file WRATHBaseSource.hpp
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




#ifndef WRATH_HEADER_BASE_SOURCE_HPP_
#define WRATH_HEADER_BASE_SOURCE_HPP_


/*! \addtogroup Group
 * @{
 */

#include "WRATHConfig.hpp"
#include <map>
#include "WRATHGLProgram.hpp"


/*!\class WRATHBaseSource
  A WRATHBaseSource provides an interface
  to add GLSL code that provides functions,
  values, etc to shader code. When code is
  added all variables and functions are 
  suffixed. 

  The main purpose is to allow to composite
  multiple WRATHBaseSource objects that
  provide functions of the same name. The
  suffixing prevents name collision.
 */
class WRATHBaseSource:boost::noncopyable
{
public:
  /*!\enum precision_t
    Enumeration to specify the precision of 
    whatever additional data/computations a
    WRATHBaseSource provides
   */
  enum precision_t
    {
      /*!
        Indicates to give _NO_ precision qualifier
        to the uniform and its functions.
       */
      default_precision=0,

      /*!
        Indicates to give mediump precision qualifier
        to the uniform and its functions.
       */
      mediump_precision=1,

      /*!
        Indicates to give highp precision qualifier
        to the uniform and its functions.
       */
      highp_precision=2,
    };
  
  /*!\enum interpolation_behaviour_t
    Enumeration type reusable to indicate
    linearization behavior of GLSL functions 
    provided by derived classes of WRATHBaseSource
   */
  enum interpolation_behaviour_t
    {
      /*!
        Computation of gradient interpolate
        is linear (or affine) and thus can
        be computed in the vertex shader.
       */
      linear_computation,

      /*!
        Computation of gradient interpolate
        is non-linear and must be computed
        at least partially in the fragment shader
       */
      nonlinear_computation,

      /*!
        Computation of gradient interpolate
        is non-linear and must be computed
        completely in the fragment shader
       */
      fully_nonlinear_computation,
    }; 

  virtual
  ~WRATHBaseSource()
  {}


  /*!\fn void add_shader_source_code()
    Adds GLSL source code to shader source code.
    The suffix parameter is appended to all macros,
    variables and function definitions. This way
    multiple objects defining the same functions/variables
    can be used within the same shader.
    \param src an std::map keyed by shader type with values
               of shader source code to which to add source code
    \param prec precision qaulifier to use
    \param suffix suffix to which to append to all function, macros, etc
                  added to the GLSL code
   */
  void
  add_shader_source_code(std::map<GLenum, WRATHGLShader::shader_source> &src,
                         enum precision_t prec,
                         const std::string &suffix="") const;

  /*!\fn const std::string& prec_string
    Provided as a conveniance, returns values a follows:\n\n
    default_precision  ----> "" \n
    mediump_precision  ----> "mediump" \n
    highp_precision    ----> "highp" \n
   */
  static
  const std::string&
  prec_string(enum precision_t prec);
  

protected:

  /*!\fn void add_shader_source_code_implement
    To be implemented by a derived class to add the
    shader source code for declaraing and implementing
    those GLSL functions that the WRATHBaseSource-derived
    object provides. Returns a created entry_type 
    object (to be deleted by the owner of the entry_map)
    recording the properties of the object added.
    \param src an std::map keyed by shader type with values
               of shader source code to which to add source code
    \param prec precision qaulifier to use
    \param suffix suffix to which to append to all function, macros, etc
                  added to the GLSL code
   */
  virtual
  void
  add_shader_source_code_implement(std::map<GLenum, WRATHGLShader::shader_source> &src,
                                   enum precision_t prec,
                                   const std::string &suffix) const=0;
};


/*! @} */


#endif
