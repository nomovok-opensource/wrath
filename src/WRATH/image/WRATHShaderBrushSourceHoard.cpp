/*! 
 * \file WRATHShaderBrushSourceHoard.cpp
 * \brief file WRATHShaderBrushSourceHoard.cpp
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
#include "WRATHShaderBrushSourceHoard.hpp"

/*
  TODO:
    1) make all macro's prefix with WRATH_BRUSH_...
    2) add #undefs for all added macros that are not described
       in the doxytag of WRATHShaderBrushSourceHoard
 */

namespace
{
  const char *image_texture_name="wrath_brush_imageTexture";
  const char *gradient_texture_name="wrath_brush_gradientTexture";
  

  void
  add_flag(bool b, const char *macro, WRATHGLShader::shader_source &dest)
    {
      if(b)
	{
          dest.add_macro(macro);
	}
    }

  void
  filter_brush(WRATHShaderBrush &brush,
               enum WRATHShaderBrushSourceHoard::brush_mapping_t brush_mapping)
  {
    /*
      a number of flags only make sense
      if there are associated sources
     */
    if(brush.m_gradient_source==NULL)
      {
        brush.gradient_alpha_test(false);
        brush.gradient_interpolate_enforce_positive(false);
        brush.gradient_interpolate_enforce_greater_than_one(false);
        brush.gradient_interpolate_enforce_by_blend(false);
      }

    if(brush.m_texture_coordinate_source==NULL)
      {
        brush.image_alpha_test(false);
        brush.flip_image_y(false);
      }

    if(brush.m_color_value_source==NULL)
      {
        brush.color_alpha_test(false);
      }

    if(brush_mapping==WRATHShaderBrushSourceHoard::nonlinear_brush_mapping)
      {
        if(brush.m_texture_coordinate_source!=NULL)
          {
            brush.m_texture_coordinate_source=brush.m_texture_coordinate_source->non_linear_facade();
          }

        if(brush.m_gradient_source!=NULL)
          {
            brush.m_gradient_source=brush.m_gradient_source->non_linear_facade();
          }
      }
  }

  
  void
  append_macros_worker(WRATHGLShader::shader_source &dest,
                       const WRATHShaderBrush &brush,
                       enum WRATHShaderBrushSourceHoard::brush_mapping_t brush_mapping)
  {
    add_flag(brush.anti_alias(), "AA_HINT", dest);
    add_flag(brush.image_alpha_test(), "IMAGE_ALPHA_TEST", dest);
    add_flag(brush.gradient_alpha_test(), "GRADIENT_ALPHA_TEST", dest);
    add_flag(brush.color_alpha_test(), "CONST_COLOR_ALPHA_TEST", dest);
    add_flag(brush.final_color_alpha_test(), "FINAL_ALPHA_TEST", dest);
    add_flag(brush.premultiply_alpha(), "PREMULTIPLY_ALPHA", dest);
    add_flag(brush.gradient_interpolate_enforce_positive(), 
             "GRADIENT_INTERPOLATE_RANGE_ENFORCE_POSITIVE", dest);
    add_flag(brush.gradient_interpolate_enforce_greater_than_one(), 
             "GRADIENT_INTERPOLATE_RANGE_ENFORCE_LESS_THAN_ONE", dest);
    add_flag(brush.gradient_interpolate_enforce_by_blend(), 
             "GRADIENT_INTERPOLATE_ENFORCE_BLEND", dest);
    add_flag(brush.flip_image_y(), "FLIP_IMAGE_Y", dest);
    
    if(brush.m_gradient_source!=NULL)
      {
        dest.add_macro("BRUSH_GRADIENT_PRESENT");
      }
    
    if(brush.m_texture_coordinate_source!=NULL)
      {
        dest.add_macro("BRUSH_IMAGE_PRESENT");
      }
    
    if(brush_mapping==WRATHShaderBrushSourceHoard::linear_brush_mapping)
      {
        dest.add_macro("LINEAR_BRUSH_PRESENT");
      }
    else if(brush_mapping==WRATHShaderBrushSourceHoard::nonlinear_brush_mapping)
      {
        dest.add_macro("NONLINEAR_BRUSH_PRESENT");
      }
    
