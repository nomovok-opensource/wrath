/*! 
 * \file WRATHLinearGradientValue.cpp
 * \brief file WRATHLinearGradientValue.cpp
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



#include "WRATHLinearGradientValue.hpp"
#include "WRATHStaticInit.hpp"

namespace 
{
  void
  generate_source(WRATHGLShader::shader_source &obj,
                  const std::string &resource_name,
                  enum WRATHGradientSource::interpolation_behaviour_t ibt,
                  const std::string &prec)
  {
    
    if(ibt==WRATHGradientSource::linear_computation)
      {
        obj
          .add_macro("WRATH_LINEAR_GRADIENT_VS");
      }

    obj
      .add_macro("WRATH_LINEAR_GRADIENT_PREC", prec)
      .add_source(resource_name, WRATHGLShader::from_resource)
      .add_source("\n#undef WRATH_LINEAR_GRADIENT_PREC\n", WRATHGLShader::from_string);

    if(ibt==WRATHGradientSource::linear_computation)
      {
        obj
          .add_source("\n#undef WRATH_LINEAR_GRADIENT_VS\n", WRATHGLShader::from_string);
      }        

    
    
  }

  class LayerItemNodeLinearGradientSource:public WRATHGradientSource
  {
  public:
    LayerItemNodeLinearGradientSource(void):
      m_varying_name("WRATH_LINEAR_GRADIENT_varying")
    {

      for(int itype=0; itype<3; ++itype)
        {
          enum interpolation_behaviour_t type;
          type=static_cast<interpolation_behaviour_t>(itype);
         
          generate_source(m_shaders[type][default_precision], 
                          "linear-gradient-values.compute.wrath-shader.glsl", type, "");

          generate_source(m_shaders[type][mediump_precision], 
                          "linear-gradient-values.compute.wrath-shader.glsl", type, "mediump");

          generate_source(m_shaders[type][highp_precision], 
                          "linear-gradient-values.compute.wrath-shader.glsl", type, "highp");

          
          generate_source(m_prec_shaders[type][default_precision], 
                          "linear-gradient-values.pre-compute.wrath-shader.glsl", type, "");

          generate_source(m_prec_shaders[type][mediump_precision], 
                          "linear-gradient-values.pre-compute.wrath-shader.glsl", type, "mediump");

          generate_source(m_prec_shaders[type][highp_precision], 
                          "linear-gradient-values.pre-compute.wrath-shader.glsl", type, "highp");
        }
    }

    virtual
    enum interpolation_behaviour_t
    adjust_interpolation_behavior(enum interpolation_behaviour_t ibt) const
    {
      return ibt;
    }

    virtual
    const WRATHGLShader::shader_source& 
    shader_code(enum precision_t prec, enum interpolation_behaviour_t ibt) const
    {
      return m_shaders[ibt][prec];
    }

    virtual
    const WRATHGLShader::shader_source&
    pre_compute_shader_code(enum precision_t prec, enum interpolation_behaviour_t ibt) const
    {
      return m_prec_shaders[ibt][prec];
    }

    virtual
    const_c_array<std::string>
    global_scoped_symbols(enum precision_t, enum interpolation_behaviour_t ibt) const
    {
      return (ibt==linear_computation)?
        const_c_array<std::string>():
        const_c_array<std::string>(&m_varying_name, 1);
    }

  private:
    vecN<vecN<WRATHGLShader::shader_source, 3>, 3> m_shaders;
    vecN<vecN<WRATHGLShader::shader_source, 3>, 3> m_prec_shaders;
    std::string m_varying_name;
  };
 
}

void
WRATHLinearGradientValue::
add_per_node_values_at(int start, WRATHLayerNodeValuePackerBase::ActiveNodeValuesCollection &spec,
			 const WRATHLayerNodeValuePackerBase::function_packet &func)
{
  WRATHGradientValueBase::add_per_node_values_at(start, spec, func);
  start+=WRATHGradientValueBase::number_per_node_values;

  /*
    The node values are only needed in different stages
    depending on the nature of the gradient computation: linear or non-linear.
    We let the WRATHLayerItemDrawerFactory shader creation inspection
    filter out unused values.
   */
  spec
    .add_source(start+0, "WRATH_LINEAR_GRADIENT_p0_x", GL_VERTEX_SHADER)
    .add_source(start+1, "WRATH_LINEAR_GRADIENT_p0_y", GL_VERTEX_SHADER)
    .add_source(start+2, "WRATH_LINEAR_GRADIENT_delta_x", GL_VERTEX_SHADER)
    .add_source(start+3, "WRATH_LINEAR_GRADIENT_delta_y", GL_VERTEX_SHADER);

  if(func.supports_per_node_value(GL_FRAGMENT_SHADER))
    {
      spec
        .add_source(start+0, "WRATH_LINEAR_GRADIENT_p0_x", GL_FRAGMENT_SHADER)
        .add_source(start+1, "WRATH_LINEAR_GRADIENT_p0_y", GL_FRAGMENT_SHADER)
        .add_source(start+2, "WRATH_LINEAR_GRADIENT_delta_x", GL_FRAGMENT_SHADER)
        .add_source(start+3, "WRATH_LINEAR_GRADIENT_delta_y", GL_FRAGMENT_SHADER);
    }
}

void
WRATHLinearGradientValue::
extract_values_at(int start, reorder_c_array<float> out_value)
{
  WRATHGradientValueBase::extract_values_at(start, out_value);
  start+=WRATHGradientValueBase::number_per_node_values;

  out_value[start+0]=start_gradient().x();
  out_value[start+1]=start_gradient().y();
  out_value[start+2]=normalized_delta_gradient().x();
  out_value[start+3]=normalized_delta_gradient().y();
  
}


const WRATHGradientSourceBase*
WRATHLinearGradientValue::
gradient_source(void)
{
  WRATHStaticInit();
  static LayerItemNodeLinearGradientSource R;
  return &R;
}
