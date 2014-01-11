/*! 
 * \file WRATHRadialGradientValue.cpp
 * \brief file WRATHRadialGradientValue.cpp
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



#include "WRATHRadialGradientValue.hpp"
#include "WRATHStaticInit.hpp"

namespace 
{
  enum
    {
      A_loc=0,
      A_r0_delta_r_loc,
      r0_r0_loc,
      p0_x_loc,
      p0_y_loc,
      A_delta_p_x_loc,
      A_delta_p_y_loc
    };

  class shader_pair
  {
  public:
    WRATHGLShader::shader_source m_shader;
    WRATHGLShader::shader_source m_pre_compute_shader;
  };

  void
  generate_source(WRATHGLShader::shader_source &obj, 
                  const std::string &prec, 
                  bool fully_linear,
                  const std::string &shader_resource)
  {
    
    obj.add_macro("WRATH_RADIAL_GRADIENT_PREC", prec);
      
    if(!fully_linear)
      {
        obj.add_macro("WRATH_RADIAL_GRADIENT_PARTIAL_LINEAR");
      }

    obj
      .add_source(shader_resource, WRATHGLShader::from_resource)
      .add_source("\n#undef WRATH_RADIAL_GRADIENT_PREC\n", WRATHGLShader::from_string);

    if(!fully_linear)
      {
        obj.add_source("\n#undef WRATH_RADIAL_GRADIENT_PARTIAL_LINEAR\n", WRATHGLShader::from_string);
      }
  }

  void
  generate_source(shader_pair &obj, const std::string &prec, bool partial_linear)
  {
    generate_source(obj.m_shader, prec, partial_linear, 
                    "radial-gradient-values.compute.wrath-shader.glsl");

    generate_source(obj.m_pre_compute_shader, prec, partial_linear, 
                    "radial-gradient-values.pre_compute.wrath-shader.glsl");
    
  }

  
  class LocalGradientSource:public WRATHGradientSource
  {
  public:
    LocalGradientSource(void)
    {
      

      generate_source(m_data[nonlinear_computation][default_precision], "", false);
      generate_source(m_data[nonlinear_computation][mediump_precision], "mediump", false);
      generate_source(m_data[nonlinear_computation][highp_precision], "highp", false);

      
      generate_source(m_data[fully_nonlinear_computation][default_precision], "", true);
      generate_source(m_data[fully_nonlinear_computation][mediump_precision], "mediump", true);
      generate_source(m_data[fully_nonlinear_computation][highp_precision], "highp", true);

      /*
        these need to match up with the code found in the shaders.
       */
      m_varyings.push_back("WRATH_RADIAL_GRADIENT_varying0");
      m_varyings.push_back("WRATH_RADIAL_GRADIENT_varying1");
    }
    
    

    virtual
    enum interpolation_behaviour_t
    adjust_interpolation_behavior(enum interpolation_behaviour_t ibt) const
    {
      return (ibt==linear_computation)?
        nonlinear_computation:
        ibt;
    }

    virtual
    const WRATHGLShader::shader_source& 
    shader_code(enum precision_t prec, enum interpolation_behaviour_t ibt) const
    {
      WRATHassert(ibt!=linear_computation);
      return m_data[ibt][prec].m_shader;
    }

    virtual
    const WRATHGLShader::shader_source&
    pre_compute_shader_code(enum precision_t prec, enum interpolation_behaviour_t ibt) const
    {
      WRATHassert(ibt!=linear_computation);
      return m_data[ibt][prec].m_pre_compute_shader;
    }

    virtual
    const_c_array<std::string>
    global_scoped_symbols(enum precision_t, enum interpolation_behaviour_t) const
    {
      return m_varyings;
    }

    virtual
    bool
    gradient_always_valid(void) const
    {
      return false;
    }

  private:
    vecN<vecN<shader_pair, 3>, 3> m_data;
    std::vector<std::string> m_varyings;
  };
 
}

