/*! 
 * \file font_analytic_base.frag.wrath-shader.glsl
 * \brief file font_analytic_base.frag.wrath-shader.glsl
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




shader_in mediump vec4 wrath_AnalyticTexCoord_Position;

#define GlyphTextureCoordinate wrath_AnalyticTexCoord_Position.xy
#define GlyphCoordinate wrath_AnalyticTexCoord_Position.zw


mediump float
wrath_glyph_compute_coverage(void)
{
  mediump float d;

  d=wrath_analytic_font_compute_distance(GlyphTextureCoordinate, GlyphCoordinate);
  
  #if defined(WRATH_DERIVATIVES_SUPPORTED)
  {
    mediump vec2 dx, dy;
    mediump float sd, a;

    dx=dFdx(GlyphCoordinate);
    dy=dFdy(GlyphCoordinate);
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
wrath_glyph_is_covered(void)
{
  mediump float d;

  d=wrath_analytic_font_compute_distance(GlyphTextureCoordinate, GlyphCoordinate);
  return step(0.0, d);
}



#undef GlyphTextureCoordinate
#undef GlyphCoordinate
