/*! 
 * \file WRATHShaderSourceResource.hpp
 * \brief file WRATHShaderSourceResource.hpp
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




#ifndef __WRATH_SHADER_SOURCE_RESOURCE_HPP__
#define __WRATH_SHADER_SOURCE_RESOURCE_HPP__

#include "WRATHConfig.hpp"
#include <string>

/*!\class WRATHShaderSourceResource
  The construction of a WRATHShaderSourceResource
  creates an entry of a map keyed by string
  with values as shader source code. The dtor
  of a WRATHShaderSourceResource does NOT
  remove entry from the map.
 */
class WRATHShaderSourceResource
{
public:

  /*!\fn WRATHShaderSourceResource
    Function dressed up as a ctor. When ctor is called,
    adds an entry.
    \param pname name of the entry, use this value in 
                 \ref WRATHGLShader::shader_source::add_source()
                 with \ref WRATHGLShader::from_resource
    \param pshader_source_code value of the entry, a string of GLSL code
   */
  WRATHShaderSourceResource(const std::string &pname, 
                            const std::string &pshader_source_code);
                            

  /*!\fn const std::string& retrieve_value
    Fetch the value of an entry, if the entry does not exist,
    returns an empty string.
    \param pname name of the entry
   */
  static
  const std::string&
  retrieve_value(const std::string &pname);
}; 

#define WRATH_SHADER_SOURCE_UNIQUE_ID_IMPLEMENT2(X, Y) X##Y
#define WRATH_SHADER_SOURCE_UNIQUE_ID_IMPLEMENT1(X, Y) WRATH_SHADER_SOURCE_UNIQUE_ID_IMPLEMENT2(X, Y)

/*!\def WRATH_SHADER_SOURCE_UNIQUE_ID(X)
  Macro to generate a unique ID (_per source file_). Macro should be placed 
  inside an anonymous namespace as well.
 */
#define WRATH_SHADER_SOURCE_UNIQUE_ID(X) WRATH_SHADER_SOURCE_UNIQUE_ID_IMPLEMENT1(X, __COUNTER__)

#endif
