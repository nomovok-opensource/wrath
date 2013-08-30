/*! 
 * \file wrath-brush.vert.wrath-shader.glsl
 * \brief file wrath-brush.vert.wrath-shader.glsl
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


/*
  "control flow" via #if macros.
  
  Some notes:
   if NONLINEAR_BRUSH_PRESENT is defined, then 
   both LINEAR_GRADIENT and LINEAR_TEXTURE_COORDINATE
   should NOT be defined.
 */
#ifdef NONLINEAR_BRUSH_PRESENT
  #ifdef LINEAR_GRADIENT
  #error "LINEAR_GRADIENT defined with NONLINEAR_BRUSH_PRESENT defined"
  #endif

  #ifdef LINEAR_TEXTURE_COORDINATE
  #error "LINEAR_TEXTURE_COORDINATE defined with NONLINEAR_BRUSH_PRESENT defined"
  #endif
#endif

#ifdef LINEAR_GRADIENT
  #ifdef WRATH_GL_FRAGMENT_SHADER_ITEM_VALUE_FETCH_OK
     shader_out mediump float wrath_brush_tex_coord;
  #else
    shader_out mediump vec2 wrath_brush_tex_coord;
  #endif
#endif

#ifdef NON_LINEAR_GRADIENT

  #ifndef NONLINEAR_BRUSH_PRESENT
    #ifdef WRATH_GL_FRAGMENT_SHADER_ITEM_VALUE_FETCH_OK
       shader_out mediump vec2 wrath_brush_frag_pos;
    #else
       shader_out mediump vec3 wrath_brush_frag_pos;
       #define wrath_brush_grad_tex_y wrath_brush_frag_pos.z
    #endif
    #define FRAG_POS_DEFINED
  #else
    #ifndef WRATH_GL_FRAGMENT_SHADER_ITEM_VALUE_FETCH_OK
       shader_out mediump float wrath_brush_grad_tex_y;
    #endif
  #endif

#endif

#if defined(LINEAR_TEXTURE_COORDINATE) || defined(NON_LINEAR_TEXTURE_COORDINATE)
  #ifndef NONLINEAR_BRUSH_PRESENT
    shader_out mediump vec2 wrath_brush_image_tex_coord;
  #endif
uniform mediump vec2 wrath_brush_imageTextureSize; //let it be available to vertex shader too
#endif

#if defined(NON_LINEAR_TEXTURE_COORDINATE) && !defined(NON_LINEAR_GRADIENT) && !defined(NONLINEAR_BRUSH_PRESENT)
  shader_out mediump vec2 wrath_brush_frag_pos;
  #define FRAG_POS_DEFINED
#endif

#if defined(CONST_COLOR_VS) && !defined(CONST_COLOR_FS)
shader_out CONST_COLOR_PREC vec4 wrath_brush_const_color;
#endif

#ifdef NONLINEAR_BRUSH_PRESENT
void wrath_shader_brush_prepare(void)
#else
void wrath_shader_brush_prepare(in vec2 highp p)
#endif
{
  #if !defined(FRAG_POS_DEFINED) && !defined(NONLINEAR_BRUSH_PRESENT)
    highp vec2 wrath_brush_frag_pos;
  #endif

  #ifndef NONLINEAR_BRUSH_PRESENT
  {
    wrath_brush_frag_pos.xy=p;
  }
  #endif
  
  #ifdef LINEAR_TEXTURE_COORDINATE
  {
    wrath_brush_image_tex_coord=compute_texture_coordinate(wrath_brush_frag_pos.xy/wrath_brush_imageTextureSize);
  }
  #elif defined(NON_LINEAR_TEXTURE_COORDINATE)
  {
    #ifndef NONLINEAR_BRUSH_PRESENT
    {
      wrath_brush_image_tex_coord=wrath_brush_frag_pos/wrath_brush_imageTextureSize;
    }
    #endif

    #ifdef FULLY_NON_LINEAR_TEXTURE_COORDINATE
    {
      pre_compute_texture_coordinate();
    }
    #else
    {
      pre_compute_texture_coordinate(image_tex_coord);
    }
    #endif
  }
  #endif

  #ifdef LINEAR_GRADIENT
  {
    #ifndef WRATH_GL_FRAGMENT_SHADER_ITEM_VALUE_FETCH_OK
    {
      wrath_brush_tex_coord.x=compute_gradient(wrath_brush_frag_pos);
      wrath_brush_tex_coord.y=fetch_node_value(WRATH_GRADIENT_y_coordinate);
    }
    #else
    {
      wrath_brush_tex_coord=compute_gradient(wrath_brush_frag_pos);
    }
    #endif
  }
  #elif defined(NON_LINEAR_GRADIENT)
  {
    #ifndef WRATH_GL_FRAGMENT_SHADER_ITEM_VALUE_FETCH_OK
    {
      wrath_brush_grad_tex_y=fetch_node_value(WRATH_GRADIENT_y_coordinate);
    }
    #endif

    #ifdef FULLY_NON_LINEAR_GRADIENT
    {
      pre_compute_gradient();
    }
    #else
    {
      pre_compute_gradient(wrath_brush_frag_pos.xy);
    }
    #endif 
  }
  #endif

  #if defined(CONST_COLOR_VS) && !defined(CONST_COLOR_FS)
  {
    wrath_brush_const_color=const_color_value();
  }
  #endif  
}
