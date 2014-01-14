/*! 
 * \file WRATHGradientSource.cpp
 * \brief file WRATHGradientSource.cpp
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
#include "WRATHGradientSource.hpp"

namespace
{
  class EmptyComputeShaderCodeType:boost::noncopyable
  {
  public:
    EmptyComputeShaderCodeType(void);
    vecN<WRATHGLShader::shader_source, 3> m_srcs;
  };

  EmptyComputeShaderCodeType&
  EmptyComputeShaderCode(void)
  {
    static EmptyComputeShaderCodeType R;
    return R;
  }
}

EmptyComputeShaderCodeType::
EmptyComputeShaderCodeType(void)
{
  m_srcs[WRATHGradientSource::default_precision]
    .add_source("empty_pre_compute_shader_code_noprec.wrath-shader.glsl", WRATHGLShader::from_resource);
  
  m_srcs[WRATHGradientSource::mediump_precision]
    .add_source("empty_pre_compute_shader_code_mediump.wrath-shader.glsl", WRATHGLShader::from_resource);
  
  m_srcs[WRATHGradientSource::highp_precision]
    .add_source("empty_pre_compute_shader_code_highp.wrath-shader.glsl", WRATHGLShader::from_resource);
}


const WRATHGLShader::shader_source&
WRATHGradientSource::
pre_compute_shader_code(enum precision_t prec, enum interpolation_behaviour_t) const
{
  return EmptyComputeShaderCode().m_srcs[prec];
}


/*
  TODO: should we add an additional parameter
  about between what stages the varying are?
 */
const_c_array<std::string>
WRATHGradientSource::
global_scoped_symbols(enum precision_t, enum interpolation_behaviour_t) const
{
  return const_c_array<std::string>();
}

void
WRATHGradientSource::
add_shader_source_code_specify_interpolation_implementation(enum interpolation_behaviour_t b,
                                                            std::map<GLenum, WRATHGLShader::shader_source> &src,
                                                            enum precision_t prec,
                                                            const std::string &suffix) const
{
  /*
    TODO: handle more than just vertex/fragment shader.
   */
  WRATHGLShader::shader_source &vs(src[GL_VERTEX_SHADER]);
  WRATHGLShader::shader_source &fs(src[GL_FRAGMENT_SHADER]);
  const_c_array<std::string> varyings;

  WRATHassert(b==adjust_interpolation_behavior(b));
  varyings=global_scoped_symbols(prec, b);

  if(!suffix.empty())
    {
      std::ostringstream ostr;

      ostr << "\n#define wrath_compute_gradient wrath_compute_gradient" << suffix 
           << "\n#define wrath_pre_compute_gradient wrath_pre_compute_gradient" << suffix << "\n";

      for(const_c_array<std::string>::iterator iter=varyings.begin(),
            end=varyings.end(); iter!=end; ++iter)
        {
          ostr << "\n#define " << *iter << " " << *iter << suffix << "\n";
        }

      vs
        .add_source(ostr.str(), WRATHGLShader::from_string);

      fs
        .add_source(ostr.str(), WRATHGLShader::from_string);
    }

  switch(b)
    {
    case linear_computation:
      vs.absorb(shader_code(prec, b));
      break;

    case nonlinear_computation:
      vs.absorb(pre_compute_shader_code(prec, b));
      fs.absorb(shader_code(prec, b));
      break;

    default:
      WRATHwarning("Bad value for interpolation in gradient source assembly, "
                   << b << " changing value to fully_nonlinear_computation");
      //fall through
    case fully_nonlinear_computation:
      vs.absorb(pre_compute_shader_code(prec, b));
      fs.absorb(shader_code(prec, b));
      break;
    }
  
  
   if(!suffix.empty())
    {
      std::ostringstream ostr;

      ostr << "\n#undef wrath_compute_gradient"
           << "\n#undef wrath_pre_compute_gradient\n";


      for(const_c_array<std::string>::iterator iter=varyings.begin(),
            end=varyings.end(); iter!=end; ++iter)
        {
          ostr << "\n#undef " << *iter << "\n";
        }

      vs
        .add_source(ostr.str(), WRATHGLShader::from_string);

      fs
        .add_source(ostr.str(), WRATHGLShader::from_string);
    }
}