void
WRATHRadialGradientValue::
update_pack_values(void)
{
  vec2 delta_p(m_p1 - m_p0);
  float delta_r(m_r1 - m_r0);
  float recipA;

  recipA= dot(delta_p, delta_p) - delta_r*delta_r;

  m_A=recipA!=0.0f?
    1.0f/recipA:
    0.0f;

  m_A_delta_p= m_A*delta_p;
  m_A_r0_delta_r= m_A*m_r0*delta_r;
  m_r0_r0=m_r0*m_r0;
}


void
WRATHRadialGradientValue::
add_per_node_values_at(int start, 
                         WRATHLayerNodeValuePackerBase::ActiveNodeValuesCollection &spec,
                         const WRATHLayerNodeValuePackerBase::function_packet &fpt)
{
  WRATHGradientValueBase::add_per_node_values_at(start, spec, fpt);
  start+=WRATHGradientValueBase::number_per_node_values;
  
  if(fpt.supports_per_node_value(GL_FRAGMENT_SHADER))
    {

      spec
        .add_source(start+A_loc, "WRATH_RADIAL_GRADIENT_A", GL_FRAGMENT_SHADER)
        .add_source(start+A_r0_delta_r_loc, "WRATH_RADIAL_GRADIENT_A_r0_delta_r", GL_FRAGMENT_SHADER)
        .add_source(start+r0_r0_loc, "WRATH_RADIAL_GRADIENT_r0_r0", GL_FRAGMENT_SHADER)
        .add_source(start+p0_x_loc, "WRATH_RADIAL_GRADIENT_p0_x", GL_FRAGMENT_SHADER)
        .add_source(start+p0_y_loc, "WRATH_RADIAL_GRADIENT_p0_y", GL_FRAGMENT_SHADER)
        .add_source(start+A_delta_p_x_loc, "WRATH_RADIAL_GRADIENT_A_delta_p_x", GL_FRAGMENT_SHADER)
        .add_source(start+A_delta_p_y_loc, "WRATH_RADIAL_GRADIENT_A_delta_p_y", GL_FRAGMENT_SHADER);

      /*
        if fragment fetching is possible, then we do NOT support partial
        linear computation since it only saves a dot in the vertex shader
       */
      
    }
  else
    {

      spec
        .add_source(start+A_loc, "WRATH_RADIAL_GRADIENT_A", GL_VERTEX_SHADER)
        .add_source(start+A_r0_delta_r_loc, "WRATH_RADIAL_GRADIENT_A_r0_delta_r", GL_VERTEX_SHADER)
        .add_source(start+r0_r0_loc, "WRATH_RADIAL_GRADIENT_r0_r0", GL_VERTEX_SHADER)
        .add_source(start+p0_x_loc, "WRATH_RADIAL_GRADIENT_p0_x", GL_VERTEX_SHADER)
        .add_source(start+p0_y_loc, "WRATH_RADIAL_GRADIENT_p0_y", GL_VERTEX_SHADER)
        .add_source(start+A_delta_p_x_loc, "WRATH_RADIAL_GRADIENT_A_delta_p_x", GL_VERTEX_SHADER)
        .add_source(start+A_delta_p_y_loc, "WRATH_RADIAL_GRADIENT_A_delta_p_y", GL_VERTEX_SHADER);
    }
}
  
void
WRATHRadialGradientValue::
extract_values_at(int start_index, reorder_c_array<float> out_value)
{
  WRATHGradientValueBase::extract_values_at(start_index, out_value);
  start_index+=WRATHGradientValueBase::number_per_node_values;

  out_value[start_index+A_loc]=m_A;
  out_value[start_index+A_r0_delta_r_loc]=m_A_r0_delta_r;
  out_value[start_index+r0_r0_loc]=m_r0_r0;
  out_value[start_index+p0_x_loc]=m_p0.x();
  out_value[start_index+p0_y_loc]=m_p0.y();
  out_value[start_index+A_delta_p_x_loc]=m_A_delta_p.x();
  out_value[start_index+A_delta_p_y_loc]=m_A_delta_p.y();

}


const WRATHGradientSourceBase*
WRATHRadialGradientValue::
gradient_source(void)
{
  WRATHStaticInit();
  static LocalGradientSource R;
  return &R;
}
