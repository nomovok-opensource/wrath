/*! 
 * \file simple_ui_font.vert.glsl
 * \brief file simple_ui_font.vert.glsl
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
  .xy: native glyph size
  .zw: minified glyph size
 */
shader_in TEX_ATTRIBUTE_TYPE vec4 glyph_size;

/*
  bottom left corner in _pixels_ of texture data
  for glyph
 */
shader_in TEX_ATTRIBUTE_TYPE vec4 glyph_bottom_left_texel;

/*
  glyph normalized coordiante, bottom left is (0,0)
  and top right is (1,1).
 */
shader_in TEX_ATTRIBUTE_TYPE vec2 glyph_normalized_coordinate;

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

/*!
  color
 */
shader_out mediump vec4 tex_color;


#if defined(MIX_FONT_SHADER) 
/*
  normalized texture coordinate 
  .xy for non-minified glyph
  .zw for minified glyph
 */
shader_out TEX_VARYING_TYPE vec4 tex_coord;

/*
  relative coordinate within the glyph NOT normalized
 */
shader_out TEX_VARYING_TYPE vec4 relative_coord;

/*
  reciprocal of the texture holding the glyph
  [0] --> for native or magnified
  [1] --> for minified
 */
uniform TEX_RECIP_TYPE vec2 reciprocal_texture_size[2];

#else
/*
  reciprocal of the texture holding the glyph,
  native only
*/
uniform TEX_RECIP_TYPE vec2 reciprocal_texture_size;
shader_out TEX_VARYING_TYPE vec2 tex_coord;
shader_out TEX_VARYING_TYPE vec2 relative_coord;
#endif



/*
  relative coordinate within the glyph as normalized
 */
shader_out TEX_VARYING_TYPE vec2 GlyphNormalizedCoordinate;

/*
  glyph_index for use in detailed coverage fonts
 */
shader_in mediump float in_glyph_index;
shader_out mediump float GlyphIndex;

void
shader_main(void)
{
  highp vec2 tpos, offset;
  highp vec4 unnormalized_tex;
  highp vec2 abs_glyph_normalized_coordinate, clipped_normalized;
  
  tex_color=color;

  clipped_normalized=
     compute_clipped_normalized_coordinate(glyph_normalized_coordinate,
                                           pos.xy, pos.w*glyph_size.xy*glyph_stretch.xy);

  abs_glyph_normalized_coordinate=abs(clipped_normalized);
  GlyphNormalizedCoordinate=abs_glyph_normalized_coordinate;

  GlyphIndex=in_glyph_index;
  relative_coord.xy=abs_glyph_normalized_coordinate*glyph_size.xy;
  unnormalized_tex.xy= relative_coord.xy + glyph_bottom_left_texel.xy;

  
  offset=pos.w*clipped_normalized*glyph_size.xy*glyph_stretch.xy;

#if defined(MIX_FONT_SHADER) 
  unnormalized_tex.zw= 
    abs_glyph_normalized_coordinate*glyph_size.zw + glyph_bottom_left_texel.zw;

  tex_coord.xy=reciprocal_texture_size[0]*unnormalized_tex.xy; 
  tex_coord.zw=reciprocal_texture_size[1]*unnormalized_tex.zw; 

  relative_coord.zw=abs_glyph_normalized_coordinate*glyph_size.zw;
#else
  tex_coord.xy=reciprocal_texture_size*unnormalized_tex.xy; 
#endif
  
  //scale from the center of the glyph:
  offset.xy*= mix(1.0, 3.0, animation_fx_interpol);
  offset.x -= mix(0.0, 1.0, animation_fx_interpol)*offset.x;
  offset=animation_matrix*offset*0.5 + offset*0.5;
 
  tpos=pos.xy + offset;
  gl_Position=compute_gl_position(vec3(tpos, pos.z));
}