    if(brush.m_texture_coordinate_source!=NULL)
      {
        dest.add_macro("BRUSH_IMAGE_PRESENT");
      }
    
    if(brush.m_gradient_source!=NULL)
      {
        dest.add_macro("BRUSH_GRADIENT_PRESENT");
      }
    
    if(brush.m_gradient_source!=NULL)
      {
        dest.add_macro("BRUSH_COLOR_PRESENT");
      }
  }
  
}

/////////////////////////////////////////////////
//WRATHShaderBrushSourceHoard methods
WRATHShaderBrushSourceHoard::
WRATHShaderBrushSourceHoard(const std::map<GLenum, WRATHGLShader::shader_source> &src,
                            uint32_t custom_mask,
                            uint32_t mask,
                            const ModifyShaderSpecifierBase::const_handle &hnd):
  m_src(src),
  m_custom_bit_mask(custom_mask),
  m_bit_mask(mask),
  m_modifier(hnd)
{}

WRATHShaderBrushSourceHoard::
WRATHShaderBrushSourceHoard(const WRATHGLShader::shader_source &vertex_shader,
                            const WRATHGLShader::shader_source &fragment_shader,
                            uint32_t custom_mask,
                            uint32_t mask,
                            const ModifyShaderSpecifierBase::const_handle &hnd):
  m_custom_bit_mask(custom_mask),
  m_bit_mask(mask),
  m_modifier(hnd)
{
  m_src[GL_VERTEX_SHADER]=vertex_shader;
  m_src[GL_FRAGMENT_SHADER]=fragment_shader;
}



WRATHShaderBrushSourceHoard::
~WRATHShaderBrushSourceHoard()
{
  for(map_type::iterator iter=m_shaders.begin(), end=m_shaders.end(); iter!=end; ++iter)
    {
      WRATHDelete(iter->second);
    }

  for(font_map_type::iterator iter=m_font_shaders.begin(), end=m_font_shaders.end(); iter!=end; ++iter)
    {
      WRATHDelete(iter->second);
    }
}

const WRATHFontShaderSpecifier&
WRATHShaderBrushSourceHoard::
fetch_font_shader(const WRATHShaderBrush &pbrush, enum WRATHBaseSource::precision_t prec,
                  enum brush_mapping_t brush_mapping) const
{
  key_type K(pbrush, prec, brush_mapping);
  font_map_type::iterator iter;

  K.get<0>().m_bits&=m_bit_mask;
  K.get<0>().m_custom_bits&=m_custom_bit_mask;
  filter_brush(K.get<0>(), brush_mapping);

  const WRATHShaderBrush &brush(K.get<0>());

  WRATHAutoLockMutex(m_font_mutex);
  iter=m_font_shaders.find(K);
  if(iter!=m_font_shaders.end())
    {
      return *iter->second;
    }

  WRATHFontShaderSpecifier *p;
  unsigned int gradient_unit(0);

  p=WRATHNew WRATHFontShaderSpecifier();
  for(std::map<GLenum, WRATHGLShader::shader_source>::const_iterator
        iter=m_src.begin(), end=m_src.end(); iter!=end; ++iter)
    {
      WRATHGLShader::shader_source &dest(p->append_shader_source(iter->first));

      append_macros_worker(dest, brush, brush_mapping);
      add_custom_macros(dest, K.get<0>().m_custom_bits);
    }

  if(brush.m_texture_coordinate_source!=NULL)
    {
      p->add_shader_source_code(brush.m_texture_coordinate_source, prec);
      p->add_sampler(0, image_texture_name);
      ++gradient_unit;
    }

  if(brush.m_gradient_source!=NULL)
    {
      p->add_shader_source_code(brush.m_gradient_source, prec);
      p->add_sampler(gradient_unit, gradient_texture_name);
    }

  if(brush.m_color_value_source!=NULL)
    {
      p->add_shader_source_code(brush.m_color_value_source, prec);
    }

  if(brush_mapping!=no_brush_function)
    {
      p->append_vertex_shader_source().add_source("wrath-brush.vert.wrath-shader.glsl", 
                                                  WRATHGLShader::from_resource);
      p->append_fragment_shader_source().add_source("wrath-brush.frag.wrath-shader.glsl", 
                                                    WRATHGLShader::from_resource);
    }

