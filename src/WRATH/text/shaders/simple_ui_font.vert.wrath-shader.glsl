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

  /*
    GlyphIndex is used as a texture coordinate
    where the texture size is 256. It is set
    in C++ code as the non-normalized coordinate,
    i.e. actual texel. Now for something interesting:

    Let I=texel you want, then the normalized
    texel coordinate to use is given by:

    (I+0.5)/256

    Let B be the value (as a byte) at in_glyph_index.
    The value of in_glyph_index is B/255, call it f.

    Then the texel coordinate we wish to use is:

    t=(255*f+0.5)/256.

    As a side note, 

    |f-t| = |f/256 - 0.5/256| and we need
    that to be no more than 1/512:

    thus we want:

    |f-0.5| < 0.5

    which is true for 0<f<1, but, round off
    issues make this dicey
   */
  GlyphIndex=(255.0*in_glyph_index+0.5)/256.0;

  relative_coord.xy=abs_glyph_normalized_coordinate*glyph_size.xy;
  unnormalized_tex.xy= relative_coord.xy + glyph_bottom_left_texel.xy;

  
  offset=pos.w*clipped_normalized*glyph_size.xy*glyph_stretch.xy;

  #if defined(MIX_FONT_SHADER) 
  {
    unnormalized_tex.zw= 
      abs_glyph_normalized_coordinate*glyph_size.zw + glyph_bottom_left_texel.zw;
    
    tex_coord.xy=reciprocal_texture_size[0]*unnormalized_tex.xy; 
    tex_coord.zw=reciprocal_texture_size[1]*unnormalized_tex.zw; 
    
    relative_coord.zw=abs_glyph_normalized_coordinate*glyph_size.zw;
  }
  #else
  {
    tex_coord.xy=reciprocal_texture_size*unnormalized_tex.xy; 
  }
  #endif
 
  tpos=pos.xy + offset;
  gl_Position=compute_gl_position(vec3(tpos, pos.z));

  #if defined(APPLY_BRUSH_RELATIVE_TO_LETTER)
  {
    wrath_shader_brush_prepare(relative_coord.xy);
  }
  #elif defined(APPLY_BRUSH_RELATIVE_TO_ITEM)
  {
    wrath_shader_brush_prepare(tpos);    
  }
  #endif
}
