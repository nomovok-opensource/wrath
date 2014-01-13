/*! 
 * \file simple_ui_font.vert.wrath-shader.glsl
 * \brief file simple_ui_font.vert.wrath-shader.glsl
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
  TODO:
    1) Support mix font shaders
 */

uniform mediump float animation_fx_interpol;
uniform mediump mat2 animation_matrix;


/*pos meanings:
 .xy: location before transformation of bottom left of glyph
 .z : z-transformation the z value to feed to transformation matrix
 .w : scaling factor that created the position.
*/
shader_in highp vec4 pos;


/*
  additional stretching to apply to glyph
  .x stretching in x
  .y stretching in y
  Note that thus the total stretch in x is glyph_stretch.x*pos.w
  Note that thus the total stretch in y is glyph_stretch.y*pos.w
 */
shader_in highp vec2 glyph_stretch;


/*
  size of glyph in _pixels_ on the texture holding the glyph.
  .xy: glyph size in texels
  .zw: bottom left corner in texel of glyph on texture page
 */
shader_in highp vec4 glyph_size_and_bottom_left_texel;

/*
  normalized coordinate within the glyph
 */
shader_in highp vec2 glyph_normalized_coordinate;

/*
  Color of the glyph, 
   comment: when clipping via normalized coordinates
   is active, we do internally interpolate the color,
   to so would require a whopping 4 shader_ins!
   [if we made it so that the color has a start
   at the bottom left and an end at the top right we
   would then need 2 shader_ins]. Since we are drawing
   letters only those that look *very carefully* with
   modulated colored text will notice that we are
   not doing the right thing.
 */
shader_in mediump vec4 color;

/*
  color
 */
shader_out mediump vec4 tex_color;




void
shader_main(void)
{
  highp vec2 tpos, offset;
  highp vec2 glyph_position;
  highp vec2 abs_glyph_normalized_coordinate, clipped_normalized;
  
  clipped_normalized=
     compute_clipped_normalized_coordinate(glyph_normalized_coordinate,
                                           pos.xy, 
                                           pos.w*glyph_size_and_bottom_left_texel.xy*glyph_stretch.xy);

  abs_glyph_normalized_coordinate=abs(clipped_normalized);

  //position of vertex inside of glyph
  glyph_position=abs_glyph_normalized_coordinate*glyph_size_and_bottom_left_texel.xy;

  
  #if defined(WRATH_FONT_CUSTOM_DATA)
  {
    wrath_font_custom_data_t custom_values_str;
  
    wrath_font_shader_custom_data_func(custom_values_str);
    pre_compute_glyph(glyph_position,
                      glyph_size_and_bottom_left_texel.zw,
                      glyph_size_and_bottom_left_texel.xy,
                      custom_values_str.values);
  }  
  #else
  {
    pre_compute_glyph(glyph_position,
                      glyph_size_and_bottom_left_texel.zw,
                      glyph_size_and_bottom_left_texel.xy);
  }
  #endif
  
  offset=pos.w*clipped_normalized*glyph_size_and_bottom_left_texel.xy*glyph_stretch.xy;
  offset.xy*= mix(1.0, 3.0, animation_fx_interpol);
  offset.x -= mix(0.0, 1.0, animation_fx_interpol)*offset.x;
  offset=animation_matrix*offset*0.5 + offset*0.5;

  tpos=pos.xy + offset;
  gl_Position=compute_gl_position(vec3(tpos, pos.z));
  tex_color=color;

  #if defined(WRATH_APPLY_BRUSH_RELATIVE_TO_LETTER)
  {
    wrath_shader_brush_prepare(glyph_position);
  }
  #elif defined(WRATH_APPLY_BRUSH_RELATIVE_TO_ITEM)
  {
    wrath_shader_brush_prepare(tpos);    
  }
  #endif
}
