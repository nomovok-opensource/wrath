/*! 
 * \file WRATHRepeatGradientValue.cpp
 * \brief file WRATHRepeatGradientValue.cpp
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



#include "WRATHRepeatGradientValue.hpp"
#include "WRATHStaticInit.hpp"

namespace 
{
  class LocalGradientSource:public WRATHGradientSourceBase
  {
  public:
    LocalGradientSource(const WRATHGradientSourceBase *src):
      m_src(src)
    {}

    enum interpolation_behaviour_t
    adjust_interpolation_behavior(enum interpolation_behaviour_t) const
    {
      return WRATHGradientSource::fully_nonlinear_computation;
    }

    
    bool
    gradient_always_valid(void) const
    {
      return m_src->gradient_always_valid();
    }

  protected:
    
    virtual
    void
    add_shader_source_code_specify_interpolation_implementation(enum interpolation_behaviour_t ibt,
                                                                std::map<GLenum, WRATHGLShader::shader_source> &src,
                                                                enum precision_t prec,
                                                                const std::string &suffix) const;

    const WRATHGradientSourceBase *m_src;
  };

  class LocalGradientSourceStorage
  {
  public:

    ~LocalGradientSourceStorage();

    const WRATHGradientSourceBase*
    fetch(const WRATHGradientSourceBase *gr);

  private:
    typedef std::map<const WRATHGradientSourceBase*, WRATHGradientSourceBase*> map_type;

    map_type m_values;
    WRATHMutex m_mutex;
  };
 
}

//////////////////////////////////////////////////
// LocalGradientSourceStorage methods
LocalGradientSourceStorage::
~LocalGradientSourceStorage()
{
  for(map_type::iterator iter=m_values.begin(), 
        end=m_values.end(); iter!=end; ++iter)
    {
      WRATHDelete(iter->second);
    }
}

const WRATHGradientSourceBase*
LocalGradientSourceStorage::
fetch(const WRATHGradientSourceBase *gr)
{
  WRATHAutoLockMutex(m_mutex);
  map_type::const_iterator iter;
  
  iter=m_values.find(gr);
  if(iter==m_values.end())
    {
      WRATHGradientSourceBase *new_value;
      
      new_value=WRATHNew LocalGradientSource(gr);
      m_values[gr]=new_value;
      return new_value;
    }
  
  return iter->second;
}


///////////////////////////////////////////////
// LocalGradientSource methods
void
LocalGradientSource::
add_shader_source_code_specify_interpolation_implementation(enum interpolation_behaviour_t ibt,
                                                            std::map<GLenum, WRATHGLShader::shader_source> &src,
                                                            enum precision_t prec,
                                                            const std::string &suffix) const
{
  WRATHassert(ibt==WRATHGradientSource::fully_nonlinear_computation);
  /*
    TODO: other shader stages?
   */
  WRATHGLShader::shader_source &vs(src[GL_VERTEX_SHADER]);
  WRATHGLShader::shader_source &fs(src[GL_FRAGMENT_SHADER]);

  std::string suffix_for_src;    
  std::string varying_label;
  std::string underlying_pre_compute, underlying_compute;

  suffix_for_src=suffix + "_underlying_gradient";
  varying_label="WRATH_GRADIENT_varying_window" + suffix;
  underlying_pre_compute="wrath_pre_compute_gradient"+suffix_for_src;
  underlying_compute="wrath_compute_gradient"+suffix_for_src;
  
    
  /*
    add the shader code from the underlying source,
    note that we force the interpolation to be
    FULLY non-linear because we tweak the input coordinate
    for the gradient computation at the fragment shader
    level. 
  */
  m_src->add_shader_source_code_specify_interpolation(ibt, src, prec, suffix_for_src);
  WRATHassert(ibt==WRATHGradientSource::fully_nonlinear_computation);

  /*
    add our code to:
      1) assign the varying the window coordinates
      2) compute the gradient.    
   */

  /*
    vertex shader code
   */
  vs
    .add_macro("WRATH_REPEAT_GRADIENT_PREC", WRATHBaseSource::prec_string(prec))
    .add_macro("WRATH_REPEAT_GRADIENT_UNDERLYING_GRADIENT_pre_compute", underlying_pre_compute)
    .add_macro("WRATH_REPEAT_GRADIENT_pre_compute", "wrath_pre_compute_gradient"+suffix)
    .add_macro("WRATH_REPEAT_VARYING_LABEL", varying_label)
    .add_source("repeat-gradient.pre-compute.wrath-shader.glsl",
                WRATHGLShader::from_resource)
    .add_source("\n#undef WRATH_REPEAT_GRADIENT_PREC", WRATHGLShader::from_string)
    .add_source("\n#undef WRATH_REPEAT_GRADIENT_UNDERLYING_GRADIENT_pre_compute", WRATHGLShader::from_string)
    .add_source("\n#undef WRATH_REPEAT_GRADIENT_pre_compute", WRATHGLShader::from_string)
    .add_source("\n#undef WRATH_REPEAT_VARYING_LABEL\n\n", WRATHGLShader::from_string);
  

  /*
    fragment shader code
   */
  fs
    .add_macro("WRATH_REPEAT_GRADIENT_PREC", WRATHBaseSource::prec_string(prec))
    .add_macro("WRATH_REPEAT_GRADIENT_UNDERLYING_GRADIENT_compute", underlying_compute)
    .add_macro("WRATH_REPEAT_GRADIENT_compute", "wrath_compute_gradient"+suffix)
    .add_macro("WRATH_REPEAT_VARYING_LABEL", varying_label)
    .add_source("repeat-gradient.wrath-shader.glsl",
                WRATHGLShader::from_resource)
    .add_source("\n#undef WRATH_REPEAT_GRADIENT_PREC", WRATHGLShader::from_string)
    .add_source("\n#undef WRATH_REPEAT_GRADIENT_UNDERLYING_GRADIENT_compute", WRATHGLShader::from_string)
    .add_source("\n#undef WRATH_REPEAT_GRADIENT_compute", WRATHGLShader::from_string)
    .add_source("\n#undef WRATH_REPEAT_VARYING_LABEL\n\n", WRATHGLShader::from_string);
}


