/*! 
 * \file WRATHColorValueSource.hpp
 * \brief file WRATHColorValueSource.hpp
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




#ifndef __WRATH_CONST_COLOR_SOURCE_HPP__
#define __WRATH_CONST_COLOR_SOURCE_HPP__

#include "WRATHConfig.hpp"
#include <boost/utility.hpp>
#include <stdint.h>
#include "WRATHBaseSource.hpp"
#include "WRATHGLProgram.hpp"

/*! \addtogroup Imaging
 * @{
 */

/*!\class WRATHColorValueSource
  A WRATHColorValueSource represents the
  GLSL source code to "get" a const color.

  WRATHColorValueSource implements add_shader_source_code()
  as follows:
  - if the bit \ref WRATHColorValueSource::fragment_shader_fetchable from \ref shader_useablity_flags()
    is up, then the macro CONST_COLOR_FS is added to both the vertex
    and fragment shader and the fragment shader abosrbs the GLSL code
    from \ref shader_code()
  - if the bit \ref WRATHColorValueSource::vertex_shader_fetchable from \ref shader_useablity_flags()
    is up, then the macro CONST_COLOR_VS is added to both the vertex
    and vertex shader and the fragment shader abosrbs the GLSL code
    from \ref shader_code()
  - the macro CONST_COLOR_PREC is added which is defined as the precision
    type

  A fixed WRATHColorValueSource is to
  be reused by WRATHShaderSource objects, 
  hence those types (for example WRATHColorValue)
  implement computation of interpolate should
  "save" their WRATHColorValueSource object
  for reuse.
 */
class WRATHColorValueSource:public WRATHBaseSource
{
public:
  
  enum
    {
      /*!
        bit flag indicates that fetching of 
        the color value is permissible from
        the vertex shader
       */
      vertex_shader_fetchable=1,

      /*!
        bit flag indicates that fetching of 
        the color value is permissible from
        the fragment shader
       */
      fragment_shader_fetchable=2,
    };

  virtual
  ~WRATHColorValueSource()
  {}

  /*!\fn uint32_t shader_useablity_flags
    To be implemented by a derived class
    to indicate from where it is legal to 
    fetch the const-color value. Default
    implementation is to indicate it is
    legal to fetch from both vertex and
    fragment shader stages.
   */
  virtual
  uint32_t
  shader_useablity_flags(void) const
  {
    return vertex_shader_fetchable|fragment_shader_fetchable;
  }

  /*!\fn const WRATHGLShader::shader_source& shader_code
    To be implemented by a derived class
    to return the GLSL source code
    for the the function:
    - prec vec4 const_color_value(void);

    where prec is the correct precision qualifier
    coming from the parameter v.
    \param v precision of the computation in the GLSL code
   */
  virtual
  const WRATHGLShader::shader_source& 
  shader_code(enum precision_t v) const=0;

protected:

  virtual
  void
  add_shader_source_code_implement(std::map<GLenum, WRATHGLShader::shader_source> &src,
                                   enum precision_t prec, const std::string &suffix) const;
};

/*! @} */
#endif