  for(std::map<GLenum, WRATHGLShader::shader_source>::const_iterator
	iter=m_src.begin(), end=m_src.end(); iter!=end; ++iter)
    {
      p->append_shader_source(iter->first).absorb(iter->second);
    }
  
  if(m_modifier.valid())
    {
      m_modifier->modify_shader(*p, K.get<0>(), K.get<1>(), K.get<2>());
    }
  
  m_font_shaders[K]=p;
  return *p;
}

const WRATHShaderSpecifier&
WRATHShaderBrushSourceHoard::
fetch(const WRATHShaderBrush &pbrush, enum WRATHBaseSource::precision_t prec, 
      enum brush_mapping_t brush_mapping) const
{
  key_type K(pbrush, prec, brush_mapping);
  map_type::iterator iter;

  K.get<0>().m_bits&=m_bit_mask;
  K.get<0>().m_custom_bits&=m_custom_bit_mask;
  filter_brush(K.get<0>(), brush_mapping);

  const WRATHShaderBrush &brush(K.get<0>());

  WRATHAutoLockMutex(m_mutex);
  iter=m_shaders.find(K);
  if(iter!=m_shaders.end())
    {
      return *iter->second;
    }

  WRATHShaderSpecifier *p;
  unsigned int gradient_texture_unit(0);

  p=WRATHNew WRATHShaderSpecifier();
  for(std::map<GLenum, WRATHGLShader::shader_source>::const_iterator
        iter=m_src.begin(), end=m_src.end(); iter!=end; ++iter)
    {
      WRATHGLShader::shader_source &dest(p->append_shader_source(iter->first));

      append_macros_worker(dest, brush, brush_mapping);
      add_custom_macros(dest, K.get<0>().m_custom_bits);
    }
 
  if(brush.m_texture_coordinate_source!=NULL)
    {
      p->add_shader_source_code(brush.m_texture_coordinate_source, prec);
      p->append_initializers().add_sampler_initializer(image_texture_name, 0);
      p->append_bindings().add_texture_binding(GL_TEXTURE0);
      ++gradient_texture_unit;
    }

  if(brush.m_gradient_source!=NULL)
    {
      p->add_shader_source_code(brush.m_gradient_source, prec);
      p->append_initializers().add_sampler_initializer(gradient_texture_name, gradient_texture_unit);
      p->append_bindings().add_texture_binding(GL_TEXTURE0+gradient_texture_unit);
    }

  if(brush.m_color_value_source!=NULL)
    {
      p->add_shader_source_code(brush.m_color_value_source, prec);
    }

  if(brush_mapping!=no_brush_function)
    {
      p->append_vertex_shader_source().add_source("wrath-brush.vert.wrath-shader.glsl", 
                                                  WRATHGLShader::from_resource);
      p->append_fragment_shader_source().add_source("wrath-brush.frag.wrath-shader.glsl", 
                                                    WRATHGLShader::from_resource);
    }

  for(std::map<GLenum, WRATHGLShader::shader_source>::const_iterator
	iter=m_src.begin(), end=m_src.end(); iter!=end; ++iter)
    {
      p->append_shader_source(iter->first).absorb(iter->second);
    }
  
  if(m_modifier.valid())
    {
      m_modifier->modify_shader(*p, K.get<0>(), K.get<1>(), K.get<2>());
    }

  m_shaders[K]=p;
  return *p;
}

void
WRATHShaderBrushSourceHoard::
add_custom_macros(WRATHGLShader::shader_source &, uint32_t) const
{}

void
WRATHShaderBrushSourceHoard::
add_state(const WRATHBrush &brush, WRATHSubItemDrawState &subkey) const
{
  WRATHassert(brush.consistent());
  GLenum grad_unit(GL_TEXTURE0);

  /*
    image and gradient take precendence.
   */
  subkey.absorb(brush.m_draw_state);

  if(brush.m_image!=NULL)
    {
      ++grad_unit;
      subkey
        .add_texture(GL_TEXTURE0, brush.m_image->texture_binder(0))
        .add_uniform(brush.m_image->texture_binder(0)->texture_size(image_texture_name));
    }

  if(brush.m_gradient!=NULL)
    {
      subkey
        .add_texture(grad_unit, brush.m_gradient->texture_binder());
    }
  
}
