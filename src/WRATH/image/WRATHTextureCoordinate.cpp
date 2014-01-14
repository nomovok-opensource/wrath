/*! 
 * \file WRATHTextureCoordinate.cpp
 * \brief file WRATHTextureCoordinate.cpp
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
#include "WRATHTextureCoordinate.hpp"

namespace
{
  void
  add_per_node_values_at_implement(int start, 
                                     WRATHLayerNodeValuePackerBase::ActiveNodeValuesCollection &spec,
                                     GLenum shader_stage)
  {
      spec
        .add_source(start+0, "WRATH_TEXTURE_subrect_x", shader_stage)
        .add_source(start+1, "WRATH_TEXTURE_subrect_y", shader_stage)
        .add_source(start+2, "WRATH_TEXTURE_subrect_w", shader_stage)
        .add_source(start+3, "WRATH_TEXTURE_subrect_h", shader_stage);
  }

  class LocalImageSource:public WRATHTextureCoordinateSource
  {
  public:
    LocalImageSource(int xmode, int ymode);

    const WRATHGLShader::shader_source& 
    shader_code(enum precision_t prec, enum interpolation_behaviour_t ibt) const
    {
      return (ibt==linear_computation and m_is_pure_simple)?
        m_simple_shader[prec]:
        m_shader[prec];
    }

    const WRATHGLShader::shader_source&
    pre_compute_shader_code(enum precision_t prec, enum interpolation_behaviour_t ibt) const
    {
      return (ibt==linear_computation and m_is_pure_simple)?
        m_simple_pre_shader[prec]:
        m_pre_shader[prec];
    }

    virtual
    enum interpolation_behaviour_t
    adjust_interpolation_behavior(enum interpolation_behaviour_t ibt) const
    {
      return m_is_pure_simple and ibt==linear_computation?
        linear_computation:
        fully_nonlinear_computation;
    }

    virtual
    const_c_array<std::string>
    global_scoped_symbols(enum precision_t, enum interpolation_behaviour_t) const;

  private:
    static
    std::string
    repeat_function(int mode);

    vecN<WRATHGLShader::shader_source, 3> m_shader, m_pre_shader;
    vecN<WRATHGLShader::shader_source, 3> m_simple_shader, m_simple_pre_shader;
    bool m_is_pure_simple;
  };

  class LocalAllSources
  {
  public:
    enum
      {
        count=WRATHTextureCoordinate::number_modes
      };

    LocalAllSources(void);
    ~LocalAllSources();

    vecN< vecN<LocalImageSource*,count>, count> m_sources;
  };

}

////////////////////////////////////////////////////////////
// LocalImageSource methods
LocalImageSource::
LocalImageSource(int x, int y):
  m_is_pure_simple(x==WRATHTextureCoordinate::simple 
                   and y==WRATHTextureCoordinate::simple)
{
  
  for(int iprec=0; iprec<3; ++iprec)
    {
      enum precision_t prec(static_cast<enum precision_t>(iprec));
      const std::string &prec_as_string(prec_string(prec));

      if(m_is_pure_simple)
        {
          m_simple_shader[iprec].add_macro("WRATH_IMAGE_REPEAT_MODE_VS");
          m_simple_pre_shader[iprec].add_macro("WRATH_IMAGE_REPEAT_MODE_VS");
        }

      m_shader[iprec]
        .add_macro("WRATH_IMAGE_REPEAT_MODE_PREC", prec_as_string)
        .add_source("image-repeat-mode-functions.wrath-shader.glsl", WRATHGLShader::from_resource)
        .add_macro("WRATH_IMAGE_REPEAT_MODE_X", repeat_function(x))
        .add_macro("WRATH_IMAGE_REPEAT_MODE_Y", repeat_function(y))
        .add_source("image-value-normalized-coordinate.compute.wrath-shader.glsl",
                    WRATHGLShader::from_resource)
        .remove_macro("WRATH_IMAGE_REPEAT_MODE_X")
        .remove_macro("WRATH_IMAGE_REPEAT_MODE_Y")
        .remove_macro("WRATH_IMAGE_REPEAT_MODE_PREC");


      m_pre_shader[iprec]
        .add_macro("WRATH_IMAGE_REPEAT_MODE_PREC", prec_as_string)
        .add_source("image-value-normalized-coordinate.pre-compute.wrath-shader.glsl",
                    WRATHGLShader::from_resource)
        .remove_macro("WRATH_IMAGE_REPEAT_MODE_PREC");

      if(m_is_pure_simple)
        {
          m_simple_shader[iprec].absorb(m_shader[iprec]);
          m_simple_pre_shader[iprec].absorb(m_pre_shader[iprec]);

          m_simple_shader[iprec].remove_macro("WRATH_IMAGE_REPEAT_MODE_VS");
          m_simple_pre_shader[iprec].remove_macro("WRATH_IMAGE_REPEAT_MODE_VS");
        }
    }
}



std::string
LocalImageSource::
repeat_function(int mode)
{
  switch(mode)
    {
    default:
      WRATHwarning("unreconized mode " << mode);

    case WRATHTextureCoordinate::simple:
      return "wrath_compute_simple";

    case WRATHTextureCoordinate::clamp:
      return "wrath_compute_clamp";

    case WRATHTextureCoordinate::repeat:
      return "wrath_compute_repeat";

    case WRATHTextureCoordinate::mirror_repeat:
      return "wrath_compute_mirror_repeat";
    }
}

const_c_array<std::string>
LocalImageSource::
global_scoped_symbols(enum precision_t, enum interpolation_behaviour_t) const
{
  static vecN<std::string, 5> values("WRATH_IMAGE_VALUE_NORMALIZED_varying",
                                     "wrath_compute_simple",
                                     "wrath_compute_repeat",
                                     "wrath_compute_clamp",
                                     "wrath_compute_mirror_repeat");
  return values;
}


//////////////////////////////////////////
// LocalAllSources methods
LocalAllSources::
LocalAllSources(void)
{
  for(int x=0; x<count; ++x)
    {
      for(int y=0; y<count; ++y)
        {
          m_sources[x][y]=WRATHNew LocalImageSource(x,y);
        }
    } 
}

LocalAllSources::
~LocalAllSources(void)
{
  for(int x=0; x<count; ++x)
    {
      for(int y=0; y<count; ++y)
        {
          WRATHDelete(m_sources[x][y]);
        }
    } 
}


////////////////////////////////////////////////////////
// WRATHTextureCoordinate methods
void
WRATHTextureCoordinate::
add_per_node_values_at(int start, WRATHLayerNodeValuePackerBase::ActiveNodeValuesCollection &spec,
                         const WRATHLayerNodeValuePackerBase::function_packet &func)
{    
  add_per_node_values_at_implement(start, spec, GL_VERTEX_SHADER);
  if(func.supports_per_node_value(GL_FRAGMENT_SHADER))
    {
      add_per_node_values_at_implement(start, spec, GL_FRAGMENT_SHADER);
    }
    
}

void
WRATHTextureCoordinate::
extract_values_at(int start, reorder_c_array<float> out_value)
{
  out_value[start+0]=m_minx_miny.x();
  out_value[start+1]=m_minx_miny.y();

  out_value[start+2]=m_wh.x();
  out_value[start+3]=m_wh.y();
}


const WRATHTextureCoordinateSourceBase*
WRATHTextureCoordinate::
source(enum repeat_mode_type repeat_mode_x, 
       enum repeat_mode_type repeat_mode_y)
{
  WRATHStaticInit();
  static LocalAllSources R;
  return R.m_sources[repeat_mode_x][repeat_mode_y];
}

void
WRATHTextureCoordinate::
set(const WRATHImage *image,
    const ivec2 &pminx_miny, const ivec2 &pwh,
    bool crop_x, bool crop_y)
{
  if(image!=NULL)
    {
      m_minx_miny=vec2(image->minX_minY() + pminx_miny);  
      m_wh=vec2(pwh);

      if(crop_x and pminx_miny.x()==0 and image->boundary_size().m_minX==0)
        {
          m_minx_miny.x()+=1.0f;
          m_wh.x()-=1.0f;
        }

      if(crop_x and pminx_miny.x()+pwh.x()==image->size().x() and image->boundary_size().m_maxX==0)
        {
          m_wh.x()-=1.0f;
        }

      if(crop_y and pminx_miny.y()==0 and image->boundary_size().m_minY==0)
        {
          m_minx_miny.y()+=1.0f;
          m_wh.y()-=1.0f;
        }

      if(crop_y and pminx_miny.y()+pwh.y()==image->size().y() and image->boundary_size().m_maxY==0)
        {
          m_wh.y()-=1.0f;
        }
      m_minx_miny/=vec2(image->atlas_size());
      m_wh/=vec2(image->atlas_size());
    }
}


void
WRATHTextureCoordinate::
set(const WRATHImage *image, bool crop_x, bool crop_y)
{
  ivec2 wh(0, 0);

  if(image!=NULL)
    {
      wh=image->size();
    }
  set(image, ivec2(0, 0), wh, crop_x, crop_y);
}
