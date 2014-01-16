/*! 
 * \file font_analytic_nonlinear.frag.wrath-shader.glsl
 * \brief file font_analytic_nonlinear.frag.wrath-shader.glsl
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


shader_in mediump vec2 wrath_AnalyticBottomLeft;

mediump float
wrath_glyph_signed_distance(in vec2 glyph_position)
{
  mediump vec2 GlyphTextureCoordinate, glyph_texture_reciprocal_size;

  glyph_texture_reciprocal_size=vec2(wrath_font_page_data(0),
                                     wrath_font_page_data(1));
  GlyphTextureCoordinate=(glyph_position + wrath_AnalyticBottomLeft)*glyph_texture_reciprocal_size;

  return wrath_analytic_font_compute_distance(GlyphTextureCoordinate, 
                                              glyph_position);
}

mediump float
wrath_glyph_compute_coverage(in vec2 glyph_position)
{
  mediump float d;

  d=wrath_glyph_signed_distance(glyph_position);
  #if defined(WRATH_DERIVATIVES_SUPPORTED)
  {
    mediump vec2 dx, dy;
    mediump float sd, a;

    dx=dFdx(glyph_position);
    dy=dFdy(glyph_position);
    sd=max(1.0, inversesqrt( (dot(dx,dx) + dot(dy,dy))/2.0 ) );
    a= 0.5 + d*sd;
    return clamp(a, 0.0, 1.0);
  }
  #else
  {
    return step(0.0, d);
  }
  #endif
  
}

mediump float 
wrath_glyph_is_covered(in vec2 glyph_position)
{
  mediump float d;

  d=wrath_glyph_signed_distance(glyph_position);
  return step(0.0, d);
}

