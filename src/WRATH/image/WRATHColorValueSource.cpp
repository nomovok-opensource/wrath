/*! 
 * \file WRATHColorValueSource.cpp
 * \brief file WRATHColorValueSource.cpp
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
#include "WRATHColorValueSource.hpp"


void
WRATHColorValueSource::
add_shader_source_code_implement(std::map<GLenum, WRATHGLShader::shader_source> &src,
                                 enum precision_t prec, const std::string &suffix) const
{
  uint32_t flags(shader_useablity_flags());
  std::string WRATH_CONST_COLOR_PREC("WRATH_CONST_COLOR_PREC");
  std::string WRATH_CONST_COLOR_VS("WRATH_CONST_COLOR_VS");
  std::string WRATH_CONST_COLOR_FS("WRATH_CONST_COLOR_FS");

  WRATHGLShader::shader_source &vs(src[GL_VERTEX_SHADER]);
  WRATHGLShader::shader_source &fs(src[GL_FRAGMENT_SHADER]);
  const char *prec_labels[]=
    {
      /*[default_precision]=*/ " ",
      /*[mediump_precision]=*/ "mediump",
      /*[highp_precision]=*/ "highp "
    };

  if(!suffix.empty())
    {
      std::ostringstream ostr;

      ostr << "\n#define wrath_const_color_value wrath_const_color_value" << suffix << "\n\n";

      WRATH_CONST_COLOR_PREC=WRATH_CONST_COLOR_PREC+suffix;
      WRATH_CONST_COLOR_VS=WRATH_CONST_COLOR_VS+suffix;
      WRATH_CONST_COLOR_FS=WRATH_CONST_COLOR_FS+suffix;

      vs
        .add_source(ostr.str(), WRATHGLShader::from_string);

      fs
        .add_source(ostr.str(), WRATHGLShader::from_string);
    }


  vs
    .add_macro(WRATH_CONST_COLOR_PREC, prec_labels[prec]);

  fs
    .add_macro(WRATH_CONST_COLOR_PREC, prec_labels[prec]);

  if(flags&fragment_shader_fetchable)
    {
      vs
        .add_macro(WRATH_CONST_COLOR_FS);

      fs
        .add_macro(WRATH_CONST_COLOR_FS)
        .absorb(shader_code(prec));
    }

  if(flags&vertex_shader_fetchable)
    {
      vs
        .add_macro(WRATH_CONST_COLOR_VS)
        .absorb(shader_code(prec));

      fs
        .add_macro(WRATH_CONST_COLOR_VS);
    }

  if(!suffix.empty())
    {
      std::ostringstream ostr;

      ostr << "\n#undef wrath_const_color_value\n";

      vs
        .add_source(ostr.str(), WRATHGLShader::from_string);

      fs
        .add_source(ostr.str(), WRATHGLShader::from_string);
    }
}
