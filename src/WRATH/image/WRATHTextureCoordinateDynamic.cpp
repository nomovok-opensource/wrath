/*! 
 * \file WRATHTextureCoordinateDynamic.cpp
 * \brief file WRATHTextureCoordinateDynamic.cpp
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
#include "WRATHTextureCoordinateSource.hpp"
#include "WRATHTextureCoordinateDynamic.hpp"

namespace
{
  class LocalShader:public WRATHTextureCoordinateSource
  {
  public:
    LocalShader(void);

    const WRATHGLShader::shader_source& 
    shader_code(enum precision_t prec, enum interpolation_behaviour_t) const
    {
      return m_shader[prec];
    }

    const WRATHGLShader::shader_source&
    pre_compute_shader_code(enum precision_t prec, enum interpolation_behaviour_t) const
    {
      return m_pre_shader[prec];
    }

    virtual
    enum interpolation_behaviour_t
    adjust_interpolation_behavior(enum interpolation_behaviour_t) const
    {
      return fully_nonlinear_computation;
    }

    virtual
    const_c_array<std::string>
    global_scoped_symbols(enum precision_t, enum interpolation_behaviour_t) const;

  private:
    vecN<WRATHGLShader::shader_source, 3> m_shader, m_pre_shader;
    std::vector<std::string> m_global_symbols;
  };
};

////////////////////////////////////////////////////
// LocalShader methods
LocalShader::
LocalShader(void)
{
  for(int iprec=0; iprec<3; ++iprec)
    {
      enum precision_t prec(static_cast<enum precision_t>(iprec));
      const std::string &prec_as_string(prec_string(prec));

      m_shader[iprec]
        .add_macro("WRATH_IMAGE_REPEAT_MODE_PREC", prec_as_string)
        .add_source("image-repeat-mode-functions.wrath-shader.glsl", WRATHGLShader::from_resource)
        .add_source("image-value-normalized-coordinate-dynamic.compute.wrath-shader.glsl",
                    WRATHGLShader::from_resource)
        .remove_macro("WRATH_IMAGE_REPEAT_MODE_PREC");


      m_pre_shader[iprec]
        .add_macro("WRATH_IMAGE_REPEAT_MODE_PREC", prec_as_string)
        .add_source("image-value-normalized-coordinate-dynamic.pre-compute.wrath-shader.glsl",
                    WRATHGLShader::from_resource)
        .remove_macro("WRATH_IMAGE_REPEAT_MODE_PREC");
    }
}

const_c_array<std::string>
LocalShader::
global_scoped_symbols(enum precision_t, enum interpolation_behaviour_t) const
{
  static vecN<std::string, 6> values("WRATH_IMAGE_VALUE_NORMALIZED_varying0",
                                     "WRATH_IMAGE_VALUE_NORMALIZED_varying1",                                     
                                     "compute_simple",
                                     "compute_repeat",
                                     "compute_clamp",
                                     "compute_mirror_repeat");
  return values;
}


////////////////////////////////////////////////////////
// WRATHTextureCoordinateDynamic methods     
void
WRATHTextureCoordinateDynamic::
set(enum repeat_mode_type pxmode,
    enum repeat_mode_type pymode)
{
  m_mode_x=pxmode;
  m_mode_y=pymode;

  const float values[4]=
    {
      /*[simple]=*/ 2,
      /*[clamp]=*/ 4,
      /*[repeat]=*/ 6,
      /*[mirror_repeat]=*/ 8,
    };

  m_shader_value= values[m_mode_x] + 0.1*values[m_mode_y];
}

void
WRATHTextureCoordinateDynamic::
add_per_node_values_at(int start, WRATHLayerNodeValuePackerBase::ActiveNodeValuesCollection &spec,
                         const WRATHLayerNodeValuePackerBase::function_packet &func)
{
  WRATHTextureCoordinate::add_per_node_values_at(start, spec, func);

  if(func.supports_per_node_value(GL_FRAGMENT_SHADER))
    {
      spec
        .add_source(start+WRATHTextureCoordinate::number_per_node_values, 
                    "WRATH_IMAGE_repeat_mode", GL_FRAGMENT_SHADER);
    }
  else
    {
      spec
        .add_source(start+WRATHTextureCoordinate::number_per_node_values, 
                    "WRATH_IMAGE_repeat_mode", GL_VERTEX_SHADER);
    }
}


void
WRATHTextureCoordinateDynamic::
extract_values_at(int start_index, reorder_c_array<float> out_value)
{
  WRATHTextureCoordinate::extract_values_at(start_index, out_value);
  out_value[start_index+WRATHTextureCoordinate::number_per_node_values]=m_shader_value;
}

    


const WRATHTextureCoordinateSourceBase*
WRATHTextureCoordinateDynamic::
source(void)
{
  WRATHStaticInit();
  static LocalShader R;
  return &R;
}
