/*! 
 * \file WRATHBaseSource.cpp
 * \brief file WRATHBaseSource.cpp
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


#include "WRATHConfig.hpp"
#include "WRATHBaseSource.hpp"
#include "WRATHStaticInit.hpp"

const std::string&
WRATHBaseSource::
prec_string(enum WRATHBaseSource::precision_t prec)
{
  WRATHStaticInit();
  static vecN<std::string, 3> R("", "mediump", "highp");
  return R[prec];
}

void
WRATHBaseSource::
add_shader_source_code(std::map<GLenum, WRATHGLShader::shader_source> &src,
                       enum precision_t prec,
                       const std::string &suffix) const
{
  add_shader_source_code_implement(src, prec, suffix);  
}
