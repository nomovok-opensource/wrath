/*! 
 * \file item.vert.glsl
 * \brief file item.vert.glsl
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


// .xy: location on unit circle
shader_in mediump vec2 circle;

// if 0, on inner ring, if 1 on outer ring
shader_in mediump float ring;

void
shader_main(void)
{
  float r, inner, outer;
  vec2 pos;
  
  inner=fetch_node_value(inner_radius);
  outer=fetch_node_value(outer_radius);

  r=mix(inner, outer, ring);

  pos=r*circle;

  // WRATHShaderBrushSourceHoard defines the macro 
  // WRATH_LINEAR_BRUSH_PRESENT if an donly if it defines 
  // a brush where the brush input coordinates are 
  // determined by the vertex shader.
  #if defined(WRATH_LINEAR_BRUSH_PRESENT)
  {
    /*
      we only change the brush position
      if an image is present too.
     */
    #if defined(WRATH_BRUSH_IMAGE_PRESENT)
    {
      vec2 brush_pos;

      //feed gradient a coordinate so that the
      //image is scaled to the ring. The
      //image size is packed in the image node values
      //WRATH_TEXTURE_subrect_w and WRATH_TEXTURE_subrect_h,
      //BUT those are in normalized coordinates relative
      //to the texture, so the actual image size is given
      //by that multiplied by the size of texture given
      //by wrath_brush_imageTextureSize which is defined
      //by WRATHShaderBrushSourceHoard

      brush_pos= 0.5*pos/outer + vec2(0.5, 0.5); //scaled to [0,1]x[0,1]
      brush_pos*=vec2(fetch_node_value(WRATH_TEXTURE_subrect_w), 
                      fetch_node_value(WRATH_TEXTURE_subrect_h));
      brush_pos*=wrath_brush_imageTextureSize;
      wrath_shader_brush_prepare(brush_pos);
    }
    #else
    {
      wrath_shader_brush_prepare(pos);
    }
    #endif
  }
  #endif

  
  gl_Position= compute_gl_position_and_apply_clipping( vec3(pos, -1.0) );
}
