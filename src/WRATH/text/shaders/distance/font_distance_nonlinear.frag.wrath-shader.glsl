/*! 
 * \file font_distance_base.frag.wrath-shader.glsl
 * \brief file font_distance_base.frag.wrath-shader.glsl
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







uniform mediump sampler2D wrath_DistanceField;
shader_in mediump vec2 wrath_DistanceFieldBottomLeft;

mediump float 
is_covered(in vec2 glyph_position)
{
  mediump float rr;
  mediump vec2 tt, glyph_texture_reciprocal_size;

  glyph_texture_reciprocal_size=vec2(wrath_font_page_data(0),
                                     wrath_font_page_data(1));

  tt=(glyph_position + wrath_DistanceFieldBottomLeft)*glyph_texture_reciprocal_size;
  rr=texture2D(wrath_DistanceField, tt).r;
  return step(0.5, rr);
}

mediump float
compute_coverage(in vec2 glyph_position)
{
  mediump float rr;
  mediump vec2 tt, glyph_texture_reciprocal_size;

  glyph_texture_reciprocal_size=vec2(wrath_font_page_data(0),
                                     wrath_font_page_data(1));

  tt=(glyph_position + wrath_DistanceFieldBottomLeft)*glyph_texture_reciprocal_size;
  rr=texture2D(wrath_DistanceField, tt).r;

  #if defined(WRATH_DERIVATIVES_SUPPORTED)
  {
    mediump vec2 dx, dy;
    mediump float scr;
    
    dx=dFdx(glyph_position);
    dy=dFdy(glyph_position);
    scr=sqrt( (dot(dx,dx) + dot(dy,dy))/2.0 );
  
    return smoothstep(0.5 - 0.2*scr,
                      0.5 + 0.2*scr,
                      rr);
  }
  #else
  {
    return step(0.5, rr);
  }
  #endif

}

