///////////////////////////////////////////////////
//WRATHRepeatGradientValue  methods
const WRATHGradientSourceBase*
WRATHRepeatGradientValue::
gradient_source(const WRATHGradientSourceBase *src)
{
  WRATHStaticInit();
  static LocalGradientSourceStorage R;
  return R.fetch(src);
}

void
WRATHRepeatGradientValue::
add_per_node_values_at(int start, 
                         WRATHLayerNodeValuePackerBase::ActiveNodeValuesCollection &spec,
                         const WRATHLayerNodeValuePackerBase::function_packet &fpt)
{
  if(fpt.supports_per_node_value(GL_FRAGMENT_SHADER))
    {
     
      spec
        .add_source(start+0, "WRATH_GRADIENT_window_x", GL_FRAGMENT_SHADER)
        .add_source(start+1, "WRATH_GRADIENT_window_y", GL_FRAGMENT_SHADER)
        .add_source(start+2, "WRATH_GRADIENT_window_delta_x", GL_FRAGMENT_SHADER)
        .add_source(start+3, "WRATH_GRADIENT_window_delta_y", GL_FRAGMENT_SHADER);
    }
  else
    {
      spec
        .add_source(start+0, "WRATH_GRADIENT_window_x", GL_VERTEX_SHADER)
        .add_source(start+1, "WRATH_GRADIENT_window_y", GL_VERTEX_SHADER)
        .add_source(start+2, "WRATH_GRADIENT_window_delta_x", GL_VERTEX_SHADER)
        .add_source(start+3, "WRATH_GRADIENT_window_delta_y", GL_VERTEX_SHADER);
    }
}

void
WRATHRepeatGradientValue::
extract_values_at(int start, reorder_c_array<float> out_value)
{
  out_value[start+0]=m_start_window.x();
  out_value[start+1]=m_start_window.y();
  out_value[start+2]=m_end_window.x() - m_start_window.x();
  out_value[start+3]=m_end_window.y() - m_start_window.y();
}
